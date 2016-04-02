#include <stdio.h>
#include <stdbool.h>
#include <bson.h>
#include <assert.h>
#include <time.h>
#include <search.h>

#define DATE_FMT "%Y-%m-%d %H:%M:%S"
#define MS_FMT ".%d"
#define VAL_BUF 4096
#define MAX_PATH 4096
#define INITIAL_LINES 32

#define CSV_ESCAPE_CHAR '\\'
#define CSV_QUOTE_CHAR '"'
#define CSV_DELIM_CHAR ','
#define CSV_NR_CHAR '\n'

#define NULL_VALUE_ATOM "NULL"
#define TRUE_VALUE_ATOM "true"
#define FALSE_VALUE_ATOM "false"

char *bson_val_to_str(bson_iter_t *iter) {
    char *str;
    char *res;
    uint32_t doclen;
    const uint8_t *data;
    bson_t doc;
    int b_converted;
    char outbuf[sizeof(DATE_FMT) * 2];
    int64_t uts;
    int ms;
    time_t ts;
    size_t offset;

    bson_type_t bt = bson_iter_type(iter);

    switch (bt) {
        case BSON_TYPE_OID:
            bson_oid_to_string(bson_iter_oid(iter), outbuf);
            return strdup(outbuf);
        case BSON_TYPE_DATE_TIME:
            uts = bson_iter_date_time(iter);
            ms = uts % 1000;
            ts = uts / 1000;
            offset = strftime(outbuf, sizeof(outbuf) - 1, DATE_FMT, gmtime(&ts));
            b_converted = snprintf(outbuf + offset, sizeof(outbuf) - offset - 1, MS_FMT, ms * 1000);
            return strdup(outbuf);
        case BSON_TYPE_UNDEFINED:
        case BSON_TYPE_NULL:
            return strdup(NULL_VALUE_ATOM);
        case BSON_TYPE_DOUBLE:
            asprintf(&res, "%g", bson_iter_double(iter));
            return res;
        case BSON_TYPE_UTF8:
            return strdup(bson_iter_utf8(iter, NULL));
        case BSON_TYPE_INT32:
            asprintf(&res, "%d", bson_iter_int32(iter));
            return res;
        case BSON_TYPE_INT64:
            asprintf(&res, "%lld", bson_iter_int64(iter));
            return res;
        case BSON_TYPE_BOOL:
            return strdup(bson_iter_bool(iter) ? TRUE_VALUE_ATOM : FALSE_VALUE_ATOM);
        case BSON_TYPE_DOCUMENT:
            bson_iter_document(iter, &doclen, &data);
            if (!bson_init_static(&doc, data, doclen))
                return strdup(NULL_VALUE_ATOM);
            str = bson_as_json(&doc, NULL);
            if (!str)
                return strdup(NULL_VALUE_ATOM);
            res = strdup(str);
            bson_free(str);
            return res;
        case BSON_TYPE_ARRAY:
            bson_iter_array(iter, &doclen, &data);
            if (!bson_init_static(&doc, data, doclen))
                return strdup(NULL_VALUE_ATOM);
            str = bson_as_json(&doc, NULL);
            if (!str)
                return strdup(NULL_VALUE_ATOM);
            res = strdup(str);
            bson_free(str);
            return res;
        default:
            fprintf(stderr, "Unsupported BSON type: %x\n", bt);
            abort();
    }
}

size_t csv_escape(char *dst, char *src) {
    *dst = CSV_QUOTE_CHAR;
    char *s = src;
    char *d = dst + 1;
    for ( ; *s ; s++) {
        switch (*s) {
            case CSV_ESCAPE_CHAR:
            case CSV_QUOTE_CHAR:
                *d = CSV_ESCAPE_CHAR;
                d++;
            default:
                *d = *s;
                d++;
        }
    }
    *d = CSV_QUOTE_CHAR;
    d++;
    *d = '\0';
    return d-dst;
}

bool _recurse_bson_iter(bson_iter_t *iter, bool (*cb)(bson_iter_t *, char *, void *), char *path_base, char *path_offset, void *cb_extra) {
    bson_iter_t desc;
    while (bson_iter_next (iter)) {
        *path_offset = '.';
        strcpy(path_offset + 1, bson_iter_key(iter));
        cb(iter, path_base + 1, cb_extra);
        if (BSON_ITER_HOLDS_DOCUMENT(iter) || BSON_ITER_HOLDS_ARRAY(iter)) {
            assert(bson_iter_recurse(iter, &desc));
            if (_recurse_bson_iter(&desc, cb, path_base, path_offset + strlen(path_offset), cb_extra))
                return true;
        }
        *path_offset = '\0';
    }
    return false;
}

void recurse_bson_doc(const bson_t *doc, bool (*cb)(bson_iter_t *, char *, void *), void *cb_extra)
{
    char pathbuf[MAX_PATH];
    bson_iter_t iter;

    if (bson_iter_init(&iter, doc)) {
        _recurse_bson_iter(&iter, cb, pathbuf, pathbuf, cb_extra);
    }
    return;
}

void usage(char *progname)
{
     fprintf (stderr, "\nUsage: %s <collection.bson> <fields.txt> [[start index] <stop index>]\n\n", progname);
     exit(3);
}

bool bson_traverse_callback(bson_iter_t *iter, char *path, void* payload)
{
    char *str;
    ENTRY ht_item;
    ht_item.key = path;
    ENTRY *found_item = hsearch(ht_item, FIND);
    if (found_item) {
        str = bson_val_to_str(iter);
        *((char **)found_item->data) = str;
    }

    return false;
}

int main (int argc, char *argv[])
{
    bson_reader_t *reader;
    const bson_t *doc;
    bson_error_t error;
    bool eof;
    char *cnv_end;

    size_t lowlimit = 0, highlimit = (size_t)-1;

    switch (argc) {
        case 3:
            break;
        case 4:
            highlimit = strtol(argv[3], &cnv_end, 10);
            if (argv[3] == cnv_end)
                usage(argv[0]);
            break;
        case 5:
            lowlimit = strtol(argv[3], &cnv_end, 10);
            if (argv[3] == cnv_end)
                usage(argv[0]);
            highlimit = strtol(argv[4], &cnv_end, 10);
            if (argv[4] == cnv_end)
                usage(argv[0]);
            break;
        default:
            usage(argv[0]);
    }

    size_t i, j, read, len;
    char *c, *line;
    FILE *fp;
    fp = fopen(argv[2], "r");
    if (!fp) {
        fprintf (stderr, "Failed to open fields file.\n");
        return 2;
    }

    char **fields = malloc(sizeof(char *) * INITIAL_LINES);
    size_t field_count = INITIAL_LINES;
    for (i=0; (read = getline(&line, &len, fp)) != -1;) {
        for (c = line + read - 1; c >= line ; c--)
            if (*c == '\r' || *c == '\n')
                *c = '\0';
            else
                break;

        if (!(*line))
            continue;

        if (i > field_count - 1) {
            field_count *= 2;
            fields = realloc(fields, field_count * sizeof(char *));
        }

        fields[i] = strdup(line);
        i++;
    }
    field_count = i;

    reader = bson_reader_new_from_file (argv[1], &error);

    if (!reader) {
        fprintf (stderr, "Failed to open BSON file.\n");
        return 1;
    }

    char **row_values = malloc(sizeof(char *) * field_count);
    assert(0 != hcreate(field_count));
    ENTRY ht_item, *found_item;
    for (i = 0; i < field_count; i++) {
        ht_item.key = fields[i];
        ht_item.data = &(row_values[i]);
        found_item = hsearch(ht_item, ENTER);
        assert(found_item);
    }

    char cnv_buf_stack[sizeof(char *) * VAL_BUF];
    char *cnv_buf = cnv_buf_stack;
    char *cnv_buf_free = NULL;
    size_t cnv_buf_size = sizeof(char *) * VAL_BUF;
    size_t field_size;

    for (i = 0; (doc = bson_reader_read (reader, &eof)) && i < highlimit; i++) {
        if (i >= lowlimit) {
            for (j = 0; j < field_count; j++) row_values[j] = NULL;

            recurse_bson_doc(doc, bson_traverse_callback, row_values);
            for (j = 0; j < field_count; j++) {
                if (row_values[j]) {
                    field_size = strlen(row_values[j]);
                    if (cnv_buf_size < 2 * field_size) {
                        cnv_buf = realloc(cnv_buf_free, (2 * field_size + 2) * sizeof(char));
                        cnv_buf_free = cnv_buf;
                        cnv_buf_size = (2 * field_size + 2) * sizeof(char);
                    }

                    size_t cnv_len = csv_escape(cnv_buf, row_values[j]);
                    fwrite(cnv_buf, 1, cnv_len, stdout);
                    free(row_values[j]);
                }
                else {
                    fwrite(NULL_VALUE_ATOM, 1, sizeof(NULL_VALUE_ATOM) - 1, stdout);
                }
                fputc((j == field_count - 1) ? CSV_NR_CHAR : CSV_DELIM_CHAR, stdout);
            }
            if ((i + 1) % 100000 == 0)
                fprintf(stderr, "\rProcessing input at record position %lu...", (i+1));
        }
    }

    if (!eof && i < highlimit) {
        fprintf (stderr, "corrupted bson document found at %u\n",
                    (unsigned)bson_reader_tell (reader));
    }

    bson_reader_destroy (reader);
    hdestroy();
    free(row_values);
    if (cnv_buf_free)
        free(cnv_buf_free);

    return 0;
}
