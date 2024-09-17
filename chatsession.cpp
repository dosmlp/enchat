#include "chatsession.h"
#include "app_config.h"
#include <functional>
#include "XLog.h"

using namespace std::placeholders;

static const uint32_t MSG_HEAD_LEN = sizeof(uint16_t);

ChatSession::ChatSession(tcp::socket&& socket):
    socket_(std::forward<tcp::socket>(socket)),
    chacha_dctx_(new mbedtls_chacha20_context),
    chacha_ectx_(new mbedtls_chacha20_context),
    chacha20_key_(new uint8_t[32],[](uint8_t* p){delete p;}),
    peer_static_pubkey_(new uint8_t[32],[](uint8_t* p){delete p;}),
    ephemeral_pub_key_(new uint8_t[32],[](uint8_t* p){delete p;}),
    ephemeral_pri_key_(new uint8_t[32],[](uint8_t* p){delete p;}),
    msg_data_(new uint8_t[std::numeric_limits<uint16_t>::max()])
{
    std::memset(peer_static_pubkey_.get(),0,32);
    std::memset(chacha20_key_.get(),0,32);
    std::memset(ephemeral_pub_key_.get(),0,32);
    std::memset(ephemeral_pri_key_.get(),0,32);
    X25519_keypair(ephemeral_pub_key_.get(),ephemeral_pri_key_.get());

}

ChatSession::~ChatSession()
{
    socket_.close();
}

tcp::socket &ChatSession::getSocket()
{
    return socket_;
}

void ChatSession::sendHandshake(uint8_t t)
{
    std::shared_ptr<HelloPacket> hp(new HelloPacket);

    hp->type = t;
    if (!name_.empty()) {
        std::memcpy(hp->name,name_.data(),name_.size());
    }

    std::memcpy(hp->static_key,AppConfig::public_key,32);
    std::memcpy(hp->ephemeral_key,ephemeral_pub_key_.get(),32);
    hp->timestamp = 1;
    ED25519_sign(hp->sig,(const uint8_t*)hp.get(),sizeof(HelloPacket)-64,AppConfig::private_key);

    auto self = shared_from_this();
    asio::async_write(socket_,asio::buffer(hp.get(),sizeof(HelloPacket)),asio::transfer_exactly(sizeof(HelloPacket)),
        [self,hp](std::error_code ec,size_t size) {
            if (ec) {
                SERROR("sendHandshake error,{}",ec.message());
                return;
            }
            if (hp->type == 1) {
                //发送请求包时等待接收响应包
                SDEBUG("waiting for handshake...");
                self->receiveHandshake();
            }
        }
    );
}

void ChatSession::receiveHandshake()
{
    std::shared_ptr<HelloPacket> hp(new HelloPacket);

    auto self = shared_from_this();
    asio::async_read(socket_,asio::buffer(hp.get(),sizeof(HelloPacket)),asio::transfer_exactly(sizeof(HelloPacket)),
        [self,hp,this](std::error_code ec,size_t size)
        {
            if (ec) {
                SERROR("receiveHandshake error,{}",ec.message());
                return;
            }
            //验证签名
            if (ED25519_verify((const uint8_t*)hp.get(),sizeof(HelloPacket)-64,hp->sig,hp->static_key) == 0) {
                SERROR("ED25519_verify Fail");
                return;
            }
            //计算公共密钥
            //TODO 使用kdf生成公共密钥
            if (X25519(chacha20_key_.get(),ephemeral_pri_key_.get(),hp->ephemeral_key)) {
                initChaCha20();
            } else {
                SERROR("gen shared key fail.");
                return;
            }

            if (hp->type == 1) {
                //收到请求包时发送一个响应包
                SDEBUG("sendHandshake(2)");
                self->sendHandshake(2);
            }
                //开始接收消息
            self->startRead();

        }
    );
}

void ChatSession::writeMsg(std::unique_ptr<uint8_t> msg, uint16_t size)
{
    //TODO 使用mbedtls_chachapoly_update
    mbedtls_chacha20_update(chacha_ectx_.get(),size,msg.get(),msg.get());

    std::shared_ptr<uint8_t> data(new uint8_t[size+MSG_HEAD_LEN]);
    std::memcpy(data.get(),&size,MSG_HEAD_LEN);
    std::memcpy(data.get()+MSG_HEAD_LEN,msg.get(),size);

    auto self = shared_from_this();
    asio::async_write(socket_,
                      asio::buffer(data.get(),size+MSG_HEAD_LEN),
                      asio::transfer_exactly(size+MSG_HEAD_LEN),
                      [self,this,data](std::error_code ec, size_t ){
                          if (ec) {
                              SERROR("write msg fail:{}",ec.message());
                              return;
                          }

                      });
}

void ChatSession::setId(uint64_t id)
{
    id_ = id;
}

void ChatSession::setName(const std::string &name)
{
    name_ = name;
}

void ChatSession::startRead()
{
    asio::async_read(socket_,
                     asio::buffer(&msg_size_,sizeof(uint16_t)),
                     asio::transfer_exactly(2),
                     std::bind(&ChatSession::handleReadMsgHead,this,_1,_2,shared_from_this()));
}

void ChatSession::handleReadMsgHead(std::error_code ec, size_t size, Ptr self)
{
    if (ec) {
        SERROR("ReadMsgHead error:{}",ec.message());
        return;
    }
    std::memset(msg_data_.get(), 0, std::numeric_limits<uint16_t>::max());
    asio::async_read(socket_,
                     asio::buffer(msg_data_.get(),std::numeric_limits<uint16_t>::max()),
                     asio::transfer_exactly(msg_size_),
                     std::bind(&ChatSession::handleReadMsg,this,_1,_2,shared_from_this()));
}

void ChatSession::handleReadMsg(std::error_code ec, size_t size, Ptr self)
{
    if (ec) {
        SERROR("ReadMsg error:{}",ec.message());
        return;
    }

    mbedtls_chacha20_update(chacha_dctx_.get(),msg_size_,msg_data_.get(),msg_data_.get());
    std::cout << "receive:" << msg_data_.get() << "\n";
    startRead();
}

void ChatSession::initChaCha20()
{
    mbedtls_chacha20_init(chacha_dctx_.get());
    mbedtls_chacha20_setkey(chacha_dctx_.get(),chacha20_key_.get());
    mbedtls_chacha20_starts(chacha_dctx_.get(),chacha20_key_.get(),0);

    mbedtls_chacha20_init(chacha_ectx_.get());
    mbedtls_chacha20_setkey(chacha_ectx_.get(),chacha20_key_.get());
    mbedtls_chacha20_starts(chacha_ectx_.get(),chacha20_key_.get(),0);
}