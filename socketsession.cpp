#include "socketsession.h"

SocketSession::SocketSession(tcp::socket socket):
    socket_(std::move(socket_))
{

}
