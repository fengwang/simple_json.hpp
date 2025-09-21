// takes a .json file as input and prints its content. `--minify` can be passed
// as the first argument to print the output minified

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SJ_IMPL
#include "../simple_json.hpp"


void print_indent(int depth, bool minify) {
    if (minify) { return; }
    for (int i = 0; i < depth; i++) {
        printf("    ");
    }
}

void print_newline(bool minify) {
    if (minify) { return; }
    printf("\n");
}

void print_value(sj_Reader *r, sj_Value val, int depth, bool minify) {
    int line, col;
    sj_Value k, v;
    int count = 0;

    switch (val.type) {
error:
    case SJ_ERROR:
        sj_location(r, &line, &col);
        fprintf(stderr, "\nerror: %d:%d: %s\n", line, col, r->error);
        exit(EXIT_FAILURE);

    case SJ_ARRAY:
        printf("[");
        while (sj_iter_array(r, val, &v)) {
            if (count++ > 0) { printf(","); }
            print_newline(minify);
            print_indent(depth + 1, minify);
            print_value(r, v, depth + 1, minify);
        }
        if (r->error) { goto error; }
        if (count > 0) {
            print_newline(minify);
            print_indent(depth, minify);
        }
        printf("]");
        break;

    case SJ_OBJECT:
        printf("{");
        while (sj_iter_object(r, val, &k, &v)) {
            if (count++ > 0) { printf(","); }
            print_newline(minify);
            print_indent(depth + 1, minify);
            print_value(r, k, depth + 1, minify);
            printf(": ");
            print_value(r, v, depth + 1, minify);
        }
        if (r->error) { goto error; }
        if (count > 0) {
            print_newline(minify);
            print_indent(depth, minify);
        }
        printf("}");
        break;

    case SJ_NUMBER:
        printf("%.*s", (int)(val.end-val.start), val.start);
        break;

    case SJ_STRING:
        printf("\"%.*s\"", (int)(val.end-val.start), val.start);
        break;

    case SJ_NULL:
        printf("null");
        break;

    case SJ_BOOL:
        printf(val.start[0] == 't' ? "true" : "false");
        break;
    }
}


void error(char *msg) {
    fprintf(stderr, "error: %s\n", msg);
    exit(EXIT_FAILURE);
}


int main(int argc, char **argv) {
    bool minify = false;

    if (argc > 1 && !strcmp(argv[1], "--minify")) {
        argv += 1; argc -= 1;
        minify = true;
    }
    if (argc < 2) { error("expected .json input file argument"); }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) { error("failed to open file"); }
    fseek(fp, 0, SEEK_END);
    int json_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* json_text = (char*)calloc(json_size + 1, 1);
    fread(json_text, 1, json_size, fp);
    fclose(fp);

    sj_Reader r = sj_reader(json_text, json_size);
    sj_Value val = sj_read(&r);
    print_value(&r, val, 0, minify);

    free(json_text);
    return 0;
}