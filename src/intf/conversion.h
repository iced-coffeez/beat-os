#pragma once

#include <stdint.h>
#include <stdbool.h>

bool to_uint32(const char* str, uint32_t* out) {
    if (!str || !*str) return false;

    uint32_t result = 0;

    // Optional: skip leading spaces
    while (*str == ' ') str++;

    // Optional: allow leading '+'
    if (*str == '+') str++;

    if (!(*str >= '0' && *str <= '9')) return false; // must start with a digit

    while (*str) {
        if (*str < '0' || *str > '9') return false;

        uint32_t digit = *str - '0';

        // Check overflow before multiplying/adding
        if (result > (0xFFFFFFFF - digit) / 10) return false;

        result = result * 10 + digit;
        str++;
    }

    *out = result;
    return true;
}
