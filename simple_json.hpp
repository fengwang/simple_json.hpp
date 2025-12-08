#ifndef SIMPLE_JSON_HPP
#define SIMPLE_JSON_HPP

 #include <cstddef>
 #include <string>
 #include <expected>

 /// Simple streaming JSON reader utilities.
 namespace sj {

 /// JSON token type produced by the reader.
 enum class Type { END, ARRAY, OBJECT, NUMBER, STRING, BOOL, NULL_ };

 /// Streaming JSON reader state.
 struct Reader {
     /// Pointer to the beginning of the JSON buffer.
     char const* data;
     /// Current cursor position within the buffer.
     char const* cur;
     /// One‑past‑the‑end pointer of the buffer.
     char const* end;
     /// Current nesting depth (arrays / objects).
     int depth;
 };

 /// View of a single JSON value within the original buffer.
 struct Value {
     /// Type of this JSON value.
     Type type;
     /// Pointer to the first character of the value’s textual representation.
     char const* start;
     /// One‑past‑the‑end pointer of the value’s textual representation.
     char const* end;
     /// Nesting depth at which this value resides.
     int depth;
 };

 /// Create a reader over a JSON buffer `[data, data + len)`.
 inline Reader reader(char const* data, std::size_t len) {
     Reader r;
     r.data = data;
     r.cur = data;
     r.end = data + len;
     r.depth = 0;
     return r;
 }

 /// Return true if the character can appear after the first character
 /// in a JSON number literal.
 inline bool is_number_cont(char c) {
     return (c >= '0' && c <= '9') ||  c == 'e' || c == 'E' || c == '.' || c == '-' || c == '+';
 }

 /// Helper that checks whether the sequence `[cur, end)` starts with `expect`.
 inline bool is_string(char const* cur, char const* end, char const* expect) {
     while (*expect) {
         if (cur == end || *cur != *expect)
             return false;
         expect++, cur++;
     }
     return true;
 }

 /// Read the next JSON token from the stream.
 ///
 /// On success, returns a `Value` whose `start`/`end` point into
 /// the original buffer managed by `Reader`. On failure, returns
 /// an error message describing the problem.
 inline std::expected<Value, std::string> read(Reader* r) {
     Value res{};

     // Process tokens in a loop to avoid goto
     while (true) {
         // Handle EOF condition
         if (r->cur == r->end) { return std::unexpected("unexpected eof"); }

         res.start = r->cur;

         switch (*r->cur) {
         case ' ': case '\n': case '\r': case '\t':
         case ':': case ',':
             r->cur++;
             // Continue to next iteration of the loop
             continue;

         case '-': case '0': case '1': case '2': case '3': case '4':
         case '5': case '6': case '7': case '8': case '9':
             res.type = Type::NUMBER;
             while (r->cur != r->end && is_number_cont(*r->cur)) { r->cur++; }
             break;

         case '"':
             res.type = Type::STRING;
             res.start = ++r->cur;
             for (;;) {
                 if ( r->cur == r->end) { return std::unexpected("unclosed string"); }
                 if (*r->cur ==    '"') { break; }
                 if (*r->cur ==   '\\') { r->cur++; }
                 if ( r->cur != r->end) { r->cur++; }
             }
             res.end = r->cur++;
             return res;

         case '{': case '[':
             if (r->depth > 1000000) { return std::unexpected("max depth reached!"); }
             res.type = (*r->cur == '{') ? Type::OBJECT : Type::ARRAY;
             res.depth = ++r->depth;
             r->cur++;
             break;

         case '}': case ']':
             res.type = Type::END;
             if (--r->depth < 0) { return std::unexpected((*r->cur == '}') ? "stray '}'" : "stray ']'"); }
             r->cur++;
             break;

         case 'n': case 't': case 'f':
             res.type = (*r->cur == 'n') ? Type::NULL_ : Type::BOOL;
             if (is_string(r->cur, r->end,  "null")) { r->cur += 4; break; }
             if (is_string(r->cur, r->end,  "true")) { r->cur += 4; break; }
             if (is_string(r->cur, r->end, "false")) { r->cur += 5; break; }
             [[fallthrough]];

         default:
             return std::unexpected("unknown token");
         }

         // If we reach here, it means we've processed a complete token
         res.end = r->cur;
         return res;
     }
 }

 /// Consume values until the reader’s depth returns to the given `depth`.
 ///
 /// This is mainly useful for skipping over unneeded subtrees.
 inline void discard_until(Reader* r, int depth) {
     while (r->depth != depth) {
         auto val = read(r);
         if (!val.has_value()) { break; }
     }
 }

 /// Iterate over the elements of a JSON array.
 ///
 /// `arr` must be a `Value` whose `type` is `Type::ARRAY` and whose
 /// `depth` matches the reader state. On success, stores the next
 /// element into `*val` and returns:
 ///  - `true`  while there are more elements,
 ///  - `false` when the end of the array is reached.
 /// On error, returns an error message.
 inline std::expected<bool, std::string> iter_array(Reader* r, Value arr, Value* val) {
     discard_until(r, arr.depth);
     auto result = read(r);
     if (!result.has_value()) { return std::unexpected(result.error()); }
     *val = result.value();
     if (val->type == Type::END) { return false; }
     return true;
 }

 /// Iterate over the key/value pairs of a JSON object.
 ///
 /// `obj` must be a `Value` whose `type` is `Type::OBJECT` and whose
 /// `depth` matches the reader state. On success, stores the next
 /// key into `*key` and its value into `*val`, and returns:
 ///  - `true`  while there are more pairs,
 ///  - `false` when the end of the object is reached.
 /// On error, returns an error message.
 inline std::expected<bool, std::string> iter_object(Reader* r, Value obj, Value* key, Value* val) {
     discard_until(r, obj.depth);
     auto key_result = read(r);
     if (!key_result.has_value()) { return std::unexpected(key_result.error()); }
     *key = key_result.value();
     if (key->type == Type::END) { return false; }

     auto val_result = read(r);
     if (!val_result.has_value()) { return std::unexpected(val_result.error()); }
     *val = val_result.value();
     if (val->type == Type::END)   { return std::unexpected("unexpected object end"); }
     return true;
 }

 /// Compute the current line and column of the reader cursor.
 ///
 /// Lines and columns are 1‑based and are counted from `Reader::data`
 /// up to, but not including, the current cursor position.
 inline void location(Reader* r, int* line, int* col) {
     int ln = 1, cl = 1;
     for (char const *p = r->data; p != r->cur; p++) {
         if (*p == '\n') { ln++; cl = 0; } cl++;
     }
     *line = ln;
     *col = cl;
 }

 } // namespace sj

#endif // #ifndef SIMPLE_JSON_HPP
