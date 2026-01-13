#ifndef IOWRAPPER_HPP_
#define IOWRAPPER_HPP_

#include "AsyncIOInterface.hpp"

enum io_mode_t {
    READ,
    WRITE,
    RW
};

using IOCallback = std::function<void(int result, int error)>;

class IOWrapper;
class FileIOWrapper;
class SocketIOWrapper;

class IOWrapper {
    public:
        static FileIOWrapper open(const std::string &path,
            bool create=true,
            io_mode_t mode=RW,
            mode_t permissions=0644);
        static SocketIOWrapper connect(const std::string &host, int port);
        static SocketIOWrapper listen(int port, int backlog = 10);
    protected:
        int fd;
};

class FileIOWrapper : public IOWrapper {
    public:
        int write(std::string &buffer, int n = -1);
        int read(std::string &buffer, int n = -1);
        int asyncWrite(std::string &buffer, int n = -1, IOCallback callback = nullptr);
        int asyncRead(std::string &buffer, int n = -1, IOCallback callback = nullptr);
        void flush();
        off_t seek(off_t offset, int whence = SEEK_SET);
        off_t tell() const;
    protected:
        io_mode_t mode;
};

class SocketIOWrapper : public IOWrapper {
    public:
        int send(std::string &buffer, int n = -1);  
        int recv(std::string &buffer, int n = -1);
        SocketIOWrapper accept();
        void setNonBlocking(bool enable);
        void setTCPNoDelay(bool enable);
    protected:
        std::string host;
        int port;
};

#endif
