#include <beatqpl.h>
#include <stdint.h>
#include <stddef.h>

char* paramCMD[] = {
    "output",
    "setcolor"
};

void output(char* text) {
    print(text);
}

void setcolor(uint8_t foreground, uint8_t background) {

}

void interperet(const char* prgm) {
    char* code;
}