docs_paper = get_option('docs_paper')
paper_opt = ['-D', 'latex_paper_size='+docs_paper]
commonopts = ['-c', '@OUTDIR@', '-W', paper_opt]
allsphinxopts = ['-d', join_paths(meson.current_build_dir(), 'doctrees'), meson.current_source_dir()]
i18nsphinxopts = []

# recheck this one
#### the i18n builder cannot share the environment and doctrees with the others
###I18NSPHINXOPTS  = $(PAPEROPT_$(PAPER)) $(SPHINXOPTS) .

# ugly way to do things, this will require adding a @var@ upstream

conf_py_tmp = configure_file(
      input: 'conf.py',
      output: 'conf.py.tmp',
      copy: true
)

# for some reason '-n' prevents conf.py from being written. Absurd.
#conf_py = custom_target(
#   'conf.py',
#   input: conf_py_tmp,
#   output: 'conf.py',
#   build_by_default: true,
#   install: false,
#   command: [sed,
#     '-e', 's|"../doxygen-src/xml/"|"@OUTDIR@/../doxygen-src/xml/"|',
#     '-e', 's|\'_static\'|\'meson._static\'',
#     '-e', 'w @OUTPUT@',
#     '@INPUT@' 
#   ]
#)


foreach item: get_option('docs_options')
   if item == 'gettext'
      custom_target(item,
         build_by_default: true,
         output: item,
         command: [sphinx_build, commonopts, '-b', item, i18nsphinxopts, 'locale'],
         install: true,
         install_dir: tizdocdir
      )
   else
      custom_target(item,
         build_by_default: true,
         output: item,
         command: [sphinx_build, commonopts, '-b', item, allsphinxopts, item],
         install: true,
         install_dir: tizdocdir
      )
   endif
endforeach


if enable_test
  test(sphinx_build,
     args: [
       '-b',
       'doctest',
       commonopts,
       allsphinxopts,
       join_paths(meson.current_build_dir(), 'doctest')
     ]
  )
endif
