#!/usr/bin/python3

from compileall import compile_dir
from os import environ, path
import sys

destdir = environ.get('DESTDIR', '')
sitelib = sys.argv[1]

print('Compiling python bytecode...')
compile_dir(destdir + path.join(sitelib), optimize=1)
