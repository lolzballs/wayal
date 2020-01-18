project(
  'wayal',
  'c',
  version: '0.1',
  license: 'ISC',
  default_options: [
    'c_std=c11',
    'warning_level=2',
    'werror=true',
  ]
)

add_project_arguments(
  [
    '-Wno-unused-parameter',
  ],
  language: 'c',
)

cc = meson.get_compiler('c')

cairo = dependency('cairo')
xkbcommon = dependency('xkbcommon')
wayland_protos = dependency('wayland-protocols', version: '>=1.13')
wl_protocol_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')
wayland_scanner = find_program('wayland-scanner')
wayland_client = dependency('wayland-client')

wayland_scanner_code = generator(
  wayland_scanner,
  output: '@BASENAME@-protocol.c',
  arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_client = generator(
  wayland_scanner,
  output: '@BASENAME@-client-protocol.h',
  arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)

client_protocols = [
  [wl_protocol_dir + '/stable/xdg-shell', 'xdg-shell.xml'],
  [meson.source_root() + '/protocols', 'wlr-layer-shell-unstable-v1.xml'],
]

foreach p : client_protocols
  xml = join_paths(p)
  src = wayland_scanner_code.process(xml)
  header = wayland_scanner_client.process(xml)

  name = p[1].split('.')[0].underscorify()

  lib = static_library(
    name,
    [src, header],
    dependencies: [wayland_client],
  )

  dep = declare_dependency(
    link_with: lib,
    sources: header,
  )

  set_variable(name, dep)
endforeach

src_files = files(
  'src/wayal.c',
  'src/input.c'
)

executable(
  'wayal',
  src_files,
  dependencies: [cairo, xkbcommon, wayland_client, wlr_layer_shell_unstable_v1, xdg_shell],
  include_directories: [include_directories('include')],
  install: true
)

