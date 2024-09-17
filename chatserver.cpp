#include "chatserver.h"
#include "XLog.h"


ChatServer::~ChatServer()
{
    acceptor_.close();
    acceptor_io_ctx_.stop();
    thread_acceptor_.join();

    for (auto& work : works_) {
        work.reset();
    }
    // for (auto& io : client_io_ctxs_) {
    //     io.stop();
    // }
    for (auto& thread : threads_ioctxs_) {
        thread.join();
    }
}

void ChatServer::doAccept()
{
    //auto client = std::make_shared<ChatSession>(tcp::socket(getIocontext()),tcp::endpoint());
    acceptor_.async_accept(getIocontext(),
                           [this](std::error_code ec,tcp::socket socket)
                           {
                               if (!ec) {
                                   std::make_shared<ChatSession>(std::move(socket))->receiveHandshake();
                                   doAccept();
                               } else {
                                   SERROR("async_accept errorss{}",0);
                               }

                           }
                           );
}
