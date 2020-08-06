#ifndef THORS_ANVIL_SERIALIZE_IMPORTER_H
#define THORS_ANVIL_SERIALIZE_IMPORTER_H
/*
 * The Importer simply wrap an object of type T so that when de-serialized
 * it creates an object of type DeSerializer and calls it appropriately.
 */

#include "Serialize.h"

namespace ThorsAnvil
{
    namespace Serialize
    {

template<typename Format, typename T>
class Importer
{
    using ParserConfig = ParserInterface::ParserConfig;
    T&              value;
    ParserConfig    config;
    public:
        Importer(T& value, ParserConfig config = ParserConfig{})
            : value(value)
            , config(config)
        {}
        friend std::istream& operator>>(std::istream& stream, Importer const& data)
        {
            try
            {
                typename Format::Parser     parser(stream, data.config);
                DeSerializer                deSerializer(parser);

                deSerializer.parse(data.value);
            }
            catch (...)
            {
                stream.setstate(std::ios::failbit);
                if (!data.config.catchExceptions)
                {
                    throw;
                }
            }
            return stream;
        }
};

template<typename Format, typename T>
Importer<Format, T> Import(T const& value, ParserInterface::ParserConfig config = ParserInterface::ParserConfig{})
{
    return Importer<Format, T>(value, config);
}


    }
}

#endif
