#include "JsonManualLexer.h"
#include "JsonLexemes.h"
#include "UnicodeIterator.h"
#include "ThorsIOUtil/Utility.h"
#include "ThorsLogging/ThorsLogging.h"

#include <limits>
#include <cstring>

using namespace ThorsAnvil::Serialize;

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
JsonManualLexer::JsonManualLexer(ParserInterface& parser)
    : parser(parser)
    , lastNull(false)
{}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
int JsonManualLexer::yylex()
{
    char    next;
    parser.readValue(next);
    buffer.clear();
    lastNull = false;
    switch (next)
    {
        case '{':
        case '}':
        case '[':
        case ']':
        case ',':
        case ':':
        {
            return lastToken = next;
        }
        case 't':
        {
            parser.unget();
            lastBool = true;
            return lastToken = ThorsAnvil::Serialize::JSON_TRUE;
        }
        case 'f':
        {
            parser.unget();
            lastBool = false;
            return lastToken = ThorsAnvil::Serialize::JSON_FALSE;
        }
        case 'n':
        {
            parser.unget();
            lastNull = true;
            return lastToken = ThorsAnvil::Serialize::JSON_NULL;
        }
        case '"':
        {
            parser.unget();
            return lastToken = ThorsAnvil::Serialize::JSON_STRING;
        }
        default:
        {
            parser.unget();
            return lastToken = ThorsAnvil::Serialize::JSON_NUMBER;
        }
    }
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonManualLexer::readTrue()
{
    checkFixed("true", 4);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonManualLexer::readFalse()
{
    checkFixed("false", 5);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonManualLexer::readNull()
{
    checkFixed("null", 4);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonManualLexer::ignoreRawValue()
{
    switch (lastToken)
    {
        case ThorsAnvil::Serialize::JSON_TRUE:      parser.ignore(4);break;
        case ThorsAnvil::Serialize::JSON_FALSE:     parser.ignore(5);break;
        case ThorsAnvil::Serialize::JSON_NULL:      parser.ignore(4);break;
        case ThorsAnvil::Serialize::JSON_NUMBER:    readNumber();break;
        case ThorsAnvil::Serialize::JSON_STRING:
        {
            char last = parser.get();  // Read the first Quote off the stream
            int next = parser.get();
            while (next != EOF && !(next == '"' && last != '\\'))
            {
                last = next;
                next = parser.get();
            }
            if (next == EOF)
            {
                error();
            }
            break;
        }
        default:break;
    }
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
std::string JsonManualLexer::getRawString()
{
    switch (lastToken)
    {
        case '{':   return "{";
        case '}':   return "}";
        case '[':   return "[";
        case ']':   return "]";
        case ',':   return ",";
        case ':':   return ":";
        case ThorsAnvil::Serialize::JSON_TRUE:  parser.ignore(4);return "true";
        case ThorsAnvil::Serialize::JSON_FALSE: parser.ignore(5);return "false";
        case ThorsAnvil::Serialize::JSON_NULL:  parser.ignore(4);return "null";
        //case ThorsAnvil::Serialize::JSON_INTEGER:
        //case ThorsAnvil::Serialize::JSON_FLOAT:
        case ThorsAnvil::Serialize::JSON_NUMBER:
        {
            readNumber();
            return buffer;
        }
        case ThorsAnvil::Serialize::JSON_STRING:
        {
            std::string result;

            char last = parser.get();  // Read the first Quote off the stream
            result.push_back(last);
            int next = parser.get();
            while (next != EOF && !(next == '"' && last != '\\'))
            {
                if (next < 0x20)
                {
                    ThorsLogAndThrow("ThorsAnvil::Serializer::JsonManualLexer",
                                     "getRawString",
                                     "Strings should not contain control characters.");
                }
                result.push_back(next);
                last = next;
                next = parser.get();
            }
            if (next == EOF)
            {
                error();
            }
            result.push_back('"');
            return result;
        }
        default:
        {
            ThorsLogAndThrow("ThorsAnvil::Serializer::JsonManualLexer",
                             "getRawString",
                             "Don't know how to retrieve the RAW data from the JSON input");
        }
    }
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void Unicode::checkBuffer(ParserInterface& i, std::string& reply)
{
    reply.clear();
    while (i.ok())
    {
        unsigned char result = i.get();
        if (result == '"') {
            return;
        }
        if (result < 0x20)
        {
            ThorsLogAndThrow("ThorsAnvil::Serialize::UnicodeWrapperIterator",
                             "heckBuffer",
                             "input character can not be smaller than 0x20");
        }
        if (result != '\\')
        {
            reply += result;
            continue;
        }
        result = i.get();
        switch (result)
        {
            case '"':   reply += '"';   continue;
            case '\\':  reply += '\\';  continue;
            case '/':   reply += '/';   continue;
            case 'b':   reply += '\b';  continue;
            case 'f':   reply += '\f';  continue;
            case 'n':   reply += '\n';  continue;
            case 'r':   reply += '\r';  continue;
            case 't':   reply += '\t';  continue;
            case 'u':
            {
                decodeUnicode(i, reply);
                continue;
            }
            default:
            {
                ThorsLogAndThrow("ThorsAnvil::Serialize::UnicodeWrapperIterator",
                                 "checkBuffer",
                                 "Escaped character must be one of [\"\\/bfnrtvu]");
            }
        }
    }
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void Unicode::decodeUnicode(ParserInterface& i, std::string& reply)
{
    long    unicodeValue    = getUnicodeHex(i);

    if (unicodeValue <= 0x7F)
    {
        // Encode as single UTF-8 character
        reply += char(unicodeValue & 0x7F);
    }
    else if (unicodeValue <= 0x7FF)
    {
        // Encode as two UTF-8 characters
        reply += char(0xC0 |((unicodeValue >>  6) & 0x1F));
        reply += char(0x80 |((unicodeValue >>  0) & 0x3F));
    }
    else if (unicodeValue <= 0xFFFF)
    {
        if ((unicodeValue & 0xFC00) != 0xD800)
        {
            // Encode as three UTF-8 characters
            reply += char(0xE0 |((unicodeValue >> 12) & 0x0F));
            reply += char(0x80 |((unicodeValue >>  6) & 0x3F));
            reply += char(0x80 |((unicodeValue >>  0) & 0x3F));
        }
        else
        {
            // We have a found first part of surrogate pair
            decodeSurrogatePairs(unicodeValue, i, reply);
        }
    }
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void Unicode::decodeSurrogatePairs(long unicodeValue, ParserInterface& i, std::string& reply)
{
    char nextChar  = i.get();
    if (nextChar != '\\')
    {
        ThorsLogAndThrow("ThorsAnvil::Serialize",
                         "UnicodeIterator",
                         "Iter->Surrogate pair(No Slash): \\uD8xx Must be followed by \\uDCxx");
    }
    nextChar  = i.get();
    if (nextChar != 'u')
    {
        ThorsLogAndThrow("ThorsAnvil::Serialize",
                         "UnicodeIterator",
                         "Iter->Surrogate pair(No u): \\uD8xx Must be followed by \\uDCxx");
    }

    unicodeValue = (unicodeValue << 16) + getUnicodeHex(i);

    // Surrogate pair
    if ((unicodeValue & 0xFC00FC00) != 0xD800DC00)
    {
        ThorsLogAndThrow("ThorsAnvil::Serialize",
                         "UnicodeIterator",
                         "Iter->Surrogate pair(No DC): \\uD8xx Must be followed by \\uDCxx");
    }

    // Decode surrogate pair
    unicodeValue    = 0x00010000 | ((unicodeValue & 0x03FF0000) >> 6) | (unicodeValue & 0x000003FF);

    // Encode as 4 UTF-8 characters
    reply += char(0xF0 |((unicodeValue >> 18) & 0x0F));
    reply += char(0x80 |((unicodeValue >> 12) & 0x3F));
    reply += char(0x80 |((unicodeValue >>  6) & 0x3F));
    reply += char(0x80 |((unicodeValue >>  0) & 0x3F));
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
long Unicode::getUnicodeHex(ParserInterface& i)
{
    long unicodeValue   = 0;

    for (int loop=0;loop < 4;++loop)
    {
        char x = i.get();

        unicodeValue <<= 4;
        unicodeValue    += convertHexToDec(x);
    }
    return unicodeValue;
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonManualLexer::getStringInto(std::string& value)
{
    //return std::string(make_UnicodeWrapperIterator(std::istreambuf_iterator<char>(str)), make_EndUnicodeWrapperIterator(std::istreambuf_iterator<char>(str)));
    char next = parser.get();
    if (next != '"')
    {
        ThorsLogAndThrow("ThorsAnvil::Serialize::UnicodeWrapperIterator",
                         "UnicodeWrapperIterator",
                         "String does not start with a \" character");
    }
    Unicode::checkBuffer(parser, value);
}
THORS_SERIALIZER_HEADER_ONLY_INCLUDE
std::string JsonManualLexer::getString()
{
    std::string result;
    getStringInto(result);
    return result;
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
bool JsonManualLexer::getLastBool()
{
    switch (lastToken)
    {
        case ThorsAnvil::Serialize::JSON_TRUE:
            readTrue();
            return lastBool;
        case ThorsAnvil::Serialize::JSON_FALSE:
            readFalse();
            return lastBool;
        default:
        {
            ThorsLogAndThrow("ThorsAnvil::Serialize::JsonParser",
                             "getLastBool",
                             "The last value was not a bool");
        }
    }
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
bool JsonManualLexer::isLastNull()
{
    return lastNull;
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
char JsonManualLexer::readDigits(char next)
{
    if (!std::isdigit(next))
    {
        error();
    }
    while (std::isdigit(next))
    {
        buffer.push_back(next);
        next = parser.get();
    }
    return next;
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonManualLexer::readNumber()
{
    buffer.clear();

    int next = parser.get();

    if (next == '-' || next == '+')
    {
        buffer.push_back(next);
        next = parser.get();
        if (next == EOF)
        {   error();
        }
    }
    if (next == '0')
    {
        buffer.push_back(next);
        next = parser.get();
    }
    else
    {
        next = readDigits(next);
    }
    if (next == '.')
    {
        buffer.push_back(next);
        next        = parser.get();
        if (next == EOF)
        {   error();
        }
        next        = readDigits(next);
    }
    if (next == 'e' || next == 'E')
    {
        buffer.push_back(next);
        next = parser.get();
        if (next == EOF)
        {   error();
        }
        if (next == '-' || next == '+')
        {
            buffer.push_back(next);
            next = parser.get();
            if (next == EOF)
            {   error();
            }
        }
        next = readDigits(next);
    }
    if (next != EOF)
    {
        parser.unget();
    }
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonManualLexer::checkFixed(char const* check, std::size_t size)
{
    buffer.resize(size);
    parser.read(&buffer[0], size);
    if (std::strncmp(&buffer[0], check, size) != 0)
    {
        error();
    }
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonManualLexer::error()
{
    ThorsLogAndThrow("ThorsAnvil::Serialize::JsonManualLexer",
                     "error",
                     "Invalid Character in Lexer");
}
