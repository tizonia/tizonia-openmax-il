# perhaps move this stuff to the main enable_docs?
doxygen = find_program('doxygen', required: true)
sphinx_build = find_program('sphinx-build', required: true)

# this is temporary until upstream is patched
sed = find_program('sed')

subdir('doxygen-src')

# this won't work without the above, xml and whatnot
# Cannot find namespace "libtizchromecast" in doxygen
# xml output for project "tizonia"
# subdir('sphinx-src')


