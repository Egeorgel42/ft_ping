#include "parsing.h"
#include "terminal_output.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

typedef const struct {
    char const	flag;
	bool const	has_arg;
    void		(*function)(ping_config_t *config, char const* arg);
} defined_flag_t;

static void print_help(ping_config_t *config, char const* arg) {
    (void)config;
    (void)arg;
    libassert(!arg, "print_help: expected null argument.");
    print_message(HELP_MESSAGE);
    exit(0);
}

static void set_verbose_mode(ping_config_t *config, char const* arg) {
    (void)arg;
    libassert(!arg, "set_verbose_mode: expected null argument.");
    config->mode = PING_MODE_VERBOSE;
}

static void parse_and_apply_argv_flags(char** current_argv, ping_config_t* config) {

    static defined_flag_t const flag_definitions[] = {
        {'h', false, print_help}, //flag, arg_count, flag_function
        {'?', false, print_help},
        {'v', false, set_verbose_mode},
        {'\0', false, NULL}
    };

	if (current_argv[0][0] != '-') {
		return;
	}
	for (size_t i = 1; current_argv[0][i]; i++) {
		size_t j = 0;
		for (; flag_definitions[j].flag != '\0'; j++) {
			if (flag_definitions[j].flag == current_argv[0][i]) {
				char const* arg = NULL;
				if (flag_definitions[j].has_arg) {
					arg = current_argv[0][i + 1]
							? current_argv[0] + i + 1
							: current_argv[1];
					error_if(!arg, OPTION_REQ_ARG, flag_definitions[j].flag);
				}
				flag_definitions[j].function(config, arg);
				break;
			}
		}
		error_if(!flag_definitions[j].flag, INVALID_OPTION, current_argv[0][i]);
	}
}

void apply_input_flags(
        ping_config_t* config,
        int argc,
		char** argv) {

    for (int i = 1; i < argc; i++) {
		parse_and_apply_argv_flags(argv + i, config);
    }
}

