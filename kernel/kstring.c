/*
kernel/string.c
This file provides basic string manipulation functions for the kernel. It includes
functions to create strings from literals, characters, and hexadecimal values.
It also provides string comparison and formatted string creation capabilities.
*/
#include "kstring.h"
#include "ram_e.h"

static uint32_t compute_length(const char *s, uint32_t max_length) {
    /*
    This function computes the length of a null-terminated string up to a maximum length.
    If max_length is 0, it computes the full length until the null terminator.
    */
    uint32_t len = 0;
    while ((max_length == 0 || len < max_length) && s[len] != '\0') {
        len++;
    }
    return len;
}

kstring string_l(const char *literal) {
    /*
    This function creates a string structure from a string literal.
    It calculates the length of the literal and returns a string object.
    A literal is a string defined directly in the code, e.g., "Hello, World!".
    */
    uint32_t len = compute_length(literal, 0);
    kstring str;
    str.data = (char *)literal;
    str.length = len;
    return str;
}

kstring string_ca_max(const char *array, uint32_t max_length) {
    /*
    This function creates a string structure from a character array with a specified maximum length.
    It computes the length of the array up to max_length and returns a string object.
    Example usage: string_ca_max("Hello, World!", 5) would create a string with data "Hello" and length 5.
    */
    uint32_t len = compute_length(array, max_length);

    kstring str;
    str.data = (char *)array;
    str.length = len;
    return str;
}

kstring string_c(const char c) {
    /*
    This function creates a string structure from a single character.
    It allocates memory for a 2-character array (the character and a null terminator)
    and returns a string object.
    Example usage: string_c('A') would create a string with data "A" and length 1.
    */
    char *buf = (char*)talloc(2);
    buf[0] = c;
    buf[1] = 0;
    return (kstring){.data = buf, .length = 1};
}

kstring string_from_hex(uint64_t value) {
    /*
    This function creates a string representation of a hexadecimal value.
    It converts the given uint64_t value into a hexadecimal string prefixed with "0x".
    Example usage: string_from_hex(255) would create a string with data "0xFF" and length 4.
    */
    char *buf = (char*)talloc(18);
    uint32_t len = 0;
    buf[len++] = '0';
    buf[len++] = 'x';

    bool started = false;
    for (int i = 60; i >= 0; i-=4) {
        uint8_t nibble = (value >> i) & 0xF;
        char curr_char = nibble < 10 ? '0' + nibble : 'A' + (nibble - 10);
        if (started || curr_char != '0' || i == 0) {
            started = true;
            buf[len++] = curr_char;
        }
    }
    buf[len] = '\0';
    return (kstring){.data = buf, .length = len};
}

bool string_equals(kstring a, kstring b) {
    /*
    This function compares two strings for equality.
    It returns 1 (true) if the strings are equal, and 0 (false) otherwise.
    Example usage: string_equals(string_l("Hello"), string_l("Hello")) would return true.
    */
    return strcmp(a.data,b.data) == 0;
}

kstring string_format_args(const char *fmt, const uint64_t *args, uint32_t arg_count) {
    /*
    This function creates a formatted string using a format specifier and a list of arguments.
    It supports format specifiers such as %h (hex), %c (char), %s (string), and %i (integer).
    Example usage: string_format_args("Value: %h", [255], 1) would create a string with data "Value: 0xFF".
    */
    char *buf = (char*)talloc(256);
    uint32_t len = 0;
    uint32_t arg_index = 0;

    for (uint32_t i = 0; fmt[i] && len < 255; i++) {
        if (fmt[i] == '%' && fmt[i+1]) {
            i++;
            if (arg_index >= arg_count) break;
            if (fmt[i] == 'h') {
                uint64_t val = args[arg_index++];
                kstring hex = string_from_hex(val);
                for (uint32_t j = 0; j < hex.length && len < 255; j++) buf[len++] = hex.data[j];
                temp_free(hex.data, 18);
            } else if (fmt[i] == 'c') {
                uint64_t val = args[arg_index++];
                buf[len++] = (char)val;
            } else if (fmt[i] == 's') {
                const char *str = (const char *)(uintptr_t)args[arg_index++];
                for (uint32_t j = 0; str[j] && len < 255; j++) buf[len++] = str[j];
            } else if (fmt[i] == 'i') {
                uint64_t val = args[arg_index++];
                char temp[21];
                uint32_t temp_len = 0;
                bool negative = false;

                if ((int)val < 0) {
                    negative = true;
                    val = (uint64_t)(-(int)val);
                }

                do {
                    temp[temp_len++] = '0' + (val % 10);
                    val /= 10;
                } while (val && temp_len < 20);

                if (negative && len < 20) {
                    temp[temp_len++] = '-';
                }

                for (int j = temp_len - 1; j >= 0 && len < 255; j--) {
                    buf[len++] = temp[j];
                }
            } else if (fmt[i] == 's') {
                const char *str = (const char *)(uintptr_t)args[arg_index++];
                for (uint32_t j = 0; str[j] && len < 255; j++) buf[len++] = str[j];
            } else {
                buf[len++] = '%';
                buf[len++] = fmt[i];
            }
        } else {
            buf[len++] = fmt[i];
        }
    }

    buf[len] = 0;
    return (kstring){.data = buf, .length = len};
}

bool strcmp(const char *a, const char *b) {
    /*
    This function compares two null-terminated strings for equality.
    It returns 1 (true) if the strings are equal, and 0 (false) otherwise.
    Example usage: strcmp("Hello", "Hello") would return true.
    */
   while (*a && *b) {
    if (*a != *b) return (unsigned char)*a - (unsigned char)*b;
    a++; b++;
   }
   return (unsigned char)*a - (unsigned char)*b;
}

bool strcont(const char *a, const char *b) {
    /*
    This function checks if string b is contained within string a.
    It returns 1 (true) if b is found in a, and 0 (false) otherwise.
    Example usage: strcont("Hello, World!", "World") would return true.
    */
    while (*a) {
        const char *p = a, *q = b;
        while (*p && *q && *p == *q) {
            p++; q++;
        }
        if (*q == 0) return 1;
        a++;
    }
    return 0;
}