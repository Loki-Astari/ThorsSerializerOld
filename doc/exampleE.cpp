#include <iostream>
#include <vector>
#include "ThorSerialize/JsonThor.h"

enum class EnumType : int {
    A, B, C
};

struct MyStruct {
    EnumType    e;
    std::string s;
};

ThorsAnvil_MakeEnum(EnumType, A, B, C);
ThorsAnvil_MakeTrait(MyStruct, e, s);

int main()
{
    using ThorsAnvil::Serialize::jsonImporter;
    using ThorsAnvil::Serialize::jsonExporter;

    MyStruct    val {EnumType::A, "This string"};
    std::cout << jsonExporter(val) << "\n";
}
