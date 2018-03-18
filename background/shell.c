#include "common/registry.h"
#include "common/buffer.h"
#include "background-client-protocol.h"

#include <wayland-util.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static const char *bg_path;
void setup_surface(struct surface *surface, struct registry *registry,
                   uint32_t width, uint32_t height, int32_t scale);

static struct z_background *shell;
static struct wl_list surfaces;

struct back_surface {
	struct z_background_surface *back_surf;
	struct wl_list link;

	struct registry *registry;
	struct surface surface;

	struct output_state *output;
};

static void handle_remove(void *data, struct z_background_surface *back_surf)
{
	struct back_surface *surface = data;

	wl_list_remove(&surface->link);
	z_background_surface_destroy(back_surf);

	if (surface->output && surface->output->user_data == surface) {
		surface->output->changed = NULL;
	}

	// FIXME: Cleanup the surface
	free(surface);
}

void render_background(struct surface *restrict surface, const char *restrict path);
static void handle_set_output(void *data, struct z_background_surface *back_surf,
                              struct wl_output *output)
{
	struct back_surface *surface = data;
	struct output_state *state = find_output(surface->registry, output);

	surface->output = state;
	surface->surface.width = state->width;
	surface->surface.height = state->height;
	state->user_data = surface;

	fprintf(stderr, "Handling set_output: %ux%u\n", surface->surface.width, surface->surface.height);
	render_background(&surface->surface, bg_path);
}

static struct z_background_surface_listener surface_listener = {
	.remove = handle_remove,
	.set_output = handle_set_output,
};

static void create_background(void *data, struct z_background *shell)
{
	struct registry *reg = data;
	struct back_surface *surface;
	surface = calloc(sizeof(struct back_surface), 1);
	surface->registry = reg;

	wl_list_insert(&surfaces, &surface->link);
	setup_surface(&surface->surface, reg, 1, 1, 1);

	surface->back_surf = z_background_get_background_surface(shell, surface->surface.surface);
	z_background_surface_add_listener(surface->back_surf, &surface_listener,
	                                  surface);
}

static struct z_background_listener background_listener = {
	.create_background = create_background,
};

void bind_shell(struct registry *registry, const char *path)
{
	assert(registry->background_name != 0);
	bg_path = path;
	shell = wl_registry_bind(registry->wl_reg, registry->background_name,
	                         &z_background_interface, 1);
	z_background_add_listener(shell, &background_listener, registry);
	wl_list_init(&surfaces);
}

