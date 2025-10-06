#include <beatqpl-old.h>


typedef void (*CommandFunc)(char* args);

typedef struct {
    char* name;
    CommandFunc func;
} CommandEntry;

CommandEntry commands[] = {
    { "echo", echo },
    { "setcolor", setcolor },
    { NULL, NULL }
};

/* char* functions[] = {
    "echo",
    "setcolor",
    NULL
}; */

void parseExec(const char* line) {
    const char* open = line;
    bool isFunc;
    while (*open && *open != '(')
        open++;
    if (!*open) {
        isFunc = false;
    }
    if (open) {
        isFunc = true;
    }

    const char* close = open + 1;
    while (*close && *close != ')')
        close++;
    if (!*close) {
        if (isFunc == true) {
            print("Ending semicolon missing.\n");
            return;
        }
    }

    int name_len = open - line;
    char name[32];
    for (int i = 0; i < name_len && i < 31; i++)
        name[i] = line[i];
    name[name_len] = '\0';

    int arg_len = close - open - 2;
    char arg[128];
    int ai = 0;
    const char* arg_start = open + 1;

    if (*arg_start == '"') arg_start++;

    while (*arg_start && *arg_start != '"' && ai < 127)
        arg[ai++] = *arg_start++;
    
    arg[ai] = '\0';

    for (int i = 0; commands[i].name; i++) {
        if (strcmp_custom(commands[i].name, name)) {
            commands[i].func(arg);
            return;
        }
    }

    if (isFunc == false) {
        return;
    }
}

void exec(const char* name, char* args) {
    
}

void echo(char* string) {
    print(echo);
}

void setcolor(uint8_t foreground, uint8_t background) {
    print_set_color(foreground, background);
}

/* void execstr(char* string) {
    char* start = 0;
    char* end = 0;
    char* str = string;
    char* FContents;
    bool isFunc;

    while (*str) {
        if (*str == '(') {
            start = str + 1;
            break;
        }
        str++;
    }

    if (!start) {
        str = string;
        isFunc = false;
    } else {
        str = string;
        isFunc = true;
    }
    
    end = start;
    while (*end && *end != ')') {
        end++;
    }

    if (*end != ')') {
        str = string;
        isFunc = false;
    }

    char *str2 = strchr(str, '(');
    if (str2) {
        char *name_end = str2;
        char *name_start = str2;

        while (name_start > str && *(name_start - 1) != ' ' && *(name_start - 1) != '\t')
            name_start--;
        
        char name[32];
        int len = name_end - name_start;
        if (len >= sizeof(name)) len = sizeof(name) - 1;

        for (int i = 0; i < len; i++)
            name[i] = name_start[i];
        name[len] = '\0';

        for (int i = 0; functions[i] != NULL; i++) {
            if (strcmp_custom(functions[i], name)) {

            }
        }
    }
    
    *end = 0;
} */