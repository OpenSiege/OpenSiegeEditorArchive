
#include "MainWindow.hpp"

#include <QFileSystemModel>
#include <QInputDialog>
#include <QDebug>

#include <spdlog/spdlog.h>

namespace ehb
{
    MainWindow::MainWindow(WritableConfig& config, QWidget* parent) : QMainWindow(parent), config(config)
    {
        ui.setupUi(this);

        if (config.getString("bits", "").empty())
        {
            bool ok;
            QString bitsDir = QInputDialog::getText(this, "Set Bits Directory", "Path", QLineEdit::Normal, config.getString("bits", "").c_str(), &ok);

            // cancel
            if (ok && !bitsDir.isEmpty())
            {
                config.setString("bits", bitsDir.toStdString());
            }
            else
            {
                // bail bail bail
                QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
            }
        }

        qDebug() << "bits path: " << config.getString("bits", "").c_str();

        QString bitsPath(config.getString("bits", "").c_str());

        QFileSystemModel* fileSystemModel = new QFileSystemModel(ui.treeView);
        {
            fileSystemModel->setFilter(QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs);
            fileSystemModel->setNameFilters(QStringList("*.sno"));
            fileSystemModel->setNameFilterDisables(false);
            fileSystemModel->setOption(QFileSystemModel::DontWatchForChanges);

            ui.treeView->setModel(fileSystemModel);
            ui.treeView->hideColumn(1);
            ui.treeView->hideColumn(2);
            ui.treeView->hideColumn(3);
            ui.treeView->setRootIndex(fileSystemModel->setRootPath(bitsPath));
            ui.treeView->header()->setStretchLastSection(true);
            ui.treeView->header()->setVisible(false);
        }

        connect(ui.treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::currentChanged);
    }

    void MainWindow::currentChanged(const QModelIndex& current, const QModelIndex& previous)
    {
        spdlog::get("log")->info("clicked item in tree");

        QFileInfo info(QString(current.data().toString()));

        // setup filters rather than this check
        if (info.suffix() != "sno") return;

        // we need to chop the extension off because OpenSiege internally puts it back
        // maybe it makes sense to be appending full paths in open siege to pass to loaders
        const std::string& nodeFilename = current.data().toString().toStdString();
        std::string file = info.baseName().toStdString();

        spdlog::get("log")->info("clicked item in tree: {}", file);
    }
}
