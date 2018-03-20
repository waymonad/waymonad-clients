#define _DEFAULT_SOURCE

#include <sys/signalfd.h>

#include <signal.h>
#include <poll.h>
#include <unistd.h>

#include "common/window.h"
#include "common/window.h"
#include "common/registry.h"
#include "common/cairo.h"
#include "common/list.h"
#include "shell.h"
#include "render.h"

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

struct background_state {
	struct window *xdg_window;
	struct background_shell *bg_shell;
};

__attribute__((nonnull(1)))
static void handle_configure(struct window *window)
{
	render_background(&window->surface, window->userdata);
}

static void init_rng()
{
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	srand(time.tv_nsec);
}

static int make_sigfd()
{
	int ret;
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
		fprintf(stderr, "Failed to block signals: %s\n", strerror(errno));
		exit(-1);
	}

	if ((ret = signalfd(-1, &mask, 0)) < 0) {
		fprintf(stderr, "Failed to create signalfd: %s\n", strerror(errno));
		exit(-1);
	}

	return ret;
}

static void handle_signal(int sfd, struct background_state *state)
{
	struct signalfd_siginfo fdsi;

	if (read(sfd, &fdsi, sizeof(fdsi)) != sizeof(fdsi)) {
		fprintf(stderr, "Failed to read from signalfd: %s\n", strerror(errno));
		return;
	}

	if (state->xdg_window) {
		handle_configure(state->xdg_window);
	} else if (state->bg_shell) {
		repick_all(state->bg_shell);
	}
}

static void main_loop(struct wl_display *display, struct background_state *state)
{
	bool cont = true;
	struct pollfd fds[2] =
		{	{ .fd = wl_display_get_fd(display),
			  .events = POLLIN,
			  .revents = 0
			},
			{ .fd = make_sigfd(),
			  .events = POLLIN,
			  .revents = 0
			},
		};

	while (cont) {
		wl_display_flush(display);
		if (poll(fds, 2, -1) < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				fprintf(stderr, "poll failed: %s\n",
				        strerror(errno));
				exit(-2);
			}
		}

		if (fds[0].revents) {
			if (wl_display_dispatch(display) < 0) {
				cont = false;
			}
			fds[0].revents = 0;
		}
		if (fds[1].revents) {
			handle_signal(fds[1].fd, state);
			fds[1].revents = 0;
		}
	}
}

int main(int argc, char **argv)
{
	struct background_state state = { 0 };
	const char *path;

	registry = registry_poll();
	init_rng();

	if (argc < 2) {
		path = ".";
	} else {
		path = argv[1];
	}

	state.bg_shell = bind_shell(registry, path);

	if (!state.bg_shell) {
		state.xdg_window = xdg_window_setup(registry, 1280, 720, 1);
		state.xdg_window->handle_configure = handle_configure;
		state.xdg_window->userdata = (char *)path;
	}

	MagickWandGenesis();
	main_loop(registry->display, &state);
	MagickWandTerminus();

	return 0;
}
