#include <CLI/CLI.hpp>
#include "app_config.h"
#include "chatclient.h"
#include "chatserver.h"
#include "xlog.h"




int main(int argc, char** argv)
{
    CLI::App app{"enchat "};
    argv = app.ensure_utf8(argv);
    XLogMgr::get()->InitLog("./","enchat","enchat");
    SINFO("-------------------------------app start-------------------------------");
    AppConfig::parse2("cfg.json");

    ChatClient client;
    client.setServer("192.168.10.105",17799);
    client.setName("client");
    client.doConnect();

    ChatServer srv(17799);
    srv.run();

    return 0;
}
