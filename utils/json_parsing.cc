/* json_parsing.cc
   Jeremy Barnes, 1 February 2012
   Copyright (c) 2012 Datacratic.  All rights reserved.

   Released under the MIT license.
*/

#include "json_parsing.h"
#include "buffers.h"


using namespace std;


namespace ML {

/*****************************************************************************/
/* JSON UTILITIES                                                            */
/*****************************************************************************/

void skipJsonWhitespace(Parse_Context & context)
{
    // Fast-path for the usual case for not EOF and no whitespace
    if (JML_LIKELY(!context.eof())) {
        char c = *context;
        if (c > ' ') {
            return;
        }
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
            return;
    }

    while (!context.eof()
           && (context.match_whitespace() || context.match_eol()));
}

char * jsonEscapeCore(const std::string & str, char * p, char * end)
{
    for (unsigned i = 0;  i < str.size();  ++i) {
        if (p + 4 >= end)
            return 0;

        char c = str[i];
        if (c >= ' ' && c < 127 && c != '\"' && c != '\\')
            *p++ = c;
        else {
            *p++ = '\\';
            switch (c) {
            case '\t': *p++ = ('t');  break;
            case '\n': *p++ = ('n');  break;
            case '\r': *p++ = ('r');  break;
            case '\f': *p++ = ('f');  break;
            case '\b': *p++ = ('b');  break;
            case '/':
            case '\\':
            case '\"': *p++ = (c);  break;
            default:
                throw Exception("invalid character in Json string");
            }
        }
    }

    return p;
}

std::string
jsonEscape(const std::string & str)
{
    size_t sz = str.size() * 4 + 4;
    char buf[sz];
    char * p = buf, * end = buf + sz;

    p = jsonEscapeCore(str, p, end);

    if (!p)
        throw ML::Exception("To fix: logic error in JSON escaping");

    return string(buf, p);
}

void jsonEscape(const std::string & str, std::ostream & stream)
{
    size_t sz = str.size() * 4 + 4;
    char buf[sz];
    char * p = buf, * end = buf + sz;

    p = jsonEscapeCore(str, p, end);

    if (!p)
        throw ML::Exception("To fix: logic error in JSON escaping");

    stream.write(buf, p - buf);
}

template <typename f>
JML_ALWAYS_INLINE
void readJsonStringAscii(Parse_Context& context, f&& push_back)
{
    readJsonString(context, push_back, push_back);
}

auto expectJsonStringAscii(Parse_Context& context) -> string
{
    auto result = growing_buffer{};
    readJsonStringAscii(context, [&](uint16_t c)
    {
        if (c > 127)
            context.exception("non-ASCII string character");

        result.push_back(c);
    });
    return result;
}

auto expectJsonStringAsciiPermissive(Parse_Context& context, char replaceWith) -> string
{
    auto result = growing_buffer{};
    readJsonStringAscii(context, [&](uint16_t c)
    {
        if (c > 127)
            c = replaceWith;

        result.push_back(c);
    });
    return result;
}

auto expectJsonStringAscii(Parse_Context& context, char* buffer, size_t maxLength) -> ssize_t
{
    try
    {
        auto result = external_buffer{buffer, maxLength};
        readJsonStringAscii(context, [&](uint16_t c)
        {
            if (c > 127)
                context.exception("non-ASCII string character");

            result.push_back(c);
        });
        result.push_back(0);
        return result.pos;
    }
    catch (out_of_range&)
    {
        return -1;
    }
}

auto matchJsonString(Parse_Context& context, string& str) -> bool
{
    try
    {
        Parse_Context::Revert_Token token{context};
        str = expectJsonStringAscii(context);
        token.ignore();
        return true;
    }
    catch (exception&)
    {
        return false;
    }
}

bool
matchJsonNull(Parse_Context & context)
{
    skipJsonWhitespace(context);
    return context.match_literal("null");
}

void
expectJsonArray(Parse_Context & context,
                const std::function<void (int, Parse_Context &)> & onEntry)
{
    skipJsonWhitespace(context);

    if (context.match_literal("null"))
        return;

    context.expect_literal('[');
    skipJsonWhitespace(context);
    if (context.match_literal(']')) return;

    for (int i = 0;  ; ++i) {
        skipJsonWhitespace(context);

        onEntry(i, context);

        skipJsonWhitespace(context);

        if (!context.match_literal(',')) break;
    }

    skipJsonWhitespace(context);
    context.expect_literal(']');
}

void
expectJsonObject(Parse_Context & context,
                 const std::function<void (std::string, Parse_Context &)> & onEntry)
{
    skipJsonWhitespace(context);

    if (context.match_literal("null"))
        return;

    context.expect_literal('{');

    skipJsonWhitespace(context);

    if (context.match_literal('}')) return;

    for (;;) {
        skipJsonWhitespace(context);

        string key = expectJsonStringAscii(context);

        skipJsonWhitespace(context);

        context.expect_literal(':');

        skipJsonWhitespace(context);

        onEntry(key, context);

        skipJsonWhitespace(context);

        if (!context.match_literal(',')) break;
    }

    skipJsonWhitespace(context);
    context.expect_literal('}');
}

void
expectJsonObjectAscii(Parse_Context & context,
                      const std::function<void (const char *, Parse_Context &)> & onEntry)
{
    skipJsonWhitespace(context);

    if (context.match_literal("null"))
        return;

    context.expect_literal('{');

    skipJsonWhitespace(context);

    if (context.match_literal('}')) return;

    for (;;) {
        skipJsonWhitespace(context);

        char keyBuffer[1024];

        ssize_t done = expectJsonStringAscii(context, keyBuffer, 1024);
        if (done == -1)
            context.exception("JSON key is too long");

        skipJsonWhitespace(context);

        context.expect_literal(':');

        skipJsonWhitespace(context);

        onEntry(keyBuffer, context);

        skipJsonWhitespace(context);

        if (!context.match_literal(',')) break;
    }

    skipJsonWhitespace(context);
    context.expect_literal('}');
}

bool
matchJsonObject(Parse_Context & context,
                const std::function<bool (std::string, Parse_Context &)> & onEntry)
{
    skipJsonWhitespace(context);

    if (context.match_literal("null"))
        return true;

    if (!context.match_literal('{')) return false;
    skipJsonWhitespace(context);
    if (context.match_literal('}')) return true;

    for (;;) {
        skipJsonWhitespace(context);

        string key = expectJsonStringAscii(context);

        skipJsonWhitespace(context);
        if (!context.match_literal(':')) return false;
        skipJsonWhitespace(context);

        if (!onEntry(key, context)) return false;

        skipJsonWhitespace(context);

        if (!context.match_literal(',')) break;
    }

    skipJsonWhitespace(context);
    if (!context.match_literal('}')) return false;

    return true;
}

} // namespace ML
