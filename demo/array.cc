// demonstrates iterating an array

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SJ_IMPL
#include "../simple_json.hpp"

char* json_text = "[ \"cat\", \"dog\", \"fox\", \"owl\" ]";

int main(void) {
    sj_Reader r = sj_reader(json_text, strlen(json_text));
    sj_Value arr = sj_read(&r);

    sj_Value val;
    while (sj_iter_array(&r, arr, &val)) {
        printf("'%.*s'\n", (int)(val.end-val.start), val.start);
    }

    return 0;
}