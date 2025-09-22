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
    auto read_result = sj_read(&r);
    if (!read_result.has_value()) {
        printf("Error: %s\n", read_result.error().c_str());
        return 1;
    }
    sj_Value obj = read_result.value();

    sj_Value key, val;
    auto iter_result = sj_iter_object(&r, obj, &key, &val);
    while (iter_result.has_value() && iter_result.value()) {
        if (eq(key, "x")) { rect.x = atoi(val.start); }
        if (eq(key, "y")) { rect.y = atoi(val.start); }
        if (eq(key, "w")) { rect.w = atoi(val.start); }
        if (eq(key, "h")) { rect.h = atoi(val.start); }
        iter_result = sj_iter_object(&r, obj, &key, &val);
    }
    
    if (!iter_result.has_value()) {
        printf("Error: %s\n", iter_result.error().c_str());
        return 1;
    }

    printf("rect: { %d, %d, %d, %d }\n", rect.x, rect.y, rect.w, rect.h);
    return 0;
}
