#ifndef IOWRAPPER_HPP_
#define IOWRAPPER_HPP_

#include "AsyncIOInterface.hpp"

enum io_mode_t {
    READ,
    WRITE,
    RW
};

class IOWrapper;
class FileIOWrapper;
class SocketIOWrapper;

class IOWrapper {
    public:
        static FileIOWrapper open(std::string &path,
            bool create=true,
            io_mode_t mode=RW,
            mode_t permissions=0644);
};

class FileIOWrapper : public IOWrapper {
    int write(std::string &buffer, int n = -1);
    int read(std::string &buffer, int n = -1);
};

class SocketIOWrapper : public IOWrapper {
    int send(std::string &buffer, int n = -1);  
    int recv(std::string &buffer, int n = -1);
};

#endif
