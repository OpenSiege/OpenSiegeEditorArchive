
#pragma once

#include <QMainWindow>
#include "ui_editor.h"

#include <vsg/io/Options.h>
#include <vsg/nodes/Group.h>

namespace vsgQt
{
    class ViewerWindow;
}

namespace ehb
{
    class Systems;
    class MainWindow : public QMainWindow
    {
    public:

        MainWindow(Systems& systems, QWidget* parent = nullptr);

        virtual ~MainWindow() = default;

    private:

        Ui::MainWindow ui;

        Systems& systems;

        // top of the graph that contains the binding pipeline
        vsg::ref_ptr<vsg::Group> vsg_scene = vsg::Group::create();

        // contains our siege nodes, you can pop children off this guy and add to it to render nodes
        vsg::ref_ptr<vsg::Group> vsg_sno = vsg::Group::create();

        void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    };
}