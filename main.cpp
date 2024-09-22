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
extern "C" {
#include <mbedtls/base64.h>
}
#include <openssl/curve25519.h>
#include <openssl/base64.h>
#include "chatclient.h"
#include "chatserver.h"
#include "xlog.h"

using namespace nlohmann;

//生成ed25519签名密钥对
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
    XLogMgr::get()->InitLog("./","enchat","enchat");
    std::ifstream cfg_file("cfg.json");
    json root = json::parse(cfg_file,nullptr,false);
    if (root.is_discarded() || root.empty()) {
        SERROR("cfg error!");
        return 0;
    }
    auto private_key = root.value("private_key","");
    std::string pub_key = root.value("public_key","");
    size_t outlen = 0;

    mbedtls_base64_decode(AppConfig::private_key, 64, &outlen, (const uint8_t*)private_key.data(), private_key.length());
    mbedtls_base64_decode(AppConfig::public_key, 32, &outlen, (const uint8_t*)pub_key.data(), pub_key.length());


    return 0;
}
