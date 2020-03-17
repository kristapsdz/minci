VERSION		 = 0.2.0
WWWPREFIX	 = /var/www/vhosts/kristaps.bsd.lv
DATADIR		 = /vhosts/kristaps.bsd.lv/data

CFLAGS	  	+= -g -W -Wall -Wextra -Wmissing-prototypes
CFLAGS	  	+= -Wstrict-prototypes -Wwrite-strings -Wno-unused-parameter
CFLAGS		+= -DDATADIR=\"$(DATADIR)\"

CFLAGS_PKG	!= pkg-config --cflags kcgi-html sqlbox
LIBS_PKG	!= pkg-config --libs --static kcgi-html sqlbox
CFLAGS		+= $(CFLAGS_PKG)
LDADD		+= $(LIBS_PKG)

OBJS 		 = db.o main.o

all: minci.cgi minci

installcgi: updatecgi
	mkdir -p $(WWWPREFIX)/data
	install -o www -m 0600 minci.db $(WWWPREFIX)/data

updatecgi: all
	mkdir -p $(WWWPREFIX)/cgi-bin
	mkdir -p $(WWWPREFIX)/htdocs
	install -o www -m 0444 minci.css $(WWWPREFIX)/htdocs
	install -o www -m 0500 minci.cgi $(WWWPREFIX)/cgi-bin

testupdatedb:
	ort-sqldiff $(PREFIX)/data/minci.ort db.ort || true

updatedb:
	mkdir -p $(WWWPREFIX)/data
	cp -f $(WWWPREFIX)/data/minci.db $(WWWPREFIX)/data/minci.db.old
	cp -f $(WWWPREFIX)/data/minci.ort $(WWWPREFIX)/data/minci.ort.old
	ort-sqldiff $(WWWPREFIX)/data/minci.ort db.ort | sqlite3 $(WWWPREFIX)/data/minci.db
	install -m 0400 db.ort $(WWWPREFIX)/data/minci.ort

minci.cgi: $(OBJS) minci.db
	$(CC) -o $@ -static $(OBJS) $(LDFLAGS) $(LDADD)

minci: minci.sh
	sed "s!@VERSION@!$(VERSION)!g" minci.sh >$@

clean:
	rm -f $(OBJS) minci.cgi db.c extern.h minci.db db.sql minci

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
