PREFIX		 = /var/www/vhosts/kristaps.bsd.lv
DATADIR		 = /vhosts/kristaps.bsd.lv/data

sinclude Makefile.local

OBJS 		 = db.o main.o
CPPFLAGS	+= -I/usr/local/include
CFLAGS	  	+= -g -W -Wall -Wextra -Wmissing-prototypes
CFLAGS	  	+= -Wstrict-prototypes -Wwrite-strings -Wno-unused-parameter
CFLAGS		+= -DDATADIR=\"$(DATADIR)\"
CFLAGS		+= `pkg-config --cflags kcgi-html sqlbox`

all: minci.cgi

install: all
	mkdir -p $(PREFIX)/cgi-bin
	mkdir -p $(PREFIX)/data
	install -o www -m 0500 minci.cgi $(PREFIX)/cgi-bin
	install -o www -m 0600 minci.db $(PREFIX)/data

update: all
	mkdir -p $(PREFIX)/cgi-bin
	install -o www -m 0500 minci.cgi $(PREFIX)/cgi-bin

minci.cgi: $(OBJS) minci.db
	$(CC) -o $@ -static $(OBJS) `pkg-config --libs --static kcgi-html sqlbox`

clean:
	rm -f $(OBJS) minci.cgi db.c extern.h minci.db db.sql

$(OBJS): extern.h

db.c: db.ort
	ort-c-source -vh extern.h db.ort >$@

extern.h: db.ort
	ort-c-header -v db.ort >$@

db.sql: db.ort
	ort-sql db.ort >$@

minci.db: db.sql
	rm -f $@
	sqlite3 $@ < db.sql
	[ ! -r db.local.sql ] || sqlite3 $@ < db.local.sql
