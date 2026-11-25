#include "ping.h"
#include "terminal_output.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

typedef const struct {
    char const*	flag;
	bool const	has_arg;
    void		(*function)(ping_config_t *config, char const* arg);
} defined_flag_t;

static void print_help(ping_config_t *config, char const* arg) {
    (void)config;
    (void)arg;
    print_message(HELP_MESSAGE);
    exit(0);
}

static void set_verbose_mode(ping_config_t *config, char const* arg) {
    (void)arg;
    config->mode = config->mode | PING_VERBOSE;
}

static void set_ttl(ping_config_t *config, char const *arg) {
	int32_t buff = 0;
	for (uint32_t i = 0; arg[i]; i++) {
		error_if(arg[i] < '0' || arg[i] > '9', USAGE_ERR, "--ttl: Only numeric characters are accepted as argument.");
		buff = buff * 10u + (arg[i] - '0');
		error_if(buff > 255, USAGE_ERR, "--ttl: parameter is not within expected range [1-255]");
	}
	error_if(buff < 1 || buff > 255, USAGE_ERR, "--ttl: parameter is not within expected range [1-255]");
	config->ttl = buff;
}

static char** parse_string_flag(char** current_argv, ping_config_t* config) {

	static defined_flag_t const string_flag_defintions[] = {
        {"ttl", true, set_ttl}, //flag, has_arg, flag_function
        {NULL, false, NULL}
	};

	size_t i = 0;
	for (;string_flag_defintions[i].flag; i++) {
		if (strcmp(string_flag_defintions[i].flag, current_argv[0] + 2) == 0) {
			char const* arg = NULL;
			if (string_flag_defintions[i].has_arg) {
				arg = current_argv[1];
				error_if(!arg, OPTION_REQ_ARG, string_flag_defintions[i].flag);
				string_flag_defintions[i].function(config, arg);
				return current_argv + 1;
			}
			string_flag_defintions[i].function(config, arg);
			return current_argv;
		}
	}
	error(INVALID_STR_OPTION, current_argv[0]);
}

static char** parse_char_flag(char** current_argv, ping_config_t* config) {

    static defined_flag_t const flag_definitions[] = {
        {"h", false, print_help}, //flag, has_arg, flag_function
        {"?", false, print_help},
        {"v", false, set_verbose_mode},
        {NULL, false, NULL}
    };

	for (size_t i = 1; current_argv[0][i]; i++) {
		size_t j = 0;
		for (; flag_definitions[j].flag; j++) {
			if (*flag_definitions[j].flag == current_argv[0][i]) {
				if (flag_definitions[j].has_arg) {
					bool last_char_arg = current_argv[0][i + 1];
					if (last_char_arg) {
						flag_definitions[j].function(config, current_argv[0] + i + 1);
						return current_argv;
					}
					else if (current_argv[1]) {
						flag_definitions[j].function(config, current_argv[1]);
						return current_argv + 1;
					}
					error(OPTION_REQ_ARG, flag_definitions[j].flag)
				}
				flag_definitions[j].function(config, NULL);
				break;
			}
		}
		error_if(!flag_definitions[j].flag, INVALID_OPTION, current_argv[0][i]);
	}
	return current_argv;
}

static char** parse_and_apply_argv_flag(char** current_argv, ping_config_t* config) {

	if (!current_argv[0]) {
		return NULL;
	}
	if (current_argv[0][0] != '-') {
		error_if(config->hostname_str, USAGE_ERR_MULT_DEST);
		config->hostname_str = current_argv[0];
		return current_argv;
	}
	error_if(current_argv[0][1] == '\0', "Expected character after '-'");
	if (current_argv[0][1] == '-') {
		return parse_string_flag(current_argv, config);
	} else {
		return parse_char_flag(current_argv, config);
	}
}

void apply_input_flags(
        ping_config_t* config,
        int argc,
		char** argv) {

	error_if(argc <= 1, MISSING_HOST_OP);
    while (argv && *(argv + 1)) {
		argv = parse_and_apply_argv_flag(argv + 1, config);
    }
	error_if(!config->hostname_str, USAGE_ERR_DEST_REQ);
}

