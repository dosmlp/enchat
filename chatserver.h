#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QObject>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <mutex>
#include "asio.hpp"
#include "chatsession.h"
#include "Peer.h"

using namespace asio::ip;
using io_context_work = asio::executor_work_guard<asio::io_context::executor_type>;

class ChatServer : public QObject
{
    Q_OBJECT
public:
    using ChatSession = ChatSession<ChatServer>;
    using lock_guard = std::lock_guard<std::mutex>;
    ChatServer(uint16_t port, QObject* parent, int size = std::thread::hardware_concurrency()):
        QObject(parent),
        acceptor_io_ctx_(1),
        acceptor_(acceptor_io_ctx_, tcp::endpoint(make_address("::"),port)),
        works_(size),
        client_io_ctxs_(size)
    {
        for (int i = 0;i < size;++i) {
            works_[i] = std::make_unique<io_context_work>(asio::make_work_guard(client_io_ctxs_[i]));
            threads_ioctxs_.emplace_back(std::thread([this,i](){
                client_io_ctxs_[i].run();
            }));
        }
        doAccept();
        run();
    }

    ~ChatServer();
    bool sendTextMsg(const QString& id,const QString& text);
    void setEcKey(const QString& pri, const QString& pub)
    {
        static_prikey_ = QByteArray::fromBase64(pri.toLatin1());
        static_pubkey_ = QByteArray::fromBase64(pub.toLatin1());
    }
    void updatePeerList(QList<Peer::Ptr> &peers);
    void getEcKey(QByteArray& pri, QByteArray& pub)
    {
        pri = static_prikey_;
        pub = static_pubkey_;
    }
    bool containsPeerPubkey(const QByteArray& peer_pubkey)
    {
        lock_guard lk(mutex_sessmap_);
        QString pub_key = peer_pubkey.toBase64();
        for (auto& p:peer_list_)
        {
            if (p->pub_key == pub_key) return true;
        }
        return false;
    }


    void onConnected(const QString& id);
    void onClose(const QString& id);
    void onHandShakeFinished(const QString& id,ChatSession::Ptr sess);
    void onTextMsg(const QString& id, const QString& text);
signals:
    void sigConnected(const QString& id);
    void sigClose(const QString& id);
    void sigHandshakeFinished(const QString& id);
    void sigTextMsg(const QString& id, const QString& text);
private:
    void doAccept();
    void run()
    {
        thread_acceptor_ = std::thread([this](){
            acceptor_io_ctx_.run();
        });
    }

    asio::io_context& getIocontext()
    {
        auto& io = client_io_ctxs_[next_io_];
        if (next_io_ == client_io_ctxs_.size()) next_io_ = 0;
        return io;
    }
    QByteArray static_prikey_;
    QByteArray static_pubkey_;

    asio::io_context acceptor_io_ctx_;
    tcp::acceptor acceptor_;
    std::thread thread_acceptor_;

    uint16_t next_io_ = 0;
    std::vector<std::unique_ptr<io_context_work>> works_;
    std::vector<asio::io_context> client_io_ctxs_;
    std::vector<std::thread> threads_ioctxs_;

    std::map<QString,ChatSession::Ptr> sess_map_;
    QList<Peer::Ptr> peer_list_;
    std::mutex mutex_sessmap_;
};

#endif // CHATSERVER_H
