#ifndef PARSING_H
# define PARSING_H

#include "ping.h"

void apply_input_flags(
        ping_config_t* config,
        int argc,
		char** argv);

#endif