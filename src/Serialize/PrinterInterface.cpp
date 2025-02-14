#include "PrinterInterface.h"
#include "ThorsIOUtil/Utility.h"
#include "ThorsLogging/ThorsLogging.h"
#include <string>

using namespace ThorsAnvil::Serialize;

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
bool PrinterInterface::preflightSize()
{
    struct PreflightSize
    {
        bool operator()(std::ostream*)          {return false;}
        bool operator()(StringOutput& output)   {return output.preflightSize();}
    };
    return std::visit(PreflightSize{}, output);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void PrinterInterface::reserveSize()
{
    struct ReserveSize
    {
        void operator()(std::ostream*)          {}
        void operator()(StringOutput& output)   {output.reserveSize();}
    };
    std::visit(ReserveSize{}, output);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void PrinterInterface::finalizePrint()
{
    struct FinalizePrint
    {
        void operator()(std::ostream*)          {}
        void operator()(StringOutput& output)   {output.finalizePrint();}
    };
    std::visit(FinalizePrint{}, output);
}
