#include <types.h>
#include <malloc.h>
#include <utils/ringbuffer.h>

RingBuffer* ring_buffer_new(size_t len)
{
    RingBuffer *rb = (RingBuffer *)malloc(sizeof(RingBuffer));
    if(rb==NULL) return NULL;

    rb->buf = (char *)malloc(len);
    if(rb->buf==NULL) {
        free(rb);
        return NULL;
    }

    rb->len = len;
    rb->read_ptr = 0;
    rb->write_ptr = 0;
    rb->refcount = 1;
    return rb;
}

void ring_buffer_unref(RingBuffer *rb)
{
    --rb->refcount;
    if(rb->refcount==0) free(rb);
}

void ring_buffer_ref(RingBuffer *rb)
{
    ++rb->refcount;
}

/**
 * Push the given char into the ring-buffer
 */
void ring_buffer_push(RingBuffer *rb, char ch)
{
    if(!rb) {
        kputs("ERROR ring_buffer_push called with null buffer!\r\n");
        return 0;
    }
    rb->buf[rb->write_ptr] = ch;
    ++rb->write_ptr;
    if(rb->write_ptr >= rb->len) rb->write_ptr = 0;
}

char ring_buffer_empty(RingBuffer *rb)
{
    return rb->read_ptr==rb->write_ptr;    
}

/**
 * Pop out the next char from the ring-buffer
 */
char ring_buffer_pop(RingBuffer *rb)
{
    if(!rb) {
        kputs("ERROR ring_buffer_pop called with null buffer!\r\n");
        return 0;
    }
    char ch;
    if(rb->read_ptr==rb->write_ptr) {
        return 0;
    } else {
        ch = rb->buf[rb->read_ptr];
        ++rb->read_ptr;
        if(rb->read_ptr>=rb->len) rb->read_ptr = 0;
        return ch;
    }
}

