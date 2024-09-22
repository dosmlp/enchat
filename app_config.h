#pragma once

#include <cstdint>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include "XLog.h"

class AppConfig
{
public:
    void parse(const QString& file)
    {
        QFile file_cfg(file);
        if (!file_cfg.open(QIODevice::ReadOnly)) {
            SERROR("open cfg fail.");
            return;
        }
        QJsonParseError parse_error;
        QJsonDocument doc = QJsonDocument::fromJson(file_cfg.readAll(), &parse_error);
        if (doc.isEmpty() || doc.isNull() || parse_error.error != QJsonParseError::NoError) {
            SERROR("parse json error.");
            return;
        }

        QJsonObject obj_root = doc.object();
        if (!obj_root.contains("private_key") || !obj_root.contains("public_key")) {
            SERROR("private_key|public_key not found.");
            return;
        }

        auto pri = obj_root.value("private_key").toString();
        pri_key = QByteArray::fromBase64(pri.toLatin1());
        auto pub = obj_root.value("public_key").toString();
        pub_key = QByteArray::fromBase64(pub.toLatin1());

        server_port = obj_root.value("server_port").toInt(17799);
    }
    QByteArray pub_key;
    QByteArray pri_key;
    uint16_t server_port;
    static uint8_t public_key[32];
    static uint8_t private_key[64];
};
