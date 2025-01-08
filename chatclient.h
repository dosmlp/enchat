#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <string>
#include <thread>
#include <QObject>
#include <QString>
#include <map>
#include "asio.hpp"
#include "chatsession.h"

using namespace asio::ip;
using io_context_guard = asio::executor_work_guard<asio::io_context::executor_type>;


class ChatClient : public QObject
{
    Q_OBJECT
public:
    // using ChatSession = ChatSession<ChatClient>;
    ChatClient(QObject* parent, const QString& name);
    ~ChatClient();
    void doConnect(const std::string &addr, uint16_t port, const QString &peer_pubkey);
    void close();

    void setName(const QString& name);
    bool sendTextMsg(const QString& id, const QString& text);
    void setEcKey(const QString& pri, const QString& pub)
    {
        static_prikey_ = QByteArray::fromBase64(pri.toLatin1());
        static_pubkey_ = QByteArray::fromBase64(pub.toLatin1());
    }
    void getEcKey(QByteArray& pri, QByteArray& pub)
    {
        pri = static_prikey_;
        pub = static_pubkey_;
    }
    bool containsPeerPubkey(const QByteArray&);

    //回调
    void onConnected(const QString& id);
    void onClose(const QString& id);
    void onHandShakeFinished(const QString& id, ChatSession<ChatClient>::Ptr sess);
    void onTextMsg(const QString& id, const QString& text);
signals:
    void sigConnected(const QString& id);
    void sigClose(const QString& id);
    void sigHandshakeFinished(const QString& id);
    void sigTextMsg(const QString& id, const QString& text);
private:
    void connect(const tcp::resolver::results_type& endpoints, const QString& peer_pubkey);
    void run();
    asio::io_context io_ctx_;
    io_context_guard io_guard_;
    std::map<QString,ChatSession<ChatClient>::Ptr> sess_map_;
    std::thread thread_run_;
    QString name_;
    QByteArray static_prikey_;
    QByteArray static_pubkey_;
};

#endif // CHATCLIENT_H
