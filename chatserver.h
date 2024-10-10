#ifndef CHATSERVER_H
#define CHATSERVER_H

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

class ChatServer
{
public:
    using ChatSession = ChatSession<ChatServer>;
    using lock_guard = std::lock_guard<std::mutex>;
    ChatServer(uint16_t port, int size = std::thread::hardware_concurrency()):
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
    }
    void run()
    {
        thread_acceptor_ = std::thread([this](){
            acceptor_io_ctx_.run();
        });
    }
    ~ChatServer();
    void setEcKey(const QByteArray& pri, const QByteArray& pub)
    {
        static_prikey_ = pri;
        static_pubkey_ = pub;
    }
    void updatePeerList(const QSet<Peer>& peers);
    void getEcKey(QByteArray& pri, QByteArray& pub)
    {
        pri = static_prikey_;
        pub = static_pubkey_;
    }
    bool containsPeerPubkey(const QByteArray& peer_pubkey)
    {
        lock_guard lk(mutex_sessmap_);
        Peer p = {QString(),peer_pubkey};
        return peer_list_.contains(p);
    }


    void onConnected(const uint64_t id);
    void onClose(const uint64_t id);
    void onHandShakeFinished(const uint64_t id);
    void onTextMsg(const uint64_t id, const QString& text);

private:
    void doAccept();

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

    std::map<uint64_t,ChatSession::Ptr> sess_map_;
    QSet<Peer> peer_list_;
    std::mutex mutex_sessmap_;
};

#endif // CHATSERVER_H
