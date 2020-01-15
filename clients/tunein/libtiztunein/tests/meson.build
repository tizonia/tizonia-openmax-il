check_tiztunein_sources = [
   'check_tiztunein.c'
]

check_tiztunein = executable(
   'check_tiztunein',
   check_tiztunein_sources,
   dependencies: [
      check_dep,
      libtiztunein_dep
   ]
)

test('check_tiztunein', check_tiztunein)
