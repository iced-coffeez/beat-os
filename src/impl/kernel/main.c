#include <print.h>
#include <config.h>
#include <stdbool.h>
#include <io.h>
#include <draw.h>
#include <beatfs.h>
#include <conversion.h>
#include <strcmp.h>

void halt(char* reason) {
    print("Halted System! Error Code: ");
    print(reason);
    print_char('\n');
    __asm__ volatile("cli");
    __asm__ volatile("hlt");
}

extern unsigned char read_key(void); // read key function from Assembly

bool shift_pressed = false;
bool ctrl_pressed  = false;
bool alt_pressed   = false;

#define LASTCMD_ 128
char LASTCMD[LASTCMD_];
size_t _LASTCMD = 0;

// split input into command and first argument
void parse_command(char *input, char **cmd, char **arg) {
    *cmd = input;

    // walk until space or end
    *arg = input;
    while (**arg && **arg != ' ')
        (*arg)++;

    if (**arg) {
        **arg = '\0';   // terminate the command string
        (*arg)++;       // move past the null
        while (**arg == ' ') (*arg)++; // skip extra spaces
    } else {
        *arg = NULL; // no args
    }
}

void parse_command2(char *input, char **cmd, char **arg1, char **arg2) {
    *cmd = input;
    *arg1 = *arg2 = NULL;

    // find first space -> end of command
    char *p = input;
    while (*p && *p != ' ') p++;

    if (*p) {
        *p = '\0';
        p++; // move past null
        while (*p == ' ') p++; // skip extra spaces
        *arg1 = p;

        // find second space -> end of arg1
        while (*p && *p != ' ') p++;

        if (*p) {
            *p = '\0';
            p++;
            while (*p == ' ') p++; // skip extra spaces
            *arg2 = p;
        }
    }
}


void ident_drv() {
    beatfs_init(0);                         // init filesystem (master drive)
    //beatfs_create("hello.txt");           // create
    //beatfs_remove("hello.txt");           // delete
    //beatfs_list();                       // should show <empty>    
}

void str_append_char(char* buffer, size_t* len, char c) {
    buffer[*len] = c;
    (*len)++;
    buffer[*len] = 0; // null-terminate
}

static inline void disable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

static void print_hex_byte(unsigned char b) {
    const char *hex = "0123456789ABCDEF";
    char hi = hex[(b >> 4) & 0xF];
    char lo = hex[b & 0xF];
    print_char(hi);
    print_char(lo);
}

void handle_modifier(uint8_t scancode) {
    switch(scancode) {
        case 0x2A: case 0x36:       // Shift down
            shift_pressed = true;
            break;
        case 0xAA: case 0xB6:       // Shift up
            shift_pressed = false;
            break;
        // add Ctrl/Alt here if desired
    }
}

void runCode(char* command) {
    if (strcmp_custom(programlang, "python")) {
        
    }
}

void checkcommand(char* ascii) {
    char *cmd;
    char *arg;
    parse_command(ascii, &cmd, &arg);

    if (strcmp_custom(ascii, "halt")) {
        halt("User Request.");
    }
    if (strcmp_custom(ascii, "clear")) {
        print_clear();
    }
    if (strcmp_custom(ascii, "help")) {
        print("Available Commands:\n");
        print("help - idk what this does\n");
        print("unalive - pretty unsafe, just executes random commands or reboots.\n");
        print("clear - clears the screen\n");
        print("halt - causes a system halt\n");
        print("reinit - recalls the kernel script\n");
        print("probe [name] - creates file with name\n");
        print("echo [text] - prints text to screen\n");
        print("ldc - lists drive contents\n");
    }
    if (strcmp_custom(ascii, "reinit")) {
        reset_cmd();
        kernel_main();
    }
    if (strcmp_custom(ascii, "ldc")) {
        beatfs_list();
    }
    if (strcmp_custom(ascii, "unalive")) {
        alive = false;
    }
    else if (strcmp_custom(cmd, "echo")) {
        if (arg) {
            print(arg);
            print_char('\n');
        } else {
            print_char('\n');
        }
    }
    else if (strcmp_custom(cmd, "probe")) {
        beatfs_create(arg);
    }
    else if (strcmp_custom(cmd, "exec")) {

    }
    else if (strcmp_custom(cmd, "\n")) {
        return 1;
    }
}

void append_char_to_cmd(char c) {
    if (_LASTCMD < LASTCMD -1) {
        LASTCMD[_LASTCMD++] = c;
        LASTCMD[_LASTCMD] = '\0';
    }
}

void revoke_char_from_cmd(char c) {
    if (_LASTCMD > 0) {
        _LASTCMD--;
        LASTCMD[_LASTCMD] = '\0';
    }
}

void reset_cmd() {
    _LASTCMD = 0;
    LASTCMD[0] = '\0';
}

char scancode_to_ascii(unsigned char scancode) {
    // handle Shift presses/releases
    if (scancode & 0x80) return 0;

    // map scancodes to ASCII, accounting for Shift
    switch(scancode) {
        // numbers
        case 0x02: return shift_pressed ? '!' : '1';
        case 0x03: return shift_pressed ? '@' : '2';
        case 0x04: return shift_pressed ? '#' : '3';
        case 0x05: return shift_pressed ? '$' : '4';
        case 0x06: return shift_pressed ? '%' : '5';
        case 0x07: return shift_pressed ? '^' : '6';
        case 0x08: return shift_pressed ? '&' : '7';
        case 0x09: return shift_pressed ? '*' : '8';
        case 0x0A: return shift_pressed ? '(' : '9';
        case 0x0B: return shift_pressed ? ')' : '0';
        case 0x0C: return shift_pressed ? '_' : '-';
        case 0x0D: return shift_pressed ? '+' : '=';

        // letters
        case 0x10: return shift_pressed ? 'Q' : 'q';
        case 0x11: return shift_pressed ? 'W' : 'w';
        case 0x12: return shift_pressed ? 'E' : 'e';
        case 0x13: return shift_pressed ? 'R' : 'r';
        case 0x14: return shift_pressed ? 'T' : 't';
        case 0x15: return shift_pressed ? 'Y' : 'y';
        case 0x16: return shift_pressed ? 'U' : 'u';
        case 0x17: return shift_pressed ? 'I' : 'i';
        case 0x18: return shift_pressed ? 'O' : 'o';
        case 0x19: return shift_pressed ? 'P' : 'p';
        case 0x1E: return shift_pressed ? 'A' : 'a';
        case 0x1F: return shift_pressed ? 'S' : 's';
        case 0x20: return shift_pressed ? 'D' : 'd';
        case 0x21: return shift_pressed ? 'F' : 'f';
        case 0x22: return shift_pressed ? 'G' : 'g';
        case 0x23: return shift_pressed ? 'H' : 'h';
        case 0x24: return shift_pressed ? 'J' : 'j';
        case 0x25: return shift_pressed ? 'K' : 'k';
        case 0x26: return shift_pressed ? 'L' : 'l';
        case 0x2C: return shift_pressed ? 'Z' : 'z';
        case 0x2D: return shift_pressed ? 'X' : 'x';
        case 0x2E: return shift_pressed ? 'C' : 'c';
        case 0x2F: return shift_pressed ? 'V' : 'v';
        case 0x30: return shift_pressed ? 'B' : 'b';
        case 0x31: return shift_pressed ? 'N' : 'n';
        case 0x32: return shift_pressed ? 'M' : 'm';

        // symbols
        case 0x27: return shift_pressed ? ':'  : ';';
        case 0x28: return shift_pressed ? '"'  : '\'';
        case 0x33: return shift_pressed ? '<'  : ',';
        case 0x34: return shift_pressed ? '>'  : '.';
        case 0x35: return shift_pressed ? '?'  : '/';
        case 0x1A: return shift_pressed ? '{'  : '[';
        case 0x1B: return shift_pressed ? '}'  : ']';
        case 0x2B: return shift_pressed ? '|'  : '\\';
        case 0x29: return shift_pressed ? '~'  : '`';

        // special keys
        case 0x39: return ' ';   // Spacebar
        case 0x0E: return '\b';  // Backspace
        case 0x1C: return '\n';  // Enter

        default: return 0; // ignore other keys
    }
}

void kernel_main() {
    print_clear();
    disable_cursor();
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);

    if (verbose_boot) {
        print("Kernel has successfully loaded.");
        
        print("\n");
    }

    if (welcome_message) {
        print("Welcome to beat!os\n\n");
    }

    if (displaydrives) {
        ident_drv();
    }

    print("\n");
/*  print("\n");
    print("\\/ \n"); */

    while (alive) {
        unsigned char sc = read_key();

        bool shouldSaveChar = true;
        
        handle_modifier(sc);

        char ascii = scancode_to_ascii(sc);
        if (ascii) {
            /* if (ascii == '\n') {
                print("\n\\/");
            } */
            print_char(ascii);

/*          if (ascii == 'q') {
                print("\nExiting kernel...\n");
                __asm__ volatile("cli");
                __asm__ volatile("hlt");
                for (;;) {}
            } */

            if (ascii == '\n') {
                shouldSaveChar = false;
                LASTCMD[_LASTCMD] = '\0';
                checkcommand(LASTCMD);
                reset_cmd();
            }

            if (ascii == '\b') {
                shouldSaveChar = false;
                revoke_char_from_cmd(ascii);
            }

            if (shouldSaveChar) {
                append_char_to_cmd(ascii);
            }

            shouldSaveChar = true;
        }
    }
    if (!alive) {
        halt("Kernel Panic!");
    }
}