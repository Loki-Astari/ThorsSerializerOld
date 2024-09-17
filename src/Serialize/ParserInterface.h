#ifndef THORSANVIL_SERIALIZER_PARSER_INTERFACE_H
#define THORSANVIL_SERIALIZER_PARSER_INTERFACE_H

#include "SerializeConfig.h"
#include "ThorsSerializerUtilTypes.h"
#include "ParserConfig.h"
#include "StringInput.h"
#include <cstddef>
#include <variant>
#include <string>
#include <string_view>
#include <iostream>
#include <map>
#include <any>
#include <memory>
#include <concepts>

namespace ThorsAnvil::Serialize
{
class StreamInputInterface
{
    public:
        virtual ~StreamInputInterface() {}
        virtual bool            read(char* dst, std::size_t size)       = 0;
        virtual bool            readTo(std::string& dst, char delim)    = 0;
        virtual std::size_t     lastReadCount() const                   = 0;
        virtual std::streampos  getPos()                                = 0;
        virtual int             get()                                   = 0;
        virtual int             peek()                                  = 0;
        virtual void            ignore(std::size_t size)                = 0;
        virtual void            clear()                                 = 0;
        virtual void            unget()                                 = 0;
        virtual bool            ok() const                              = 0;
        virtual void            setFail()                               = 0;

        virtual bool            readValue(short int&)                   = 0;
        virtual bool            readValue(int&)                         = 0;
        virtual bool            readValue(long int&)                    = 0;
        virtual bool            readValue(long long int&)               = 0;

        virtual bool            readValue(unsigned short int&)          = 0;
        virtual bool            readValue(unsigned int&)                = 0;
        virtual bool            readValue(unsigned long int&)           = 0;
        virtual bool            readValue(unsigned long long int&)      = 0;

        virtual bool            readValue(float&)                       = 0;
        virtual bool            readValue(double&)                      = 0;
        virtual bool            readValue(long double&)                 = 0;

        virtual bool            readValue(char& value)                  = 0;
        virtual int             peekNextNonSpaceValue()                 = 0;
};
class StreamInputInterfaceStdStream: public StreamInputInterface
{
    std::istream&   input;
    public:
        StreamInputInterfaceStdStream(std::istream& input)
            : input(input)
        {}
        virtual bool            read(char* dst, std::size_t size)       override {return static_cast<bool>(input.read(dst, size));}
        virtual bool            readTo(std::string& dst, char delim)    override {return static_cast<bool>(std::getline(input, dst, delim));}
        virtual std::size_t     lastReadCount() const                   override {return input.gcount();}
        virtual std::streampos  getPos()                                override {return input.tellg();}
        virtual int             get()                                   override {return input.get();}
        virtual int             peek()                                  override {return input.peek();}
        virtual void            ignore(std::size_t size)                override {input.ignore(size);}
        virtual void            clear()                                 override {input.clear();}
        virtual void            unget()                                 override {input.unget();}
        virtual bool            ok() const                              override {return !input.fail();}
        virtual void            setFail()                               override {input.setstate(std::ios::failbit);}

        template<typename T>
        bool readInteger(T& value)
        {
            bool ok = static_cast<bool>(input >> value);
            char next = input.peek();
            return ok && next != '.' && next != 'e' && next != 'E';
        }

        virtual bool            readValue(short int& value)             override {return readInteger(value);}
        virtual bool            readValue(int& value)                   override {return readInteger(value);}
        virtual bool            readValue(long int& value)              override {return readInteger(value);}
        virtual bool            readValue(long long int& value)         override {return readInteger(value);}

        virtual bool            readValue(unsigned short int& value)    override {return readInteger(value);}
        virtual bool            readValue(unsigned int& value)          override {return readInteger(value);}
        virtual bool            readValue(unsigned long int& value)     override {return readInteger(value);}
        virtual bool            readValue(unsigned long long int& value)override {return readInteger(value);}

        template<typename T>
        bool readFloat(T& value)
        {
            return static_cast<bool>(input >> value);
        }

        virtual bool            readValue(float& value)                 override {return readFloat(value);}
        virtual bool            readValue(double& value)                override {return readFloat(value);}
        virtual bool            readValue(long double& value)           override {return readFloat(value);}

        virtual bool            readValue(char& value)                  override {return static_cast<bool>(input >> value);}
        virtual int             peekNextNonSpaceValue()                 override
        {
            char value;
            bool ok = static_cast<bool>(input >> value);
            switch (value)
            {
                case '{':
                case '}':
                case '[':
                case ']':
                case ',':
                case ':':
                    break;
                default:
                    input.unget();
            }
            return ok ? value : -1;
        }
};
class StreamInputInterfaceString: public StreamInputInterface
{
    StringInput   input;
    public:
        StreamInputInterfaceString(std::string_view const& input)
            : input(input)
        {}
        virtual bool            read(char* dst, std::size_t size)       override {return input.read(dst, size);}
        virtual bool            readTo(std::string& dst, char delim)    override {return input.readTo(dst, delim);}
        virtual std::size_t     lastReadCount() const                   override {return input.getLastReadCount();}
        virtual std::streampos  getPos()                                override {return input.getPos();}
        virtual int             get()                                   override {return input.get();}
        virtual int             peek()                                  override {return input.peek();}
        virtual void            ignore(std::size_t size)                override {input.ignore(size);}
        virtual void            clear()                                 override {input.clear();}
        virtual void            unget()                                 override {input.unget();}
        virtual bool            ok() const                              override {return input.isOk();}
        virtual void            setFail()                               override {input.setFail();}

        virtual bool            readValue(short int& value)             override {return readInteger(value);}
        virtual bool            readValue(int& value)                   override {return readInteger(value);}
        virtual bool            readValue(long int& value)              override {return readInteger(value);}
        virtual bool            readValue(long long int& value)         override {return readInteger(value);}

        virtual bool            readValue(unsigned short int& value)    override {return readInteger(value);}
        virtual bool            readValue(unsigned int& value)          override {return readInteger(value);}
        virtual bool            readValue(unsigned long int& value)     override {return readInteger(value);}
        virtual bool            readValue(unsigned long long int& value)override {return readInteger(value);}

        template<typename T>
        bool readInteger(T& value)
        {
            bool ok = input.readValue(value);
            char next = input.peek();
            return ok && next != '.' && next != 'e' && next != 'E';
        }

        virtual bool            readValue(float& value)                 override {return readFloat(value);}
        virtual bool            readValue(double& value)                override {return readFloat(value);}
        virtual bool            readValue(long double& value)           override {return readFloat(value);}

        template<typename T>
        bool readFloat(T& value)
        {
            return input.readValue(value);
        }

        virtual bool            readValue(char& value)                  override {return input.readValue(value);}
        virtual int             peekNextNonSpaceValue()                 override
        {
            char value;
            input.readValue(value);
            switch (value)
            {
                case '{':
                case '}':
                case '[':
                case ']':
                case ',':
                case ':':
                    break;
                default:
                    input.unget();
            }
            return input.isOk() ? value : -1;
        }
};


class ParserInterface
{
    public:
        ParserConfig const config;

        ParserInterface(std::string_view const& str, ParserConfig  config = ParserConfig{})
            : config(config)
            , input(std::make_unique<StreamInputInterfaceString>(str))
            , pushBack(ParserToken::Error)
        {}
        ParserInterface(std::istream& stream, ParserConfig  config = ParserConfig{})
            : config(config)
            , input(std::make_unique<StreamInputInterfaceStdStream>(stream))
            , pushBack(ParserToken::Error)
        {}
        virtual ~ParserInterface() {}
        virtual FormatType       formatType()            = 0;
                ParserToken      getToken();
                void             pushBackToken(ParserToken token);
        virtual ParserToken      getNextToken()          = 0;
        virtual std::string_view getKey()                = 0;

        virtual void    ignoreDataValue()               {}
        virtual void    ignoreDataMap(bool)             {}
        virtual void    ignoreDataArray(bool)           {}

        virtual void    getValue(short int&)             = 0;
        virtual void    getValue(int&)                   = 0;
        virtual void    getValue(long int&)              = 0;
        virtual void    getValue(long long int&)         = 0;

        virtual void    getValue(unsigned short int&)    = 0;
        virtual void    getValue(unsigned int&)          = 0;
        virtual void    getValue(unsigned long int&)     = 0;
        virtual void    getValue(unsigned long long int&)= 0;

        virtual void    getValue(float&)                 = 0;
        virtual void    getValue(double&)                = 0;
        virtual void    getValue(long double&)           = 0;

        virtual void    getValue(bool&)                  = 0;

        virtual void    getValue(std::string&)           = 0;

        virtual bool    isValueNull()                    = 0;

        virtual std::string_view getRawValue()           = 0;

        void    ignoreValue();

        bool            read(char* dst, std::size_t size)       {return input->read(dst, size);}
        bool            readTo(std::string& dst, char delim)    {return input->readTo(dst, delim);}
        std::size_t     lastReadCount() const                   {return input->lastReadCount();}
        std::streampos  getPos()                                {return input->getPos();}
        int             get()                                   {return input->get();}
        int             peek()                                  {return input->peek();}
        void            ignore(std::size_t size)                {return input->ignore(size);}
        void            clear()                                 {return input->clear();}
        void            unget()                                 {return input->unget();}
        bool            ok() const                              {return input->ok();}
        void            setFail()                               {return input->setFail();}
        template<typename T>
        bool            readValue(T& value)                     {return input->readValue(value);}
        int             peekNextNonSpaceValue()                 {return input->peekNextNonSpaceValue();}

        template<typename T>
        void getShared(SharedInfo<T> const& info, std::shared_ptr<T>& object)
        {
            std::intmax_t index = info.sharedPtrName;
            if (info.data.has_value())
            {
                object.reset(info.data.value());
                savedSharedPtr[index] = object;
                return;
            }
            std::shared_ptr<T>  sharedPtr = std::any_cast<std::shared_ptr<T>>(savedSharedPtr[index]);
            object = sharedPtr;
        }

    private:
        //using DataInputStream = std::variant<std::istream*, StringInput>;
        //DataInputStream input;
        std::unique_ptr<StreamInputInterface>   input;

        ParserToken     pushBack;
        std::map<std::intmax_t, std::any>     savedSharedPtr;
        void    ignoreTheValue();
        void    ignoreTheMap();
        void    ignoreTheArray();

};


}

#if defined(THORS_SERIALIZER_HEADER_ONLY) && THORS_SERIALIZER_HEADER_ONLY == 1
#include "ParserInterface.source"
#endif

#endif
