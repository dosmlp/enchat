#ifndef SOCKETSESSION_H
#define SOCKETSESSION_H

#include "asio.hpp"
#include <memory>
using namespace asio::ip;
class SocketSession : public std::enable_shared_from_this<SocketSession>
{

public:
    SocketSession(tcp::socket socket);
private:
    tcp::socket socket_;
};

#endif // SOCKETSESSION_H
