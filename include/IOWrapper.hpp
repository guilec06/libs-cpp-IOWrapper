#ifndef IOWRAPPER_HPP_
#define IOWRAPPER_HPP_

#include "AsyncIOInterface.hpp"

enum io_mode_t {
    READ,
    WRITE,
    RW
};

class IOWrapper {
    public:
        static IOWrapper open(std::string &path,
            bool create=true,
            io_mode_t mode=RW,
            mode_t permissions=0644);
};

#endif
