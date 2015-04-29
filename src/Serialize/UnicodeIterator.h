
#ifndef THORS_ANVIL_SERIALIZE_UNICODE_ITERATOR_H
#define THORS_ANVIL_SERIALIZE_UNICODE_ITERATOR_H
/*
 * An iterator used to scan the inside of a quoted string.
 * It will convert all escaped characters into the actual UTF-8 character.
 *
 * ie \n    Will be converted to the actual newline character code.
 *    \uxxx Will be converted into the unicode code point xxxx encoded in UTF-8
 *          Note: (which may take be more than one character lone).
 *
 * Usage:
 *
 *  std::string     input    = readQuotedTextFromInput();
 *  std::string     text1(make_UnicodeWrapperIterator(std::begin(input)), make_UnicodeWrapperIterator(std::end(input)));
 *
 *  // Or
 *  std::string     text;
 *  std::copy(std::bin(input), std::end(input), make_UnicodePushBackIterator(text));
 */

#include <iterator>

namespace ThorsAnvil
{
    namespace Serialize
    {

        namespace
        {

long convertHexToDec(char x)
{
    if (x >= '0' && x <= '9')
    {
        return (x - '0');
    }
    else if (x >= 'A' && x <= 'F')
    {
        return 10 + (x - 'A');
    }
    else if (x >= 'a' && x <= 'f')
    {
        return 10 + (x - 'a');
    }
    throw std::runtime_error("ThorsAnvil::Serialize::UnicodeIterator: Invalid Hex Digit in unicode string");
}
        }

template<typename C>
struct UnicodePushBackIterator: std::iterator<std::output_iterator_tag, char, ptrdiff_t, char*,char&>
{
    C&       cont;
    bool     lastWasSlash;
    int      unicodeCount;
    uint32_t unicodeValue;
    UnicodePushBackIterator(C& c)
        : cont(c)
        , lastWasSlash(false)
        , unicodeCount(0)
    {}
    UnicodePushBackIterator& operator++()       {return *this;}
    UnicodePushBackIterator& operator*()        {return *this;}
    void operator=(char x)
    {
        if (unicodeCount)
        {
            if (unicodeCount == 6)
            {
                if (x != '\\')
                {
                    throw std::runtime_error("ThorsAnvil::Serialize::UnicodeIterator: Push->Surrogate pair(No Slash): \\uD8xx Must be followed by \\uDCxx");
                }
                --unicodeCount;
            }
            else if (unicodeCount == 5)
            {
                if (x != 'u')
                {
                    throw std::runtime_error("ThorsAnvil::Serialize::UnicodeIterator: Push->Surrogate pair(No u): \\uD8xx Must be followed by \\uDCxx");
                }
                --unicodeCount;
            }
            else
            {
                unicodeValue <<= 4;
                unicodeValue += convertHexToDec(x);
                --unicodeCount;
                if (unicodeCount == 0)
                {
                    if (unicodeValue <= 0x7F)
                    {
                        // Encode as single UTF-8 character
                        cont.push_back(unicodeValue);
                    }
                    else if (unicodeValue <= 0x7FF)
                    {
                        // Encode as two UTF-8 characters
                        cont.push_back(0xC0 |((unicodeValue >>  6)));
                        cont.push_back(0x80 |((unicodeValue >>  0) & 0x3F));
                    }
                    else if (unicodeValue <= 0xFFFF)
                    {
                        if ((unicodeValue & 0xFC00) != 0xD800)
                        {
                            // Encode as three UTF-8 characters
                            cont.push_back(0xE0 |((unicodeValue >> 12)));
                            cont.push_back(0x80 |((unicodeValue >>  6) & 0x3F));
                            cont.push_back(0x80 |((unicodeValue >>  0) & 0x3F));
                        }
                        else
                        {
                            // We have a found first part of surrogate pair
                            unicodeCount    = 6;
                        }
                    }
                    else
                    {
                        // Surrogate pair
                        if ((unicodeValue & 0xFC00FC00) != 0xD800DC00)
                        {
                            throw std::runtime_error("ThorsAnvil::Serialize::UnicodeIterator: Push->Surrogate pair(No DC): \\uD8xx Must be followed by \\uDCxx");
                        }

                        // Decode surrogate pair
                        unicodeValue    = 0x00010000 | ((unicodeValue & 0x03FF0000) >> 6) | (unicodeValue & 0x000003FF);

                        // Encode as 4 UTF-8 characters
                        cont.push_back(0xF0 |((unicodeValue >> 18)));
                        cont.push_back(0x80 |((unicodeValue >> 12) & 0x3F));
                        cont.push_back(0x80 |((unicodeValue >>  6) & 0x3F));
                        cont.push_back(0x80 |((unicodeValue >>  0) & 0x3F));
                    }
                }
            }
        }
        else if (lastWasSlash)
        {
            switch(x)
            {
                case '"':   cont.push_back('"');    break;
                case '\\':  cont.push_back('\\');   break;
                case '/':   cont.push_back('/');    break;
                case 'b':   cont.push_back('\b');   break;
                case 'f':   cont.push_back('\f');   break;
                case 'n':   cont.push_back('\n');   break;
                case 'r':   cont.push_back('\r');   break;
                case 't':   cont.push_back('\t');   break;
                case 'u':   unicodeCount = 4; unicodeValue = 0; break;
            }
            lastWasSlash    = false;
        }
        else
        {
            if (x == '\\')
            {
                lastWasSlash    = true;
            }
            else
            {
                cont.push_back(x);
            }
        }
    }
};

template<typename I>
struct UnicodeWrapperIterator: std::iterator<std::input_iterator_tag, char, ptrdiff_t, char*,char&>
{
    I                   iter;
    std::vector<char>   cont;
    std::size_t         index;
    UnicodeWrapperIterator(I iter)
        : iter(iter)
        , index(0)
    {}
    bool operator==(UnicodeWrapperIterator const& rhs) const
    {
        return iter == rhs.iter;
    }
    bool operator!=(UnicodeWrapperIterator const& rhs) const
    {
        return !(*this == rhs);
    }
    UnicodeWrapperIterator& operator++()
    {
        ++index;
        if (index == cont.size())
        {
            ++iter;
        }
        return *this;
    }
    char operator*()
    {
        checkBuffer();
        return cont[index];
    }
    private:
    void checkBuffer()
    {
        if (index == cont.size())
        {
            cont.clear();
            index   = 0;

            cont.push_back(*iter);

            if (cont[0] == '\\')
            {
                ++iter;
                char next   = *iter;

                switch(next)
                {
                    case '"':   cont[0] = '"';    break;
                    case '\\':  cont[0] = '\\';   break;
                    case '/':   cont[0] = '/';    break;
                    case 'b':   cont[0] = '\b';   break;
                    case 'f':   cont[0] = '\f';   break;
                    case 'n':   cont[0] = '\n';   break;
                    case 'r':   cont[0] = '\r';   break;
                    case 't':   cont[0] = '\t';   break;
                    case 'u':   decodeUnicode();  break;
                    default:    cont[0] = next;   break;
                }
            }
        }
    }
    void decodeUnicode()
    {
        cont.clear();
        long    unicodeValue    = getUnicodeHex();

        if (unicodeValue <= 0x7F)
        {
            // Encode as single UTF-8 character
            cont.push_back(unicodeValue);
        }
        else if (unicodeValue <= 0x7FF)
        {
            // Encode as two UTF-8 characters
            cont.push_back(0xC0 |((unicodeValue >>  6)));
            cont.push_back(0x80 |((unicodeValue >>  0) & 0x3F));
        }
        else if (unicodeValue <= 0xFFFF)
        {
            if ((unicodeValue & 0xFC00) != 0xD800)
            {
                // Encode as three UTF-8 characters
                cont.push_back(0xE0 |((unicodeValue >> 12)));
                cont.push_back(0x80 |((unicodeValue >>  6) & 0x3F));
                cont.push_back(0x80 |((unicodeValue >>  0) & 0x3F));
            }
            else
            {
                // We have a found first part of surrogate pair
                decodeSurrogatePairs(unicodeValue);
            }
        }
    }
    void decodeSurrogatePairs(long unicodeValue)
    {
        ++iter;
        char nextChar  = *iter;
        if (nextChar != '\\')
        {
            throw std::runtime_error("ThorsAnvil::Serialize::UnicodeIterator: Iter->Surrogate pair(No Slash): \\uD8xx Must be followed by \\uDCxx");
        }
        ++iter;
        nextChar  = *iter;
        if (nextChar != 'u')
        {
            throw std::runtime_error("ThorsAnvil::Serialize::UnicodeIterator: Iter->Surrogate pair(No u): \\uD8xx Must be followed by \\uDCxx");
        }

        unicodeValue = (unicodeValue << 16) + getUnicodeHex();

        // Surrogate pair
        if ((unicodeValue & 0xFC00FC00) != 0xD800DC00)
        {
            throw std::runtime_error("ThorsAnvil::Serialize::UnicodeIterator: Iter->Surrogate pair(No DC): \\uD8xx Must be followed by \\uDCxx");
        }

        // Decode surrogate pair
        unicodeValue    = 0x00010000 | ((unicodeValue & 0x03FF0000) >> 6) | (unicodeValue & 0x000003FF);

        // Encode as 4 UTF-8 characters
        cont.push_back(0xF0 |((unicodeValue >> 18)));
        cont.push_back(0x80 |((unicodeValue >> 12) & 0x3F));
        cont.push_back(0x80 |((unicodeValue >>  6) & 0x3F));
        cont.push_back(0x80 |((unicodeValue >>  0) & 0x3F));
    }
    long getUnicodeHex()
    {
        long unicodeValue   = 0;

        for(int loop=0;loop < 4;++loop)
        {
            ++iter;
            char x = *iter;

            unicodeValue <<= 4;
            unicodeValue    += convertHexToDec(x);
        }
        return unicodeValue;
    }
};

template<typename C> UnicodePushBackIterator<C> make_UnicodePushBackIterator(C& cont)   {return UnicodePushBackIterator<C>(cont);}
template<typename I> UnicodeWrapperIterator<I>  make_UnicodeWrapperIterator(I iter)     {return UnicodeWrapperIterator<I>(iter);}

    }
}

#endif
