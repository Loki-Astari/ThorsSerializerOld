#include "JsonPrinter.h"
#include "JsonThor.h"
#include "ThorsIOUtil/Utility.h"
#include "ThorsLogging/ThorsLogging.h"
#include <iomanip>
#include <algorithm>
#include <charconv>

using namespace ThorsAnvil::Serialize;

namespace
{
    struct Prefix
    {

        static char const*  space[];
        static char const*  comma[];
        static char const*  colon[];

        OutputType                  characteristics;
        std::size_t                 size;
        PrintState&                 state;
        bool                        prefixValue;
        public:
            Prefix(OutputType characteristics, std::size_t size, PrintState& state, bool prefixValue = false)
                : characteristics(characteristics)
                , size(size - 1)
                , state(state)
                , prefixValue(prefixValue)
            {}
            void printSeporator(PrinterInterface& printer, bool key) const
            {
                char const*(&seporator)[] = (!key && std::get<1>(state) == TraitType::Map)
                                            ? colon
                                            : (std::get<0>(state) != 0) ? comma : space;
                printer.write(seporator[static_cast<int>(characteristics)]);
            }
            void printIndent(PrinterInterface& printer)
            {
                static const    std::string indent(1000, '\t');
                printer.write("\n", 1);
                printer.write(std::string_view(&indent[0], size));
            }
            virtual void write(PrinterInterface& printer) = 0;
    };
    struct PrefixKey: public Prefix
    {
        using Prefix::Prefix;
        virtual void write(PrinterInterface& printer) override
        {
            printSeporator(printer, true);
            if (characteristics == OutputType::Stream) {
                return;
            }
            printIndent(printer);
        }
    };
    struct PrefixValue: public Prefix
    {
        using Prefix::Prefix;
        virtual void write(PrinterInterface& printer) override
        {
            printSeporator(printer, false);
            ++std::get<0>(state);
            std::get<2>(state) = true;

            if (characteristics == OutputType::Stream) {
                return;
            }
            if (std::get<1>(state) == TraitType::Array && std::get<0>(state) != 1) {
                printer.write(" ", 1);
            }
            if (std::get<1>(state) == TraitType::Map) {
                printer.write(" ", 1);
            }
        }
    };
    struct PrefixMap: public Prefix
    {
        using Prefix::Prefix;
        virtual void write(PrinterInterface& printer) override
        {
            printSeporator(printer, false);
            if (characteristics == OutputType::Stream) {
                return;
            }
            printIndent(printer);
        }
    };
    struct PrefixMapClose: public Prefix
    {
        using Prefix::Prefix;
        virtual void write(PrinterInterface& printer) override
        {
            ++std::get<0>(state);
            if (characteristics == OutputType::Stream) {
                return;
            }
            printIndent(printer);
        }
    };
    struct PrefixArray: public Prefix
    {
        using Prefix::Prefix;
        virtual void write(PrinterInterface& printer) override
        {
            printSeporator(printer, false);
            if (characteristics == OutputType::Stream) {
                return;
            }
            printIndent(printer);
        }
    };
    struct PrefixArrayClose: public Prefix
    {
        using Prefix::Prefix;
        virtual void write(PrinterInterface& printer) override
        {
            ++std::get<0>(state);
            if (characteristics == OutputType::Stream) {
                return;
            }
            if (prefixValue) {
                return;
            }
            printIndent(printer);
        }
    };
}

char const*  Prefix::space[]   = {"",  "",  ""};
char const*  Prefix::comma[]   = {",", ",", ","};
char const*  Prefix::colon[]   = {":", ":", ":"};


THORS_SERIALIZER_HEADER_ONLY_INCLUDE
JsonPrinter::JsonPrinter(std::ostream& output, PrinterConfig config)
    : PrinterInterface(output, config)
{
    state.emplace_back(0, TraitType::Value, false);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
JsonPrinter::JsonPrinter(std::string& output, PrinterConfig config)
    : PrinterInterface(output, config)
{
    state.emplace_back(0, TraitType::Value, false);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonPrinter::reset()
{
    state.clear();
    state.emplace_back(0, TraitType::Value, false);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonPrinter::openDoc()
{}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonPrinter::closeDoc()
{}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonPrinter::openMap(std::size_t)
{
    PrefixMap       prefix(config.characteristics, state.size(), state.back());
    prefix.write(*this);
    write("{", 1);
    state.emplace_back(0, TraitType::Map, false);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonPrinter::closeMap()
{
    if (std::get<1>(state.back()) != TraitType::Map)
    {
        ThorsLogAndThrow("ThorsAnvil::Serialize::JsonPrinter",
                         "closeMap",
                         "Invalid call to closeMap(): Currently not in a map");
    }
    bool prefixValue = std::get<2>(state.back());
    state.pop_back();
    PrefixMapClose  prefix(config.characteristics, state.size(), state.back(), prefixValue);
    prefix.write(*this);
    write("}", 1);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonPrinter::openArray(std::size_t)
{
    PrefixArray prefix(config.characteristics, state.size(), state.back());
    prefix.write(*this);
    write("[", 1);
    state.emplace_back(0, TraitType::Array, false);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonPrinter::closeArray()
{
    if (std::get<1>(state.back()) != TraitType::Array)
    {
        ThorsLogAndThrow("ThorsAnvil::Serialize::JsonPrinter",
                         "closeArray",
                         "Invalid call to closeArray(): Currently not in an array");
    }
    bool prefixValue = std::get<2>(state.back());
    state.pop_back();
    PrefixArrayClose    prefix(config.characteristics, state.size(), state.back(), prefixValue);
    prefix.write(*this);
    write("]", 1);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonPrinter::addKey(std::string_view const& key)
{
    if (std::get<1>(state.back()) != TraitType::Map)
    {
        ThorsLogAndThrow("ThorsAnvil::Serialize::JsonPrinter",
                         "addKey",
                         "Invalid call to addKey(): Currently not in a map");
    }
    PrefixKey   prefix(config.characteristics, state.size(), state.back());
    prefix.write(*this);
    write("\"", 1);
    write(key);
    write("\"", 1);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonPrinter::addPrefix()
{
    PrefixValue prefix(config.characteristics, state.size(), state.back());
    prefix.write(*this);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void JsonPrinter::addIndent()
{
    PrefixValue     prefix(config.characteristics, state.size(), state.back());
    prefix.write(*this);
}

template<typename T>
struct FormatDouble
{
    T const &  value;
    FormatDouble(T const& value): value(value){}
    friend std::ostream& operator<<(std::ostream& str, FormatDouble const& formater)
    {
        if (formater.value == 0)
        {
            return str << "0.0";
        }
        else
        {
            return str << formater.value;
        }
    }
};

struct BoolFormatter
{
    bool  value;
    BoolFormatter(bool value)
        : value(value)
    {}
    friend std::ostream& operator<<(std::ostream& str, BoolFormatter const& formatter)
    {
        return str << std::boolalpha << formatter.value;
    }
};

template<typename T>
inline std::to_chars_result to_chars(char* first, char* last, FormatDouble<T> const& value)
{
    static char   doubleZero[] = "0.0";
    if (value.value == 0)
    {
        std::copy(std::begin(doubleZero), std::end(doubleZero), first);
        return std::to_chars_result{first + 3, static_cast<std::errc>(0)};
    }
#if defined(HOMEBREW_OLD_VERSION_OF_MAC) && (HOMEBREW_OLD_VERSION_OF_MAC >= 1)
    ((void)last);
    std::string v = std::to_string(value.value);
    first = std::copy(std::begin(v), std::end(v), first);
    return std::to_chars_result{first, static_cast<std::errc>(0)};
#else
    return std::to_chars(first, last, value.value, std::chars_format::fixed, 6);
#endif
}
inline std::to_chars_result to_chars(char* first, char*, BoolFormatter const& value)
{
    static char const*  boolStr[2]   = {"false", "true"};
    static int          boolSize[2]  = {5, 4};
    std::copy(boolStr[value.value], boolStr[value.value] + boolSize[value.value], first);
    return std::to_chars_result{first + boolSize[value.value], static_cast<std::errc>(0)};
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addValue(short int value)             {addIndent(); writeValue(value);}
THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addValue(int value)                   {addIndent(); writeValue(value);}
THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addValue(long int value)              {addIndent(); writeValue(value);}
THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addValue(long long int value)         {addIndent(); writeValue(value);}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addValue(unsigned short int value)    {addIndent(); writeValue(value);}
THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addValue(unsigned int value)          {addIndent(); writeValue(value);}
THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addValue(unsigned long int value)     {addIndent(); writeValue(value);}
THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addValue(unsigned long long int value){addIndent(); writeValue(value);}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addValue(float value)                 {addIndent(); writeValue(FormatDouble<float>{value});}
THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addValue(double value)                {addIndent(); writeValue(FormatDouble<double>{value});}
THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addValue(long double value)           {addIndent(); writeValue(FormatDouble<long double>{value});}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addValue(bool value)                  {addIndent(); writeValue(BoolFormatter{value});}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addValue(std::string_view const&value)     {addIndent(); write("\"", 1); escapeString(*this, value); write("\"", 1);}
THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addRawValue(std::string_view const& value) {addIndent(); escapeString(*this, value);}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE void JsonPrinter::addNull()                             {addIndent(); write("null", 4);}
