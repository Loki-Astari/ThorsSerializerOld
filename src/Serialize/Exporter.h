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
    using OutputType = PrinterInterface::OutputType;
    T const&    value;
    OutputType  characteristics;

    public:
        Exporter(T const& value, OutputType characteristics)
            : value(value)
            , characteristics(characteristics)
        {}
        friend std::ostream& operator<<(std::ostream& stream, Exporter const& data)
        {
            typename Format::Printer    printer(stream, data.characteristics);
            Serializer                  serializer(printer);

            serializer.print(data.value);

            return stream;
        }
};

template<typename Format, typename T>
Exporter<Format, T> Export(T const& value, PrinterInterface::OutputType characteristics = PrinterInterface::OutputType::Default)
{
    return Exporter<Format, T>(value, characteristics);
}


    }
}

#endif
