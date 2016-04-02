CC=gcc
INSTALL=install
CFLAGS=-O2
LDFLAGS=
BIN_PREFIX=/usr/local/bin/

PKG_CONF_TEST=pkg-config --version
BSON_CFLAGS=`pkg-config --cflags libbson-1.0`
BSON_LDFLAGS=`pkg-config --libs libbson-1.0`

bson2csv: bson2csv.c
	$(PKG_CONF_TEST) && $(CC) $(CFLAGS) $(BSON_CFLAGS) -o $@ $(LDFLAGS) $(BSON_LDFLAGS) $<

bson2csv.static: bson2csv.c
	$(PKG_CONF_TEST) && $(CC) -static $(CFLAGS) $(BSON_CFLAGS) -o $@ $(LDFLAGS) $< $(BSON_LDFLAGS) -lpthread

install: bson2csv
	$(INSTALL) -m 755 $< $(BIN_PREFIX)

clean:
	rm -f bson2csv bson2csv.static
