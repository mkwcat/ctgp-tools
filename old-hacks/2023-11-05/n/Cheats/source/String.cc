/*
 * Author : MikeIsAStar
 * Date   : 13 Aug 2023
 * File   : String.cc
 */

#include "String.hh"

void *memmem(const void *haystack, unsigned int haystackLength, const void *needle,
        unsigned int needleLength) {
    unsigned char *pHaystack = const_cast<unsigned char *>(haystack);
    unsigned char *pNeedle = const_cast<unsigned char *>(needle);

    while (haystackLength >= needleLength) {
        unsigned int n = 0;
        while (pHaystack[n] == pNeedle[n]) {
            n++;
            if (n == needleLength) {
                return reinterpret_cast<void *>(pHaystack);
            }
        }

        pHaystack++;
        haystackLength--;
    }

    return 0;
}
