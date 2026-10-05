#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

typedef struct {
    uint8_t* buffer;
    uint32_t buffer_size;
    uint32_t const element_size;

} circular_buffer_t;

circular_buffer_t create_circular_buffer(uint32_t element_size, uint32_t buffer_size);
void write_element_at_index(circular_buffer_t* buffer, uint32_t index, void const* element);
void const* read_element_at_index(circular_buffer_t* buffer, uint32_t index);
void const* read_element_at_index_and_pop(circular_buffer_t* buffer, uint32_t index);
void pop_element_at_index(circular_buffer_t* buffer, uint32_t index);
void free_circular_buffer(circular_buffer_t* buffer);

#include "terminal_output.h"

#define CIRC_BUFF_WRITE_FULL_SLOT_ERR    PROGRAM_PREFIX_MESSAGE "Attempted to write to non-empty slot in circular buffer at index %d\n"
#define CIRC_BUFF_POP_EMPTY_SLOT_ERR     PROGRAM_PREFIX_MESSAGE "Attempted to pop empty slot in circular buffer at index %d\n"
#define CIRC_BUFF_READ_EMPTY_SLOT_ERR    PROGRAM_PREFIX_MESSAGE "Attempted to read from empty slot in circular buffer at index %d\n"

#endif