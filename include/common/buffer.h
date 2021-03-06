#ifndef _BUFFER_H
#define _BUFFER_H

#include "common/window.h"

struct buffer *get_next_buffer(struct window *state);
struct buffer *surface_next_buffer(struct surface *surface);

#endif
