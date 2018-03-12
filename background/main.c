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

__attribute__((nonnull(5, 6)))
static void get_sizes(size_t width, size_t height,
                      size_t target_width, size_t target_height,
                      size_t *cal_width, size_t *cal_height)
{
	double wfactor;
	double hfactor;
	double factor;

	if (width < target_width && height < target_height) {
		/* Ratio preserving 2D blowup */
		wfactor = target_width / (double) width;
		hfactor = target_height / (double) height;
	} else if (width > target_width && height > target_height) {
		/* Ratio preserving 2D shrink */
		wfactor = target_width / (double) width;
		hfactor = target_height / (double) height;
	} else if (width > target_width) {
		factor = target_width / (double) width;
		*cal_width = factor * width;
		*cal_height = factor * height;
		return;
	} else if (height > target_height) {
		factor = target_height / (double) height;
		*cal_width = factor * width;
		*cal_height = factor * height;
		return;
	} else if (width == target_width && height == target_height) {
		/* Already fits */
		*cal_width = width;
		*cal_height = height;
		return;
	}

	if (abs(wfactor - hfactor) < 0.001) {
		*cal_width = target_width;
		*cal_height = target_height;
		return;
	}

	factor = wfactor < hfactor ? wfactor : hfactor;
	*cal_width = factor * width;
	*cal_height = factor * height;
	return;
}

__attribute__((nonnull(1))) __attribute__((warn_unused_result))
static cairo_surface_t *get_image(const char *restrict path,
                                  size_t width, size_t height)
{
	cairo_surface_t *ret;
	MagickWand *wand;
	MagickWand *wand_blur;
	PixelWand *pwand;
	unsigned char *data;
	size_t cal_width, cal_height;
	size_t offset_x, offset_y;

	wand = NewMagickWand();

	if (!MagickReadImage(wand, path)) {
		fprintf(stderr, "Failed to read image\n");
		DestroyMagickWand(wand);
		return NULL;
	}
	wand_blur = NewMagickWand();

	get_sizes(MagickGetImageWidth(wand), MagickGetImageHeight(wand),
	          width, height, &cal_width, &cal_height);

	if (cal_width != width || cal_height != height) {
		/* If either size is smaller, we need to do the blurry stuff */
		pwand = NewPixelWand();
		MagickNewImage(wand_blur, MagickGetImageWidth(wand), MagickGetImageHeight(wand), pwand);
		DestroyPixelWand(pwand);
		MagickCompositeImage(wand_blur, wand, OverCompositeOp, 0, 0);

		MagickResizeImage(wand_blur, width, height, GaussianFilter, 20);
	} else {
		/* Otherwise just set things up simliar to be lazy later */
		pwand = NewPixelWand();
		MagickNewImage(wand_blur, width, height, pwand);
		DestroyPixelWand(pwand);
	}

	if (!MagickResizeImage(wand, cal_width, cal_height, QuadraticFilter, 1)) {
		fprintf(stderr, "Failed to scale image non-blurry\n");
	}

	offset_x = (width - cal_width) / 2;
	offset_y = (height - cal_height) / 2;

	if (!MagickCompositeImage(wand_blur, wand, OverCompositeOp, offset_x, offset_y)) {
		fprintf(stderr, "Failed to composit the non-blured over the blurred\n");
		DestroyMagickWand(wand);
		DestroyMagickWand(wand_blur);
		return NULL;
	}

	ret = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
	                                 MagickGetImageWidth(wand_blur),
	                                 MagickGetImageHeight(wand_blur));

	data = cairo_image_surface_get_data(ret);
	MagickExportImagePixels(wand_blur, 0, 0, MagickGetImageWidth(wand_blur),
	                        MagickGetImageHeight(wand_blur), "BGRA", CharPixel,
	                        data);
	
	DestroyMagickWand(wand);
	DestroyMagickWand(wand_blur);

	cairo_surface_mark_dirty(ret);
	return ret;
}

__attribute__((nonnull(1)))
static void render_fallback(struct window *restrict window)
{
	window_prerender(window);
	cairo_set_source_u32(window->cairo, 0x000000FF);
	cairo_paint(window->cairo);
	window_render(window);
}

__attribute__((nonnull(1, 2)))
static void render_image(struct window *restrict window, const char *restrict path)
{
	cairo_surface_t *surface;

	window_prerender(window);
	surface = get_image(path, window->width, window->height);
	cairo_set_source_surface(window->cairo, surface, 0, 0);
	cairo_paint(window->cairo);
	cairo_surface_destroy(surface);
	window_render(window);
}

__attribute__((nonnull(1))) __attribute((warn_unused_result))
static bool is_file_suitable(const char *restrict file, size_t width, size_t height)
{
	size_t iwidth, iheight;
	size_t comp;
	MagickWand *wand;
	wand = NewMagickWand();
	if (!MagickPingImage(wand, file)) {
		fprintf(stderr, "Failed to read file: '%s'\n", file);
		return false;
	}

	iwidth = MagickGetImageWidth(wand);
	iheight = MagickGetImageHeight(wand);
	DestroyMagickWand(wand);

	if (iwidth == 0 || iheight == 0) {
		fprintf(stderr, "Couldn't read file: %s\n", file);
		return false;
	}

	comp = (size_t)(100 * (width * iheight)) / (size_t)(height * iwidth);

	return comp > 90 && comp < 112;
}

__attribute__((nonnull(1))) __attribute((warn_unused_result))
static list_t *find_candidates(const char *restrict dir_path,
                               size_t width, size_t height, bool recursive)
{
	list_t *sub;
	list_t *ret = create_list();
	char path_buffer[4096] = { 0 };
	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir(dir_path))) {
		fprintf(stderr, "Failed to open directory: %s\n", strerror(errno));
		list_foreach(ret, free);
		list_free(ret);
		NULL;
	}

	while ((entry = readdir(dir))) {
		if (entry->d_name[0] == '.') {
			continue;
		}
		switch (entry->d_type) {
		case DT_DIR: // TODO: handle recursive
			if (!recursive) {
				break;
			}
			snprintf(path_buffer, sizeof(path_buffer), "%s/%s", dir_path, entry->d_name);
			sub = find_candidates(path_buffer, width, height, true);
			if (!sub) {
				fprintf(stderr, "Failed to read subdir: %s\n",
				        path_buffer);
				break;
			}
			list_append(ret, sub);
			list_free(sub);
			break;
		case DT_REG:
			snprintf(path_buffer, sizeof(path_buffer), "%s/%s", dir_path, entry->d_name);
			if (is_file_suitable(path_buffer, width, height)) {
				list_add(ret, strdup(path_buffer));
			}
			break;
		default:
			break;
		}

		errno = 0;
	}

	if (errno) {
		fprintf(stderr, "Failed to get directory entry: %s\n", strerror(errno));
		closedir(dir);
		list_foreach(ret, free);
		list_free(ret);
		return NULL;
	}

	closedir(dir);
	return ret;
}

__attribute__((nonnull(1)))
static void handle_configure(struct window *window)
{
	int index;
	list_t *candidates = find_candidates(window->userdata, window->width, window->height, true);


	if (!candidates || !candidates->length) {
		fprintf(stderr, "Couldn't find a suitable candidate\n");
		render_fallback(window);
	} else {
		index = rand() % candidates->length;
		render_image(window, candidates->items[index]);
	}

	if (candidates) {
		list_foreach(candidates, free);
		list_free(candidates);
	}
}

static void init_rng()
{
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	srand(time.tv_nsec);
	registry = registry_poll();
}

int main(int argc, char **argv)
{
	struct window *window;
	init_rng();

	window = xdg_window_setup(registry, 1280, 720, 1);
	window->handle_configure = handle_configure;

	if (argc < 2) {
		window->userdata = ".";
	} else {
		window->userdata = argv[1];
	}

	MagickWandGenesis();
	wl_display_roundtrip(registry->display);
	while (wl_display_dispatch(registry->display) != -1) {
		/* NOOP */
	}
	MagickWandTerminus();

	return 0;
}
