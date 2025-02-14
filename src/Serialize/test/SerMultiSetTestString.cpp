#include "SerializeConfig.h"
#include "gtest/gtest.h"
#include "JsonThor.h"
#include "BsonThor.h"
#include "SerUtil.h"
#include <algorithm>

namespace TS = ThorsAnvil::Serialize;

TEST(SerMultiSetTestString, Jsonserialize)
{
    std::multiset<int>  data{34,24,8,11,2,2,2};

    std::string         stream;
    stream << TS::jsonExporter(data, false);
    std::string result = stream;
    result.erase(std::remove_if(std::begin(result), std::end(result), [](char x){return ::isspace(x);}), std::end(result));

    EXPECT_EQ(result, R"([2,2,2,8,11,24,34])");
}

TEST(SerMultiSetTestString, JsonbeSerialize)
{
    std::multiset<int>  data;

    std::string         stream(R"([5,6,8,8,101,123])");
    stream >> TS::jsonImporter(data, false);

    EXPECT_TRUE(data.find(5) != data.end());
    EXPECT_TRUE(data.find(6) != data.end());
    EXPECT_TRUE(data.find(8) != data.end());
    EXPECT_TRUE(data.find(101) != data.end());
    EXPECT_TRUE(data.find(123) != data.end());
    EXPECT_EQ(data.count(8), 2);
}

TEST(SerMultiSetTestString, Bsonserialize)
{
    std::multiset<int>  data{34,24,8,11,2,2,2};

    std::string         stream;
    stream << TS::bsonExporter(data, false);
    std::string result = stream;

    static const char expectedRaw[]
                = "\x36\x00\x00\x00"
                  "\x10" "0\x00" "\x02\x00\x00\x00"
                  "\x10" "1\x00" "\x02\x00\x00\x00"
                  "\x10" "2\x00" "\x02\x00\x00\x00"
                  "\x10" "3\x00" "\x08\x00\x00\x00"
                  "\x10" "4\x00" "\x0B\x00\x00\x00"
                  "\x10" "5\x00" "\x18\x00\x00\x00"
                  "\x10" "6\x00" "\x22\x00\x00\x00"
                  "\x00";
    std::string expected(std::begin(expectedRaw), std::end(expectedRaw) - 1);
    EXPECT_EQ(result, expected);
    //NOTE OUTPUT (result, R"([2,2,2,8,11,24,34])");
}

TEST(SerMultiSetTestString, BsonbeSerialize)
{
    std::multiset<int>  data;

    //NOTE INPUT (R"([5,6,8,8,101,123])");
    static const char inputRaw[]
                = "\x2F\x00\x00\x00"
                  "\x10" "0\x00" "\x05\x00\x00\x00"
                  "\x10" "1\x00" "\x06\x00\x00\x00"
                  "\x10" "2\x00" "\x08\x00\x00\x00"
                  "\x10" "3\x00" "\x08\x00\x00\x00"
                  "\x10" "4\x00" "\x65\x00\x00\x00"
                  "\x10" "5\x00" "\x7b\x00\x00\x00"
                  "\x00";
    std::string input(std::begin(inputRaw), std::end(inputRaw) - 1);
    std::string         stream(input);
    stream >> TS::bsonImporter(data, false);

    EXPECT_TRUE(data.find(5) != data.end());
    EXPECT_TRUE(data.find(6) != data.end());
    EXPECT_TRUE(data.find(8) != data.end());
    EXPECT_TRUE(data.find(101) != data.end());
    EXPECT_TRUE(data.find(123) != data.end());
    EXPECT_EQ(data.count(8), 2);
}

