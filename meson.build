project(
	'waymonad-clients',
	'c',
	version: '0.0.1',
	license: 'LGPL v2',
	meson_version: '>=0.43.0',
	default_options: [
		'c_std=c11',
		'warning_level=2',
		'werror=true',
	],
)

add_project_arguments(
	'-I@0@/include'.format(meson.source_root()),
	language: 'c',
)

add_project_arguments('-Wno-unused-parameter', language: 'c')
add_project_arguments(
	'-DSRC_DIR="@0@"'.format(meson.source_root()),
	language: 'c',
)

cc = meson.get_compiler('c')

# Clang complains about some zeroed initializer lists (= {0}), even though they
# are valid
if cc.get_id() == 'clang'
	add_project_arguments('-Wno-missing-field-initializers', language: 'c')
	add_project_arguments('-Wno-missing-braces', language: 'c')
endif

wayland_client = dependency('wayland-client')
wayland_egl    = dependency('wayland-egl')
wayland_cursor = dependency('wayland-cursor')
wayland_protos = dependency('wayland-protocols', version: '>=1.12')
egl            = dependency('egl')
glesv2         = dependency('glesv2')
pixman         = dependency('pixman-1')
cairo          = dependency('cairo')
pango          = dependency('pango')
pango_cairo    = dependency('pangocairo')
xkbcommon      = dependency('xkbcommon')
magickwand     = dependency('MagickWand-6.Q16HDRI')
math           = cc.find_library('m', required: false)

subdir('protocols')
subdir('common')

subdir('background')
