#pragma once

// returns 1 if equal, 0 if not
int strcmp_custom(const char *a, const char *b) {
    while (*a && *b) {           // loop until one hits null
        if (*a != *b) return 0;  // mismatch
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0'); // both ended? equal
}