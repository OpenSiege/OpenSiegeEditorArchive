
#pragma once

#include <QMainWindow>
#include "ui_editor.h"

#include "cfg/WritableConfig.hpp"

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

        void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    };
}