#include "common/registry.h"
#include "common/buffer.h"
#include "background-client-protocol.h"
#include "shell.h"
#include "render.h"

#include <wayland-util.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void setup_surface(struct surface *surface, struct registry *registry,
                   uint32_t width, uint32_t height, int32_t scale);

struct background_shell {
	const char *bg_path;

	struct z_background *shell;
	struct wl_list surfaces;
	struct registry *reg;
};

struct back_surface {
	struct z_background_surface *back_surf;
	struct wl_list link;

	struct registry *registry;
	struct surface surface;

	struct output_state *output;
	struct background_shell *shell;
};

static void handle_output_change(struct output_state *state, void *data)
{
	struct back_surface *surface = data;

	surface->surface.width = state->width;
	surface->surface.height = state->height;
	surface->surface.scale = state->scale;
	render_background(&surface->surface, surface->shell->bg_path);
}

void repick_all(struct background_shell *shell)
{
	struct back_surface *pos;

	wl_list_for_each(pos, &shell->surfaces, link) {
		render_background(&pos->surface, shell->bg_path);
	}
}

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

static void handle_set_output(void *data, struct z_background_surface *back_surf,
                              struct wl_output *output)
{
	struct back_surface *surface = data;
	struct output_state *state = find_output(surface->registry, output);

	surface->output = state;
	surface->surface.width = state->width;
	surface->surface.height = state->height;

	state->user_data = surface;
	state->changed = handle_output_change;

	fprintf(stderr, "Handling set_output: %ux%u\n", surface->surface.width, surface->surface.height);
	render_background(&surface->surface, surface->shell->bg_path);
}

static struct z_background_surface_listener surface_listener = {
	.remove = handle_remove,
	.set_output = handle_set_output,
};

static void create_background(void *data, struct z_background *z_shell)
{
	struct background_shell *shell = data;
	struct back_surface *surface;
	surface = calloc(sizeof(struct back_surface), 1);
	surface->registry = shell->reg;
	surface->shell = shell;

	wl_list_insert(&shell->surfaces, &surface->link);
	setup_surface(&surface->surface, shell->reg, 1, 1, 1);

	surface->back_surf = z_background_get_background_surface(z_shell, surface->surface.surface);
	z_background_surface_add_listener(surface->back_surf, &surface_listener,
	                                  surface);
}

static struct z_background_listener background_listener = {
	.create_background = create_background,
};

struct background_shell *bind_shell(struct registry *registry, const char *path)
{
	struct background_shell *shell;
	if (registry->background_name == 0) {
		return NULL;
	}

	if (!(shell = calloc(sizeof(*shell), 1))) {
		fprintf(stderr, "Failed to allocate background shell: %s\n",
		        strerror(errno));
		exit(-1);
	}

	shell->reg = registry;
	shell->bg_path = path;
	shell->shell = wl_registry_bind(registry->wl_reg, registry->background_name,
	                         &z_background_interface, 1);
	z_background_add_listener(shell->shell, &background_listener, shell);
	wl_list_init(&shell->surfaces);

	return shell;
}

