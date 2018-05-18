#include "common/buffer.h"
#include "render.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include <common/registry.h>
#include <stdlib.h>

void setup_surface(struct surface *surface, struct registry *registry,
                   uint32_t width, uint32_t height, int32_t scale);

struct layer_surface {
	struct surface surface;
	const char *path;
};

static void handle_layer_surface_configure(void *data,
                                           struct zwlr_layer_surface_v1 *layer_surface,
                                           uint32_t serial,
                                           uint32_t width, uint32_t height)
{
	fprintf(stderr, "Handling a layer surface configure: %ux%u\n",
	        width, height);
	struct layer_surface *surface = data;
	surface->surface.width = width;
	surface->surface.height = height;

	zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
	render_background(&surface->surface, surface->path);
}

static void handle_layer_surface_close(void *data, struct zwlr_layer_surface_v1 *layer_surface)
{	/*TODO: finish/free the actual surface */
	struct layer_surface *surface = data;
	zwlr_layer_surface_v1_destroy(layer_surface);
	free(surface);
}

static const struct zwlr_layer_surface_v1_listener surface_listener = {
	.configure = handle_layer_surface_configure,
	.closed = handle_layer_surface_close,
};


static void surface_for_output(struct registry *registry,
                               const char *path, struct output_state *output)
{
	fprintf(stderr, "Creating a layer surface for an output\n");
	struct layer_surface *surface = calloc(1, sizeof(*surface));
	surface->path = path;
	setup_surface(&surface->surface, registry, output->width, output->height, output->scale);
	struct zwlr_layer_surface_v1 *layer_surface =
		zwlr_layer_shell_v1_get_layer_surface(registry->layer_shell, surface->surface.surface, output->output, ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND, "background");

	zwlr_layer_surface_v1_add_listener(layer_surface, &surface_listener, surface);
	zwlr_layer_surface_v1_set_anchor(layer_surface,
	                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
	                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
	                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
	                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP);
	zwlr_layer_surface_v1_set_size(layer_surface, 0, 0);
	zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, -1);
	zwlr_layer_surface_v1_set_margin(layer_surface, 0, 0, 0, 0);
	zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface, false);
	wl_surface_commit(surface->surface.surface);
}

void user_layer_shell(struct registry *registry, const char *path)
{
	fprintf(stderr, "Using layer shell\n");
	for (size_t i = 0; i < registry->outputs->length; ++i) {
		surface_for_output(registry, path,
		                   registry->outputs->items[i]);
	}
}
