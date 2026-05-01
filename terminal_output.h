#ifndef TERMINAL_OUTPUT_H
# define TERMINAL_OUTPUT_H

# define PROGRAM_NAME "ft_ping"
# define ICMP_TYPE_ERR "Invalid ICMP type"
# define CHECKSUM_SIZE_ERR "Unexpected Checksum size, data is not 2byte aligned"

void error(const char* message);

#endif