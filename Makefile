CC=gcc
INSTALL=install
CFLAGS=-g
LDFLAGS=
BIN_PREFIX=/usr/local/bin/

BSON_CFLAGS=`pkg-config --cflags libbson-1.0`
BSON_LDFLAGS=`pkg-config --libs libbson-1.0`

bson2csv: bson2csv.c
	$(CC) $(CFLAGS) $(BSON_CFLAGS) -o $@ $(LDFLAGS) $(BSON_LDFLAGS) $<

install: bson2csv
	$(INSTALL) -m 755 $< $(BIN_PREFIX)

clean:
	rm bson2csv
