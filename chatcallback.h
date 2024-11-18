#ifndef CHATCALLBACK_H
#define CHATCALLBACK_H

#include <stdint.h>
#include <QString>
#include <QObject>

class ChatCallBack : public QObject
{
    Q_OBJECT
public:
    ChatCallBack(QObject* parent = nullptr);
    virtual void onConnected(const uint64_t id) = 0;
    virtual void onClose(const uint64_t id) = 0;
    virtual void onHandShakeFinished(const uint64_t id) = 0;
    virtual void onTextMsg(const uint64_t id, const QString& text) = 0;
};

#endif // CHATCALLBACK_H
