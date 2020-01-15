if get_option('bashcompletiondir') != ''
   bashcompletiondir = get_option('bashcompletiondir')
else
   bashcompletion = dependency('bash-completion', required: false)
   if bashcompletion.found()
      bashcompletiondir = bashcompletion.get_pkgconfig_variable('completionsdir')
   else
      bashcompletiondir = join_paths(datadir, 'bash-completion', 'completions')
   endif
endif


if get_option('zshcompletiondir') != ''
   zshcompletiondir = get_option('zshcompletiondir')
else
   zshcompletiondir = join_paths(datadir, 'zsh', 'vendor-completions')
endif


install_data(
   'tizonia',
   install_dir: bashcompletiondir
)


install_data(
   '_tizonia',
   install_dir: zshcompletiondir
)


desktop_dir = join_paths(datadir, 'applications')
icons_dir = join_paths(datadir, 'icons', 'hicolor', '256x256', 'apps')


# create tizonia.desktop
config_desktop = configuration_data()
config_desktop.set('bindir', bindir)

configure_file(input: 'tizonia.desktop.in',
               output: 'tizonia.desktop',
               configuration: config_desktop,
               install_dir: desktop_dir
               )

# create tizonia-remote
config_remote = configuration_data()
config_remote.set('PACKAGE_VERSION', tizversion)

configure_file(input: 'tizonia-remote.in',
               output: 'tizonia-remote',
               configuration: config_remote,
               install_dir: bindir,
               install_mode: 'rwxr-xr-x'
               )


install_data(
   'icons/256x256/tizonia.png',
   install_dir: icons_dir
)
