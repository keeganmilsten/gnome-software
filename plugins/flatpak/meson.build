subdir('tests')

cargs = ['-DG_LOG_DOMAIN="GsPluginFlatpak"']

shared_module(
  'gs_plugin_flatpak',
  sources : [
    'gs-appstream.c',
    'gs-flatpak.c',
    'gs-flatpak-symlinks.c',
    'gs-plugin-flatpak.c'
  ],
  include_directories : [
    include_directories('../..'),
    include_directories('../../lib'),
  ],
  install : true,
  install_dir: plugin_dir,
  c_args : cargs,
  dependencies : [ plugin_libs, flatpak ]
)
metainfo = 'org.gnome.Software.Plugin.Flatpak.metainfo.xml'

i18n.merge_file(
  input: metainfo + '.in',
  output: metainfo,
  type: 'xml',
  po_dir: join_paths(meson.source_root(), 'po'),
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo')
)

if get_option('enable-tests')
  cargs += ['-DLOCALPLUGINDIR="' + meson.current_build_dir() + '"']
  cargs += ['-DLOCALPLUGINDIR_CORE="' + meson.current_build_dir() + '/../core"']
  cargs += ['-DTESTDATADIR="' + join_paths(meson.current_build_dir(), 'tests') + '"']
  e = executable('gs-self-test-flatpak',
    sources : [
      'gs-self-test.c'
    ],
    include_directories : [
      include_directories('../..'),
      include_directories('../../lib'),
    ],
    dependencies : [
      plugin_libs,
    ],
    link_with : [
      libgnomesoftware
    ],
    c_args : cargs,
  )
  test('gs-self-test-flatpak', e)
endif
