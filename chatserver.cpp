#include "chatserver.h"
#include "xlog.h"


ChatServer::~ChatServer()
{
    acceptor_.close();
    acceptor_io_ctx_.stop();
    thread_acceptor_.join();

    for (auto& work : works_) {
        work.reset();
    }
    for (auto& io : client_io_ctxs_) {
        io.stop();
    }
    for (auto& thread : threads_ioctxs_) {
        thread.join();
    }
}

void ChatServer::updatePeerList(const QSet<Peer> &peers)
{
    lock_guard lk(mutex_sessmap_);
    peer_list_ = peers;
}

void ChatServer::onConnected(const uint64_t id)
{

}

void ChatServer::onClose(const uint64_t id)
{
    lock_guard lk(mutex_sessmap_);
    sess_map_.erase(id);

}

void ChatServer::onHandShakeFinished(const uint64_t id)
{

}

void ChatServer::onTextMsg(const uint64_t id, const QString &text)
{

}

void ChatServer::doAccept()
{
    acceptor_.async_accept(getIocontext(),
                           [this](std::error_code ec,tcp::socket socket)
                           {
                               if (!ec) {
                                   auto sess = std::make_shared<ChatSession>(std::move(socket),this);
                                   sess->receiveHandshake();
                                   mutex_sessmap_.lock();
                                   sess_map_.insert({sess->id(),sess});
                                   mutex_sessmap_.unlock();
                                   onConnected(sess->id());
                                   doAccept();
                               } else {
                                   SERROR("async_accept error:{}",ec.message());
                               }

                           }
    );
}
