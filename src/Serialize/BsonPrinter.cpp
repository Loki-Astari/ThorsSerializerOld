#include "BsonPrinter.h"
#include "MongoUtility.h"
#include "ThorsIOUtil/Utility.h"
#include "ThorsLogging/ThorsLogging.h"
#include <iomanip>
#include <algorithm>

using namespace ThorsAnvil::Serialize;

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
BsonPrinter::BsonPrinter(std::ostream& output, BsonPrinterConfig config)
    : PrinterInterface(output, config)
    , idStore(config.idStore)
    , projection(false)
{}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
BsonPrinter::BsonPrinter(std::string& output, BsonPrinterConfig config)
    : PrinterInterface(output, config)
    , idStore(config.idStore)
    , projection(false)
{}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::pushLevel(bool isMap)
{
    currentContainer.emplace_back(isMap ? BsonContainer::Map : BsonContainer::Array);
}
THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::popLevel()
{
    currentContainer.pop_back();
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
bool BsonPrinter::needToInsertId() const
{
    // We will add the _id field on the following condition.
    // 1: The user has requested this by proving the idStore so we can output the new ObjectID.
    // 2: We are in an array of objects.
    // 3: The array is inside a map
    // The array being inside a amp mirrors the pattern used by Mongo.
    // See: https://www.mongodb.com/docs/manual/reference/command/insert/
    // The "Insert" command is the Map which contains the field "documents" which is an array
    // of documents that will be inserted.
    return idStore.has_value() && currentContainer.size() == 2 && currentContainer[0] == BsonContainer::Map && currentContainer[1] == BsonContainer::Array;
}

// MAP
THORS_SERIALIZER_HEADER_ONLY_INCLUDE
std::size_t BsonPrinter::getSizeMap(std::size_t count)
{
    /*
     * A map is a document:
     *      <size 4 bytes> <Element-List> <Terminator 1 byte>
     * Each element int the <Element-List> consists of:
     *      <type 1 byte> <e-name (size accounted for) + '\0' 1 byte> <object (size accounted for)>
     * If AddId is true then we will insert and _id member into the map.
     *      <type 1byte(0x07) <e_name _id\x00>  <Object ID 12-byte>
     */
    std::size_t addIdSize = 0;
    if (needToInsertId()) {
        addIdSize = 17;
    }

    return 4                            // Size
           + (count * (1 + 1))          // Type plus '\0' for each item
           + addIdSize                  // Size from extra ID field being inserted.
           + 1;                         // Terminator
}

// ARRAY
THORS_SERIALIZER_HEADER_ONLY_INCLUDE
std::size_t BsonPrinter::getSizeArray(std::size_t count)
{
    /*
     * A map is a document:
     *      <size 4bytes> <Element-List> <Terminator 1byte>
     * Each element int the <Element-List> consists of:
     *      <type 1byte> <e-name (which is a string of the index number) + '\0' 1byte> <object (size accounted for)>
     */
     /*
      * This is the same as Map (see above)
      * The difference we have not accounted for the string that define the index
      * So we must calculate the string length need to store all the index strings.
      */
    std::size_t indexTotalStringLen     = 0;
    std::size_t accountedFor            = 0;
    std::size_t nextLevel               = 10;
    std::size_t numberOfDigitsThisLevel = 1;

    while (nextLevel <= count)
    {
        indexTotalStringLen     += (nextLevel - accountedFor) * numberOfDigitsThisLevel;
        accountedFor             = nextLevel;
        numberOfDigitsThisLevel += 1;
        nextLevel               *= 10;
    }
    indexTotalStringLen += (count - accountedFor) * numberOfDigitsThisLevel;

    return 4                            // Size
           + (count * (1 + 1))          // Type plus '\0' for each item
           + indexTotalStringLen        // The string lengths.
           + 1;                         // Terminator
}

// Add a new Key
THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::addKey(std::string_view const& key)
{
    if (currentContainer.back() != BsonContainer::Map)
    {
        ThorsLogAndThrow("ThorsAnvil::Serialize::BsonPrinter",
                         "addKey",
                         "Adding a Key to a non map object");
    }
    currentKey = key;
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::writeKey(char value, std::size_t size)
{
    if (!currentContainer.empty())
    {
        write(&value, 1);
        if (currentContainer.back() == BsonContainer::Array)
        {
            std::stringstream   ss;
            ss << arrayIndex.back();
            write(ss.str());
            write("", 1);
            ++arrayIndex.back();
        }
        else
        {
            write(currentKey.c_str(), currentKey.size() + 1);
        }
    }
    else if (size != static_cast<std::size_t>(-1))
    {
        // This happens if you try and write a basic type directly to the stream.
        // BSON only supports Map/Array as the top level object.
        // So when we write a single value we wrap it just like an array.
        //
        // <4 byte Doc Size> <1 byte Type info> <2 byte Index "0"> <value> <1 byte doc term>
        std::int32_t totalSize = static_cast<std::int32_t>(4 + 1 + 2 + size + 1);
        writeSize<4, std::int32_t>(totalSize);
        write(&value, 1);
        write("0", 2);
        // The value will now write itself.
        // then the docClose() will at the document terminator.
    }
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::openDoc()
{}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::closeDoc()
{
    if (config.parserInfo == static_cast<long>(BsonContainer::Value))
    {
        // The Map and Array close themselves.
        // But values need to be closed here.
        // See:  writeKey() for details.
        write("", 1);
    }
}
THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::openMap(std::size_t size)
{
    writeKey('\x03', -1);
    writeSize<4, std::int32_t>(static_cast<std::int32_t>(size));

    bool shouldAddID = needToInsertId();
    currentContainer.emplace_back(BsonContainer::Map);

    if (shouldAddID)
    {
        addKey("_id");
        writeKey('\x07', 12);
        idStore->get().emplace_back();
        (*this) << idStore->get().back();
    }
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::closeMap()
{
    if (currentContainer.back() != BsonContainer::Map)
    {
        ThorsLogAndThrow("ThorsAnvil::Serialize::BsonPrinter",
                         "closeMap",
                         "Closing an unopened Map");
    }
    write("",1);
    currentContainer.pop_back();
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::openArray(std::size_t size)
{
    writeKey('\x04', -1);
    writeSize<4, std::int32_t>(static_cast<std::int32_t>(size));
    currentContainer.emplace_back(BsonContainer::Array);
    arrayIndex.emplace_back(0);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::closeArray()
{
    if (currentContainer.back() != BsonContainer::Array)
    {
        ThorsLogAndThrow("ThorsAnvil::Serialize::BsonPrinter",
                         "closeArray",
                         "Closing an unopened Array");
    }
    write("",1);
    currentContainer.pop_back();
    arrayIndex.pop_back();
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::writeBool(bool value)
{
    writeKey('\x08', 1);
    char outVal = (value ? '\x01' : '\x00');
    write(&outVal, 1);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::writeString(std::string_view const& value)
{
    writeKey('\x02', 4 + value.size() + 1);
    writeSize<4, std::int32_t>(static_cast<std::int32_t>(value.size() + 1));
    escapeString(*this, value);
    write("", 1);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::writeNull()
{
    writeKey('\x0A', 0);
}

THORS_SERIALIZER_HEADER_ONLY_INCLUDE
void BsonPrinter::writeBinary(std::string_view const& value)
{
    writeKey('\x05', 4 + 1 + value.size());    // binary
    writeSize<4, std::int32_t>(static_cast<std::int32_t>(value.size()));
    write("\x80", 1);
    write(std::begin(value), value.size());
}
