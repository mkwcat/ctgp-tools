/*
 directory.c
 Reading, writing and manipulation of the directory structure on
 a FAT partition

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

#include "directory.h"

#include "bit_ops.h"
#include "cache.h"
#include "common.h"
#include "file_allocation_table.h"
#include "filetime.h"
#include "partition.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

// Directory entry codes
enum {
    DIR_ENTRY_LAST = 0x00,
    DIR_ENTRY_FREE = 0xE5,
};

typedef uint16_t ucs2_t;

// Long file name directory entry
enum LFN_offset {
    LFN_offset_ordinal   = 0x00, // Position within LFN
    LFN_offset_char0     = 0x01,
    LFN_offset_char1     = 0x03,
    LFN_offset_char2     = 0x05,
    LFN_offset_char3     = 0x07,
    LFN_offset_char4     = 0x09,
    LFN_offset_flag      = 0x0B, // Should be equal to ATTRIB_LFN
    LFN_offset_reserved1 = 0x0C, // Always 0x00
    LFN_offset_checkSum  = 0x0D, // Checksum of short file name (alias)
    LFN_offset_char5     = 0x0E,
    LFN_offset_char6     = 0x10,
    LFN_offset_char7     = 0x12,
    LFN_offset_char8     = 0x14,
    LFN_offset_char9     = 0x16,
    LFN_offset_char10    = 0x18,
    LFN_offset_reserved2 = 0x1A, // Always 0x0000
    LFN_offset_char11    = 0x1C,
    LFN_offset_char12    = 0x1E,
};

static const int32_t LFN_offset_table[13] = {0x01, 0x03, 0x05, 0x07, 0x09, 0x0E, 0x10,
                                             0x12, 0x14, 0x16, 0x18, 0x1C, 0x1E};

#define LFN_END 0x40
#define LFN_DEL 0x80

static const char ILLEGAL_ALIAS_CHARACTERS[] = "\\/:;*?\"<>|&+,=[] ";
static const char ILLEGAL_LFN_CHARACTERS[]   = "\\/:*?\"<>|";

/*
Returns number of UCS-2 characters needed to encode an LFN
Returns -1 if it is an invalid LFN
*/
#define ABOVE_UCS_RANGE 0xF0

static int32_t fat_directory_lfnLength(
    const char* name
) {
    uint32_t    i;
    uint32_t    nameLength;
    int32_t     ucsLength;
    const char* tempName = name;

    nameLength           = (uint32_t) strnlen(name, fat_NAME_MAX);
    // Make sure the name is short enough to be valid
    if (nameLength >= fat_NAME_MAX) {
        return -1;
    }
    // Make sure it doesn't contain any invalid characters
    if (strpbrk(name, ILLEGAL_LFN_CHARACTERS) != NULL) {
        return -1;
    }
    // Make sure the name doesn't contain any control codes or codes not representable in UCS-2
    for (i = 0; i < nameLength; i++) {
        uint8_t ch = (uint8_t) name[i];
        if (ch < 0x20 || ch >= ABOVE_UCS_RANGE) {
            return -1;
        }
    }
    // Convert to UCS-2 and get the resulting length
    ucsLength = (int32_t) mbsrtowcs(NULL, &tempName, MAX_LFN_LENGTH, NULL);
    if (ucsLength < 0 || ucsLength >= MAX_LFN_LENGTH) {
        return -1;
    }

    // Otherwise it is valid
    return ucsLength;
}

/*
Convert a multibyte encoded string into a NUL-terminated UCS-2 string, storing at most len
characters return number of characters stored
*/
static uint32_t fat_directory_mbstoucs2(
    ucs2_t* dst, const char* src, uint32_t len
) {
    mbstate_t ps = {0};
    wchar_t   tempChar;
    int32_t   bytes;
    uint32_t  count = 0;

    while (count < len - 1 && *src != '\0') {
        bytes = (int32_t) mbrtowc(&tempChar, src, MB_CUR_MAX, &ps);
        if (bytes > 0) {
            *dst = (ucs2_t) tempChar;
            src += bytes;
            dst++;
            count++;
        } else if (bytes == 0) {
            break;
        } else {
            return -1;
        }
    }
    *dst = '\0';

    return count;
}

/*
Convert a UCS-2 string into a NUL-terminated multibyte string, storing at most len chars
return number of chars stored, or (uint32_t)-1 on error
*/
static uint32_t fat_directory_ucs2tombs(
    char* dst, const ucs2_t* src, uint32_t len
) {
    mbstate_t ps    = {0};
    uint32_t  count = 0;
    int32_t   bytes;
    char      buff[MB_CUR_MAX];

    while (count < len - 1 && *src != '\0') {
        bytes = (int32_t) wcrtomb(buff, *src, &ps);
        if (bytes < 0) {
            return -1u;
        }
        if (count + bytes < len && bytes > 0) {
            for (int32_t i = 0; i < bytes; i++) {
                *dst++ = buff[i];
            }
            src++;
            count += bytes;
        } else {
            break;
        }
    }
    *dst = L'\0';

    return count;
}

/*
Case-independent comparison of two multibyte encoded strings
*/
static int32_t fat_directory_mbsncasecmp(
    const char* s1, const char* s2, uint32_t len1
) {
    wchar_t   wc1, wc2;
    mbstate_t ps1 = {0};
    mbstate_t ps2 = {0};
    int32_t   b1  = 0;
    int32_t   b2  = 0;

    if (len1 == 0) {
        return 0;
    }

    do {
        s1 += b1;
        s2 += b2;
        b1 = (int32_t) mbrtowc(&wc1, s1, MB_CUR_MAX, &ps1);
        b2 = (int32_t) mbrtowc(&wc2, s2, MB_CUR_MAX, &ps2);
        if (b1 < 0 || b2 < 0) {
            break;
        }
        len1 -= b1;
    } while (len1 > 0 && towlower(wc1) == towlower(wc2) && wc1 != 0);

    return towlower(wc1) - towlower(wc2);
}

static bool fat_directory_entryGetAlias(
    const uint8_t* entryData, char* destName
) {
    char    c;
    bool    caseInfo;
    int32_t i   = 0;
    int32_t j   = 0;

    destName[0] = '\0';
    if (entryData[0] != DIR_ENTRY_FREE) {
        if (entryData[0] == '.') {
            destName[0] = '.';
            if (entryData[1] == '.') {
                destName[1] = '.';
                destName[2] = '\0';
            } else {
                destName[1] = '\0';
            }
        } else {
            // Copy the filename from the dirEntry to the string
            caseInfo = entryData[fat_dir_entry_caseInfo] & CASE_LOWER_BASE;
            for (i = 0; (i < 8) && (entryData[fat_dir_entry_name + i] != ' '); i++) {
                c           = entryData[fat_dir_entry_name + i];
                destName[i] = (caseInfo ? (char) tolower(c) : c);
            }
            // Copy the extension from the dirEntry to the string
            if (entryData[fat_dir_entry_extension] != ' ') {
                destName[i++] = '.';
                caseInfo      = entryData[fat_dir_entry_caseInfo] & CASE_LOWER_EXT;
                for (j = 0; (j < 3) && (entryData[fat_dir_entry_extension + j] != ' '); j++) {
                    c             = entryData[fat_dir_entry_extension + j];
                    destName[i++] = (caseInfo ? (char) tolower(c) : c);
                }
            }
            destName[i] = '\0';
        }
    }

    return (destName[0] != '\0');
}

uint32_t fat_directory_entryGetCluster(
    fat_partition* partition, const uint8_t* entryData
) {
    if (partition->filesysType == FS_FAT32) {
        // Only use high 16 bits of start cluster when we are certain they are correctly defined
        return u8array_to_u16(entryData, fat_dir_entry_cluster) |
               (u8array_to_u16(entryData, fat_dir_entry_clusterHigh) << 16);
    } else {
        return u8array_to_u16(entryData, fat_dir_entry_cluster);
    }
}

static bool fat_directory_incrementDirEntryPosition(
    fat_partition* partition, fat_dir_entry_position* entryPosition, bool extendDirectory
) {
    fat_dir_entry_position position = *entryPosition;
    uint32_t               tempCluster;

    // Increment offset, wrapping at the end of a sector
    ++position.offset;
    if (position.offset == partition->bytesPerSector / DIR_ENTRY_DATA_SIZE) {
        position.offset = 0;
        // Increment sector when wrapping
        ++position.sector;
        // But wrap at the end of a cluster
        if ((position.sector == partition->sectorsPerCluster) &&
            (position.cluster != FAT16_ROOT_DIR_CLUSTER)) {
            position.sector = 0;
            // Move onto the next cluster, making sure there is another cluster to go to
            tempCluster     = fat_fat_nextCluster(partition, position.cluster);
            if (tempCluster == CLUSTER_EOF) {
                if (extendDirectory) {
                    tempCluster = fat_fat_linkFreeClusterCleared(partition, position.cluster);
                    if (!fat_fat_isValidCluster(partition, tempCluster)) {
                        return false; // This will only happen if the disc is full
                    }
                } else {
                    return false; // Got to the end of the directory, not extending it
                }
            }
            position.cluster = tempCluster;
        } else if ((position.cluster == FAT16_ROOT_DIR_CLUSTER) &&
                   (position.sector == (partition->dataStart - partition->rootDirStart))) {
            return false; // Got to end of root directory, can't extend it
        }
    }
    *entryPosition = position;
    return true;
}

bool fat_directory_getNextEntry(
    fat_partition* partition, fat_dir_entry* entry
) {
    fat_dir_entry_position entryStart;
    fat_dir_entry_position entryEnd;
    uint8_t                entryData[0x20];
    ucs2_t                 lfn[MAX_LFN_LENGTH];
    bool                   notFound, found;
    int32_t                lfnPos;
    uint8_t                lfnChkSum, chkSum;
    bool                   lfnExists;
    int32_t                i;

    lfnChkSum  = 0;

    entryStart = entry->dataEnd;

    // Make sure we are using the correct root directory, in case of FAT32
    if (entryStart.cluster == FAT16_ROOT_DIR_CLUSTER) {
        entryStart.cluster = partition->rootDirCluster;
    }

    entryEnd  = entryStart;

    lfnExists = false;

    found     = false;
    notFound  = false;

    while (!found && !notFound) {
        if (fat_directory_incrementDirEntryPosition(partition, &entryEnd, false) == false) {
            notFound = true;
            break;
        }

        fat_cache_readPartialSector(
            partition->cache, entryData,
            fat_fat_clusterToSector(partition, entryEnd.cluster) + entryEnd.sector,
            entryEnd.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE
        );

        if (entryData[fat_dir_entry_attributes] == ATTRIB_LFN) {
            // It's an LFN
            if (entryData[LFN_offset_ordinal] & LFN_DEL) {
                lfnExists = false;
            } else if (entryData[LFN_offset_ordinal] & LFN_END) {
                // Last part of LFN, make sure it isn't deleted using previous if(Thanks MoonLight)
                entryStart = entryEnd; // This is the start of a directory entry
                lfnExists  = true;
                lfnPos     = (entryData[LFN_offset_ordinal] & ~LFN_END) * 13;
                if (lfnPos > MAX_LFN_LENGTH - 1) {
                    lfnPos = MAX_LFN_LENGTH - 1;
                }
                lfn[lfnPos] = '\0'; // Set end of lfn to null character
                lfnChkSum   = entryData[LFN_offset_checkSum];
            }
            if (lfnChkSum != entryData[LFN_offset_checkSum]) {
                lfnExists = false;
            }
            if (lfnExists) {
                lfnPos = ((entryData[LFN_offset_ordinal] & ~LFN_END) - 1) * 13;
                for (i = 0; i < 13; i++) {
                    if (lfnPos + i < MAX_LFN_LENGTH - 1) {
                        lfn[lfnPos + i] = (ucs2_t) entryData[LFN_offset_table[i]] |
                                          (ucs2_t) (entryData[LFN_offset_table[i] + 1] << 8);
                    }
                }
            }
        } else if (entryData[fat_dir_entry_attributes] & ATTRIB_VOL) {
            // This is a volume name, don't bother with it
        } else if (entryData[0] == DIR_ENTRY_LAST) {
            notFound = true;
        } else if ((entryData[0] != DIR_ENTRY_FREE) && (entryData[0] > 0x20) &&
                   !(entryData[fat_dir_entry_attributes] & ATTRIB_VOL)) {
            if (lfnExists) {
                // Calculate file checksum
                chkSum = 0;
                for (i = 0; i < 11; i++) {
                    // NOTE: The operation is an uint8_t rotate right
                    chkSum = ((chkSum & 1) ? 0x80 : 0) + (chkSum >> 1) + entryData[i];
                }
                if (chkSum != lfnChkSum) {
                    lfnExists          = false;
                    entry->filename[0] = '\0';
                }
            }

            if (lfnExists) {
                if (fat_directory_ucs2tombs(entry->filename, lfn, fat_NAME_MAX) == (uint32_t) -1) {
                    // Failed to convert the file name to UTF-8. Maybe the wrong locale is set?
                    return false;
                }
            } else {
                entryStart = entryEnd;
                fat_directory_entryGetAlias(entryData, entry->filename);
            }
            found = true;
        }
    }

    // If no file is found, return false
    if (notFound) {
        return false;
    } else {
        // Fill in the directory entry struct
        entry->dataStart = entryStart;
        entry->dataEnd   = entryEnd;
        memcpy(entry->entryData, entryData, DIR_ENTRY_DATA_SIZE);
        return true;
    }
}

bool fat_directory_getFirstEntry(
    fat_partition* partition, fat_dir_entry* entry, uint32_t dirCluster
) {
    entry->dataStart.cluster = dirCluster;
    entry->dataStart.sector  = 0;
    entry->dataStart.offset  = -1; // Start before the beginning of the directory

    entry->dataEnd           = entry->dataStart;

    return fat_directory_getNextEntry(partition, entry);
}

static bool fat_directory_getRootEntry(
    fat_partition* partition, fat_dir_entry* entry
) {
    entry->dataStart.cluster = 0;
    entry->dataStart.sector  = 0;
    entry->dataStart.offset  = 0;

    entry->dataEnd           = entry->dataStart;

    memset(entry->filename, '\0', fat_NAME_MAX);
    entry->filename[0] = '.';

    memset(entry->entryData, 0, DIR_ENTRY_DATA_SIZE);
    memset(entry->entryData, ' ', 11);
    entry->entryData[0]                        = '.';

    entry->entryData[fat_dir_entry_attributes] = ATTRIB_DIR;

    u16_to_u8array(entry->entryData, fat_dir_entry_cluster, (uint16_t) partition->rootDirCluster);
    u16_to_u8array(entry->entryData, fat_dir_entry_clusterHigh, partition->rootDirCluster >> 16);

    return true;
}

bool fat_directory_getVolumeLabel(
    fat_partition* partition, char* label
) {
    fat_dir_entry          entry;
    fat_dir_entry_position entryEnd;
    uint8_t                entryData[DIR_ENTRY_DATA_SIZE];
    int32_t                i;
    bool                   end;

    fat_directory_getRootEntry(partition, &entry);

    entryEnd = entry.dataEnd;

    // Make sure we are using the correct root directory, in case of FAT32
    if (entryEnd.cluster == FAT16_ROOT_DIR_CLUSTER) {
        entryEnd.cluster = partition->rootDirCluster;
    }

    label[0]  = '\0';
    label[11] = '\0';
    end       = false;
    // this entry should be among the first 3 entries in the root directory table, if not, then
    // system can have trouble displaying the right volume label
    while (!end) {
        if (!fat_cache_readPartialSector(
                partition->cache, entryData,
                fat_fat_clusterToSector(partition, entryEnd.cluster) + entryEnd.sector,
                entryEnd.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE
            )) { // error reading
            return false;
        }

        if (entryData[fat_dir_entry_attributes] == ATTRIB_VOL &&
            entryData[0] != DIR_ENTRY_FREE) {
            for (i = 0; i < 11; i++) {
                label[i] = entryData[fat_dir_entry_name + i];
            }
            return true;
        } else if (entryData[0] == DIR_ENTRY_LAST) {
            end = true;
        }

        if (fat_directory_incrementDirEntryPosition(partition, &entryEnd, false) == false) {
            end = true;
        }
    }
    return false;
}

bool fat_directory_entryFromPosition(
    fat_partition* partition, fat_dir_entry* entry
) {
    fat_dir_entry_position entryStart = entry->dataStart;
    fat_dir_entry_position entryEnd   = entry->dataEnd;
    bool                   entryStillValid;
    bool                   finished;
    ucs2_t                 lfn[MAX_LFN_LENGTH];
    int32_t                i;
    int32_t                lfnPos;
    uint8_t                entryData[DIR_ENTRY_DATA_SIZE];

    memset(entry->filename, '\0', fat_NAME_MAX);

    // Create an empty directory entry to overwrite the old ones with
    for (entryStillValid = true, finished = false; entryStillValid && !finished;
         entryStillValid = fat_directory_incrementDirEntryPosition(partition, &entryStart, false)) {
        fat_cache_readPartialSector(
            partition->cache, entryData,
            fat_fat_clusterToSector(partition, entryStart.cluster) + entryStart.sector,
            entryStart.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE
        );

        if ((entryStart.cluster == entryEnd.cluster) && (entryStart.sector == entryEnd.sector) &&
            (entryStart.offset == entryEnd.offset)) {
            // Copy the entry data and stop, since this is the last section of the directory entry
            memcpy(entry->entryData, entryData, DIR_ENTRY_DATA_SIZE);
            finished = true;
        } else {
            // Copy the long file name data
            lfnPos = ((entryData[LFN_offset_ordinal] & ~LFN_END) - 1) * 13;
            for (i = 0; i < 13; i++) {
                if (lfnPos + i < MAX_LFN_LENGTH - 1) {
                    lfn[lfnPos + i] = (ucs2_t) entryData[LFN_offset_table[i]] |
                                      (ucs2_t) (entryData[LFN_offset_table[i] + 1] << 8);
                }
            }
        }
    }

    if (!entryStillValid) {
        return false;
    }

    entryStart = entry->dataStart;
    if ((entryStart.cluster == entryEnd.cluster) && (entryStart.sector == entryEnd.sector) &&
        (entryStart.offset == entryEnd.offset)) {
        // Since the entry doesn't have a long file name, extract the short filename
        if (!fat_directory_entryGetAlias(entry->entryData, entry->filename)) {
            return false;
        }
    } else {
        // Encode the long file name into a multibyte string
        if (fat_directory_ucs2tombs(entry->filename, lfn, fat_NAME_MAX) == (uint32_t) -1) {
            return false;
        }
    }

    return true;
}

bool fat_directory_entryFromPath(
    fat_partition* partition, fat_dir_entry* entry, const char* path, const char* pathEnd
) {
    uint32_t    dirnameLength;
    const char* pathPosition;
    const char* nextPathPosition;
    uint32_t    dirCluster;
    bool        foundFile;
    char        alias[MAX_ALIAS_LENGTH];
    bool        found, notFound;

    pathPosition = path;

    found        = false;
    notFound     = false;

    if (pathEnd == NULL) {
        // Set pathEnd to the end of the path string
        pathEnd = strchr(path, '\0');
    }

    if (pathPosition[0] == DIR_SEPARATOR) {
        // Start at root directory
        dirCluster = partition->rootDirCluster;
        // Consume separator(s)
        while (pathPosition[0] == DIR_SEPARATOR) {
            pathPosition++;
        }
        // If the path is only specifying a directory in the form of "/" return it
        if (pathPosition >= pathEnd) {
            fat_directory_getRootEntry(partition, entry);
            found = true;
        }
    } else {
        // Start in current working directory
        dirCluster = partition->cwdCluster;
    }

    while (!found && !notFound) {
        // Get the name of the next required subdirectory within the path
        nextPathPosition = strchr(pathPosition, DIR_SEPARATOR);
        if (nextPathPosition != NULL) {
            dirnameLength = (uint32_t) (nextPathPosition - pathPosition);
        } else {
            dirnameLength = (uint32_t) strlen(pathPosition);
        }

        if (dirnameLength > fat_NAME_MAX) {
            // The path is too long to bother with
            return false;
        }

        // Check for "." or ".." when the dirCluster is root cluster
        // These entries do not exist, so we must fake it
        if ((dirCluster == partition->rootDirCluster) &&
            ((strncmp(".", pathPosition, dirnameLength) == 0) ||
             (strncmp("..", pathPosition, dirnameLength) == 0))) {
            foundFile = true;
            fat_directory_getRootEntry(partition, entry);
        } else {
            // Look for the directory within the path
            foundFile = fat_directory_getFirstEntry(partition, entry, dirCluster);

            while (foundFile && !found && !notFound) { // It hasn't already found the file
                // Check if the filename matches
                if ((dirnameLength == strnlen(entry->filename, fat_NAME_MAX)) &&
                    (fat_directory_mbsncasecmp(pathPosition, entry->filename, dirnameLength) ==
                     0)) {
                    found = true;
                }

                // Check if the alias matches
                fat_directory_entryGetAlias(entry->entryData, alias);
                if ((dirnameLength == strnlen(alias, MAX_ALIAS_LENGTH)) &&
                    (strncasecmp(pathPosition, alias, dirnameLength) == 0)) {
                    found = true;
                }

                if (found && !(entry->entryData[fat_dir_entry_attributes] & ATTRIB_DIR) &&
                    (nextPathPosition != NULL)) {
                    // Make sure that we aren't trying to follow a file instead of a directory in
                    // the path
                    found = false;
                }

                if (!found) {
                    foundFile = fat_directory_getNextEntry(partition, entry);
                }
            }
        }

        if (!foundFile) {
            // Check that the search didn't get to the end of the directory
            notFound = true;
            found    = false;
        } else if ((nextPathPosition == NULL) || (nextPathPosition >= pathEnd)) {
            // Check that we reached the end of the path
            found = true;
        } else if (entry->entryData[fat_dir_entry_attributes] & ATTRIB_DIR) {
            dirCluster = fat_directory_entryGetCluster(partition, entry->entryData);
            if (dirCluster == CLUSTER_ROOT) {
                dirCluster = partition->rootDirCluster;
            }
            pathPosition = nextPathPosition;
            // Consume separator(s)
            while (pathPosition[0] == DIR_SEPARATOR) {
                pathPosition++;
            }
            // The requested directory was found
            if (pathPosition >= pathEnd) {
                found = true;
            } else {
                found = false;
            }
        }
    }

    if (found && !notFound) {
        if (partition->filesysType == FS_FAT32 &&
            (entry->entryData[fat_dir_entry_attributes] & ATTRIB_DIR) &&
            fat_directory_entryGetCluster(partition, entry->entryData) == CLUSTER_ROOT) {
            // On FAT32 it should specify an actual cluster for the root entry,
            // not cluster 0 as on FAT16
            fat_directory_getRootEntry(partition, entry);
        }
        return true;
    } else {
        return false;
    }
}

bool fat_directory_removeEntry(
    fat_partition* partition, fat_dir_entry* entry
) {
    fat_dir_entry_position entryStart = entry->dataStart;
    fat_dir_entry_position entryEnd   = entry->dataEnd;
    bool                   entryStillValid;
    bool                   finished;
    uint8_t                entryData[DIR_ENTRY_DATA_SIZE];

    // Create an empty directory entry to overwrite the old ones with
    for (entryStillValid = true, finished = false; entryStillValid && !finished;
         entryStillValid = fat_directory_incrementDirEntryPosition(partition, &entryStart, false)) {
        fat_cache_readPartialSector(
            partition->cache, entryData,
            fat_fat_clusterToSector(partition, entryStart.cluster) + entryStart.sector,
            entryStart.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE
        );
        entryData[0] = DIR_ENTRY_FREE;
        fat_cache_writePartialSector(
            partition->cache, entryData,
            fat_fat_clusterToSector(partition, entryStart.cluster) + entryStart.sector,
            entryStart.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE
        );
        if ((entryStart.cluster == entryEnd.cluster) && (entryStart.sector == entryEnd.sector) &&
            (entryStart.offset == entryEnd.offset)) {
            finished = true;
        }
    }

    if (!entryStillValid) {
        return false;
    }

    return true;
}

static bool fat_directory_findEntryGap(
    fat_partition* partition, fat_dir_entry* entry, uint32_t dirCluster, uint32_t size
) {
    fat_dir_entry_position gapStart;
    fat_dir_entry_position gapEnd;
    uint8_t                entryData[DIR_ENTRY_DATA_SIZE];
    uint32_t               dirEntryRemain;
    bool                   endOfDirectory, entryStillValid;

    // Scan Dir for free entry
    gapEnd.offset   = 0;
    gapEnd.sector   = 0;
    gapEnd.cluster  = dirCluster;

    gapStart        = gapEnd;

    entryStillValid = true;
    dirEntryRemain  = size;
    endOfDirectory  = false;

    while (entryStillValid && !endOfDirectory && (dirEntryRemain > 0)) {
        fat_cache_readPartialSector(
            partition->cache, entryData,
            fat_fat_clusterToSector(partition, gapEnd.cluster) + gapEnd.sector,
            gapEnd.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE
        );
        if (entryData[0] == DIR_ENTRY_LAST) {
            if (dirEntryRemain == size) {
                gapStart = gapEnd;
            }
            --dirEntryRemain;
            endOfDirectory = true;
        } else if (entryData[0] == DIR_ENTRY_FREE) {
            if (dirEntryRemain == size) {
                gapStart = gapEnd;
            }
            --dirEntryRemain;
        } else {
            dirEntryRemain = size;
        }

        if (!endOfDirectory && (dirEntryRemain > 0)) {
            entryStillValid = fat_directory_incrementDirEntryPosition(partition, &gapEnd, true);
        }
    }

    // Make sure the scanning didn't fail
    if (!entryStillValid) {
        return false;
    }

    // Save the start entry, since we know it is valid
    entry->dataStart = gapStart;

    if (endOfDirectory) {
        memset(entryData, DIR_ENTRY_LAST, DIR_ENTRY_DATA_SIZE);
        dirEntryRemain += 1; // Increase by one to take account of End Of Directory Marker
        while ((dirEntryRemain > 0) && entryStillValid) {
            // Get the gapEnd before incrementing it, so the second to last one is saved
            entry->dataEnd  = gapEnd;
            // Increment gapEnd, moving onto the next entry
            entryStillValid = fat_directory_incrementDirEntryPosition(partition, &gapEnd, true);
            --dirEntryRemain;
            // Fill the entry with blanks
            fat_cache_writePartialSector(
                partition->cache, entryData,
                fat_fat_clusterToSector(partition, gapEnd.cluster) + gapEnd.sector,
                gapEnd.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE
            );
        }
        if (!entryStillValid) {
            return false;
        }
    } else {
        entry->dataEnd = gapEnd;
    }

    return true;
}

static bool fat_directory_entryExists(
    fat_partition* partition, const char* name, uint32_t dirCluster
) {
    fat_dir_entry tempEntry;
    bool          foundFile;
    char          alias[MAX_ALIAS_LENGTH];
    uint32_t      dirnameLength;

    dirnameLength = (uint32_t) strnlen(name, fat_NAME_MAX);

    if (dirnameLength >= fat_NAME_MAX) {
        return false;
    }

    // Make sure the entry doesn't already exist
    foundFile = fat_directory_getFirstEntry(partition, &tempEntry, dirCluster);

    while (foundFile) { // It hasn't already found the file
        // Check if the filename matches
        if ((dirnameLength == strnlen(tempEntry.filename, fat_NAME_MAX)) &&
            (fat_directory_mbsncasecmp(name, tempEntry.filename, dirnameLength) == 0)) {
            return true;
        }

        // Check if the alias matches
        fat_directory_entryGetAlias(tempEntry.entryData, alias);
        if ((strncasecmp(name, alias, MAX_ALIAS_LENGTH) == 0)) {
            return true;
        }
        foundFile = fat_directory_getNextEntry(partition, &tempEntry);
    }
    return false;
}

/*
Creates an alias for a long file name. If the alias is not an exact match for the
filename, it returns the number of characters in the alias. If the two names match,
it returns 0. If there was an error, it returns -1.
*/
static int32_t fat_directory_createAlias(
    char* alias, const char* lfn
) {
    bool        lossyConversion = false; // Set when the alias had to be modified to be valid
    int32_t     lfnPos          = 0;
    int32_t     aliasPos        = 0;
    wchar_t     lfnChar;
    int32_t     oemChar;
    mbstate_t   ps        = {0};
    int32_t     bytesUsed = 0;
    const char* lfnExt;
    int32_t     aliasExtLen;

    // Strip leading periods
    while (lfn[lfnPos] == '.') {
        lfnPos++;
        lossyConversion = true;
    }

    // Primary portion of alias
    while (aliasPos < 8 && lfn[lfnPos] != '.' && lfn[lfnPos] != '\0') {
        bytesUsed = (int32_t) mbrtowc(&lfnChar, lfn + lfnPos, fat_NAME_MAX - lfnPos, &ps);
        if (bytesUsed < 0) {
            return -1;
        }
        oemChar = wctob(towupper((wint_t) lfnChar));
        if (wctob((wint_t) lfnChar) != oemChar) {
            // Case of letter was changed
            lossyConversion = true;
        }
        if (oemChar == ' ') {
            // Skip spaces in filename
            lossyConversion = true;
            lfnPos += bytesUsed;
            continue;
        }
        if (oemChar == EOF) {
            oemChar         = '_'; // Replace unconvertable characters with underscores
            lossyConversion = true;
        }
        if (strchr(ILLEGAL_ALIAS_CHARACTERS, oemChar) != NULL) {
            // Invalid Alias character
            oemChar         = '_'; // Replace illegal characters with underscores
            lossyConversion = true;
        }

        alias[aliasPos] = (char) oemChar;
        aliasPos++;
        lfnPos += bytesUsed;
    }

    if (lfn[lfnPos] != '.' && lfn[lfnPos] != '\0') {
        // Name was more than 8 characters long
        lossyConversion = true;
    }

    // Alias extension
    lfnExt = strrchr(lfn, '.');
    if (lfnExt != NULL && lfnExt != strchr(lfn, '.')) {
        // More than one period in name
        lossyConversion = true;
    }
    if (lfnExt != NULL && lfnExt[1] != '\0') {
        lfnExt++;
        alias[aliasPos] = '.';
        aliasPos++;
        memset(&ps, 0, sizeof(ps));
        for (aliasExtLen = 0; aliasExtLen < MAX_ALIAS_EXT_LENGTH && *lfnExt != '\0';
             aliasExtLen++) {
            bytesUsed = (int32_t) mbrtowc(&lfnChar, lfnExt, fat_NAME_MAX - lfnPos, &ps);
            if (bytesUsed < 0) {
                return -1;
            }
            oemChar = wctob(towupper((wint_t) lfnChar));
            if (wctob((wint_t) lfnChar) != oemChar) {
                // Case of letter was changed
                lossyConversion = true;
            }
            if (oemChar == ' ') {
                // Skip spaces in alias
                lossyConversion = true;
                lfnExt += bytesUsed;
                continue;
            }
            if (oemChar == EOF) {
                oemChar         = '_'; // Replace unconvertable characters with underscores
                lossyConversion = true;
            }
            if (strchr(ILLEGAL_ALIAS_CHARACTERS, oemChar) != NULL) {
                // Invalid Alias character
                oemChar         = '_'; // Replace illegal characters with underscores
                lossyConversion = true;
            }

            alias[aliasPos] = (char) oemChar;
            aliasPos++;
            lfnExt += bytesUsed;
        }
        if (*lfnExt != '\0') {
            // Extension was more than 3 characters long
            lossyConversion = true;
        }
    }

    alias[aliasPos] = '\0';
    if (lossyConversion) {
        return aliasPos;
    } else {
        return 0;
    }
}

bool fat_directory_addEntry(
    fat_partition* partition, fat_dir_entry* entry, uint32_t dirCluster
) {
    uint32_t               entrySize;
    uint8_t                lfnEntry[DIR_ENTRY_DATA_SIZE];
    int32_t                i, j; // Must be signed for use when decrementing in for loop
    char*                  tmpCharPtr;
    fat_dir_entry_position curEntryPos;
    bool                   entryStillValid;
    uint8_t                aliasCheckSum = 0;
    char                   alias[MAX_ALIAS_LENGTH];
    int32_t                aliasLen;
    int32_t                lfnLen;

    // Remove trailing spaces
    for (i = (int32_t) strlen(entry->filename) - 1; (i >= 0) && (entry->filename[i] == ' '); --i) {
        entry->filename[i] = '\0';
    }
#if 0
	// Remove leading spaces
	for (i = 0; entry->filename[i] == ' '; ++i) ;
	if (i > 0) {
		memmove (entry->filename, entry->filename + i, strlen (entry->filename + i));
	}
#endif

    // Make sure the filename is not 0 length
    if (strnlen(entry->filename, fat_NAME_MAX) < 1) {
        return false;
    }

    // Make sure the filename is at least a valid LFN
    lfnLen = fat_directory_lfnLength(entry->filename);
    if (lfnLen < 0) {
        return false;
    }

    // Remove junk in filename
    i = (int32_t) strlen(entry->filename);
    memset(entry->filename + i, '\0', fat_NAME_MAX - i);

    // Make sure the entry doesn't already exist
    if (fat_directory_entryExists(partition, entry->filename, dirCluster)) {
        return false;
    }

    // Clear out alias, so we can generate a new one
    memset(entry->entryData, ' ', 11);

    if (strncmp(entry->filename, ".", fat_NAME_MAX) == 0) {
        // "." entry
        entry->entryData[0] = '.';
        entrySize           = 1;
    } else if (strncmp(entry->filename, "..", fat_NAME_MAX) == 0) {
        // ".." entry
        entry->entryData[0] = '.';
        entry->entryData[1] = '.';
        entrySize           = 1;
    } else {
        // Normal file name
        aliasLen = fat_directory_createAlias(alias, entry->filename);
        if (aliasLen < 0) {
            return false;
        } else if (aliasLen == 0) {
            // It's a normal short filename
            entrySize = 1;
        } else {
            // It's a long filename with an alias
            entrySize = ((lfnLen + LFN_ENTRY_LENGTH - 1) / LFN_ENTRY_LENGTH) + 1;

            // Generate full alias for all cases except when the alias is simply an upper case
            // version of the LFN and there isn't already a file with that name
            if (strncasecmp(alias, entry->filename, MAX_ALIAS_LENGTH) != 0 ||
                fat_directory_entryExists(partition, alias, dirCluster)) {
                // expand primary part to 8 characters long by padding the end with underscores
                i = 0;
                j = MAX_ALIAS_PRI_LENGTH;
                // Move extension to last 3 characters
                while (alias[i] != '.' && alias[i] != '\0') {
                    i++;
                }
                if (i < j) {
                    memmove(alias + j, alias + i, aliasLen - i + 1);
                    // Pad primary component
                    memset(alias + i, '_', j - i);
                }
                // Generate numeric tail
                for (i = 1; i <= MAX_NUMERIC_TAIL; i++) {
                    j          = i;
                    tmpCharPtr = alias + MAX_ALIAS_PRI_LENGTH - 1;
                    while (j > 0) {
                        *tmpCharPtr = '0' + (j % 10); // ASCII numeric value
                        tmpCharPtr--;
                        j /= 10;
                    }
                    *tmpCharPtr = '~';
                    if (!fat_directory_entryExists(partition, alias, dirCluster)) {
                        break;
                    }
                }
                if (i > MAX_NUMERIC_TAIL) {
                    // Couldn't get a valid alias
                    return false;
                }
            }
        }

        // Copy alias or short file name into directory entry data
        for (i = 0, j = 0; (j < 8) && (alias[i] != '.') && (alias[i] != '\0'); i++, j++) {
            entry->entryData[j] = alias[i];
        }
        while (j < 8) {
            entry->entryData[j] = ' ';
            ++j;
        }
        if (alias[i] == '.') {
            // Copy extension
            ++i;
            while ((alias[i] != '\0') && (j < 11)) {
                entry->entryData[j] = alias[i];
                ++i;
                ++j;
            }
        }
        while (j < 11) {
            entry->entryData[j] = ' ';
            ++j;
        }

        // Generate alias checksum
        for (i = 0; i < ALIAS_ENTRY_LENGTH; i++) {
            // NOTE: The operation is an uint8_t rotate right
            aliasCheckSum =
                ((aliasCheckSum & 1) ? 0x80 : 0) + (aliasCheckSum >> 1) + entry->entryData[i];
        }
    }

    // Find or create space for the entry
    if (fat_directory_findEntryGap(partition, entry, dirCluster, entrySize) == false) {
        return false;
    }

    // Write out directory entry
    curEntryPos = entry->dataStart;

    {
        // lfn is only pushed onto the stack here, reducing overall stack usage
        ucs2_t lfn[MAX_LFN_LENGTH] = {0};
        fat_directory_mbstoucs2(lfn, entry->filename, MAX_LFN_LENGTH);

        for (entryStillValid = true, i = entrySize; entryStillValid && i > 0;
             entryStillValid =
                 fat_directory_incrementDirEntryPosition(partition, &curEntryPos, false),
            --i) {
            if (i > 1) {
                // Long filename entry
                lfnEntry[LFN_offset_ordinal] =
                    (uint8_t) (i - 1) | (uint8_t) ((uint32_t) i == entrySize ? LFN_END : 0);
                for (j = 0; j < 13; j++) {
                    if (lfn[(i - 2) * 13 + j] == '\0') {
                        if ((j > 1) && (lfn[(i - 2) * 13 + (j - 1)] == '\0')) {
                            u16_to_u8array(lfnEntry, LFN_offset_table[j], 0xffff); // Padding
                        } else {
                            u16_to_u8array(
                                lfnEntry, LFN_offset_table[j], 0x0000
                            ); // Terminating null character
                        }
                    } else {
                        u16_to_u8array(lfnEntry, LFN_offset_table[j], lfn[(i - 2) * 13 + j]);
                    }
                }

                lfnEntry[LFN_offset_checkSum]  = aliasCheckSum;
                lfnEntry[LFN_offset_flag]      = ATTRIB_LFN;
                lfnEntry[LFN_offset_reserved1] = 0;
                u16_to_u8array(lfnEntry, LFN_offset_reserved2, 0);
                fat_cache_writePartialSector(
                    partition->cache, lfnEntry,
                    fat_fat_clusterToSector(partition, curEntryPos.cluster) + curEntryPos.sector,
                    curEntryPos.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE
                );
            } else {
                // Alias & file data
                fat_cache_writePartialSector(
                    partition->cache, entry->entryData,
                    fat_fat_clusterToSector(partition, curEntryPos.cluster) + curEntryPos.sector,
                    curEntryPos.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE
                );
            }
        }
    }

    return true;
}

bool fat_directory_chdir(
    fat_partition* partition, const char* path
) {
    fat_dir_entry entry;

    if (!fat_directory_entryFromPath(partition, &entry, path, NULL)) {
        return false;
    }

    if (!(entry.entryData[fat_dir_entry_attributes] & ATTRIB_DIR)) {
        return false;
    }

    partition->cwdCluster = fat_directory_entryGetCluster(partition, entry.entryData);

    return true;
}

void fat_directory_entryStat(
    fat_partition* partition, fat_dir_entry* entry, struct fat_stat* st
) {
    // Fill in the stat struct
    // Some of the values are faked for the sake of compatibility
    st->st_dev = fat_disc_hostType(partition->disc); // The device is the 32bit ioType value
    st->st_ino = fat_directory_entryGetCluster(
        partition, entry->entryData
    ); // The file serial number is the start cluster
    st->st_mode = (fat_directory_isDirectory(entry) ? fat_S_IFDIR : fat_S_IFREG) |
                  (fat_S_IRUSR | fat_S_IRGRP | fat_S_IROTH) |
                  (fat_directory_isWritable(entry) ? (fat_S_IWUSR | fat_S_IWGRP | fat_S_IWOTH)
                                                   : 0); // Mode bits based on dirEntry ATTRIB byte
    st->st_size  = u8array_to_u32(entry->entryData, fat_dir_entry_fileSize); // File size
    st->st_atime = fat_filetime_to_time_t(0, u8array_to_u16(entry->entryData, fat_dir_entry_aDate));
    st->st_mtime = fat_filetime_to_time_t(
        u8array_to_u16(entry->entryData, fat_dir_entry_mTime),
        u8array_to_u16(entry->entryData, fat_dir_entry_mDate)
    );
    st->st_ctime = fat_filetime_to_time_t(
        u8array_to_u16(entry->entryData, fat_dir_entry_cTime),
        u8array_to_u16(entry->entryData, fat_dir_entry_cDate)
    );
    st->st_blksize = partition->bytesPerSector; // Prefered file I/O block size
    st->st_blocks  = (st->st_size + partition->bytesPerSector - 1) /
                    partition->bytesPerSector; // File size in blocks
}
