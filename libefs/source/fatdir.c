/*
 fatdir.c

 Functions used by the newlib disc stubs to interface with
 this library

 Copyright (c) 2006 Michael "Chishm" Chisholm

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "fatdir.h"

#include "bit_ops.h"
#include "cache.h"
#include "directory.h"
#include "file_allocation_table.h"
#include "filetime.h"
#include "lock.h"
#include "partition.h"
#include <ctype.h>
#include <errno.h>
#include <string.h>

int efs_stat(
    int* error, const char* path, struct fat_stat* st
) {
    fat_partition* partition = NULL;
    fat_dir_entry  dirEntry;

    // Get the partition this file is on
    partition = fat_partition_getPartitionFromPath(path);
    if (partition == NULL) {
        *error = ENODEV;
        return -1;
    }

    // Move the path pointer to the start of the actual path
    if (strchr(path, ':') != NULL) {
        path = strchr(path, ':') + 1;
    }
    if (strchr(path, ':') != NULL) {
        *error = EINVAL;
        return -1;
    }

    fat_lock(&partition->lock);

    // Search for the file on the disc
    if (!fat_directory_entryFromPath(partition, &dirEntry, path, NULL)) {
        fat_unlock(&partition->lock);
        *error = ENOENT;
        return -1;
    }

    // Fill in the stat struct
    fat_directory_entryStat(partition, &dirEntry, st);

    fat_unlock(&partition->lock);
    return 0;
}

int efs_link(
    int* error, const char* existing, const char* newLink
) {
    (void) existing;
    (void) newLink;

    *error = ENOTSUP;
    return -1;
}

static int fat_unlinkCommon(
    int* error, const char* path, bool isRmDir
) {
    fat_partition* partition = NULL;
    fat_dir_entry  dirEntry;
    fat_dir_entry  dirContents;
    uint32_t       cluster;
    bool           nextEntry;
    bool           errorOccured = false;

    // Get the partition this directory is on
    partition                   = fat_partition_getPartitionFromPath(path);
    if (partition == NULL) {
        *error = ENODEV;
        return -1;
    }

    // Make sure we aren't trying to write to a read-only disc
    if (partition->readOnly) {
        *error = EROFS;
        return -1;
    }

    // Move the path pointer to the start of the actual path
    if (strchr(path, ':') != NULL) {
        path = strchr(path, ':') + 1;
    }
    if (strchr(path, ':') != NULL) {
        *error = EINVAL;
        return -1;
    }

    fat_lock(&partition->lock);

    // Search for the file on the disc
    if (!fat_directory_entryFromPath(partition, &dirEntry, path, NULL)) {
        fat_unlock(&partition->lock);
        *error = ENOENT;
        return -1;
    }

    cluster = fat_directory_entryGetCluster(partition, dirEntry.entryData);

    // If this is a directory, make sure it is empty
    if (fat_directory_isDirectory(&dirEntry)) {
        if (!isRmDir) {
            fat_unlock(&partition->lock);
            *error = EISDIR;
            return -1;
        }

        nextEntry = fat_directory_getFirstEntry(partition, &dirContents, cluster);

        while (nextEntry) {
            if (!fat_directory_isDot(&dirContents)) {
                // The directory had something in it that isn't a reference to itself or it's parent
                fat_unlock(&partition->lock);
                *error = ENOTEMPTY;
                return -1;
            }
            nextEntry = fat_directory_getNextEntry(partition, &dirContents);
        }
    } else if (isRmDir) {
        fat_unlock(&partition->lock);
        *error = ENOTDIR;
        return -1;
    }

    if (fat_fat_isValidCluster(partition, cluster)) {
        // Remove the cluster chain for this file
        if (!fat_fat_clearLinks(partition, cluster)) {
            *error       = EIO;
            errorOccured = true;
        }
    }

    // Remove the directory entry for this file
    if (!fat_directory_removeEntry(partition, &dirEntry)) {
        *error       = EIO;
        errorOccured = true;
    }

    // Flush any sectors in the disc cache
    if (!fat_cache_flush(partition->cache)) {
        *error       = EIO;
        errorOccured = true;
    }

    fat_unlock(&partition->lock);
    if (errorOccured) {
        return -1;
    } else {
        return 0;
    }
}

int efs_unlink(
    int* error, const char* path
) {
    return fat_unlinkCommon(error, path, false);
}

int efs_chdir(
    int* error, const char* path
) {
    fat_partition* partition = NULL;

    // Get the partition this directory is on
    partition                = fat_partition_getPartitionFromPath(path);
    if (partition == NULL) {
        *error = ENODEV;
        return -1;
    }

    // Move the path pointer to the start of the actual path
    if (strchr(path, ':') != NULL) {
        path = strchr(path, ':') + 1;
    }
    if (strchr(path, ':') != NULL) {
        *error = EINVAL;
        return -1;
    }

    fat_lock(&partition->lock);

    // Try changing directory
    if (fat_directory_chdir(partition, path)) {
        // Successful
        fat_unlock(&partition->lock);
        return 0;
    } else {
        // Failed
        fat_unlock(&partition->lock);
        *error = ENOTDIR;
        return -1;
    }
}

int efs_rename(
    int* error, const char* oldName, const char* newName
) {
    fat_partition* partition = NULL;
    fat_dir_entry  oldDirEntry;
    fat_dir_entry  newDirEntry;
    const char*    pathEnd;
    uint32_t       dirCluster;

    // Get the partition this directory is on
    partition = fat_partition_getPartitionFromPath(oldName);
    if (partition == NULL) {
        *error = ENODEV;
        return -1;
    }

    fat_lock(&partition->lock);

    // Make sure the same partition is used for the old and new names
    if (partition != fat_partition_getPartitionFromPath(newName)) {
        fat_unlock(&partition->lock);
        *error = EXDEV;
        return -1;
    }

    // Make sure we aren't trying to write to a read-only disc
    if (partition->readOnly) {
        fat_unlock(&partition->lock);
        *error = EROFS;
        return -1;
    }

    // Move the path pointer to the start of the actual path
    if (strchr(oldName, ':') != NULL) {
        oldName = strchr(oldName, ':') + 1;
    }
    if (strchr(oldName, ':') != NULL) {
        fat_unlock(&partition->lock);
        *error = EINVAL;
        return -1;
    }
    if (strchr(newName, ':') != NULL) {
        newName = strchr(newName, ':') + 1;
    }
    if (strchr(newName, ':') != NULL) {
        fat_unlock(&partition->lock);
        *error = EINVAL;
        return -1;
    }

    // Search for the file on the disc
    if (!fat_directory_entryFromPath(partition, &oldDirEntry, oldName, NULL)) {
        fat_unlock(&partition->lock);
        *error = ENOENT;
        return -1;
    }

    // Make sure there is no existing file / directory with the new name
    if (fat_directory_entryFromPath(partition, &newDirEntry, newName, NULL)) {
        fat_unlock(&partition->lock);
        *error = EEXIST;
        return -1;
    }

    // Create the new file entry
    // Get the directory it has to go in
    pathEnd = strrchr(newName, DIR_SEPARATOR);
    if (pathEnd == NULL) {
        // No path was specified
        dirCluster = partition->cwdCluster;
        pathEnd    = newName;
    } else {
        // Path was specified -- get the right dirCluster
        // Recycling newDirEntry, since it needs to be recreated anyway
        if (!fat_directory_entryFromPath(partition, &newDirEntry, newName, pathEnd) ||
            !fat_directory_isDirectory(&newDirEntry)) {
            fat_unlock(&partition->lock);
            *error = ENOTDIR;
            return -1;
        }
        dirCluster = fat_directory_entryGetCluster(partition, newDirEntry.entryData);
        // Move the pathEnd past the last DIR_SEPARATOR
        pathEnd += 1;
    }

    // Copy the entry data
    memcpy(&newDirEntry, &oldDirEntry, sizeof(fat_dir_entry));

    // Set the new name
    strncpy(newDirEntry.filename, pathEnd, fat_NAME_MAX - 1);

    // Write the new entry
    if (!fat_directory_addEntry(partition, &newDirEntry, dirCluster)) {
        fat_unlock(&partition->lock);
        *error = ENOSPC;
        return -1;
    }

    // Remove the old entry
    if (!fat_directory_removeEntry(partition, &oldDirEntry)) {
        fat_unlock(&partition->lock);
        *error = EIO;
        return -1;
    }

    // Flush any sectors in the disc cache
    if (!fat_cache_flush(partition->cache)) {
        fat_unlock(&partition->lock);
        *error = EIO;
        return -1;
    }

    fat_unlock(&partition->lock);
    return 0;
}

int efs_mkdir(
    int* error, const char* path
) {
    fat_partition* partition = NULL;
    bool           fileExists;
    fat_dir_entry  dirEntry;
    const char*    pathEnd;
    uint32_t       parentCluster, dirCluster;
    uint8_t        newEntryData[DIR_ENTRY_DATA_SIZE];

    partition = fat_partition_getPartitionFromPath(path);
    if (partition == NULL) {
        *error = ENODEV;
        return -1;
    }

    // Move the path pointer to the start of the actual path
    if (strchr(path, ':') != NULL) {
        path = strchr(path, ':') + 1;
    }
    if (strchr(path, ':') != NULL) {
        *error = EINVAL;
        return -1;
    }

    fat_lock(&partition->lock);

    // Search for the file/directory on the disc
    fileExists = fat_directory_entryFromPath(partition, &dirEntry, path, NULL);

    // Make sure it doesn't exist
    if (fileExists) {
        fat_unlock(&partition->lock);
        *error = EEXIST;
        return -1;
    }

    if (partition->readOnly) {
        // We can't write to a read-only partition
        fat_unlock(&partition->lock);
        *error = EROFS;
        return -1;
    }

    // Get the directory it has to go in
    pathEnd = strrchr(path, DIR_SEPARATOR);
    if (pathEnd == NULL) {
        // No path was specified
        parentCluster = partition->cwdCluster;
        pathEnd       = path;
    } else {
        // Path was specified -- get the right parentCluster
        // Recycling dirEntry, since it needs to be recreated anyway
        if (!fat_directory_entryFromPath(partition, &dirEntry, path, pathEnd) ||
            !fat_directory_isDirectory(&dirEntry)) {
            fat_unlock(&partition->lock);
            *error = ENOTDIR;
            return -1;
        }
        parentCluster = fat_directory_entryGetCluster(partition, dirEntry.entryData);
        // Move the pathEnd past the last DIR_SEPARATOR
        pathEnd += 1;
    }
    // Create the entry data
    strncpy(dirEntry.filename, pathEnd, fat_NAME_MAX - 1);
    memset(dirEntry.entryData, 0, DIR_ENTRY_DATA_SIZE);

    // Set the creation time and date
    dirEntry.entryData[fat_dir_entry_cTime_ms] = 0;
    u16_to_u8array(dirEntry.entryData, fat_dir_entry_cTime, fat_filetime_getTimeFromRTC());
    u16_to_u8array(dirEntry.entryData, fat_dir_entry_cDate, fat_filetime_getDateFromRTC());
    u16_to_u8array(dirEntry.entryData, fat_dir_entry_mTime, fat_filetime_getTimeFromRTC());
    u16_to_u8array(dirEntry.entryData, fat_dir_entry_mDate, fat_filetime_getDateFromRTC());
    u16_to_u8array(dirEntry.entryData, fat_dir_entry_aDate, fat_filetime_getDateFromRTC());

    // Set the directory attribute
    dirEntry.entryData[fat_dir_entry_attributes] = ATTRIB_DIR;

    // Get a cluster for the new directory
    dirCluster = fat_fat_linkFreeClusterCleared(partition, CLUSTER_FREE);
    if (!fat_fat_isValidCluster(partition, dirCluster)) {
        // No space left on disc for the cluster
        fat_unlock(&partition->lock);
        *error = ENOSPC;
        return -1;
    }
    u16_to_u8array(dirEntry.entryData, fat_dir_entry_cluster, (uint16_t) dirCluster);
    u16_to_u8array(dirEntry.entryData, fat_dir_entry_clusterHigh, dirCluster >> 16);

    // Write the new directory's entry to it's parent
    if (!fat_directory_addEntry(partition, &dirEntry, parentCluster)) {
        fat_unlock(&partition->lock);
        *error = ENOSPC;
        return -1;
    }

    // Create the dot entry within the directory
    memset(newEntryData, 0, DIR_ENTRY_DATA_SIZE);
    memset(newEntryData, ' ', 11);
    newEntryData[fat_dir_entry_name]       = '.';
    newEntryData[fat_dir_entry_attributes] = ATTRIB_DIR;
    u16_to_u8array(newEntryData, fat_dir_entry_cluster, (uint16_t) dirCluster);
    u16_to_u8array(newEntryData, fat_dir_entry_clusterHigh, dirCluster >> 16);

    // Write it to the directory, erasing that sector in the process
    fat_cache_eraseWritePartialSector(
        partition->cache, newEntryData, fat_fat_clusterToSector(partition, dirCluster), 0,
        DIR_ENTRY_DATA_SIZE
    );

    // Create the double dot entry within the directory

    // if ParentDir == Rootdir then ".."" always link to Cluster 0
    if (parentCluster == partition->rootDirCluster) {
        parentCluster = FAT16_ROOT_DIR_CLUSTER;
    }

    newEntryData[fat_dir_entry_name + 1] = '.';
    u16_to_u8array(newEntryData, fat_dir_entry_cluster, (uint16_t) parentCluster);
    u16_to_u8array(newEntryData, fat_dir_entry_clusterHigh, parentCluster >> 16);

    // Write it to the directory
    fat_cache_writePartialSector(
        partition->cache, newEntryData, fat_fat_clusterToSector(partition, dirCluster),
        DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE
    );

    // Flush any sectors in the disc cache
    if (!fat_cache_flush(partition->cache)) {
        fat_unlock(&partition->lock);
        *error = EIO;
        return -1;
    }

    fat_unlock(&partition->lock);
    return 0;
}

int efs_rmdir(
    int* error, const char* path
) {
    return fat_unlinkCommon(error, path, true);
}

#if 0
int fat_statvfs_r(
    int* error, const char* path, struct statvfs* buf
) {
    fat_partition*   partition = NULL;
    unsigned int freeClusterCount;

    // Get the partition of the requested path
    partition = fat_partition_getPartitionFromPath(path);
    if (partition == NULL) {
        *error = ENODEV;
        return -1;
    }

    fat_lock(&partition->lock);

    if (partition->filesysType == FS_FAT32) {
        // Sync FSinfo block
        fat_partition_readFSinfo(partition);
        freeClusterCount = partition->fat.numberFreeCluster;
    } else {
        freeClusterCount = fat_fat_freeClusterCount(partition);
    }

    // FAT clusters = POSIX blocks
    buf->f_bsize  = partition->bytesPerCluster; // File system block size.
    buf->f_frsize = partition->bytesPerCluster; // Fundamental file system block size.

    buf->f_blocks = partition->fat.lastCluster - CLUSTER_FIRST +
                    1;                // Total number of blocks on file system in units of f_frsize.
    buf->f_bfree  = freeClusterCount; // Total number of free blocks.
    buf->f_bavail = freeClusterCount; // Number of free blocks available to non-privileged process.

    // Treat requests for info on inodes as clusters
    buf->f_files =
        partition->fat.lastCluster - CLUSTER_FIRST + 1; // Total number of file serial numbers.
    buf->f_ffree = freeClusterCount;                    // Total number of free file serial numbers.
    buf->f_favail =
        freeClusterCount; // Number of file serial numbers available to non-privileged process.

    // File system ID. 32bit ioType value
    buf->f_fsid = fat_disc_hostType(partition->disc);

    // Bit mask of f_flag values.
    buf->f_flag = ST_NOSUID /* No support for ST_ISUID and ST_ISGID file mode bits */
                  | (partition->readOnly ? ST_RDONLY /* Read only file system */ : 0);
    // Maximum filename length.
    buf->f_namemax = NAME_MAX;

    fat_unlock(&partition->lock);
    return 0;
}
#endif

fat_dir_iter* efs_diropen(
    int* error, fat_dir_iter* dirState, const char* path
) {
    fat_dir_entry  dirEntry;
    fat_dir_state* state = (fat_dir_state*) (dirState->dirStruct);
    bool           fileExists;

    state->partition = fat_partition_getPartitionFromPath(path);
    if (state->partition == NULL) {
        *error = ENODEV;
        return NULL;
    }

    // Move the path pointer to the start of the actual path
    if (strchr(path, ':') != NULL) {
        path = strchr(path, ':') + 1;
    }
    if (strchr(path, ':') != NULL) {
        *error = EINVAL;
        return NULL;
    }

    fat_lock(&state->partition->lock);

    // Get the start cluster of the directory
    fileExists = fat_directory_entryFromPath(state->partition, &dirEntry, path, NULL);

    if (!fileExists) {
        fat_unlock(&state->partition->lock);
        *error = ENOENT;
        return NULL;
    }

    // Make sure it is a directory
    if (!fat_directory_isDirectory(&dirEntry)) {
        fat_unlock(&state->partition->lock);
        *error = ENOTDIR;
        return NULL;
    }

    // Save the start cluster for use when resetting the directory data
    state->startCluster = fat_directory_entryGetCluster(state->partition, dirEntry.entryData);

    // Get the first entry for use with a call to dirnext
    state->validEntry =
        fat_directory_getFirstEntry(state->partition, &(state->currentEntry), state->startCluster);

    // We are now using this entry
    state->inUse = true;
    fat_unlock(&state->partition->lock);
    return (fat_dir_iter*) state;
}

int efs_dirreset(
    int* error, fat_dir_iter* dirState
) {
    fat_dir_state* state = (fat_dir_state*) (dirState->dirStruct);

    fat_lock(&state->partition->lock);

    // Make sure we are still using this entry
    if (!state->inUse) {
        fat_unlock(&state->partition->lock);
        *error = EBADF;
        return -1;
    }

    // Get the first entry for use with a call to dirnext
    state->validEntry =
        fat_directory_getFirstEntry(state->partition, &(state->currentEntry), state->startCluster);

    fat_unlock(&state->partition->lock);
    return 0;
}

int efs_dirnext(
    int* error, fat_dir_iter* dirState, char* filename, struct fat_stat* filestat
) {
    fat_dir_state* state = (fat_dir_state*) (dirState->dirStruct);

    fat_lock(&state->partition->lock);

    // Make sure we are still using this entry
    if (!state->inUse) {
        fat_unlock(&state->partition->lock);
        *error = EBADF;
        return -1;
    }

    // Make sure there is another file to report on
    if (!state->validEntry) {
        fat_unlock(&state->partition->lock);
        return -1;
    }

    // Get the filename
    strncpy(filename, state->currentEntry.filename, fat_NAME_MAX);
    // Get the stats, if requested
    if (filestat != NULL) {
        fat_directory_entryStat(state->partition, &(state->currentEntry), filestat);
    }

    // Look for the next entry for use next time
    state->validEntry = fat_directory_getNextEntry(state->partition, &(state->currentEntry));

    fat_unlock(&state->partition->lock);
    return 0;
}

int efs_dirclose(
    int* error, fat_dir_iter* dirState
) {
    (void) error;

    fat_dir_state* state = (fat_dir_state*) (dirState->dirStruct);

    // We are no longer using this entry
    fat_lock(&state->partition->lock);
    state->inUse = false;
    fat_unlock(&state->partition->lock);

    return 0;
}
