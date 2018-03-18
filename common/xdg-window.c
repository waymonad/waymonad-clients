#include <wayland-client.h>
#include <wayland-cursor.h>
#include "xdg-shell-client-protocol.h"
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include "common/window.h"
#include "common/buffer.h"
#include "common/list.h"
//#include "log.h"

static void pointer_handle_enter(void *data, struct wl_pointer *pointer,
		     uint32_t serial, struct wl_surface *surface, wl_fixed_t sx_w, wl_fixed_t sy_w) {
	struct window *window = data;
	if (window->registry->pointer) {
		struct wl_cursor_image *image = window->cursor.cursor->images[0];
		wl_pointer_set_cursor(pointer, serial, window->cursor.surface, image->hotspot_x, image->hotspot_y);
	}
}

static void pointer_handle_leave(void *data, struct wl_pointer *pointer,
		     uint32_t serial, struct wl_surface *surface) {
}

static void pointer_handle_motion(void *data, struct wl_pointer *pointer,
		      uint32_t time, wl_fixed_t sx_w, wl_fixed_t sy_w) {
	struct window *window = data;

	window->pointer_input.last_x = wl_fixed_to_int(sx_w);
	window->pointer_input.last_y = wl_fixed_to_int(sy_w);
}

static void pointer_handle_button(void *data, struct wl_pointer *pointer, uint32_t serial,
		      uint32_t time, uint32_t button, uint32_t state_w) {
	struct window *window = data;
	struct pointer_input *input = &window->pointer_input;

	if (window->pointer_input.notify_button) {
		window->pointer_input.notify_button(window, input->last_x, input->last_y, button, state_w);
	}
}

static void pointer_handle_axis(void *data, struct wl_pointer *pointer,
				uint32_t time, uint32_t axis, wl_fixed_t value) {
	struct window *window = data;
	enum scroll_direction direction;

	switch (axis) {
	case 0:
		direction = wl_fixed_to_double(value) < 0 ? SCROLL_UP : SCROLL_DOWN;
		break;
	case 1:
		direction = wl_fixed_to_double(value) < 0 ? SCROLL_LEFT : SCROLL_RIGHT;
		break;
	default:
		//sway_log(L_DEBUG, "Unexpected axis value on mouse scroll");
		return;
	}

	if (window->pointer_input.notify_scroll) {
		window->pointer_input.notify_scroll(window, direction);
	}
}

static const struct wl_pointer_listener pointer_listener = {
	.enter = pointer_handle_enter,
	.leave = pointer_handle_leave,
	.motion = pointer_handle_motion,
	.button = pointer_handle_button,
	.axis = pointer_handle_axis
};

static void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel,
                                   int32_t width, int32_t height,
                                   struct wl_array *states)
{
	struct window *window = data;

	if (height != 0) {
		window->next_state.height = height;
	}
	
	if (width != 0) {
		window->next_state.width = width;
	}
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	exit(0);
}

static const struct xdg_toplevel_listener toplevel_listener = {
	.configure = xdg_toplevel_configure,
	.close = xdg_toplevel_close,
};

void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
	static bool mapped = false;
	bool notify;
	struct window *window = data;

	notify = window->surface.width != window->next_state.width || window->surface.height != window->next_state.height;
	window->surface.width = window->next_state.width;
	window->surface.height = window->next_state.height;

	xdg_surface_ack_configure(xdg_surface, serial);

	if ((notify || !mapped) && window->handle_configure) {
		mapped = true;
		window->handle_configure(window);
	}
}

static const struct xdg_surface_listener surface_listener = {
	.configure = xdg_surface_configure
};

void window_make_shell(struct window *window) {
	window->xdg_surface = xdg_wm_base_get_xdg_surface(window->registry->xdg_shell, window->surface.surface);
	xdg_surface_add_listener(window->xdg_surface, &surface_listener, window);

	window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
	xdg_toplevel_add_listener(window->xdg_toplevel, &toplevel_listener, window);
}

void setup_surface(struct surface *surface, struct registry *registry,
                   uint32_t width, uint32_t height, int32_t scale)
{
	surface->height = height;
	surface->width = width;
	surface->scale = scale;
	surface->registry = registry;

	surface->surface = wl_compositor_create_surface(registry->compositor);

	surface_next_buffer(surface);
}

struct window *xdg_window_setup(struct registry *registry,
                                uint32_t width, uint32_t height, int32_t scale)
{
	struct window *window = malloc(sizeof(struct window));
	memset(window, 0, sizeof(struct window));
	window->next_state.height = height;
	window->next_state.width = width;
	setup_surface(&window->surface, registry, width, height, scale);
	window->registry = registry;
	window->font = "monospace 10";

	window_make_shell(window);
	wl_surface_commit(window->surface.surface);

	if (registry->pointer) {
		wl_pointer_add_listener(registry->pointer, &pointer_listener, window);
	}

	get_next_buffer(window);

	if (registry->pointer) {
		char *cursor_theme = getenv("SWAY_CURSOR_THEME");
		if (!cursor_theme) {
			cursor_theme = "default";
		}
		char *cursor_size = getenv("SWAY_CURSOR_SIZE");
		if (!cursor_size) {
			cursor_size = "16";
		}

		//sway_log(L_DEBUG, "Cursor scale: %d", scale);
		window->cursor.cursor_theme = wl_cursor_theme_load(cursor_theme,
				atoi(cursor_size) * scale, registry->shm);
		window->cursor.cursor = wl_cursor_theme_get_cursor(window->cursor.cursor_theme, "left_ptr");
		window->cursor.surface = wl_compositor_create_surface(registry->compositor);

		struct wl_cursor_image *image = window->cursor.cursor->images[0];
		struct wl_buffer *cursor_buf = wl_cursor_image_get_buffer(image);
		wl_surface_attach(window->cursor.surface, cursor_buf, 0, 0);
		wl_surface_set_buffer_scale(window->cursor.surface, scale);
		wl_surface_damage(window->cursor.surface, 0, 0,
				image->width, image->height);
		wl_surface_commit(window->cursor.surface);
	}

	return window;
}

static void frame_callback(void *data, struct wl_callback *callback, uint32_t time) {
	struct surface *surface = data;
	wl_callback_destroy(callback);
	surface->frame_cb = NULL;
}

static const struct wl_callback_listener listener = {
	frame_callback
};

int surface_prerender(struct surface *surface) {
	if (surface->frame_cb) {
		return 0;
	}

	surface_next_buffer(surface);
	return 1;
}

int surface_render(struct surface *surface)
{
	surface->frame_cb = wl_surface_frame(surface->surface);
	wl_callback_add_listener(surface->frame_cb, &listener, surface);

	wl_surface_attach(surface->surface, surface->buffer->buffer, 0, 0);
	wl_surface_set_buffer_scale(surface->surface, surface->scale);
	wl_surface_damage(surface->surface, 0, 0, surface->width, surface->height);
	wl_surface_commit(surface->surface);

	return 1;

}

int window_prerender(struct window *window) {
	return surface_prerender(&window->surface);
}

int window_render(struct window *window) {
	return surface_render(&window->surface);
}

void window_teardown(struct window *window) {
	// TODO
}
