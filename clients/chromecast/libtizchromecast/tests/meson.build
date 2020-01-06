check_tizchromecast_sources = [
   'check_tizchromecast.c'
]

check_tizchromecast = executable(
   'check_tizchromecast',
   check_tizchromecast_sources,
   dependencies: [
      check_dep,
      libtizchromecast_dep
   ]
)

test('check_tizchromecast', check_tizchromecast)
