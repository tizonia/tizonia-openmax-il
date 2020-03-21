check_tiziheart_sources = [
   'check_tiziheart.c'
]

check_tiziheart = executable(
   'check_tiziheart',
   check_tiziheart_sources,
   dependencies: [
      check_dep,
      libtiziheart_dep
   ]
)

test('check_tiziheart', check_tiziheart)
