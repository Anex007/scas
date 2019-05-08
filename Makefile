.POSIX:

include config.mk

SRC = scas.c ui.c
OBJ = $(SRC:.c=.o)

override CFLAGS += -Wall

all: options scas

options:
	@echo st build options:
	@echo "CFLAGS  = $(STCFLAGS)"
	@echo "LDFLAGS = $(STLDFLAGS)"
	@echo "CC      = $(CC)"

config.h:
	cp config.def.h config.h

.c.o:
	$(CC) $(STCFLAGS) -c $<

scas.o: arg.h config.h scas.h
ui.o:

$(OBJ): config.h config.mk

scas: $(OBJ)
	$(CC) -o $@ $(OBJ) $(STLDFLAGS)

clean:
	rm -f scas $(OBJ) scas-$(VERSION).tar.gz

dist: clean
	mkdir -p scas-$(VERSION)
	cp -R Makefile arg.h config.def.h config.mk scas.c scas.h ui.c\
		scas-$(VERSION)
	tar -cf - scas-$(VERSION) | gzip > scas-$(VERSION).tar.gz
	rm -rf scas-$(VERSION)

install: st
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f st $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/scas
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < st.1 > $(DESTDIR)$(MANPREFIX)/man1/scas.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/scas.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/scas
	rm -f $(DESTDIR)$(MANPREFIX)/man1/scas.1

.PHONY: all options clean dist install uninstall
