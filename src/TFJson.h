#ifndef TFJSON_H
#define TFJSON_H

#include <stddef.h>
#include <sys/types.h> // for ssize_t
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include <limits>
#include <functional>

#define TFJSON_USE_STRLEN std::numeric_limits<size_t>::max()

struct TFJsonSerializer {
    char * const buf;
    const size_t buf_size;
    char *head;
    size_t buf_required;
    bool in_empty_container = true;

    // To get the required buffer size, construct with buf = nullptr and buf_size = 0 and construct your JSON payload.
    // TFJsonSerializer::end() will return the required buffer size WITHOUT NULL TERMINATOR!
    TFJsonSerializer(char *buf, size_t buf_size);

    // Disallow copying the serializer, because why would you?
    TFJsonSerializer(const TFJsonSerializer&) = delete;
    TFJsonSerializer &operator=(const TFJsonSerializer&) = delete;

    // Object
    void addMemberNumber(const char *key, uint64_t u);
    void addMemberNumber(const char *key, int64_t i);
    void addMemberNumber(const char *key, uint32_t u);
    void addMemberNumber(const char *key, int32_t i);
    void addMemberNumber(const char *key, uint16_t u);
    void addMemberNumber(const char *key, int16_t i);
    void addMemberNumber(const char *key, uint8_t u);
    void addMemberNumber(const char *key, int8_t i);
    void addMemberNumber(const char *key, double f);
    void addMemberNumber(const char *key, float f);
    void addMemberBoolean(const char *key, bool b);
    void addMemberNull(const char *key);
    void addMemberString(const char *key, const char *c);
    [[gnu::format(__printf__, 3, 0)]] void addMemberStringVF(const char *key, const char *fmt, va_list args);
    [[gnu::format(__printf__, 3, 4)]] void addMemberStringF(const char *key, const char *fmt, ...);
    void addMemberArray(const char *key);
    void addMemberObject(const char *key);

    // Array or top level
    void addNumber(uint64_t u, bool enquote = false);
    void addNumber(int64_t i);
    void addNumber(uint32_t u);
    void addNumber(int32_t i);
    void addNumber(uint16_t u);
    void addNumber(int16_t i);
    void addNumber(uint8_t u);
    void addNumber(int8_t i);
    void addNumber(double f);
    void addNumber(float f);
    void addBoolean(bool b);
    void addNull();
    void addString(const char *c, size_t len = TFJSON_USE_STRLEN, bool enquote = true);
    [[gnu::format(__printf__, 2, 0)]] void addStringVF(const char *fmt, va_list args);
    [[gnu::format(__printf__, 2, 3)]] void addStringF(const char *fmt, ...);
    void addArray();
    void addObject();

    // Both
    void endArray();
    void endObject();
    size_t end();

private:
    void addKey(const char *key);
    void writeEscaped(const char *c, size_t len = TFJSON_USE_STRLEN);
    [[gnu::format(__printf__, 2, 0)]] void writeEscapedVF(const char *fmt, va_list args);
    [[gnu::format(__printf__, 2, 3)]] void writeEscapedF(const char *fmt, ...);
    void writePlain(char c);
    void writePlain(const char *c, size_t len);
    [[gnu::format(__printf__, 2, 0)]] void writePlainVF(const char *fmt, va_list args);
    [[gnu::format(__printf__, 2, 3)]] void writePlainF(const char *fmt, ...);
};

struct TFJsonDeserializer {
    enum class Error {
        Aborted,
        ExpectingEndOfInput,
        ExpectingValue,
        ExpectingOpeningCurlyBracket,
        ExpectingClosingCurlyBracket,
        ExpectingColon,
        ExpectingOpeningSquareBracket,
        ExpectingClosingSquareBracket,
        ExpectingOpeningQuote,
        ExpectingClosingQuote,
        ExpectingNumber,
        ExpectingFractionDigits,
        ExpectingExponentDigits,
        ExpectingNull,
        ExpectingTrue,
        ExpectingFalse,
        InvalidEscapeSequence,
        UnescapedControlCharacter,
        ForbiddenNullInString,
        NestingTooDeep,
        InlineNullByte,
        InvalidUTF8StartByte,
        InvalidUTF8ContinuationByte,
        BufferTooShort,
        OutOfMemory,
        ElementTooLong,
        RefillFailure,
    };

    const size_t nesting_depth_max;
    const size_t malloc_size_max;
    const bool allow_null_in_string;
    size_t nesting_depth;
    size_t utf8_count;
    char *buf;
    size_t buf_len;
    ssize_t idx_nul;  // (virtual) nul-terminator
    ssize_t idx_cur;  // current character
    ssize_t idx_okay; // no parsing error until here [inclusive]
    ssize_t idx_done; // data is not needed anymore until here [inclusive]
    char cur;
    std::function<void(Error, char *, size_t)> error_handler;
    std::function<ssize_t(char *, size_t)> refill_handler;
    std::function<bool(void)> begin_handler;
    std::function<bool(void)> end_handler;
    std::function<bool(void)> object_begin_handler;
    std::function<bool(void)> object_end_handler;
    std::function<bool(void)> array_begin_handler;
    std::function<bool(void)> array_end_handler;
    std::function<bool(char *, size_t)> member_handler;
    std::function<bool(char *, size_t)> string_handler;
    std::function<bool(double)> double_handler;
    std::function<bool(int64_t)> int64_handler;
    std::function<bool(uint64_t)> uint64_handler;
    std::function<bool(char *, size_t)> number_handler;
    std::function<bool(bool)> boolean_handler;
    std::function<bool(void)> null_handler;

    TFJsonDeserializer(size_t nesting_depth_max, size_t malloc_size_max, bool allow_null_in_string = true);

    // Disallow copying the deserializer, because why would you?
    TFJsonDeserializer(const TFJsonDeserializer&) = delete;
    TFJsonDeserializer &operator=(const TFJsonDeserializer&) = delete;

    static const char *getErrorName(Error error);

    void setErrorHandler(std::function<void(Error, char *, size_t)> &&error_handler);
    void setRefillHandler(std::function<ssize_t(char *, size_t)> &&refill_handler);
    void setBeginHandler(std::function<bool(void)> &&begin_handler);
    void setEndHandler(std::function<bool(void)> &&end_handler);
    void setObjectBeginHandler(std::function<bool(void)> &&object_begin_handler);
    void setObjectEndHandler(std::function<bool(void)> &&object_end_handler);
    void setArrayBeginHandler(std::function<bool(void)> &&array_begin_handler);
    void setArrayEndHandler(std::function<bool(void)> &&array_end_handler);
    void setMemberHandler(std::function<bool(char *, size_t)> &&member_handler);
    void setStringHandler(std::function<bool(char *, size_t)> &&string_handler);
    void setDoubleHandler(std::function<bool(double)> &&double_handler);
    void setInt64Handler(std::function<bool(int64_t)> &&int64_handler);
    void setUInt64Handler(std::function<bool(uint64_t)> &&uint64_handler);
    void setNumberHandler(std::function<bool(char *, size_t)> &&number_handler);
    void setBooleanHandler(std::function<bool(bool)> &&boolean_handler);
    void setNullHandler(std::function<bool(void)> &&null_handler);

    bool parse(char *buf, size_t len = TFJSON_USE_STRLEN);

private:
    void reportError(Error error);
    size_t shift();
    bool next(size_t *offset = nullptr);
    void okay(ssize_t offset = 0);
    void done();
    bool enterNesting();
    void leaveNesting();
    bool isWhitespace();
    bool isDigit();
    bool isHexDigit();
    bool isControl();
    bool skipWhitespace();
    bool parseElements();
    bool parseElement();
    bool parseValue();
    bool parseObject();
    bool parseMembers();
    bool parseMember();
    bool parseArray();
    bool parseString(bool report_as_member_name = false);
    bool parseNumber();
    bool parseNull();
    bool parseTrue();
    bool parseFalse();
};

#endif

#ifdef TFJSON_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <inttypes.h>
#include <assert.h>
#include <limits.h> // for CHAR_MIN

static bool isctrl(char c) {
    // JSON allows 0x7F unescaped
#if CHAR_MIN == 0
    return c <= 0x1F;
#else
    return c <= 0x1F && c >= 0; // UTF-8 compatibility
#endif
}

#if 0
#define debugf(...) printf("TFJsonDeserializer: " __VA_ARGS__)
#else
#define debugf(...) (void)0
#endif

// Use this macro and pass length to writePlain so that the compiler can see (and create constants of) the string literal lengths.
#define WRITE_PLAIN_LITERAL(x) this->writePlain((x), strlen((x)))

TFJsonSerializer::TFJsonSerializer(char *buf, size_t buf_size) : buf(buf), buf_size(buf_size), head(buf), buf_required(0) {}

void TFJsonSerializer::addMemberNumber(const char *key, uint64_t u) {
    this->addKey(key);
    this->addNumber(u);
}

void TFJsonSerializer::addMemberNumber(const char *key, int64_t i) {
    this->addKey(key);
    this->addNumber(i);
}

void TFJsonSerializer::addMemberNumber(const char *key, uint32_t u) {
    this->addKey(key);
    this->addNumber(u);
}

void TFJsonSerializer::addMemberNumber(const char *key, int32_t i) {
    this->addKey(key);
    this->addNumber(i);
}

void TFJsonSerializer::addMemberNumber(const char *key, uint16_t u) {
    this->addKey(key);
    this->addNumber(u);
}

void TFJsonSerializer::addMemberNumber(const char *key, int16_t i) {
    this->addKey(key);
    this->addNumber(i);
}

void TFJsonSerializer::addMemberNumber(const char *key, uint8_t u) {
    this->addKey(key);
    this->addNumber(u);
}

void TFJsonSerializer::addMemberNumber(const char *key, int8_t i) {
    this->addKey(key);
    this->addNumber(i);
}

void TFJsonSerializer::addMemberNumber(const char *key, double f) {
    this->addKey(key);
    this->addNumber(f);
}

void TFJsonSerializer::addMemberNumber(const char *key, float f) {
    this->addKey(key);
    this->addNumber(f);
}

void TFJsonSerializer::addMemberBoolean(const char *key, bool b) {
    this->addKey(key);
    this->addBoolean(b);
}

void TFJsonSerializer::addMemberNull(const char *key) {
    this->addKey(key);
    this->addNull();
}

void TFJsonSerializer::addMemberString(const char *key, const char *c) {
    this->addKey(key);
    this->addString(c);
}

void TFJsonSerializer::addMemberStringVF(const char *key, const char *fmt, va_list args) {
    this->addKey(key);
    this->addStringVF(fmt, args);
}

void TFJsonSerializer::addMemberStringF(const char *key, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    this->addMemberStringVF(key, fmt, args);
    va_end(args);
}

void TFJsonSerializer::addMemberArray(const char *key) {
    this->addKey(key);
    this->writePlain('[');
}

void TFJsonSerializer::addMemberObject(const char *key) {
    this->addKey(key);
    this->writePlain('{');
}

void TFJsonSerializer::addNumber(uint64_t u, bool enquote) {
    if (!in_empty_container)
        this->writePlain(',');

    in_empty_container = false;

    if (enquote)
        this->writePlain('"');

    this->writePlainF("%" PRIu64, u);

    if (enquote)
        this->writePlain('"');
}

void TFJsonSerializer::addNumber(int64_t i) {
    if (!in_empty_container)
        this->writePlain(',');

    in_empty_container = false;

    this->writePlainF("%" PRIi64, i);
}

void TFJsonSerializer::addNumber(uint32_t u) {
    if (!in_empty_container)
        this->writePlain(',');

    in_empty_container = false;

    this->writePlainF("%" PRIu32, u);
}

void TFJsonSerializer::addNumber(int32_t i) {
    if (!in_empty_container)
        this->writePlain(',');

    in_empty_container = false;

    this->writePlainF("%" PRIi32, i);
}

void TFJsonSerializer::addNumber(uint16_t u) {
    this->addNumber(static_cast<uint32_t>(u));
}

void TFJsonSerializer::addNumber(int16_t i) {
    this->addNumber(static_cast<int32_t>(i));
}

void TFJsonSerializer::addNumber(uint8_t u) {
    this->addNumber(static_cast<uint32_t>(u));
}

void TFJsonSerializer::addNumber(int8_t i) {
    this->addNumber(static_cast<int32_t>(i));
}

void TFJsonSerializer::addNumber(double f) {
    if (!in_empty_container)
        this->writePlain(',');

    in_empty_container = false;

    if (isfinite(f))
        this->writePlainF("%f", f);
    else
        WRITE_PLAIN_LITERAL("null");
}

void TFJsonSerializer::addNumber(float f) {
    this->addNumber(static_cast<double>(f));
}

void TFJsonSerializer::addBoolean(bool b) {
    if (!in_empty_container)
        this->writePlain(',');

    in_empty_container = false;

    if (b)
        WRITE_PLAIN_LITERAL("true");
    else
        WRITE_PLAIN_LITERAL("false");
}

void TFJsonSerializer::addNull() {
    if (!in_empty_container)
        this->writePlain(',');

    in_empty_container = false;

    WRITE_PLAIN_LITERAL("null");
}

void TFJsonSerializer::addString(const char *c, size_t len, bool enquote) {
    if (!in_empty_container)
        this->writePlain(',');

    in_empty_container = false;

    if (enquote)
        this->writePlain('\"');

    this->writeEscaped(c, len);

    if (enquote)
        this->writePlain('\"');
}

void TFJsonSerializer::addStringVF(const char *fmt, va_list args) {
    if (!in_empty_container)
        this->writePlain(',');

    in_empty_container = false;

    this->writePlain('\"');
    this->writeEscapedVF(fmt, args);
    this->writePlain('\"');
}

void TFJsonSerializer::addStringF(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    this->addStringVF(fmt, args);
    va_end(args);
}

void TFJsonSerializer::addArray() {
    if (!in_empty_container)
        this->writePlain(',');

    in_empty_container = true;

    this->writePlain('[');
}

void TFJsonSerializer::addObject() {
    if (!in_empty_container)
        this->writePlain(',');

    in_empty_container = true;

    this->writePlain('{');
}

void TFJsonSerializer::endArray() {
    in_empty_container = false;

    this->writePlain(']');
}

void TFJsonSerializer::endObject() {
    in_empty_container = false;

    this->writePlain('}');
}

size_t TFJsonSerializer::end() {
    // Return required buffer size _without_ the null terminator.
    // This mirrors the behaviour of snprintf.
    size_t result = buf_required;

    this->writePlain('\0');

    if (buf_size > 0 && result >= buf_size)
        buf[buf_size - 1] = '\0';

    return result;
}

void TFJsonSerializer::addKey(const char *key) {
    if (!in_empty_container)
        this->writePlain(',');

    in_empty_container = true;

    this->writePlain('\"');
    this->writeEscaped(key);
    WRITE_PLAIN_LITERAL("\":");
}

/*
    All code points may
    be placed within the quotation marks except for the code points that must be escaped: quotation mark
    (U+0022), reverse solidus (U+005C), and the control characters U+0000 to U+001F.
*/
void TFJsonSerializer::writeEscaped(const char *c, size_t len) {
    const char *end = c + (len == TFJSON_USE_STRLEN ? strlen(c) : len);

    while(c != end) {
        switch (*c) {
            case '\\':
                writePlain('\\');
                writePlain('\\');
                break;
            case '"':
                writePlain('\\');
                writePlain('"');
                break;
            case '\b':
                writePlain('\\');
                writePlain('b');
                break;
            case '\f':
                writePlain('\\');
                writePlain('f');
                break;
            case '\n':
                writePlain('\\');
                writePlain('n');
                break;
            case '\r':
                writePlain('\\');
                writePlain('r');
                break;
            case '\t':
                writePlain('\\');
                writePlain('t');
                break;
            default:
                if (isctrl(*c)) {
                    char x = *c;

                    writePlain('\\');
                    writePlain('u');
                    writePlain('0');
                    writePlain('0');
                    writePlain(x & 0x10 ? '1' : '0');

                    x &= 0x0F;

                    if (x >= 10)
                        writePlain('A' + (x - 10));
                    else
                        writePlain('0' + (x));
                }
                else {
                    writePlain(*c);
                }

                break;
        }
        ++c;
    }
}

void TFJsonSerializer::writeEscapedVF(const char *fmt, va_list args) {
    char *tmp;
    int tmp_len = vasprintf(&tmp, fmt, args); // FIXME: don't use vasprintf here, implement this without allocating extra memeory

    if (tmp_len > 0) {
        writeEscaped(tmp, tmp_len);
    }
}

void TFJsonSerializer::writeEscapedF(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    writeEscapedVF(fmt, args);
    va_end(args);
}

void TFJsonSerializer::writePlain(char c) {
    ++buf_required;

    if (buf_size == 0 || (size_t)(head - buf) > (buf_size - 1))
        return;

    *head = c;
    ++head;
}

void TFJsonSerializer::writePlain(const char *c, size_t len) {
    buf_required += len;

    if (len > buf_size || (size_t)(head - buf) > (buf_size - len))
        return;

    memcpy(head, c, len);
    head += len;
}

void TFJsonSerializer::writePlainVF(const char *fmt, va_list args) {
    size_t buf_left = (head >= buf + buf_size) ? 0 : buf_size - (size_t)(head - buf);
    int w = vsnprintf(head, buf_left, fmt, args);

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
}

void TFJsonSerializer::writePlainF(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    writePlainVF(fmt, args);
    va_end(args);
}

TFJsonDeserializer::TFJsonDeserializer(size_t nesting_depth_max, size_t malloc_size_max, bool allow_null_in_string) :
    nesting_depth_max(nesting_depth_max),
    malloc_size_max(malloc_size_max),
    allow_null_in_string(allow_null_in_string) {
}

const char *TFJsonDeserializer::getErrorName(Error error) {
    switch (error) {
        case Error::Aborted: return "Aborted";
        case Error::ExpectingEndOfInput: return "ExpectingEndOfInput";
        case Error::ExpectingValue: return "ExpectingValue";
        case Error::ExpectingOpeningCurlyBracket: return "ExpectingOpeningCurlyBracket";
        case Error::ExpectingClosingCurlyBracket: return "ExpectingClosingCurlyBracket";
        case Error::ExpectingColon: return "ExpectingColon";
        case Error::ExpectingOpeningSquareBracket: return "ExpectingOpeningSquareBracket";
        case Error::ExpectingClosingSquareBracket: return "ExpectingClosingSquareBracket";
        case Error::ExpectingOpeningQuote: return "ExpectingOpeningQuote";
        case Error::ExpectingClosingQuote: return "ExpectingClosingQuote";
        case Error::ExpectingNumber: return "ExpectingNumber";
        case Error::ExpectingFractionDigits: return "ExpectingFractionDigits";
        case Error::ExpectingExponentDigits: return "ExpectingExponentDigits";
        case Error::ExpectingNull: return "ExpectingNull";
        case Error::ExpectingTrue: return "ExpectingTrue";
        case Error::ExpectingFalse: return "ExpectingFalse";
        case Error::InvalidEscapeSequence: return "InvalidEscapeSequence";
        case Error::UnescapedControlCharacter: return "UnescapedControlCharacter";
        case Error::ForbiddenNullInString: return "ForbiddenNullInString";
        case Error::NestingTooDeep: return "NestingTooDeep";
        case Error::InlineNullByte: return "InlineNullByte";
        case Error::InvalidUTF8StartByte: return "InvalidUTF8StartByte";
        case Error::InvalidUTF8ContinuationByte: return "InvalidUTF8ContinuationByte";
        case Error::BufferTooShort: return "BufferTooShort";
        case Error::OutOfMemory: return "OutOfMemory";
        case Error::ElementTooLong: return "ElementTooLong";
        case Error::RefillFailure: return "RefillFailure";
    }
    return "Unknown";
}

void TFJsonDeserializer::setErrorHandler(std::function<void(Error, char *, size_t)> &&error_handler_) { error_handler = std::move(error_handler_); }

void TFJsonDeserializer::setRefillHandler(std::function<ssize_t(char *, size_t)> &&refill_handler_) { refill_handler = std::move(refill_handler_); }

void TFJsonDeserializer::setBeginHandler(std::function<bool(void)> &&begin_handler_) { begin_handler = std::move(begin_handler_); }

void TFJsonDeserializer::setEndHandler(std::function<bool(void)> &&end_handler_) { end_handler = std::move(end_handler_); }

void TFJsonDeserializer::setObjectBeginHandler(std::function<bool(void)> &&object_begin_handler_) { object_begin_handler = std::move(object_begin_handler_); }

void TFJsonDeserializer::setObjectEndHandler(std::function<bool(void)> &&object_end_handler_) { object_end_handler = std::move(object_end_handler_); }

void TFJsonDeserializer::setArrayBeginHandler(std::function<bool(void)> &&array_begin_handler_) { array_begin_handler = std::move(array_begin_handler_); }

void TFJsonDeserializer::setArrayEndHandler(std::function<bool(void)> &&array_end_handler_) { array_end_handler = std::move(array_end_handler_); }

void TFJsonDeserializer::setMemberHandler(std::function<bool(char *, size_t)> &&member_handler_) { member_handler = std::move(member_handler_); }

void TFJsonDeserializer::setStringHandler(std::function<bool(char *, size_t)> &&string_handler_) { string_handler = std::move(string_handler_); }

void TFJsonDeserializer::setDoubleHandler(std::function<bool(double)> &&double_handler_) { double_handler = std::move(double_handler_); }

void TFJsonDeserializer::setInt64Handler(std::function<bool(int64_t)> &&int64_handler_) { int64_handler = std::move(int64_handler_); }

void TFJsonDeserializer::setUInt64Handler(std::function<bool(uint64_t)> &&uint64_handler_) { uint64_handler = std::move(uint64_handler_); }

void TFJsonDeserializer::setNumberHandler(std::function<bool(char *, size_t)> &&number_handler_) { number_handler = std::move(number_handler_); }

void TFJsonDeserializer::setBooleanHandler(std::function<bool(bool)> &&boolean_handler_) { boolean_handler = std::move(boolean_handler_); }

void TFJsonDeserializer::setNullHandler(std::function<bool(void)> &&null_handler_) { null_handler = std::move(null_handler_); }

bool TFJsonDeserializer::parse(char *buf_, size_t buf_len_) {
    nesting_depth = 0;
    utf8_count = 0;
    buf = buf_;

    if (buf_len_ == TFJSON_USE_STRLEN) {
        idx_nul = strlen(buf);
        buf_len = idx_nul + 1;
    }
    else {
        idx_nul = buf_len_;
        buf_len = buf_len_;
    }

    idx_cur = -1;
    idx_okay = -1;
    idx_done = -1;

    debugf("parse(%p, %zu) -> \"%.*s\"\n", buf, buf_len, (int)idx_nul, buf);

    if (begin_handler && !begin_handler()) {
        reportError(Error::Aborted);
        return false;
    }

    if (!next()) {
        return false;
    }

    if (!parseElement()) {
        return false;
    }

    if (idx_done + 1 < idx_nul) {
        reportError(Error::ExpectingEndOfInput);
        return false;
    }

    if (end_handler && !end_handler()) {
        reportError(Error::Aborted);
        return false;
    }

    debugf("parse(...) -> buf_len: %zu, idx_nul: %zd, idx_cur: %zd, idx_okay: %zd, idx_done: %zd\n", buf_len, idx_nul, idx_cur, idx_okay, idx_done);

    return true;
}

void TFJsonDeserializer::reportError(Error error) {
    debugf("reportError(%s, idx_cur: %zd, idx_okay: %zd) -> \"%.*s\"\n", getErrorName(error), idx_cur, idx_okay, (int)(idx_nul - (idx_okay + 1)), buf + idx_okay + 1);

    if (error_handler) {
        error_handler(error, buf + idx_okay + 1, idx_nul - (idx_okay + 1));
    }
}

static int count_leading_ones_intrinsic(char value) {
    uint8_t bits = ~(uint8_t)value;

    if (bits == 0) {
        return 8;
    }

    return __builtin_clz(bits) - 24;
}

size_t TFJsonDeserializer::shift() {
    size_t done_len = (size_t)idx_done + 1;

    debugf("shift() -> idx_nul: %zd, done_len: %zu\n", idx_nul, done_len);

    memmove(buf, buf + done_len, idx_nul - done_len);

    idx_nul -= done_len;
    idx_cur -= done_len;
    idx_okay -= done_len;
    idx_done -= done_len;

    return done_len;
}

bool TFJsonDeserializer::next(size_t *offset) {
    if (offset != nullptr) {
        *offset = 0;
    }

    if (idx_cur + 1 >= idx_nul && refill_handler) {
        // reached the end of the current input, try to refill. first move remaining
        // input to the front of the buffer to avoid having to deal with wrapping
        size_t shift_len = shift();

        if (offset != nullptr) {
            *offset = shift_len;
        }

        size_t unused_len = buf_len - idx_nul;

        if (unused_len > 0) {
            ssize_t refilled_len = refill_handler(buf + idx_nul, unused_len);

            if (refilled_len < 0) {
                reportError(Error::RefillFailure);
                return false;
            }

            debugf("next() / refilled -> \"%.*s\"\n", (int)refilled_len, buf + idx_nul);

            idx_nul += refilled_len;
        }
        else if (refill_handler(nullptr, 0) > 0) {
            // the buffer is full with undone input and there is more input. the current
            // element has to fit into the buffer. if there is more input after the
            // current element then there has to be at least one char more than the current
            // element in the buffer for the parser to be able to tell that the current
            // element has ended
            reportError(Error::ElementTooLong);
            return false;
        }
    }

    if (idx_cur + 1 >= idx_nul) {
        idx_cur = idx_nul;
        cur = '\0';
    }
    else {
        ++idx_cur;
        cur = buf[idx_cur];

        if (cur == '\0') {
            okay(-1);

            reportError(Error::InlineNullByte);
            return false;
        }
    }

    debugf("next() -> idx_cur: %zd, utf8_count: %zu, cur: '%c' [0x%02x]\n", idx_cur, utf8_count, cur, (uint8_t)cur);

    if (utf8_count > 0) {
        if (((uint8_t)cur & 0xC0) != 0x80) {
            okay(-1);

            reportError(Error::InvalidUTF8ContinuationByte);
            return false;
        }

        --utf8_count;
    }
    else {
        utf8_count = count_leading_ones_intrinsic(cur);

        if (utf8_count != 0 && (utf8_count < 2 || utf8_count > 4)) {
            okay(-1);

            reportError(Error::InvalidUTF8StartByte);
            return false;
        }

        if (utf8_count > 0) {
            --utf8_count;
        }
    }

    return true;
}

void TFJsonDeserializer::okay(ssize_t offset) {
    idx_okay = idx_cur + offset;

    debugf("okay(offset: %zd) -> idx_okay: %zd\n", offset, idx_okay);
}

void TFJsonDeserializer::done() {
    idx_done = idx_okay;

    debugf("done() -> idx_done: %zd\n", idx_done);
}

bool TFJsonDeserializer::enterNesting() {
    if (nesting_depth >= nesting_depth_max) {
        reportError(Error::NestingTooDeep);
        return false;
    }

    ++nesting_depth;

    return true;
}

void TFJsonDeserializer::leaveNesting() {
    assert(nesting_depth > 0);

    --nesting_depth;
}

bool TFJsonDeserializer::isWhitespace() {
    switch (cur) {
        case ' ':
        case '\r':
        case '\n':
        case '\t':
            return true;

        default:
            return false;
    }
}

bool TFJsonDeserializer::isDigit() {
    return cur >= '0' && cur <= '9';
}

bool TFJsonDeserializer::isHexDigit() {
    return isDigit() || (cur >= 'a' && cur <= 'f') || (cur >= 'A' && cur <= 'F');
}

bool TFJsonDeserializer::isControl() {
    return isctrl(cur);
}

bool TFJsonDeserializer::skipWhitespace() {
    while (isWhitespace()) {
        debugf("skipWhitespace(cur: '%c' [0x%02x])\n", cur, (uint8_t)cur);

        okay();
        done();

        if (!next()) {
            return false;
        }
    }

    return true;
}

bool TFJsonDeserializer::parseElements() {
    if (!parseElement()) {
        return false;
    }

    while (cur == ',') {
        okay();
        done();

        if (!next()) {
            return false;
        }

        if (!parseElement()) {
            return false;
        }
    }

    return true;
}

bool TFJsonDeserializer::parseElement() {
    if (!skipWhitespace()) {
        return false;
    }

    if (!parseValue()) {
        return false;
    }

    if (!skipWhitespace()) {
        return false;
    }

    return true;
}

bool TFJsonDeserializer::parseValue() {
    switch (cur) {
        case '{':
            return parseObject();

        case '[':
            return parseArray();

        case '"':
            return parseString(false);

        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return parseNumber();

        case 'n':
            return parseNull();

        case 't':
            return parseTrue();

        case 'f':
            return parseFalse();

        default:
            reportError(Error::ExpectingValue);
            return false;
    }
}

bool TFJsonDeserializer::parseObject() {
    if (cur != '{') {
        reportError(Error::ExpectingOpeningCurlyBracket);
        return false;
    }

    okay();
    done();

    if (!enterNesting()) {
        return false;
    }

    if (object_begin_handler && !object_begin_handler()) {
        reportError(Error::Aborted);
        return false;
    }

    if (!next()) {
        return false;
    }

    if (!skipWhitespace()) {
        return false;
    }

    if (cur == '}') {
        okay();
        done();

        if (object_end_handler && !object_end_handler()) {
            reportError(Error::Aborted);
            return false;
        }

        leaveNesting();

        if (!next()) {
            return false;
        }

        return true;
    }

    if (!parseMembers()) {
        return false;
    }

    if (cur != '}') {
        reportError(Error::ExpectingClosingCurlyBracket);
        return false;
    }

    okay();
    done();

    if (object_end_handler && !object_end_handler()) {
        reportError(Error::Aborted);
        return false;
    }

    leaveNesting();

    if (!next()) {
        return false;
    }

    return true;
}

bool TFJsonDeserializer::parseMembers() {
    if (!parseMember()) {
        return false;
    }

    while (cur == ',') {
        okay();
        done();

        if (!next()) {
            return false;
        }

        if (!parseMember()) {
            return false;
        }
    }

    return true;
}

bool TFJsonDeserializer::parseMember() {
    if (!skipWhitespace()) {
        return false;
    }

    if (!parseString(true)) {
        return false;
    }

    if (!skipWhitespace()) {
        return false;
    }

    if (cur != ':') {
        reportError(Error::ExpectingColon);
        return false;
    }

    okay();
    done();

    if (!next()) {
        return false;
    }

    return parseElement();
}

bool TFJsonDeserializer::parseArray() {
    if (cur != '[') {
        reportError(Error::ExpectingOpeningSquareBracket);
        return false;
    }

    okay();
    done();

    if (!enterNesting()) {
        return false;
    }

    if (array_begin_handler && !array_begin_handler()) {
        reportError(Error::Aborted);
        return false;
    }

    if (!next()) {
        return false;
    }

    if (!skipWhitespace()) {
        return false;
    }

    if (cur == ']') {
        okay();
        done();

        if (array_end_handler && !array_end_handler()) {
            reportError(Error::Aborted);
            return false;
        }

        leaveNesting();

        if (!next()) {
            return false;
        }

        return true;
    }

    if (!parseElements()) {
        return false;
    }

    if (cur != ']') {
        reportError(Error::ExpectingClosingSquareBracket);
        return false;
    }

    okay();
    done();

    if (array_end_handler && !array_end_handler()) {
        reportError(Error::Aborted);
        return false;
    }

    leaveNesting();

    if (!next()) {
        return false;
    }

    return true;
}

bool TFJsonDeserializer::parseString(bool report_as_member) {
    if (cur != '"') {
        reportError(Error::ExpectingOpeningQuote);
        return false;
    }

    okay();
    done();

    if (!next()) {
        return false;
    }

    char *str = buf + idx_cur;
    char *end = str;
    size_t offset;

    while (cur != '"') {
        if (cur == '\0') {
            reportError(Error::ExpectingClosingQuote);
            return false;
        }

        if (cur != '\\') {
            if (isControl()) {
                reportError(Error::UnescapedControlCharacter);
                return false;
            }

            *end++ = cur;

            okay();

            if (!next(&offset)) {
                return false;
            }

            str -= offset;
            end -= offset;

            continue;
        }

        if (!next(&offset)) {
            return false;
        }

        str -= offset;
        end -= offset;

        char unescaped = '\0';

        switch (cur) {
            case '"':
                unescaped = '"';
                break;

            case '\\':
                unescaped = '\\';
                break;

            case '/':
                unescaped = '/';
                break;

            case 'b':
                unescaped = '\b';
                break;

            case 'f':
                unescaped = '\f';
                break;

            case 'n':
                unescaped = '\n';
                break;

            case 'r':
                unescaped = '\r';
                break;

            case 't':
                unescaped = '\t';
                break;
        }

        if (unescaped != '\0') {
            *end++ = unescaped;

            okay();

            if (!next(&offset)) {
                return false;
            }

            str -= offset;
            end -= offset;

            continue;
        }

        if (cur == 'u') {
            if (!next(&offset)) {
                return false;
            }

            str -= offset;
            end -= offset;

            char hex[5] = {0};

            for (int i = 0; i < 4; ++i) {
                if (!isHexDigit()) {
                    reportError(Error::InvalidEscapeSequence);
                    return false;
                }

                hex[i] = cur;

                if (!next(&offset)) {
                    return false;
                }

                str -= offset;
                end -= offset;
            }

            // Four hex digits can't encode a value > UINT32_MAX.
            uint32_t code_point = (uint32_t) strtoul(hex, nullptr, 16);

            if (!allow_null_in_string && code_point == 0) {
                reportError(Error::ForbiddenNullInString);
                return false;
            }

            if (code_point <= 0x7F) {
                *end++ = (char)code_point;
            }
            else if (code_point <= 0x07FF) {
                *end++ = (char)(((code_point >> 6) & 0x1F) | 0xC0);
                *end++ = (char)(((code_point >> 0) & 0x3F) | 0x80);
            }
            else if (code_point <= 0xFFFF) {
                *end++ = (char)(((code_point >> 12) & 0x0F) | 0xE0);
                *end++ = (char)(((code_point >>  6) & 0x3F) | 0x80);
                *end++ = (char)(((code_point >>  0) & 0x3F) | 0x80);
            }
            else if (code_point <= 0x10FFFF) {
                *end++ = (char)(((code_point >> 18) & 0x07) | 0xF0);
                *end++ = (char)(((code_point >> 12) & 0x3F) | 0x80);
                *end++ = (char)(((code_point >>  6) & 0x3F) | 0x80);
                *end++ = (char)(((code_point >>  0) & 0x3F) | 0x80);
            }
            else {
                reportError(Error::InvalidEscapeSequence);
                return false;
            }

            okay();

            continue;
        }

        reportError(Error::InvalidEscapeSequence);
        return false;
    }

    okay();

    size_t str_len = end - str;

    debugf("parseString(report_as_member: %s) -> \"%.*s\"\n", report_as_member ? "true" : "false", (int)str_len, str);

    if (report_as_member) {
        if (member_handler && !member_handler(str, str_len)) {
            reportError(Error::Aborted);
            return false;
        }
    }
    else {
        if (string_handler && !string_handler(str, str_len)) {
            reportError(Error::Aborted);
            return false;
        }
    }

    done();

    if (!next()) {
        return false;
    }

    return true;
}

bool TFJsonDeserializer::parseNumber() {
    char *number = buf + idx_cur;
    size_t offset;

    if (cur == '-') {
        if (!next(&offset)) {
            return false;
        }

        number -= offset;
    }

    if (!isDigit()) {
        reportError(Error::ExpectingNumber);
        return false;
    }

    char first_digit = cur;

    if (!next(&offset)) {
        return false;
    }

    number -= offset;

    if (first_digit != '0') {
        while (isDigit()) {
            if (!next(&offset)) {
                return false;
            }

            number -= offset;
        }
    }

    bool has_fraction_or_exponent = false;

    if (cur == '.') {
        if (!next(&offset)) {
            return false;
        }

        number -= offset;

        has_fraction_or_exponent = true;

        if (!isDigit()) {
            okay(-1);

            reportError(Error::ExpectingFractionDigits);
            return false;
        }

        while (isDigit()) {
            if (!next(&offset)) {
                return false;
            }

            number -= offset;
        }
    }

    if (cur == 'e' || cur == 'E') {
        if (!next(&offset)) {
            return false;
        }

        number -= offset;
        has_fraction_or_exponent = true;

        if (cur == '-' || cur == '+') {
            if (!next(&offset)) {
                return false;
            }

            number -= offset;
        }

        if (!isDigit()) {
            okay(-1);

            reportError(Error::ExpectingExponentDigits);
            return false;
        }

        while (isDigit()) {
            if (!next(&offset)) {
                return false;
            }

            number -= offset;
        }
    }

    size_t number_len = buf + idx_cur - number;
    char *number_buf = nullptr;

    debugf("parseNumber() -> \"%.*s\"\n", (int)number_len, number);

    if (number + number_len >= buf + buf_len) {
        // if number + number_len == buf + buf_len then there is no space
        // for temporarily nul-terminating the number to parse it
        offset = shift();

        if (offset > 0) {
            number -= offset;
        }
        else if (number_len + 1 > malloc_size_max) {
            reportError(Error::BufferTooShort);
            return false;
        }
        else {
            number_buf = strndup(number, number_len);

            if (number_buf == nullptr) {
                reportError(Error::OutOfMemory);
                return false;
            }

            number = number_buf;
        }
    }

    if (has_fraction_or_exponent) {
        if (double_handler) {
            char backup = number[number_len];

            number[number_len] = '\0';
            errno = 0;

            double result = strtod(number, nullptr);

            number[number_len] = backup;

            okay(-1);

            if (errno != 0) {
                debugf("parseNumber() -> \"%.*s\", errno: %d\n", (int)number_len, number, errno);

                if (number_handler && !number_handler(number, number_len)) {
                    free(number_buf);
                    reportError(Error::Aborted);
                    return false;
                }
            }
            else {
                debugf("parseNumber() -> \"%.*s\" = %f\n", (int)number_len, number, result);

                if (!double_handler(result)) {
                    free(number_buf);
                    reportError(Error::Aborted);
                    return false;
                }
            }
        }
        else if (number_handler) {
            if (!number_handler(number, number_len)) {
                free(number_buf);
                reportError(Error::Aborted);
                return false;
            }
        }
    }
    else if (*number == '-') {
        if (int64_handler) {
            char backup = number[number_len];

            number[number_len] = '\0';
            errno = 0;

            int64_t result = strtoll(number, nullptr, 10);

            number[number_len] = backup;

            okay(-1);

            if (errno != 0) {
                debugf("parseNumber() -> \"%.*s\", errno: %d\n", (int)number_len, number, errno);

                if (number_handler && !number_handler(number, number_len)) {
                    free(number_buf);
                    reportError(Error::Aborted);
                    return false;
                }
            }
            else {
                debugf("parseNumber() -> \"%.*s\" = %" PRIi64 "\n", (int)number_len, number, result);

                if (!int64_handler(result)) {
                    free(number_buf);
                    reportError(Error::Aborted);
                    return false;
                }
            }
        }
        else if (number_handler) {
            if (!number_handler(number, number_len)) {
                free(number_buf);
                reportError(Error::Aborted);
                return false;
            }
        }
    }
    else {
        if (uint64_handler) {
            char backup = number[number_len];

            number[number_len] = '\0';
            errno = 0;

            uint64_t result = strtoull(number, nullptr, 10);

            number[number_len] = backup;

            okay(-1);

            if (errno != 0) {
                debugf("parseNumber() -> \"%.*s\", errno: %d\n", (int)number_len, number, errno);

                if (number_handler && !number_handler(number, number_len)) {
                    free(number_buf);
                    reportError(Error::Aborted);
                    return false;
                }
            }
            else {
                debugf("parseNumber() -> \"%.*s\" = %" PRIu64 "\n", (int)number_len, number, result);

                if (!uint64_handler(result)) {
                    free(number_buf);
                    reportError(Error::Aborted);
                    return false;
                }
            }
        }
        else if (number_handler) {
            if (!number_handler(number, number_len)) {
                free(number_buf);
                reportError(Error::Aborted);
                return false;
            }
        }
    }

    free(number_buf);
    done();

    return true;
}

bool TFJsonDeserializer::parseNull() {
    if (cur != 'n') {
        reportError(Error::ExpectingNull);
        return false;
    }

    if (!next()) {
        return false;
    }

    if (cur != 'u') {
        reportError(Error::ExpectingNull);
        return false;
    }

    if (!next()) {
        return false;
    }

    if (cur != 'l') {
        reportError(Error::ExpectingNull);
        return false;
    }

    if (!next()) {
        return false;
    }

    if (cur != 'l') {
        reportError(Error::ExpectingNull);
        return false;
    }

    okay();

    if (null_handler && !null_handler()) {
        reportError(Error::Aborted);
        return false;
    }

    done();

    if (!next()) {
        return false;
    }

    return true;
}

bool TFJsonDeserializer::parseTrue() {
    if (cur != 't') {
        reportError(Error::ExpectingTrue);
        return false;
    }

    if (!next()) {
        return false;
    }

    if (cur != 'r') {
        reportError(Error::ExpectingTrue);
        return false;
    }

    if (!next()) {
        return false;
    }

    if (cur != 'u') {
        reportError(Error::ExpectingTrue);
        return false;
    }

    if (!next()) {
        return false;
    }

    if (cur != 'e') {
        reportError(Error::ExpectingTrue);
        return false;
    }

    okay();

    if (boolean_handler && !boolean_handler(true)) {
        reportError(Error::Aborted);
        return false;
    }

    done();

    if (!next()) {
        return false;
    }

    return true;
}

bool TFJsonDeserializer::parseFalse() {
    if (cur != 'f') {
        reportError(Error::ExpectingFalse);
        return false;
    }

    if (!next()) {
        return false;
    }

    if (cur != 'a') {
        reportError(Error::ExpectingFalse);
        return false;
    }

    if (!next()) {
        return false;
    }

    if (cur != 'l') {
        reportError(Error::ExpectingFalse);
        return false;
    }

    if (!next()) {
        return false;
    }

    if (cur != 's') {
        reportError(Error::ExpectingFalse);
        return false;
    }

    if (!next()) {
        return false;
    }

    if (cur != 'e') {
        reportError(Error::ExpectingFalse);
        return false;
    }

    okay();

    if (boolean_handler && !boolean_handler(false)) {
        reportError(Error::Aborted);
        return false;
    }

    done();

    if (!next()) {
        return false;
    }

    return true;
}

#endif
