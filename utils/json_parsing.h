/* json_parsing.h                                                  -*- C++ -*-
   Jeremy Barnes, 1 February 2012
   Copyright (c) 2012 Datacratic Inc.  All rights reserved.

   Released under the MIT license.

   Functionality to ease parsing of JSON from within a parse_function.
*/

#ifndef __jml__utils__json_parsing_h__
#define __jml__utils__json_parsing_h__

#include <string>
#include <functional>
#include "parse_context.h"
#include "jml/arch/format.h"


namespace ML {

/*****************************************************************************/
/* JSON UTILITIES                                                            */
/*****************************************************************************/

std::string jsonEscape(const std::string & str);

void jsonEscape(const std::string & str, std::ostream & out);

/*
 * If non-ascii characters are found an exception is thrown
 */
std::string expectJsonStringAscii(Parse_Context & context);

/*
 * If non-ascii characters are found an exception is thrown.
 * Output goes into the given buffer, of the given maximum length.
 * If it doesn't fit, then return zero.
 */
ssize_t expectJsonStringAscii(Parse_Context & context, char * buf,
                             size_t maxLength);

/*
 * if non-ascii characters are found we replace them by an ascii character that is supplied
 */
std::string expectJsonStringAsciiPermissive(Parse_Context & context, char c);

bool matchJsonString(Parse_Context & context, std::string & str);

bool matchJsonNull(Parse_Context & context);

void
expectJsonArray(Parse_Context & context,
                const std::function<void (int, Parse_Context &)> & onEntry);

void
expectJsonObject(Parse_Context & context,
                 const std::function<void (std::string, Parse_Context &)> & onEntry);

/** Expect a Json object and call the given callback.  The keys are assumed
    to be ASCII which means no embedded nulls, and so the key can be passed
    as a const char *.
*/
void
expectJsonObjectAscii(Parse_Context & context,
                      const std::function<void (const char *, Parse_Context &)> & onEntry);

bool
matchJsonObject(Parse_Context & context,
                const std::function<bool (std::string, Parse_Context &)> & onEntry);

void skipJsonWhitespace(Parse_Context & context);

inline bool expectJsonBool(Parse_Context & context)
{
    if (context.match_literal("true"))
        return true;
    else if (context.match_literal("false"))
        return false;
    context.exception("expected bool (true or false)");
}

#ifdef CPPTL_JSON_H_INCLUDED

inline Json::Value
expectJson(Parse_Context & context)
{
    context.skip_whitespace();
    if (*context == '"')
        return expectJsonStringAscii(context);
    else if (context.match_literal("null"))
        return Json::Value();
    else if (context.match_literal("true"))
        return Json::Value(true);
    else if (context.match_literal("false"))
        return Json::Value(false);
    else if (*context == '[') {
        Json::Value result(Json::arrayValue);
        expectJsonArray(context,
                        [&] (int i, Parse_Context & context)
                        {
                            result[i] = expectJson(context);
                        });
        return result;
    } else if (*context == '{') {
        Json::Value result(Json::objectValue);
        expectJsonObject(context,
                         [&] (std::string key, Parse_Context & context)
                         {
                             result[key] = expectJson(context);
                         });
        return result;
    } else return context.expect_double();
}

#endif

JML_ALWAYS_INLINE
auto fromHex(char hex, Parse_Context & context) -> uint8_t
{
    if (hex >= '0' && hex <= '9')
        return hex - '0';
    else if (hex >= 'a' && hex <= 'f')
        return hex - 'a' + 10;
    else if (hex >= 'A' && hex <= 'F')
        return hex - 'A' + 10;
    else
        context.exception(format("invalid hexadecimal: %c", hex));
}

template <typename T>
JML_ALWAYS_INLINE
auto fromHex(Parse_Context & context) -> T
{
    auto code = T{0};
    for (auto i = 0; i < (2*sizeof(T)); ++i)
        code = (code << 4) | fromHex(*context++, context);
    return code;
}

template <typename f_char, typename f_char16t>
JML_ALWAYS_INLINE
void readJsonString(Parse_Context& context, f_char&& push_back_byte, f_char16t&& push_back_utf16)
{
    skipJsonWhitespace(context);
    context.expect_literal('"');

    while (!context.match_literal('"'))
    {
        auto c = *context++;
        if (c != '\\')
        {
            push_back_byte(c);
            continue;
        }

        c = *context++;
        switch (c) {
            case 't': push_back_byte('\t'); break;
            case 'n': push_back_byte('\n'); break;
            case 'r': push_back_byte('\r'); break;
            case 'f': push_back_byte('\f'); break;
            case 'b': push_back_byte('\b'); break;
            case '/': push_back_byte('/');  break;
            case '\\':push_back_byte('\\'); break;
            case '"': push_back_byte('"');  break;
            case 'u': push_back_utf16(char16_t(fromHex<uint16_t>(context))); break;
            default: context.exception(format("invalid escape sequence: \\%c", c));
        }
    }
}

} // namespace ML


#endif /* __jml__utils__json_parsing_h__ */

