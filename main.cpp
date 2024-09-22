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

    AppConfig::ed25519();
    return 0;
}
