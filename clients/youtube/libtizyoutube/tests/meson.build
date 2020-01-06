check_tizyoutube_sources = [
   'check_tizyoutube.c'
]

check_tizyoutube = executable(
   'check_tizyoutube',
   check_tizyoutube_sources,
   dependencies: [
      check_dep,
      libtizyoutube_dep
   ]
)

test('check_tizyoutube', check_tizyoutube)
