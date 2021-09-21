
#pragma once

#include <QMainWindow>
#include "ui_editor.h"

namespace ehb
{
    class IConfig;
    class MainWindow : public QMainWindow
    {
    public:

        MainWindow(IConfig& config, QWidget* parent = nullptr);

        virtual ~MainWindow() = default;

    private:

        Ui::MainWindow ui;

        IConfig& config;

        void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    };
}