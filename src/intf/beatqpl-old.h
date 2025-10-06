#pragma once

typedef void (*CommandFunc)(char* args);

#include <print.h>
#include <stdbool.h>
#include <strchr.h>
#include <strcmp.h>

char* functions[];

void exec(const char* name, char* args);
void echo(char* string);
void setcolor(uint8_t foreground, uint8_t background);