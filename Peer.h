#ifndef PEER_H
#define PEER_H
#include <QJsonValue>
#include <QJsonObject>
#include <QSet>

struct Peer {
    typedef std::shared_ptr<Peer> Ptr;
    QString name;
    QByteArray pub_key;

    void fromJson(const QJsonValue& j)
    {
        if (const QJsonValue& v = j["public_key"]; v.isString()) {
            pub_key = QByteArray::fromBase64(v.toString().toLatin1());
        }
        if (const QJsonValue& v = j["name"]; v.isString()) {
            name = v.toString();
        }
    }
    QJsonValue toJson() const
    {
        QJsonObject o;
        o["public_key"] = QString(pub_key.toBase64());
        o["name"] = name;
        return o;
    }
};
inline bool operator==(const Peer &e1, const Peer &e2)
{
    return e1.pub_key == e2.pub_key;
}

inline size_t qHash(const Peer &key, size_t seed)
{
    return qHash(key.pub_key, seed);
}

#endif // PEER_H
