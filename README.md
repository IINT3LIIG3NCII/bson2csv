# bson2csv
BSON to CSV converter written in C. This program is mostly intended to export data from MongoDB to SQL databases. BSON dump is safe and convenient way to get data from Mongo and CSV import is common way to bulk-load data into SQL database (sometimes prefferable over sending SQL queries, as in my case with MonetDB).

## Installation

### Requirements

bson2csv depends on [libbson](https://api.mongodb.org/libbson/current/) and requires `pkg-config` for build. Steps for it\`s installation varies on different operation systems and distros. It is not required if you are decided to use [static binaries](https://github.com/Snawoot/bson2csv#static-binaries) for Linux ([x86_64](https://github.com/Snawoot/bson2csv/releases/download/v0.2/bson2csv.static.linux_x86_64) / [i386](https://github.com/Snawoot/bson2csv/releases/download/v0.2/bson2csv.static.linux_i386)).

#### Mac

For Mac OS X all dependencies available in Homebrew repository:
```
host:~ user$ brew install pkg-config libbson
...
```

#### Linux

Depends on distro and package manager. Also, there are available pre-built [static binaries](https://github.com/Snawoot/bson2csv#static-binaries) for Linux ([x86_64](https://github.com/Snawoot/bson2csv/releases/download/v0.2/bson2csv.static.linux_x86_64) / [i386](https://github.com/Snawoot/bson2csv/releases/download/v0.2/bson2csv.static.linux_i386)).

#### Windows

I didn\`t even tried, but it seems possible.

### Building

#### Linux / MacOS / *BSD / ...

```
host:~ user$ git clone https://github.com/Snawoot/bson2csv.git
...
host:~ user$ cd bson2csv/
host:bson2csv user$ make
host:bson2csv user$ sudo make install
```

### Static binaries
There are static binaries available for Linux ([x86_64](https://github.com/Snawoot/bson2csv/releases/download/v0.2/bson2csv.static.linux_x86_64) / [i386](https://github.com/Snawoot/bson2csv/releases/download/v0.2/bson2csv.static.linux_i386)). Static binaries does not depend on any libraries and can be runned directly after download.
#### Linux
```bash
wget 'https://github.com/Snawoot/bson2csv/releases/download/v0.2/bson2csv.static.linux_x86_64' -O bson2csv
chmod +x bson2csv
./bson2csv
```

## Usage

### Invocation

```
bson2csv <collection.bson> <fields.txt> [[start index] <stop index>]
```
`collection.bson` - BSON file with documents (produced by mongodump, for example)

`fields.txt` - File with the field or fields to include in the export. The file must have only one field per line, and the line(s) must end with the LF character (0x0A). Field paths must be specified in standard Mongo dot notation. Order of extracted fields in resulting CSV file is the same as in fields file. All values are casted to string and escaped; structured fields are serialised and exported as JSON.

`start index` - How many records from BSON file program should skip.

`stop index` - Consecutive number of last exported document from BSON file.

### Example

Let\`s assume we have BSON file with documents having following structure:
```
$ bsondump sample.bson
{"_id":{"$oid":"000000000000000000012345"},"tags":["1","2","3","4"],"text":"asdfanother","source":{"name":"blah"},"type":["source"],"missing":["server_created_at"]}
2016-04-02T16:06:43.332+0300	1 objects found
```
And we need to extract `_id` field, first and second element of `tags` array, `text` field and `name` field from object in `source` field.

If so, we need to write following fields file:
```
_id
tags.0
tags.1
text
source.name
```

And run `bson2csv`:
```
$ bson2csv sample.bson sample.txt > example.csv
```

It will produce output file:
```
$ cat example.csv
"000000000000000000012345","1","2","asdfanother","blah"
```

## Issues

1. Due to file addressing limitations, libbson (and this program) does not support files larger than 2 GB on 32bit systems.
