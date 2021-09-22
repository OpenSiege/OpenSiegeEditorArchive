#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

// make sure to include vsg headers before qt ones
#include <vsg/io/Options.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

#include "cfg/WritableConfig.hpp"
#include "SiegePipeline.hpp"
#include "MainWindow.hpp"

int main(int argc, char* argv[])
{
    using namespace ehb;

    auto log = spdlog::stdout_color_mt("log");

    // setup our application right away so we can start working with modal config windows
    QApplication application(argc, argv);

    WritableConfig config(argc, argv);
    config.dump("log");

    // initiate blocking siege systems for now, this should probably be refactored
    Systems systems(config);
    systems.init();

    MainWindow* mainWindow = new MainWindow(systems);
    mainWindow->show();

    return application.exec();
}

