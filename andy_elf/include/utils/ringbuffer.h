#include <types.h>

#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H

typedef struct RingBuffer {
    char *buf;
    size_t len;
    size_t read_ptr;
    size_t write_ptr;
    size_t refcount;
} RingBuffer;


RingBuffer* ring_buffer_new(size_t len);
void ring_buffer_unref(RingBuffer *rb);
void ring_buffer_ref(RingBuffer *rb);
/**
 * Push the given char into the ring-buffer
 */
void ring_buffer_push(RingBuffer *rb, char ch);
char ring_buffer_empty(RingBuffer *rb);
/**
 * Pop out the next char from the ring-buffer
 */
char ring_buffer_pop(RingBuffer *rb);

#endif