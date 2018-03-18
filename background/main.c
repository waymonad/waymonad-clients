#define _DEFAULT_SOURCE

#include "common/window.h"
#include "common/window.h"
#include "common/registry.h"
#include "common/cairo.h"
#include "common/list.h"

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

#include <GLES2/gl2.h>
#include <wayland-client.h>

#include <cairo.h>
#include <wand/MagickWand.h>

struct registry *registry;

// void render_background(struct surface *restrict surface, const char *restrict path);
// __attribute__((nonnull(1)))
// static void handle_configure(struct window *window)
// {
// 	render_background(&window->surface, window->userdata);
// }

static void init_rng()
{
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	srand(time.tv_nsec);
}

void bind_shell(struct registry *registry, const char *path);
int main(int argc, char **argv)
{
	const char *path;
	//struct window *window;
	registry = registry_poll();
	init_rng();

	if (argc < 2) {
		path = ".";
	} else {
		path = argv[1];
	}

	bind_shell(registry, path);

	//window = xdg_window_setup(registry, 1280, 720, 1);
	//window->handle_configure = handle_configure;
	//window->userdata = path;

	MagickWandGenesis();
	wl_display_roundtrip(registry->display);
	while (wl_display_dispatch(registry->display) != -1) {
		/* NOOP */
	}
	MagickWandTerminus();

	return 0;
}
