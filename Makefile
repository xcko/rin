# rin - raw in n out

include config.mk

SRC = rin.c
OBJ = ${SRC:.c=.o}

all: options rin

options:
	@echo rin build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

rin: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f rin ${OBJ} rin-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p rin-${VERSION}
	@cp -R LICENSE Makefile config.mk rin.1 rin.c rin-${VERSION}
	@tar -cf rin-${VERSION}.tar rin-${VERSION}
	@gzip rin-${VERSION}.tar
	@rm -rf rin-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f rin ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/rin
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < rin.1 > ${DESTDIR}${MANPREFIX}/man1/rin.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/rin.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/rin
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/rin.1

.PHONY: all options clean dist install uninstall
