executable(
	'background',
	['main.c', 'shell.c', 'render.c'],
	dependencies: [wayland_client, wayland_cursor, wl_protos, pango, cairo, pango_cairo, glesv2, magickwand],
	link_with: lib_shared,
)

