#ifndef TFJSON_H
#define TFJSON_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include <limits>

struct TFJsonSerializer {
    char * const buf;
    const size_t buf_size;
    char *head;
    size_t buf_required;
    bool in_empty_container = true;

    // To get the required buffer size, construct with buf = nullptr and buf_size = 0 and construct your JSON payload.
    // TFJsonSerializer::end() will return the required buffer size WITHOUT NULL TERMINATOR!
    TFJsonSerializer(char *buf, size_t buf_size);

    // Object
    void add(const char *key, uint64_t u);
    void add(const char *key, int64_t i);
    void add(const char *key, double f);
    void add(const char *key, bool b);
    void addNull(const char *key);
    void add(const char *key, const char *c);
    void addArray(const char *key);
    void addObject(const char *key);

    // Array or top level
    void add(uint64_t u, bool enquote = false);
    void add(int64_t i);
    void add(double f);
    void add(bool b);
    void addNull();
    #define TFJSON_USE_STRLEN std::numeric_limits<size_t>::max()
    void add(const char *c, size_t len = TFJSON_USE_STRLEN, bool enquote = true);
    void addArray();
    void addObject();

    // Both
    void endArray();
    void endObject();
    size_t end();

private:
    void addKey(const char *key);
    void write(const char *c, size_t len = TFJSON_USE_STRLEN);
    void write(char c);
    void writeUnescaped(const char *c, size_t len);
    void writeFmt(const char *fmt, ...) __attribute__((__format__(__printf__, 2, 3)));
};
#endif

#ifdef TFJSON_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <math.h>


// Use this macro and pass length to writeUnescaped so that the compiler can see (and create constants of) the string literal lengths.
#define WRITE_LITERAL(x) this->writeUnescaped((x), strlen((x)))

TFJsonSerializer::TFJsonSerializer(char *buf, size_t buf_size) : buf(buf), buf_size(buf_size), head(buf), buf_required(0) {}

void TFJsonSerializer::add(const char *key, uint64_t u) {
    this->addKey(key);
    this->add(u);
}

void TFJsonSerializer::add(const char *key, int64_t i) {
    this->addKey(key);
    this->add(i);
}

void TFJsonSerializer::add(const char *key, double f) {
    this->addKey(key);
    this->add(f);
}

void TFJsonSerializer::add(const char *key, bool b) {
    this->addKey(key);
    this->add(b);
}

void TFJsonSerializer::addNull(const char *key) {
    this->addKey(key);
    this->addNull();
}

void TFJsonSerializer::add(const char *key, const char *c) {
    this->addKey(key);
    this->add(c);
}

void TFJsonSerializer::addArray(const char *key) {
    this->addKey(key);
    this->write('[');
}

void TFJsonSerializer::addObject(const char *key) {
    this->addKey(key);
    this->write('{');
}

void TFJsonSerializer::add(uint64_t u, bool enquote) {
    if (!in_empty_container)
        this->write(',');

    in_empty_container = false;
    if (enquote)
        this->write('"');

    this->writeFmt("%" PRIu64, u);

    if (enquote)
        this->write('"');
}

void TFJsonSerializer::add(int64_t i) {
    if (!in_empty_container)
        this->write(',');

    in_empty_container = false;

    this->writeFmt("%" PRIi64, i);
}

void TFJsonSerializer::add(double f) {
    if (!in_empty_container)
        this->write(',');

    in_empty_container = false;

    if (isfinite(f))
        this->writeFmt("%f", f);
    else
        WRITE_LITERAL("null");
}

void TFJsonSerializer::add(bool b) {
    if (!in_empty_container)
        this->write(',');

    in_empty_container = false;

    if (b)
        WRITE_LITERAL("true");
    else
        WRITE_LITERAL("false");
}

void TFJsonSerializer::addNull() {
    if (!in_empty_container)
        this->write(',');

    in_empty_container = false;

    WRITE_LITERAL("null");
}

void TFJsonSerializer::add(const char *c, size_t len, bool enquote) {
    if (!in_empty_container)
        this->write(',');

    in_empty_container = false;

    if (enquote)
        this->write('\"');

    this->write(c, len);

    if (enquote)
        this->write('\"');
}

void TFJsonSerializer::addArray() {
    if (!in_empty_container)
        this->write(',');

    in_empty_container = true;

    WRITE_LITERAL("[");
}

void TFJsonSerializer::addObject() {
    if (!in_empty_container)
        this->write(',');

    in_empty_container = true;

    WRITE_LITERAL("{");
}

void TFJsonSerializer::endArray() {
    in_empty_container = false;

    WRITE_LITERAL("]");
}

void TFJsonSerializer::endObject() {
    in_empty_container = false;

    WRITE_LITERAL("}");
}

size_t TFJsonSerializer::end() {
    // Return required buffer size _without_ the null terminator.
    // This mirrors the behaviour of snprintf.
    size_t result = buf_required;
    this->write('\0');
    if (buf_size > 0 && result >= buf_size)
        buf[buf_size - 1] = '\0';

    return result;
}

void TFJsonSerializer::addKey(const char *key) {
    if (!in_empty_container)
        this->write(",");

    in_empty_container = true;

    this->write('\"');
    this->write(key);
    WRITE_LITERAL("\":");
}

/*
    All code points may
    be placed within the quotation marks except for the code points that must be escaped: quotation mark
    (U+0022), reverse solidus (U+005C), and the control characters U+0000 to U+001F.
*/
void TFJsonSerializer::write(const char *c, size_t len) {
    const char *end = c + (len == TFJSON_USE_STRLEN ? strlen(c) : len);

    while(c != end) {
        switch (*c) {
            case '\\':
                write('\\');
                write('\\');
                break;
            case '"':
                write('\\');
                write('"');
                break;
            case '\b':
                write('\\');
                write('b');
                break;
            case '\f':
                write('\\');
                write('f');
                break;
            case '\n':
                write('\\');
                write('n');
                break;
            case '\r':
                write('\\');
                write('r');
                break;
            case '\t':
                write('\\');
                write('t');
                break;
            default:
#if CHAR_MIN == 0
                if (*c <= 0x1F) {
#else
                if (*c <= 0x1F && *c >= 0/*UTF-8 compatibility*/) {
#endif
                    char x = *c;

                    write('\\');
                    write('u');
                    write('0');
                    write('0');
                    write(x & 0x10 ? '1' : '0');

                    x &= 0x0F;

                    if (x >= 10)
                        write('A' + (x - 10));
                    else
                        write('0' + (x));
                }
                else
                    write(*c);
                break;
        }
        ++c;
    }
}

void TFJsonSerializer::write(char c) {
    ++buf_required;

    if (buf_size == 0 || (size_t)(head - buf) > (buf_size - 1))
        return;

    *head = c;
    ++head;
}

void TFJsonSerializer::writeUnescaped(const char *c, size_t len) {
    buf_required += len;

    if (len > buf_size || (head - buf) > (buf_size - len))
        return;

    memcpy(head, c, len);
    head += len;
}

void TFJsonSerializer::writeFmt(const char *fmt, ...) {
    size_t buf_left = (head >= buf + buf_size) ? 0 : buf_size - (size_t)(head - buf);

    va_list args;
    va_start(args, fmt);
    int w = vsnprintf(head, buf_left, fmt, args);
    va_end(args);

    if (w < 0) {
        // don't move head if vsnprintf fails completely.
        return;
    }

    buf_required += (size_t)w;

    if (buf_size == 0)
        return;

    if ((size_t)w >= buf_left) {
        head = buf + buf_size;

        buf[buf_size - 1] = '\0';
        return;
    }

    head += (size_t)w;
    return;
}

#endif
