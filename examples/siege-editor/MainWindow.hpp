
#pragma once

#include <QMainWindow>
#include "ui_editor.h"

#include "cfg/WritableConfig.hpp"

#include "io/LocalFileSys.hpp"
#include "io/FileNameMap.hpp"

#include <vsg/io/Options.h>
#include <vsg/nodes/Group.h>

namespace vsgQt
{
    class ViewerWindow;
}

namespace ehb
{
    class WritableConfig;
    class MainWindow : public QMainWindow
    {
    public:

        MainWindow(WritableConfig& config, QWidget* parent = nullptr);

        virtual ~MainWindow() = default;

    private:

        Ui::MainWindow ui;

        WritableConfig& config;

        LocalFileSys fileSys;
        FileNameMap fileNameMap;

        vsgQt::ViewerWindow* vsgViewerWindow;

        // need options to make sure we can call properly loading of rendering objects
        vsg::ref_ptr<vsg::Options> siege_options = vsg::Options::create();

        // top of the graph that contains the binding pipeline
        vsg::ref_ptr<vsg::Group> vsg_scene = vsg::Group::create();

        // contains our siege nodes, you can pop children off this guy and add to it to render nodes
        vsg::ref_ptr<vsg::Group> vsg_sno = vsg::Group::create();

        void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    };
}