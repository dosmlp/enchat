#pragma once
#include "asio.hpp"
#include <memory>
#include <string>
#include <iostream>
#include <QString>
#include <QByteArray>
extern "C" {
#include <mbedtls/chacha20.h>
#include <mbedtls/chachapoly.h>
}
#include <openssl/curve25519.h>


using namespace asio::ip;
using uint8UPtr = std::unique_ptr<uint8_t,void(*)(uint8_t*)>;

#pragma pack(push,1)
struct HelloPacket {
    uint8_t type = 1;//1请求2响应
    uint8_t name[32];
    uint8_t static_key[32];//ed25519永久公钥,用于签名
    uint8_t ephemeral_key[32];//临时公钥,交换密钥
    uint32_t timestamp = 0;
    uint8_t sig[64];
    HelloPacket()
    {
        std::memset(name, 0, sizeof(name));
        std::memset(static_key, 0, sizeof(static_key));
        std::memset(ephemeral_key, 0, sizeof(ephemeral_key));
        std::memset(sig, 0, sizeof(sig));
    }
};
#pragma pack(pop)
struct MsgPacket {
    typedef std::shared_ptr<MsgPacket> Ptr;
    uint16_t size = 0;
    uint16_t protocol = 0;
    std::unique_ptr<uint8_t> msg;
    MsgPacket(const uint16_t size):msg(new uint8_t[size])
    {
    }
};

class ChatSession : public std::enable_shared_from_this<ChatSession>
{
public:
    typedef std::shared_ptr<ChatSession> Ptr;
    ChatSession(tcp::socket &&socket);
    ~ChatSession();
    tcp::socket& getSocket();
    void sendHandshake(uint8_t t);
    void receiveHandshake();

    void writeMsg(std::unique_ptr<uint8_t> msg, uint16_t size);
    void setId(uint64_t id);
    void setName(const QString& name);
private:
    void startRead();
    void handleReadMsgHead(std::error_code ec, size_t size, std::shared_ptr<uint8_t> msg, ChatSession::Ptr self);
    void handleReadMsg(std::error_code ec, size_t size, std::shared_ptr<uint8_t> msg, ChatSession::Ptr self);
    void initChaCha20();
    tcp::socket socket_;

    uint64_t id_;
    QByteArray name_;

    uint8UPtr chacha20_key_;
    std::unique_ptr<mbedtls_chacha20_context> chacha_dctx_;
    std::unique_ptr<mbedtls_chacha20_context> chacha_ectx_;

    uint8UPtr peer_static_pubkey_;
    uint8UPtr ephemeral_pub_key_;
    uint8UPtr ephemeral_pri_key_;

    uint16_t msg_size_ = 0;
    std::unique_ptr<uint8_t> msg_data_;
};
