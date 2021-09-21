#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

#include "cfg/WritableConfig.hpp"
#include "MainWindow.hpp"

int main(int argc, char* argv[])
{
    using namespace ehb;

    auto log = spdlog::stdout_color_mt("log");

    // setup our application right away so we can start working with modal config windows
    QApplication application(argc, argv);

    WritableConfig config(argc, argv);
    config.dump("log");

    MainWindow* mainWindow = new MainWindow(config);
    mainWindow->show();

    return application.exec();
}

