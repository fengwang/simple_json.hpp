#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../simple_json.hpp"

char const* json_text = "[ \"cat\", \"dog\", \"fox\", \"owl\" ]";

int main(void) {
    sj::Reader r = sj::reader(json_text, strlen(json_text));
    auto read_result = sj::read(&r);
    if (!read_result.has_value()) {
        printf("Error: %s\n", read_result.error().c_str());
        return 1;
    }
    sj::Value arr = read_result.value();
    
    printf("Array type: %d, depth: %d\n", static_cast<int>(arr.type), arr.depth);

    sj::Value val{};
    int count = 0;
    while (true) {
        auto iter_result = sj::iter_array(&r, arr, &val);
        if (!iter_result.has_value()) {
            printf("Error: %s\n", iter_result.error().c_str());
            return 1;
        }
        if (!iter_result.value()) {
            // End of array reached
            printf("End of array reached\n");
            break;
        }
        printf("Item %d: '%.*s'\n", count++, (int)(val.end-val.start), val.start);
    }

    return 0;
}
