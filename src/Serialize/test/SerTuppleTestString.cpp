#include "SerializeConfig.h"
#include "gtest/gtest.h"
#include "JsonThor.h"
#include "BsonThor.h"
#include "SerUtil.h"
#include <algorithm>

namespace TS = ThorsAnvil::Serialize;

TEST(SerTuppleTestString, Jsonserialize)
{
    std::tuple<int, double>  data {56, 78.901};

    std::string              stream;
    stream << TS::jsonExporter(data, false);
    std::string result = stream;
    result.erase(std::remove_if(std::begin(result), std::end(result), [](char x){return ::isspace(x);}), std::end(result));

    EXPECT_EQ(result, R"([56,78.901000])");
}

TEST(SerTuppleTestString, JsondeSerialize)
{
    std::string         stream(R"([59,22.801])");
    std::tuple<int, double>  data {56, 78.925};

    stream >> TS::jsonImporter(data, false);
    EXPECT_EQ(59,     std::get<0>(data));
    EXPECT_EQ(22.801, std::get<1>(data));
}

TEST(SerTuppleTestString, Bsonserialize)
{
    std::tuple<int, double>  data {56, 78.901};

    std::string              stream;
    stream << TS::bsonExporter(data, false);
    std::string result = stream;

    static const char expectedRaw[]
                = "\x17\x00\x00\x00"
                  "\x10" "0\x00" "\x38\x00\x00\x00"
                  "\x01" "1\x00" "\x8b\x6c\xe7\xfb\xa9\xb9\x53\x40"
                  "\x00";
    std::string expected(std::begin(expectedRaw), std::end(expectedRaw) - 1);
    EXPECT_EQ(result, expected);
    //NOTE OUTPUT (result, R"([56,78.901])");
}

TEST(SerTuppleTestString, BsondeSerialize)
{
    //NOTE INPUT (R"([59,22.801])");
    static const char inputRaw[]
                = "\x17\x00\x00\x00"
                  "\x10" "0\x00"    "\x3B\x00\x00\x00"
                  "\x01" "1\x00"    "\x93\x18\x04\x56\x0e\xcd\x36\x40"
                  "\x00";
    std::string input(std::begin(inputRaw), std::end(inputRaw));
    std::string         stream(input);
    std::tuple<int, double>  data {56, 78.925};

    stream >> TS::bsonImporter(data, false);
    EXPECT_EQ(59,     std::get<0>(data));
    EXPECT_EQ(22.801, std::get<1>(data));
}

