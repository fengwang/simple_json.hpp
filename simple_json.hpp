#ifndef SIMPLE_JSON_HPP
#define SIMPLE_JSON_HPP

#include <cstddef>
typedef struct {
    char const* data;
    char const* cur;
    char const* end;
    int depth;
    char const* error;
} sj_Reader;

typedef struct {
    int type;
    char const* start;
    char const* end;
    int depth;
} sj_Value;

enum { SJ_ERROR, SJ_END, SJ_ARRAY, SJ_OBJECT, SJ_NUMBER, SJ_STRING, SJ_BOOL, SJ_NULL };

inline sj_Reader sj_reader(char const *data, size_t len) {
    return (sj_Reader){ .data = data, .cur = data, .end = data + len };
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


inline sj_Value sj_read(sj_Reader *r) {
    sj_Value res;
    
    // Handle error condition at the start
    if (r->error) { 
        return (sj_Value){ .type = SJ_ERROR, .start = r->cur, .end = r->cur }; 
    }
    
    // Process tokens in a loop to avoid goto
    while (true) {
        // Handle EOF condition
        if (r->cur == r->end) { 
            r->error = "unexpected eof"; 
            return (sj_Value){ .type = SJ_ERROR, .start = r->cur, .end = r->cur }; 
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
                    r->error = "unclosed string"; 
                    return (sj_Value){ .type = SJ_ERROR, .start = r->cur, .end = r->cur }; 
                }
                if (*r->cur ==    '"') { break; }
                if (*r->cur ==   '\\') { r->cur++; }
                if ( r->cur != r->end) { r->cur++; }
            }
            res.end = r->cur++;
            return res;

        case '{': case '[':
            if (r->depth > 1000000) { 
                r->error = "max depth reached!"; 
                return (sj_Value){ .type = SJ_ERROR, .start = r->cur, .end = r->cur }; 
            }
            res.type = (*r->cur == '{') ? SJ_OBJECT : SJ_ARRAY;
            res.depth = ++r->depth;
            r->cur++;
            break;

        case '}': case ']':
            res.type = SJ_END;
            if (--r->depth < 0) {
                r->error = (*r->cur == '}') ? "stray '}'" : "stray ']'";
                return (sj_Value){ .type = SJ_ERROR, .start = r->cur, .end = r->cur }; 
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
            r->error = "unknown token";
            return (sj_Value){ .type = SJ_ERROR, .start = r->cur, .end = r->cur }; 
        }
        
        // If we reach here, it means we've processed a complete token
        res.end = r->cur;
        return res;
    }
}


static void sj__discard_until(sj_Reader *r, int depth) {
    sj_Value val;
    val.type = SJ_NULL;
    while (r->depth != depth && val.type != SJ_ERROR) {
        val = sj_read(r);
    }
}


inline bool sj_iter_array(sj_Reader *r, sj_Value arr, sj_Value *val) {
    sj__discard_until(r, arr.depth);
    *val = sj_read(r);
    if (val->type == SJ_ERROR || val->type == SJ_END) { return false; }
    return true;
}


inline bool sj_iter_object(sj_Reader *r, sj_Value obj, sj_Value *key, sj_Value *val) {
    sj__discard_until(r, obj.depth);
    *key = sj_read(r);
    if (key->type == SJ_ERROR || key->type == SJ_END) { return false; }
    *val = sj_read(r);
    if (val->type == SJ_END)   { r->error = "unexpected object end"; return false; }
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
