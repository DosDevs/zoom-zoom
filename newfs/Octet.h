#ifndef OCTET_H__INCLUDED
#define OCTET_H__INCLUDED


#include <string>


namespace NewFs {
    using std::string;

    class Octet 
    {
        private:
            unsigned char _value;

        public:
            Octet(unsigned char value = 0):
                _value(value)
            {}

            operator unsigned char() const
            { return int(_value) < 256? _value: 255; }
    };  // Octet
}

#endif  // OCTET_H__INCLUDED
