[![endorse](http://api.coderwall.com/lokiastari/endorsecount.png)](http://coderwall.com/lokiastari)
[![Build Status](https://travis-ci.org/Loki-Astari/ThorsSerializer.svg?branch=master)](https://travis-ci.org/Loki-Astari/ThorsSerializer)

![ThorStream](../img/stream.jpg)

##Objective:

    The objective is to make serialization/de-serialization of C++ object to/from
    JSON/YAML/Binary trivial.

    This means:
        1) does not build a JSON/YAML object. Reads data directly into C++ object.
        2) In normal usage there should be NO need to write any code.
        3) User should not need to understand JSON/YAML or validate its input.
        4) Should work seamlessly with streams.
        5) Standard containers should automatically work

    I am not concerned about speed.
    Though my trivial test work just fine in terms of speed.
    
    The design was done with the primary goal of communicating with WEB-Servers
    that speak JSON. The main envisioned usage was for mobile devices were many
    small JSON objects are transfered in both directions.

##Marking your class Serializeable/Deserializable

* Include the header file: `ThorSerialize/Traits.h`
* Use one of these macros to declare your type as serializable
 * `ThorsAnvil_MakeTrait(<Type>, <members>...)`
 * `ThorsAnvil_ExpandTrait(<Parent-Type>, <Type>, <members>...)`

````bash
    Type:           The name of a class (includeing namespace) of the type
                    you want to be able to serialize at some point.
    Parent-Type:    The name of a class that has previously been declared
                    using `ThorsAnvil_MakeTrait` or `ThorsAnvil_ExpandTrait`
                    and the parent of `Type`
    members:        A list of member (names) of the class `Type` that need
                    to be serialized.
````

The two macros above build a template specialization of the class `ThorsAnvil::Serialize::Traits<Type>` specific to your class. As a consequence these macros should not be placed inside any namespace blocks.

````c++
    #include "ThorSerialize/Traits.h"

    namespace MyNameSpace
    {
        class MyClass
        {
            public:
                int x;
        };
        // This will fail as it is being used inside the `MyNameSpace` namespace
        ThorsAnvil_MakeTrait(MyClass, x);
    }

    // The correct location to use the macro is here
    ThorsAnvil_MakeTrait(MyNameSpace::MyClass, x);
````

##Private Members
If any members of the class that need to be serialized are private you must define a friendship to allow the `Traits<X>` class to have access to the private members.

````c++
    #include "ThorSerialize/Traits.h"

    namespace MyNameSpace
    {
        class MyClass
        {
            // Allow the traits class to access private members of your class.
            friend class ThorsAnvil::Serialize::Traits<MyClass>;
            double  y;
            public:
                int x;

        };
    }

    ThorsAnvil_MakeTrait(MyNameSpace::MyClass, x, y);
````

##Standard containers
The appropriate declarations for all the standard containers are provided. You simply need to include "ThorSerialize/SerUtil.h" to include these declarations.

````c++
    #include "ThorSerialize/SerUtil.h"
    #include "ThorSerialize/JsonThor.h"
    #include <vector>
    #include <iostream>

    int main()
    {
        using ThorsAnvil::Serialize::jsonExport;
        using ThorsAnvil::Serialize::PrinterInterface;
        std::vector<int>    data {1,2,3,4,5};
        std::cout << ThorsAnvil::Serialize::jsonExport(data, PrinterInterface::OutputType::Stream);
    }
````

##Serialization

###Json
* Include the header file "ThorSerialize/JsonThor.h".
* There are two functions in the namespace `ThorsAnvil::Serialize`.
 * `jsonExport(<YourObject>, characteristics = Default);`
 * `jsonImport(<YourObject>);`

Both these methods return an object that simply contains a reference to `YourObject` (no actual serialization happens). When this object is serialized to a stream using `operator<<` or `operator>>` respectively then the code will read/write the appropriate members and serialize/deserialzie them to/from the stream. Because the returned object contains a reference to the object that needs to be serialized; the lifespan should be shorted than `YourObject` to avoid a dangling reference. Therefore it is usually best to just use them directly in the stream operation.

````c++
    std::vector<int>        data{1,2,3,4,5,6};

    std::cout << jsonExport(data);
    std::cin  >> jsonImport(data);
````

On export there is a second parameter `characteristics` that allows some simple control on serialization (it basically affects white space to make debugging easier). Values are:
````bash
     Default:     What ever the implementation likes.
                  Currently this means `Config` but I have plans for an
                  application level setting that is checked.
     Stream:      Compressed for over the wire protocol.            ie. No Space.
     Config:      Human readable                                    Potentially config file like.
````

###Yaml

The description above is for Json Serialization/Deserialization. But the exact same description can be used for Yaml. Simply replace Json with Yaml and replace json with yaml.

The export parameter `characteristics` has slightly different meaning for printing yaml. See the [libyaml documentation](http://libyaml.sourcearchive.com/documentation/0.1.1/group__styles_g1efef592e2e3df6f00432c04ef77d98f.html) for the meaning of these flags.
````bash
     Default:     What ever the implementation likes.
                  Currently this means YAML_BLOCK_MAPPING_STYLE but I have plans for an
                  application level setting that is checked.
     Stream:      YAML_FLOW_MAPPING_STYLE
     Config:      YAML_BLOCK_MAPPING_STYLE
````
###Binary

The description above is for Json Serialization/Deserialization. But the exact same description can be used for Binary versions. Simply replace Json with Binary and replace json with binary.

The export parameter `characteristics` has no affect on binary.

##Notes on std::map (Json)

The JSON "Object" is a set of "name"/"value" pairs. But the name part is always a "String". If you use a `std::map<Key, Value>` where the "Key" is a `std::string` then the `std::map<>` will be represented by a JSON "Object". If any other type is used as the "Key" then `std::map<>` will be represented as a Json "Array" where each member of the array is `std::pair<Key,Value>`.

````c++
    // Example:
    int main()
    {
        std::map<std::string, int>      data1{{"M": 1}};
        std::cout << jsonExport(data1) << "\n";             // {"M":1}

        std::map<int,int>               data2{{15,2}};
        std::cout << jsonExport(data2) << "\n";             // [{"first":15, "second":2}]
    }

##Example-1 [See doc/example1.cpp](example1.cpp)

````c++
    #include "ThorSerialize/Traits.h"
    #include "ThorSerialize/JsonThor.h"

    struct Color
    {
        int     red;
        int     green;
        int     blue;
    };    
    class TeamMember
    {
        std::string     name;
        int             score;
        int             damage;
        Color           team;
        public:
            TeamMember(std::string const& name, int score, int damage, Color const& team)
                : name(name)
                , score(score)
                , damage(damage)
                , team(team)
            {}
            // Define the trait as a friend to get accesses to private
            // Members.
            friend class ThorsAnvil::Serialize::Traits<TeamMember>;
    };

    // Declare the traits.
    // Specifying what members need to be serialized.
    ThorsAnvil_MakeTrait(Color, red, green, blue);
    ThorsAnvil_MakeTrait(TeamMember, name, score, damage, team);

    int main()
    {
        using ThorsAnvil::Serialize::jsonExport;

        TeamMember          mark("mark", 10, 5, Color{255,0,0});
        // Use the export function to serialize
        std::cout << jsonExport(mark) << "\n";
    }
````

###Build and run
````bash
    > g++ -std=c++14 expample1.cpp -lThorSerialize14
    > ./a.out
        { 
            "name": "mark", 
            "score": 10, 
            "damage": 5, 
            "team": 
            { 
                "red": 255, 
                "green": 0, 
                "blue": 0
            }
        }
````
##Example-2: [see doc/example2.cpp](example2.cpp)

````c++
    #include <string>
    #include "ThorSerialize/Traits.h"
    #include "ThorSerialize/SerUtil.h"
    #include "ThorSerialize/JsonThor.h"

    /* A class that you want to serialize. */
    class MyClass
    {   
        int         data1;
        float       data2;
        std::string data3;
        public:
            MyClass(int data1, float data2, std::string const& data3)
                : data1(data1)
                , data2(data2)
                , data3(data3)
            {}

        // This is only required if the members are private.
        friend struct ThorsAnvil::Serialize::Traits<MyClass>;
    };  


    /*  
     * Though there is no code involved, you do need to set up
     * this structure to tell the library what fields need to be serialized.
     * To do this use the macro:  ThorsAnvil_MakeTrait()
     * Specifying your class, and a list of members to serialize.
     */
    ThorsAnvil_MakeTrait(MyClass, data1, data2, data3);
````
    
This allows us to import and export object of the above class really easily.

````c++
    int main()
    {
        using ThorsAnvil::Serialize::jsonExport;
        using ThorsAnvil::Serialize::PrinterInterface;

        MyClass   data {56, 23.456, "Hi there"};


        // This generates a simple Json Object (wordy)
        std::cout << "Version 1\n";
        std::cout << jsonExport(data) << "\n\n\n";

        // This generates a compact Json 
        std::cout << "Version 2 (Stream)\n";
        std::cout << jsonExport(data, PrinterInterface::OutputType::Stream) << "\n\n\n";

        // Standard containers work automatically.
        // As long as the type held by the container has had an appropriate
        // Traits declaration.
        std::vector<MyClass>   vec(4, data);
        std::cout << "Vector\n";
        std::cout << jsonExport(vec) << "\n";
    }
````
    
This generates:

````bash
    > g++ -std=c++14 -o example2 example2.cpp -lThorSerialize14
    > ./example2
    Version 1
     
        { 
            "data1": 56, 
            "data2": 23.456, 
            "data3": "Hi there"
        }


    Version 2 (Stream)
    {"data1":56,"data2":23.456,"data3":"Hi there"}


    Vector
     [ 
            { 
                "data1": 56, 
                "data2": 23.456, 
                "data3": "Hi there"
            }, 
            { 
                "data1": 56, 
                "data2": 23.456, 
                "data3": "Hi there"
            }, 
            { 
                "data1": 56, 
                "data2": 23.456, 
                "data3": "Hi there"
            }, 
            { 
                "data1": 56, 
                "data2": 23.456, 
                "data3": "Hi there"
            }]
````

#Building Instructions:
````bash
    > git clone git@github.com:Loki-Astari/ThorsSerializer.git
    > cd ThorsSerializer
    > ./configure --disable-binary
    > make
    > sudo make install
````
## Description
By default installation will be in `/usr/local/[include/lib]`. You can override this with the normal auto-tools defaults. Use `./configure --help` to get details.

###What is installed:
* `/usr/local/include/ThorSerialize/*`
* `/usr/local/include/ThorBinaryRep/*`
* `/usr/local/lib/libThorSerialize14.so`
* `/usr/local/lib/libThorSerialize14D.so`

Note:
libThorSerialize14.so is build using `-O3` and thus is fully optimized and debug symbols have been stripped.  
libThorSerialize14D.so is build using `-g` and is useful for debugging purposes.


###What is Downloaded
The configuration processes will download the generic makefiles (using git) from [ThorMaker](https://github.com/Loki-Astari/ThorMaker) which in turn will download and build google's gtest library that is used in running the unit tests.

##Requirements
This library uses features from C++14 so you will need a compiler that supports this. The generic makefile also does code coverage tests so your compiler will also need to support a code coverage tool that has an interface similar to `gcov`.

It has been tested on [travis-ci.org](https://travis-ci.org/Loki-Astari/ThorsSerializer) using clang 3.5 and g++ 4.9 (on mac and ubuntu). Though clang 3.4 also supports C++14 its code coverage tool is very basic and the generic makefiles will fail when attempting to perform code coverage tests.

The yaml serialization uses libyaml so you will need that installed locally. The configuration files attempt to detect in the default location. If it is not there detailed instructions of your options are provided.

##Configuration Flags

You can disable some of the serialization code with:
````bash
    --disable-yaml
    --disable-binary
````

Note: Because the binary serialization is still experimental I force you to explicitly turn in on with:
````bash
    --with-thors-network-byte-order
````
