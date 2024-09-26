#include "chatsession.h"
#include "app_config.h"
#include <functional>
#include "chatclient.h"



static const uint32_t MSG_HEAD_LEN = sizeof(uint16_t);

// template<class CLIENT>
// ChatSession<CLIENT>::ChatSession(tcp::socket&& socket, CLIENT *client):

// {


// }
// template<class CLIENT>
// ChatSession<CLIENT>::~ChatSession()
// {

// }
// template<class CLIENT>
// void ChatSession<CLIENT>::close()
// {

// }
// template<class CLIENT>
// tcp::socket &ChatSession<CLIENT>::getSocket()
// {
//     return socket_;
// }
// template<class CLIENT>
// void ChatSession<CLIENT>::sendHandshake(uint8_t t)
// {

// }
// template<class CLIENT>
// void ChatSession<CLIENT>::receiveHandshake()
// {


// }

// template<class CLIENT>
// void ChatSession<CLIENT>::startRead()
// {
// }
// template<class CLIENT>
// void ChatSession<CLIENT>::handleReadMsgHead(std::error_code ec, size_t size, std::shared_ptr<uint8_t> msg, Ptr self)
// {
// }
// template<class CLIENT>
// void ChatSession<CLIENT>::handleReadMsg(std::error_code ec, size_t size, std::shared_ptr<uint8_t> msg, Ptr self)
// {
// }
// template<class CLIENT>
// void ChatSession<CLIENT>::initChaCha20()
// {
// }
