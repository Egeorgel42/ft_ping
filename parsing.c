#include "parsing.h"
#include "terminal_output.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static void print_help(ping_config_t *config, const char* arg) {
    (void)config;
    (void)arg;
    assert(!arg);
    print_message(HELP_MESSAGE);
    exit(0);
}

static void set_verbose_mode(ping_config_t *config, const char* arg) {
    (void)arg;
    assert(!arg);
    config->mode = PING_MODE_VERBOSE;
}

const flag_t* init_flag_definition() {
    const flag_t flag_definitions[] = {
        {'h', print_help},
        {'?', print_help},
        {'v', set_verbose_mode},
        {'\0', NULL}
    };
    flag_t* flag_alloc = malloc(sizeof(flag_definitions));
    error_if(flag_alloc == NULL, ALLOC_ERR);
    memcpy(flag_alloc, flag_definitions, sizeof(flag_definitions));
    return flag_alloc;
}

void apply_flags(
        ping_config_t* config,
        flag_input_t* input,
        const flag_t* flag_defs) {

    for (size_t i = 0; input[i].flag; i++) {
        size_t j = 0;
        for (; flag_defs[j].flag; j++) {
            if (flag_defs[j].flag == input[i].flag) {
                flag_defs[j].function(config, input[i].arg);
                break;
            }
        }
        if (!flag_defs[j].flag) {
            print_message(INVALID_OPTION, input[i].flag);
            error(HELP_MESSAGE);
        }
    }
}

