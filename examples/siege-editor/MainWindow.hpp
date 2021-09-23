
#pragma once

#include "ui_editor.h"
#include <QMainWindow>

#include <vsg/io/Options.h>
#include <vsg/nodes/Group.h>

#include <vsg/viewer/Viewer.h>
#include <vsgQt/ViewerWindow.h>

#include <QSettings>

#include "SiegePipeline.hpp"


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

        vsgQt::ViewerWindow* viewerWindow;

        Systems& systems;

        // top of the graph that contains the binding pipeline
        vsg::ref_ptr<vsg::Group> vsg_scene = vsg::Group::create();

        // contains our siege nodes, you can pop children off this guy and add to it to render nodes
        vsg::ref_ptr<vsg::Group> vsg_sno = vsg::Group::create();

        vsg::ref_ptr<DynamicLoadAndCompile> dynamic_load_and_compile;

        void closeEvent(QCloseEvent* event) override;

        void readSettings();

        void currentChanged(const QModelIndex& current, const QModelIndex& previous);

        void loadNewMap();
    };

    inline void MainWindow::closeEvent(QCloseEvent* event)
    {
        QSettings settings("editor.ini", QSettings::IniFormat);
        settings.beginGroup("MainWindow");
        settings.setValue("size", size());
        settings.setValue("position", pos());
        settings.endGroup();

        QMainWindow::closeEvent(event);
    }

    inline void MainWindow::readSettings()
    {
        QSettings settings("editor.ini", QSettings::IniFormat);
        settings.beginGroup("MainWindow");
        resize(settings.value("size", QSize(800, 600)).toSize());
        move(settings.value("position").toPoint());
    }

} // namespace ehb