#ifndef PARSING_H
# define PARSING_H

#include "ping.h"

const flag_t* init_flag_definition();

void apply_flags(
        ping_config_t *config,
        flag_input_t* input,
        const flag_t* flag_defs);

#endif