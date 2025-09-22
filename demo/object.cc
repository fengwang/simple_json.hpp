// demonstrates iterating an object which itself contains a nested array and
// object. The `address` field is deliberately unhandled by the usage code to
// demonstrate its value being correctly discarded by the library

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SJ_IMPL
#include "../simple_json.hpp"


char* json_text = "{\n"
    "    \"first_name\": \"John\",\n"
    "    \"last_name\": \"Smith\",\n"
    "    \"age\": 27,\n"
    "    \"address\": {\n"
    "        \"street_address\": \"21 2nd Street\",\n"
    "        \"city\": \"New York\",\n"
    "        \"state\": \"NY\",\n"
    "    },"
    "    \"phone_numbers\": [ \"212 555-1234\", \"646 555-4567\" ],\n"
    "}";


bool eq(sj_Value val, char *s) {
    size_t len = val.end - val.start;
    return strlen(s) == len && !memcmp(s, val.start, len);
}

void print(sj_Value val) {
    printf("%.*s\n", (int)(val.end-val.start), val.start);
}

int main(void) {
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
        /**/ if (eq(key, "first_name")) { print(val); }
        else if (eq(key, "last_name"))  { print(val); }
        else if (eq(key, "age"))        { print(val); }
        else if (eq(key, "phone_numbers")) {
            sj_Value v;
            auto array_result = sj_iter_array(&r, val, &v);
            while (array_result.has_value() && array_result.value()) {
                print(v);
                array_result = sj_iter_array(&r, val, &v);
            }
            if (!array_result.has_value()) {
                printf("Error: %s\n", array_result.error().c_str());
                return 1;
            }
        } else {
            printf("(discarded '%.*s')\n", (int)(key.end-key.start), key.start);
        }
        iter_result = sj_iter_object(&r, obj, &key, &val);
    }
    
    if (!iter_result.has_value()) {
        printf("Error: %s\n", iter_result.error().c_str());
        return 1;
    }

    return 0;
}
