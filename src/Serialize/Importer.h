#ifndef THORSANVIL_SERIALIZE_IMPORTER_H
#define THORSANVIL_SERIALIZE_IMPORTER_H
/*
 * The Importer simply wrap an object of type T so that when de-serialized
 * it creates an object of type DeSerializer and calls it appropriately.
 */

#include "SerializeConfig.h"
#include "Serialize.h"

namespace ThorsAnvil::Serialize
{

template<typename Format, typename T>
class Importer
{
    T&              value;
    ParserConfig    config;
    public:
        Importer(T& value, ParserConfig config = ParserConfig{})
            : value(value)
            , config(config)
        {}
        template<typename I>
        bool extract(I& stream) const
        {
            typename Format::Parser     parser(stream, config);
            try
            {
                DeSerializer                deSerializer(parser);

                deSerializer.parse(value);
            }
            catch (ThorsAnvil::Logging::CriticalException const& e)
            {
                ThorsCatchMessage("ThorsAnvil::Serialize::Importer", "operator>>", e.what());
                ThorsRethrowMessage("ThorsAnvil::Serialize::Importer", "operator>>", e.what());
                // This exception is thrown because you are using deprecated code
                // that was not designed to be used with the bsonExporter
                // This must be fixed. So we are forcing a re-throw becuase
                // the generated binary object is probably bad.
                throw;
            }
            catch (std::exception const& e)
            {
                ThorsCatchMessage("ThorsAnvil::Serialize::Importer", "operator>>", e.what());
                parser.setFail();
                if (!config.catchExceptions)
                {
                    ThorsRethrowMessage("ThorsAnvil::Serialize::Importer", "operator>>", e.what());
                    throw;
                }
            }
            catch (...)
            {
                ThorsCatchMessage("ThorsAnvil::Serialize::Importer", "operator>>", "UNKNOWN");
                parser.setFail();
                if (!config.catchExceptions)
                {
                    ThorsRethrowMessage("ThorsAnvil::Serialize::Importer", "operator>>", "UNKNOWN");
                    throw;
                }
            }
            return parser.ok();
        }
        friend std::istream& operator>>(std::istream& stream, Importer const& data)
        {
            data.extract(stream);
            return stream;
        }
        friend bool operator>>(std::string const& stream, Importer const& data)
        {
            return data.extract(stream);
        }
};

template<typename Format, typename T>
Importer<Format, T> Import(T const& value, ParserConfig config = ParserConfig{})
{
    return Importer<Format, T>(value, config);
}


}

#endif
