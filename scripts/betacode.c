/*
 * CTGP Revolution Beta Code Generator
 * Copyright (c) 2021 TheLordScruffy
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sha1.h"

#define is_digit(n)                                                            \
    (((n) >= '0' && (n) <= '9')                                                \
    || ((n) >= 'a' && (n) <= 'f') || ((n) >= 'A' && (n) <= 'F'))

const char* program_name;

const uint8_t salt[] = { 0x19, 0xA5, 0x7F, 0x15 };
const uint8_t salt2[] = { 0x03, 0x8F, 0xA5, 0x67 };

/* 
 * This is normally calculated on runtime using
 * XOR on 16-bits of the first hash with the equivalent bits
 * of the entered beta code.
 * This should always be zero.
 */
const uint8_t diff_salt[] = { 0, 0 };


uint32_t
rotl32 (uint32_t value, uint32_t count)
{
    uint32_t mask = 8 * sizeof(value) - 1;
    count &= mask;
    return (value << count) | (value >> (-count & mask));
}


int
mac_format_error (void)
{
    printf("%s: ", program_name);
    puts(
        "Invalid or missing argument"
    );
    printf(
        "%s <MAC with hyphens, example: 12-34-56-78-9a-bc>\n"
        , program_name
    );
    return 1;
}


int
main (int argc, char** argv)
{
    SHA1_CTX ctx;
    uint8_t result[20], mac[6];
    int i, j, k;
    int beta_code;
    char* sptr;

    program_name = argv[0];

    if (argc < 2
        || strlen(argv[1]) != 17) {
        return mac_format_error();
    }

    sptr = argv[1];
    for (
        i = 0; i < 6; i++
    )
    {
        if ( !is_digit(sptr[0])
          || !is_digit(sptr[1])) {
            return mac_format_error();
        }

        if (i < 5 && sptr[2] != '-')
            return mac_format_error();

        mac[i] = strtoul(sptr, NULL, 16);
        
        sptr += 3;
    }

    {
        SHA1Init(&ctx);
        SHA1Update(&ctx, mac, 6);
        SHA1Update(&ctx, salt, 4);
        SHA1Final(result, &ctx);
    }

    beta_code  = result[0] << 24;
    beta_code |= result[1] << 16;

    {
        SHA1Init(&ctx);
        SHA1Update(&ctx, mac, 6);
        SHA1Update(&ctx, salt2, 4);
        SHA1Update(&ctx, diff_salt, 2);
        SHA1Final(result, &ctx);
    }

    beta_code |= result[0] << 8;
    beta_code |= result[1];

    printf(
        "beta_code = %d\n"
        , beta_code
    );

    puts("arrow: ");
    for (
        i = 0; i < 2; i++
    )
    {
        printf(
            "    "
        );
        for (j = 0; j < 2; j++)
        {
            for (k = 0; k < 4; k++)
            {
                beta_code = rotl32(beta_code, 2);
                printf(
                      (beta_code & 3) == 0 ? "left,  "
                    : (beta_code & 3) == 1 ? "right, "
                    : (beta_code & 3) == 2 ? "up,    "
                                           : "down,  "
                );
            }
            putchar(' ');
        }
        putchar('\n');
    }

    return 0;
}
