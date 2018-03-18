#ifndef _CLIENT_H
#define _CLIENT_H

#include <wayland-client.h>
#include "desktop-shell-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <stdbool.h>
#include "list.h"
#include "common/registry.h"

struct window;

struct buffer {
	struct wl_buffer *buffer;
	cairo_surface_t *surface;
	cairo_t *cairo;
	PangoContext *pango;
	uint32_t width, height;
	bool busy;
};

struct cursor {
	struct wl_surface *surface;
	struct wl_cursor_theme *cursor_theme;
	struct wl_cursor *cursor;
	struct wl_pointer *pointer;
};

enum scroll_direction {
	SCROLL_UP,
	SCROLL_DOWN,
	SCROLL_LEFT,
	SCROLL_RIGHT,
};

struct pointer_input {
	int last_x;
	int last_y;

	void (*notify_button)(struct window *window, int x, int y, uint32_t button, uint32_t state_w);
	void (*notify_scroll)(struct window *window, enum scroll_direction direction);
};

struct surface {
	struct registry *registry;
	struct buffer buffers[2];
	struct buffer *buffer;
	struct wl_surface *surface;
	struct wl_callback *frame_cb;

	uint32_t width, height;
	int32_t scale;
	cairo_t *cairo;
};

struct window {
	struct registry *registry;
	struct surface surface;
	struct cursor cursor;
	char *font;
	struct pointer_input pointer_input;

	union {
		struct wl_shell_surface *shell_surface;
		struct {
			struct xdg_surface *xdg_surface;
			struct xdg_toplevel *xdg_toplevel;
			struct {
				uint32_t width;
				uint32_t height;
			} next_state;
		};
	};

	void (*handle_configure)(struct window *window);
	void *userdata;
};

struct window *window_setup(struct registry *registry, uint32_t width, uint32_t height,
		int32_t scale, bool shell_surface);
void window_teardown(struct window *state);
int window_prerender(struct window *state);
int window_render(struct window *state);
void window_make_shell(struct window *window);
struct window *xdg_window_setup(struct registry *registry,
                                uint32_t width, uint32_t height, int32_t scale);

#endif
