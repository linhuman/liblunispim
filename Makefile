ifeq (${PREFIX},)
	PREFIX=/usr
endif
sharedir = $(DESTDIR)$(PREFIX)/share
libexecdir = $(DESTDIR)$(PREFIX)/bin
builddir=build

all: lunispim

lunispim:
	mkdir -p $(builddir)
	(cd $(builddir); cmake -DCMAKE_INSTALL_PREFIX=$(PREFIX)  -DDESTDIR=$(DESTDIR) .. && make)
	@echo ':)'

install:
	make -C $(builddir) install

clean:
	if  [ -e $(builddir) ]; then rm -R $(builddir); fi
