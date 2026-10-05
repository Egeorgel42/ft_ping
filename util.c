#include "util.h"
#include "compiler.h"
#include "terminal_output.h"
#include <string.h>

circular_buffer_t create_circular_buffer(uint32_t element_size, uint32_t buffer_size) {
    libassert(element_size > 0, "create_circular_buffer: element_size must be greater than 0.");
    libassert(buffer_size > 0, "create_circular_buffer: buffer_size must be greater than 0.");

    uint32_t const slot_size = element_size + sizeof(bool); // +1 byte to check for slot usage
    libassert(slot_size > element_size, "element_size is too large.");

    uint64_t const alloc_size = slot_size * buffer_size;
    circular_buffer_t buffer = {
        .buffer = malloc(alloc_size),
        .element_size = slot_size,
        .buffer_size = buffer_size
    };
    error_if(!buffer.buffer, ALLOC_ERR);
    return buffer;
}

static void realloc_circular_buffer(circular_buffer_t* buffer) {
    uint32_t const slot_size = buffer->element_size;
    uint64_t const alloc_size = slot_size * buffer->buffer_size * 2;
    buffer->buffer = realloc(buffer->buffer, alloc_size);
    error_if(!buffer->buffer, ALLOC_ERR);
    buffer->buffer_size = alloc_size;
}

void write_element_at_index(circular_buffer_t* buffer, uint32_t index, void const* element) {
    uint32_t const pos_in_buff = index % buffer->buffer_size;
    uint8_t* slot_addr = buffer->buffer + (buffer->element_size * pos_in_buff);
    bool const is_slot_used = *(bool*)slot_addr;
    if (BRANCH_UNLIKELY(pos_in_buff == 0 && is_slot_used)) {
        realloc_circular_buffer(buffer);
        write_element_at_index(buffer, index, element);
        return;
    }
    error_if(is_slot_used, CIRC_BUFF_WRITE_FULL_SLOT_ERR, index);
    *(bool*)slot_addr = true;
    slot_addr++;
    memcpy(slot_addr, element, buffer->element_size - sizeof(bool));
}

void const* read_element_at_index(circular_buffer_t* buffer, uint32_t index) {
    uint8_t* slot_addr = buffer->buffer + (buffer->element_size * (index % buffer->buffer_size));
    bool const is_slot_used = *(bool*)slot_addr;
    error_if(!is_slot_used, CIRC_BUFF_READ_EMPTY_SLOT_ERR, index);
    slot_addr++;
    return slot_addr;
}

void const* read_element_at_index_and_pop(circular_buffer_t* buffer, uint32_t index) {
    uint8_t* slot_addr = buffer->buffer + (buffer->element_size * (index % buffer->buffer_size));
    bool const is_slot_used = *(bool*)slot_addr;
    error_if(!is_slot_used, CIRC_BUFF_READ_EMPTY_SLOT_ERR, index);
    *(bool*)slot_addr = false;
    slot_addr++;
    return slot_addr;
}

void pop_element_at_index(circular_buffer_t* buffer, uint32_t index) {
    uint8_t* slot_addr = buffer->buffer + (buffer->element_size * (index % buffer->buffer_size));
    bool const is_slot_used = *(bool*)slot_addr;
    error_if(!is_slot_used, CIRC_BUFF_POP_EMPTY_SLOT_ERR, index);
    *(bool*)slot_addr = false;
}

void free_circular_buffer(circular_buffer_t* buffer) {
    free(buffer->buffer);
}