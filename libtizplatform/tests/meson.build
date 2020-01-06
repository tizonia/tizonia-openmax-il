# create check_tizplatform.h
config_check_tizplatform_h = configuration_data()
config_check_tizplatform_h.set('abs_top_builddir', meson.source_root())

configure_file(input: 'check_tizplatform.h.in',
               output: 'check_tizplatform.h',
               configuration: config_check_tizplatform_h 
               )


check_tizplatform_sources = [
   'check_tizplatform.c'
]

check_tizplatform = executable(
   'check_tizplatform',
    check_tizplatform_sources,
    dependencies: [
       check_dep,
       tizilheaders_dep,
       libtizplatform_dep
    ]
)

test('check_tizplatform', check_tizplatform)
