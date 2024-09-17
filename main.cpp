#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include "app_config.h"
#include "mbedtls.hpp"
#include <string>
#include "json.hpp"
#include "app_config.h"
#include <openssl/curve25519.h>
#include <openssl/base64.h>
#include "chatclient.h"
#include "chatserver.h"
#include "xlog.h"

using namespace nlohmann;

void ed25519()
{
    uint8_t out_public_key[32] = {0};
    uint8_t out_private_key[64] = {0};
    ED25519_keypair(out_public_key, out_private_key);

    uint8_t out_base64[128] = {0};
    EVP_EncodeBlock(out_base64, out_public_key, 32);
    for (int i = 0;i < 32;++i) {
        std::cout << std::hex << (uint32_t)(out_public_key[i]);
    }
    std::cout << "\n";
    std::cout << out_base64 << "\n";

    memset(out_base64, 0, 128);
    EVP_EncodeBlock(out_base64, out_private_key, 64);
    for (int i = 0;i < 64;++i) {
        std::cout << std::hex << (uint32_t)(out_private_key[i]);
    }
    std::cout << "\n";
    std::cout << out_base64 << "\n";
}

int main(int, char**)
{
    XLogMgr::get()->InitLog("./","enchat");
    std::ifstream cfg_file("cfg.json");
    json root = json::parse(cfg_file,nullptr,false);
    if (root.is_discarded() || root.empty()) {
        SERROR("cfg error!");
        return 0;
    }
    auto private_key = root.value("private_key","");
    std::string pub_key = root.value("public_key","");
    auto server_port = root.value("server_port",17799);
    size_t outlen = 0;
    EVP_DecodeBase64(AppConfig::private_key, &outlen, 66, (const unsigned char*)private_key.data(), private_key.length());
    EVP_DecodeBase64(AppConfig::public_key, &outlen, 66, (const unsigned char*)pub_key.data(), pub_key.length());

    // for (int i = 0;i < 32;++i) {
    //     std::cout << std::hex << (uint32_t)(AppConfig::public_key[i]);
    // }
    // std::cout << "\n";
    // for (int i = 0;i < 64;++i) {
    //     std::cout << std::hex << (uint32_t)(AppConfig::private_key[i]);
    // }
    // std::cout << "\n";
    //uint8_t seed[32],pub_key[32],pri_key[64];
    //ed25519_create_seed(seed);
    //ed25519_create_keypair(pub_key, pri_key, seed);

    //size_t base64_outlen = 0;
    //uint8_t base64_buf[128] = {0};
    //mbedtls_base64_encode(base64_buf, 128, &base64_outlen, pri_key, 64);
    //std::cout << base64_buf << "\n";
    //memset(base64_buf, 0, sizeof(base64_buf));
    //mbedtls_base64_encode(base64_buf, 128, &base64_outlen, pub_key, 32);
    //std::cout << base64_buf << "\n";
    //ed25519();
    SINFO("logtext");
    ChatServer server(tcp::endpoint(asio::ip::make_address("::"),9999));
    server.run();
    // ChatClient client;
    // client.exec();
    for (int i = 0;i < 100;++i) {
        SERROR("sdadasd{}",i);
    }
    XLogMgr::release();
    return 0;
}
