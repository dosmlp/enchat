#include <CLI/CLI.hpp>
#include "base/mbedtls.h"
#include "app_config.h"
#include "chatclient.h"
#include "chatserver.h"
#include "base/xlog.h"
#include "base/qjson_helper.h"

int main(int argc, char** argv)
{
    CLI::App app{"enchat "};
    argv = app.ensure_utf8(argv);
    XLogMgr::get()->InitLog("./","enchat","enchat");
    SINFO("-------------------------------app start-------------------------------");
    init_mbedtls();
    // AppConfig::parse2("cfg.json");
    AppConfig ac;
    ac.parse("cfg.json");

    // ChatClient client;
    // client.setName("client");
    // client.doConnect();

    // ChatServer srv(17799);
    // srv.run();
    // for (int i = 0;i < 10;++i) {
    //     AppConfig::ed25519();
    // }
    return 0;
}
