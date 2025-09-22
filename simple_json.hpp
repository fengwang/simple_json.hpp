#ifndef SIMPLE_JSON_HPP
#define SIMPLE_JSON_HPP

#include <cstddef>
#include <string>
#include <expected>

typedef struct {
    char const* data;
    char const* cur;
    char const* end;
    int depth;
} sj_Reader;

typedef struct {
    int type;
    char const* start;
    char const* end;
    int depth;
} sj_Value;

enum { SJ_ERROR, SJ_END, SJ_ARRAY, SJ_OBJECT, SJ_NUMBER, SJ_STRING, SJ_BOOL, SJ_NULL };

inline sj_Reader sj_reader(char const *data, size_t len) {
    sj_Reader r;
    r.data = data;
    r.cur = data;
    r.end = data + len;
    r.depth = 0;
    return r;
}

static bool sj__is_number_cont(char c) {
    return (c >= '0' && c <= '9') ||  c == 'e' || c == 'E' || c == '.' || c == '-' || c == '+';
}

static bool sj__is_string(char const *cur, char const *end, char const *expect) {
    while (*expect) {
        if (cur == end || *cur != *expect)
            return false;
        expect++, cur++;
    }
    return true;
}


inline std::expected<sj_Value, std::string> sj_read(sj_Reader *r) {
    sj_Value res;
    
    // Process tokens in a loop to avoid goto
    while (true) {
        // Handle EOF condition
        if (r->cur == r->end) { 
            return std::unexpected("unexpected eof"); 
        }
        
        res.start = r->cur;

        switch (*r->cur) {
        case ' ': case '\n': case '\r': case '\t':
        case ':': case ',':
            r->cur++;
            // Continue to next iteration of the loop
            continue;

        case '-': case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            res.type = SJ_NUMBER;
            while (r->cur != r->end && sj__is_number_cont(*r->cur)) { r->cur++; }
            break;

        case '"':
            res.type = SJ_STRING;
            res.start = ++r->cur;
            for (;;) {
                if ( r->cur == r->end) { 
                    return std::unexpected("unclosed string"); 
                }
                if (*r->cur ==    '"') { break; }
                if (*r->cur ==   '\\') { r->cur++; }
                if ( r->cur != r->end) { r->cur++; }
            }
            res.end = r->cur++;
            return res;

        case '{': case '[':
            if (r->depth > 1000000) { 
                return std::unexpected("max depth reached!"); 
            }
            res.type = (*r->cur == '{') ? SJ_OBJECT : SJ_ARRAY;
            res.depth = ++r->depth;
            r->cur++;
            break;

        case '}': case ']':
            res.type = SJ_END;
            if (--r->depth < 0) {
                return std::unexpected((*r->cur == '}') ? "stray '}'" : "stray ']'");
            }
            r->cur++;
            break;

        case 'n': case 't': case 'f':
            res.type = (*r->cur == 'n') ? SJ_NULL : SJ_BOOL;
            if (sj__is_string(r->cur, r->end,  "null")) { r->cur += 4; break; }
            if (sj__is_string(r->cur, r->end,  "true")) { r->cur += 4; break; }
            if (sj__is_string(r->cur, r->end, "false")) { r->cur += 5; break; }
            [[fallthrough]];

        default:
            return std::unexpected("unknown token");
        }
        
        // If we reach here, it means we've processed a complete token
        res.end = r->cur;
        return res;
    }
}


static void sj__discard_until(sj_Reader *r, int depth) {
    // Don't call sj_read initially, check condition first like in the original
    while (r->depth != depth) {
        auto val = sj_read(r);
        if (!val.has_value() || val.value().type == SJ_ERROR) {
            break;
        }
    }
}


inline std::expected<bool, std::string> sj_iter_array(sj_Reader *r, sj_Value arr, sj_Value *val) {
    sj__discard_until(r, arr.depth);
    auto result = sj_read(r);
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }
    *val = result.value();
    if (val->type == SJ_ERROR || val->type == SJ_END) { return false; }
    return true;
}


inline std::expected<bool, std::string> sj_iter_object(sj_Reader *r, sj_Value obj, sj_Value *key, sj_Value *val) {
    sj__discard_until(r, obj.depth);
    auto key_result = sj_read(r);
    if (!key_result.has_value()) {
        return std::unexpected(key_result.error());
    }
    *key = key_result.value();
    if (key->type == SJ_ERROR || key->type == SJ_END) { return false; }
    
    auto val_result = sj_read(r);
    if (!val_result.has_value()) {
        return std::unexpected(val_result.error());
    }
    *val = val_result.value();
    if (val->type == SJ_END)   { return std::unexpected("unexpected object end"); }
    if (val->type == SJ_ERROR) { return false; }
    return true;
}

inline void sj_location(sj_Reader *r, int *line, int *col) {
    int ln = 1, cl = 1;
    for (char const *p = r->data; p != r->cur; p++) {
        if (*p == '\n') { ln++; cl = 0; }
        cl++;
    }
    *line = ln;
    *col = cl;
}

#endif // #ifndef SIMPLE_JSON_HPP
