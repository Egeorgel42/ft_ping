#ifndef TERMINAL_OUTPUT_H
# define TERMINAL_OUTPUT_H

# define PROGRAM_NAME "ft_ping"
# define USAGE_ERR "%s: usage error: %s\n", PROGRAM_NAME
# define ICMP_TYPE_ERR "%s: Invalid ICMP type\n", PROGRAM_NAME
# define CHECKSUM_SIZE_ERR "%s: Unexpected Checksum size, data is not 2byte aligned\n", PROGRAM_NAME
# define HELP_MESSAGE \
"\n\
Usage:\n\
    %s [options] <destination>\n\
Options:\n\
    -h, -?    Show this help message\n\
    -v        Verbose output\n\
\n", PROGRAM_NAME
# define INVALID_OPTION "%s: invalid option -- '%c'\n", PROGRAM_NAME
# define ALLOC_ERR "%s: Memory allocation failed\n", PROGRAM_NAME

void error(const char* format, ...);
void print_message(const char* format, ...);

# define error_if(condition, format, ...) {\
    if (condition) { \
        error(format, ##__VA_ARGS__); \
    }} \

#endif