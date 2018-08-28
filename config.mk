# rin version
VERSION = 0.1

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/man

# includes and libs
INCS = -I. -I/usr/include
LIBS = -L/usr/lib

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_BSD_SOURCE
CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}
LDFLAGS = -s ${LIBS}

# compiler and linker
CC = cc
