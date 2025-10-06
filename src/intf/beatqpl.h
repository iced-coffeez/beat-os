#pragma once

#include <stdint.h>
#include <stddef.h>

char* paramCMD[];

void output(char* text);

void setcolor(uint8_t foreground, uint8_t background);

void interperet(const char* prgm);