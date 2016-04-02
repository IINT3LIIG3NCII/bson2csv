CC=gcc
CFLAGS=-g
LDFLAGS=

BSON_CFLAGS=`pkg-config --cflags libbson-1.0`
BSON_LDFLAGS=`pkg-config --libs libbson-1.0`

bson2csv: bson2csv.c
	$(CC) $(CFLAGS) $(BSON_CFLAGS) -o $@ $(LDFLAGS) $(BSON_LDFLAGS) $<

clean:
	rm bson2csv
