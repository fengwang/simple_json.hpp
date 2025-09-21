// demonstrates loading a rectangle into a struct from a json string

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SJ_IMPL
#include "../simple_json.hpp"


char *json_text = "{ \"x\": 10, \"y\": 20, \"w\": 30, \"h\": 40 }";

typedef struct { int x, y, w, h; } Rect;

bool eq(sj_Value val, char *s) {
    size_t len = val.end - val.start;
    return strlen(s) == len && !memcmp(s, val.start, len);
}

int main(void) {
    Rect rect = {0};

    sj_Reader r = sj_reader(json_text, strlen(json_text));
    sj_Value obj = sj_read(&r);

    sj_Value key, val;
    while (sj_iter_object(&r, obj, &key, &val)) {
        if (eq(key, "x")) { rect.x = atoi(val.start); }
        if (eq(key, "y")) { rect.y = atoi(val.start); }
        if (eq(key, "w")) { rect.w = atoi(val.start); }
        if (eq(key, "h")) { rect.h = atoi(val.start); }
    }

    printf("rect: { %d, %d, %d, %d }\n", rect.x, rect.y, rect.w, rect.h);
    return 0;
}