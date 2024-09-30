#pragma once
#include "asio.hpp"
#include <Windows.h>
#include <memory>
#include <set>
#include <string>
#include <iostream>
#include <functional>
#include <mutex>
#include <QString>
#include <QByteArray>
extern "C" {
#include <mbedtls/chacha20.h>
#include <mbedtls/chachapoly.h>
}
#include <openssl/curve25519.h>
#include "base/xlog.h"
#include "app_config.h"

using namespace asio::ip;
using namespace std::placeholders;
using uint8UPtr = std::unique_ptr<uint8_t>;

#pragma pack(push,1)
struct HelloPacket {
    uint8_t type = 1;//1请求2响应
    uint8_t name[32];
    uint8_t static_key[32];//ed25519永久公钥,用于验证签名
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

template<class CLIENT>
class ChatSession : public std::enable_shared_from_this<ChatSession<CLIENT>>
{
public:
    typedef std::shared_ptr<ChatSession<CLIENT>> Ptr;
    ChatSession(tcp::socket &&socket, CLIENT* client):
        socket_(std::forward<tcp::socket>(socket)),
        chacha_dctx_(new mbedtls_chacha20_context),
        chacha_ectx_(new mbedtls_chacha20_context),
        chacha20_key_(new uint8_t[32]),
        peer_name_(32,'\0'),
        ephemeral_pub_key_(new uint8_t[32]),
        ephemeral_pri_key_(new uint8_t[32]),
        client_(client)
    {
        id_ = genid();
        std::memset(chacha20_key_.get(),0,32);
        std::memset(ephemeral_pub_key_.get(),0,32);
        std::memset(ephemeral_pri_key_.get(),0,32);
        X25519_keypair(ephemeral_pub_key_.get(),ephemeral_pri_key_.get());
    }
    ~ChatSession()
    {
        socket_.close();
    }
    void close()
    {
        client_->onClose(id_);
        socket_.close();
    }
    tcp::socket& getSocket()
    {
        return socket_;
    }
    void sendHandshake(uint8_t t)
    {
        std::shared_ptr<HelloPacket> hp(new HelloPacket);

        hp->type = t;
        if (!name_.isEmpty()) {
            std::memcpy(hp->name,name_.data(),name_.size());
        }

        std::memcpy(hp->static_key,AppConfig::public_key,32);
        std::memcpy(hp->ephemeral_key,ephemeral_pub_key_.get(),32);
        hp->timestamp = 1;
        ED25519_sign(hp->sig,(const uint8_t*)hp.get(),sizeof(HelloPacket)-64,AppConfig::private_key);

        auto self = this->shared_from_this();
        SDEBUG("send handshake type:{}",hp->type);
        asio::async_write(socket_,
                          asio::buffer(hp.get(),sizeof(HelloPacket)),
                          asio::transfer_exactly(sizeof(HelloPacket)),
                          [self,hp](std::error_code ec,size_t size) {
                              if (ec) {
                                  SERROR("sendHandshake error,{}",ec.message());
                                  return;
                              }
                              if (hp->type == 1) {
                                  //发送请求包时等待接收响应包
                                  SDEBUG("waiting for hello...");
                                  self->receiveHandshake();
                              }
                          }
        );
    }
    void receiveHandshake()
    {
        std::shared_ptr<HelloPacket> hp(new HelloPacket);

        auto self = this->shared_from_this();

        asio::async_read(socket_,
                         asio::buffer(hp.get(),sizeof(HelloPacket)),
                         asio::transfer_exactly(sizeof(HelloPacket)),
                         std::bind(&ChatSession::onHandShake,this,_1,_2,hp,self)
        );
    }

    void writeMsg(std::unique_ptr<uint8_t> msg, uint16_t size)
    {
        //TODO 使用mbedtls_chachapoly_update
        mbedtls_chacha20_update(chacha_ectx_.get(),size,msg.get()+4,msg.get()+4);

        std::shared_ptr<uint8_t> data(msg.release());
        std::memcpy(data.get(),&size,2);

        auto self = this->shared_from_this();
        asio::async_write(socket_,
                          asio::buffer(data.get(),size+4),
                          asio::transfer_exactly(size+4),
                          [self,this,data] (std::error_code ec, size_t ) {
                              if (ec) {
                                  SERROR("write msg fail:{}",ec.message());
                                  return;
                              }

                          }
        );
    }
    void setId(uint64_t id) { id_ = id; }
    uint64_t id() const { return id_; }
    void setName(const QString& name) { name_ = name.toUtf8(); }
    void setPeerPubkey(const QByteArray& peer_pubkey) { peer_static_pubkey_ = peer_pubkey; }
private:
    void startRead()
    {
        std::shared_ptr<uint8_t> msg(new uint8_t[4096]);
        std::memset(msg.get(), 0, 4096);
        asio::async_read(socket_,
                         asio::buffer(msg.get(),sizeof(uint16_t)),
                         asio::transfer_exactly(2),
                         std::bind(&ChatSession::handleReadMsgHead,this,_1,_2,msg,this->shared_from_this())
        );

    }
    void handleReadMsgHead(std::error_code ec, size_t size,
                           std::shared_ptr<uint8_t> msg, ChatSession::Ptr self)
    {
        if (ec) {
            SERROR("ReadMsgHead error:{}",ec.message());
            return;
        }

        uint16_t msgsize = *((uint16_t*)msg.get());
        asio::async_read(socket_,
                         asio::buffer(msg.get()+2,msgsize+2),
                         asio::transfer_exactly(msgsize+2),
                         std::bind(&ChatSession::handleReadMsg,this,_1,_2,msg,this->shared_from_this())
        );

    }
    void handleReadMsg(std::error_code ec, size_t size,
                       std::shared_ptr<uint8_t> msg, ChatSession::Ptr self)
    {
        if (ec) {
            SERROR("ReadMsg error:{}",ec.message());
            return;
        }
        uint16_t msgsize = *((uint16_t*)msg.get());
        uint16_t protocol = *((uint16_t*)msg.get()+2);

        if (protocol == 1) {
            mbedtls_chacha20_update(chacha_dctx_.get(),
                                    msgsize, msg.get()+4,
                                    msg.get()+4);
            client_->onTextMsg(id_, QString::fromUtf8(msg.get()+4));
        }


        startRead();

    }
    void onHandShake(std::error_code ec, size_t size,
                     std::shared_ptr<HelloPacket> hp, std::shared_ptr<ChatSession<CLIENT>> self)
    {
        if (ec) {
            SERROR("receiveHandshake error,{}",ec.message());
            return;
        }
        //验证签名
        if (peer_static_pubkey_.isEmpty()) {
            QByteArray pk((const char*)hp->static_key,32);
            if (AppConfig::containsPeerPubkey(pk)) {
                peer_static_pubkey_ = pk;
            } else {
                self->close();
                return;
            }
        }
        if (ED25519_verify((const uint8_t*)hp.get(),sizeof(HelloPacket)-64,hp->sig,(const uint8_t*)peer_static_pubkey_.data()) == 0) {
            SERROR("ED25519_verify Fail");
            self->close();
            return;
        }

        std::memcpy(this->peer_name_.data(), hp->name, 32);
        //计算公共密钥
        //TODO 使用kdf生成公共密钥
        if (X25519(this->chacha20_key_.get(),this->ephemeral_pri_key_.get(),hp->ephemeral_key)) {
            this->initChaCha20();
            SINFO("handshake success.");
        } else {
            SERROR("gen shared key fail.");
            return;
        }

        if (hp->type == 1) {
            //收到请求包时发送一个响应包
            SDEBUG("sendHandshake(2)");
            this->sendHandshake(2);
        }
        //握手完成开始接收消息
        SDEBUG("handshake done,start read msg.");
        client_->onHandShakeFinished(id_);
        self->startRead();

    }
    void initChaCha20()
    {
        mbedtls_chacha20_init(chacha_dctx_.get());
        mbedtls_chacha20_setkey(chacha_dctx_.get(),chacha20_key_.get());
        mbedtls_chacha20_starts(chacha_dctx_.get(),chacha20_key_.get(),0);

        mbedtls_chacha20_init(chacha_ectx_.get());
        mbedtls_chacha20_setkey(chacha_ectx_.get(),chacha20_key_.get());
        mbedtls_chacha20_starts(chacha_ectx_.get(),chacha20_key_.get(),0);

    }
    tcp::socket socket_;

    uint64_t id_;

    static uint64_t genid()
    {
        static std::set<uint64_t> set_id_;
        static std::mutex mutex_set_;

        mutex_set_.lock();
        uint64_t gid = 0;
        for (;;) {
            BCryptGenRandom(NULL,(PUCHAR)(&gid),sizeof(uint64_t),BCRYPT_USE_SYSTEM_PREFERRED_RNG);
            if (set_id_.contains(gid)) {
                continue;
            }
            break;
        }
        set_id_.insert(gid);
        mutex_set_.unlock();
        return gid;
    }
    QByteArray name_;

    //chacha20密钥
    uint8UPtr chacha20_key_;
    std::unique_ptr<mbedtls_chacha20_context> chacha_dctx_;
    std::unique_ptr<mbedtls_chacha20_context> chacha_ectx_;

    //对端永久公钥,用于身份认证
    QByteArray peer_static_pubkey_;
    //对端名称
    QByteArray peer_name_;
    //临时公钥
    uint8UPtr ephemeral_pub_key_;
    //临时私钥
    uint8UPtr ephemeral_pri_key_;

    CLIENT* client_;
};
