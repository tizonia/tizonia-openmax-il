check_tizsoundcloud_sources = [
   'check_tizsoundcloud.c'
]

check_tizsoundcloud = executable(
   'check_tizsoundcloud',
   check_tizsoundcloud_sources,
   dependencies: [
      check_dep,
      libtizsoundcloud_dep
   ]
)

test('check_tizsoundcloud', check_tizsoundcloud)
