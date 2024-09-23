#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <openssl/curve25519.h>
extern "C" {
#include <mbedtls/base64.h>
}
#include "XLog.h"

using namespace nlohmann;

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
    static void parse2(const std::string& file)
    {
        std::ifstream cfg_file(file);
        json root = json::parse(cfg_file,nullptr,false);
        if (root.is_discarded() || root.empty()) {
            SERROR("cfg error!");
            return;
        }
        auto private_key = root.value("private_key","");
        std::string pub_key = root.value("public_key","");
        size_t outlen = 0;

        mbedtls_base64_decode(AppConfig::private_key, 64, &outlen, (const uint8_t*)private_key.data(), private_key.length());
        mbedtls_base64_decode(AppConfig::public_key, 32, &outlen, (const uint8_t*)pub_key.data(), pub_key.length());
    }
    //生成ed25519签名密钥对
    static void ed25519()
    {
        uint8_t public_key[32] = {0};
        uint8_t private_key[64] = {0};
        ED25519_keypair(public_key, private_key);

        uint8_t out_base64[128] = {0};
        size_t olen = 0;
        mbedtls_base64_encode(out_base64, 128, &olen, public_key, 32);
        std::cout << "----------public_key----------\n";
        for (int i = 0;i < 32;++i) {
            std::cout << std::hex << (uint32_t)(public_key[i]);
        }
        std::cout << "\n";
        std::cout << out_base64 << "\n";

        memset(out_base64, 0, 128);
        mbedtls_base64_encode(out_base64, 128, &olen, private_key, 64);
        std::cout << "----------private_key----------\n";
        for (int i = 0;i < 64;++i) {
            std::cout << std::hex << (uint32_t)(private_key[i]);
        }
        std::cout << "\n";
        std::cout << out_base64 << "\n";
    }
    QByteArray pub_key;
    QByteArray pri_key;
    uint16_t server_port;
    static uint8_t public_key[32];
    static uint8_t private_key[64];
};
