lib_shared = static_library(
	'common',
	['buffers.c', 'cairo.c', 'pango.c', 'registry.c', 'list.c', 'xdg-window.c'],
	dependencies: [cairo, pango, wl_protos, xkbcommon]
	#include_directories: include_directories('support')
)
