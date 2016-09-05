
#ifndef THORS_ANVIL_SERIALIZE_TEST_BINARY_PARSER_TEST_H
#define THORS_ANVIL_SERIALIZE_TEST_BINARY_PARSER_TEST_H

#include "../Traits.h"
#include "../SerUtil.h"

class MapEmptyTest {};
class MapOneValue
{
    public:
        int     One;
};
class MapTwoValue
{
    public:
        int one;
        int two;
};
class MapThreeValue
{
    public:
        int one;
        int two;
        int three;
};
class MapWithArray
{
    public:
        std::vector<int>    one;
};
class MapWithTwoArray
{
    public:
        std::vector<int>        one;
        std::vector<int>        two;
};
class MapWithMap
{
    public:
        MapEmptyTest    one;
};
class MapWithTwoMap
{
    public:
        MapEmptyTest        one;
        MapEmptyTest        two;
};
class Base
{
    public:
        int                 ace;
        int                 val;
};
class Derived: public Base
{
    public:
        int                 der;
        float               flt;
};
ThorsAnvil_MakeTrait(MapEmptyTest);
ThorsAnvil_MakeTrait(MapOneValue, One);
ThorsAnvil_MakeTrait(MapTwoValue, one, two);
ThorsAnvil_MakeTrait(MapThreeValue, one, two, three);
ThorsAnvil_MakeTrait(MapWithArray, one);
ThorsAnvil_MakeTrait(MapWithTwoArray, one, two);
ThorsAnvil_MakeTrait(MapWithMap, one);
ThorsAnvil_MakeTrait(MapWithTwoMap, one, two);
ThorsAnvil_MakeTrait(Base, ace, val);
ThorsAnvil_ExpandTrait(Base, Derived, der, flt);

#endif

