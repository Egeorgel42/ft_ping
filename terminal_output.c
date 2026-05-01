#include "terminal_output.h"
#include <stdio.h>
#include <stdlib.h>

void error(const char* message) {
    fprintf(stderr, "%s: %s\n", PROGRAM_NAME, message);
    exit(1);
}