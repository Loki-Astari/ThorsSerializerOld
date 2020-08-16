#ifndef THORS_ANVIL_SERIALIZE_EXPORTER_H
#define THORS_ANVIL_SERIALIZE_EXPORTER_H
/*
 * The Exporter simply wrap an object of type T so that when serialized
 * it creates an object of type Serializer and calls it appropriately.
 */

#include "Serialize.h"

namespace ThorsAnvil
{
    namespace Serialize
    {

template<typename Format, typename T>
class Exporter
{
    using PrinterConfig = PrinterInterface::PrinterConfig;
    T const&        value;
    PrinterConfig   config;
    public:
        Exporter(T const& value, PrinterConfig config)
            : value(value)
            , config(config)
        {}
        friend std::ostream& operator<<(std::ostream& stream, Exporter const& data)
        {
            try
            {
                typename Format::Printer    printer(stream, data.config);
                Serializer                  serializer(printer);

                serializer.print(data.value);
            }
            catch (ThorsAnvil::Serialize::CriticalException const&)
            {
                // This exception is thrown because you are using deprecated code
                // that was not designed to be used with the bsonExporter
                // This must be fixed. So we are forcing a re-throw becuase
                // the generated binary object is probably bad.
                throw;
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
Exporter<Format, T> Export(T const& value, PrinterInterface::PrinterConfig config = PrinterInterface::PrinterConfig{})
{
    return Exporter<Format, T>(value, config);
}


    }
}

#endif
