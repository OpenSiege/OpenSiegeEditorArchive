
#include "MainWindow.hpp"

#include "SiegePipeline.hpp"

// work-around for weird VK_NO_PROTOTYPES issue with Qt
#include <vsg/all.h>
#include <vsg/vk/CommandPool.h>
#include <vsg/vk/Fence.h>

#include <vsg/viewer/Viewer.h>
#include <vsgQt/ViewerWindow.h>

#include <QDebug>
#include <QFileSystemModel>
#include <QInputDialog>

#include "LoadMapDialog.hpp"

#include <spdlog/spdlog.h>

#include "SiegePipeline.hpp"
#include "cfg/WritableConfig.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace ehb
{

    MainWindow::MainWindow(Systems& systems, QWidget* parent) :
        QMainWindow(parent), systems(systems)
    {
        auto log = spdlog::get("log");

        ui.setupUi(this);

        readSettings();

        connect(ui.actionOpen, &QAction::triggered, this, &MainWindow::loadNewMap);

        if (systems.config.getString("bits", "").empty())
        {
            bool ok;
            QString bitsDir = QInputDialog::getText(this, "Set Bits Directory", "Path", QLineEdit::Normal, systems.config.getString("bits", "").c_str(), &ok);

            if (ok && !bitsDir.isEmpty())
            {
                systems.config.setString("bits", bitsDir.toStdString());
            }
            else
            {
                // bail bail bail
                QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
            }
        }

        QString bitsPath(systems.config.getString("bits", "").c_str());

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

        auto windowTraits = vsg::WindowTraits::create();
        windowTraits->windowTitle = "Open Siege Editor";
        windowTraits->debugLayer = false;
        windowTraits->apiDumpLayer = false;

        // since we are using QSettings make sure we just read inthe size of the main window
        windowTraits->width = size().width();
        windowTraits->height = size().height();

        viewerWindow = new vsgQt::ViewerWindow();
        viewerWindow->traits = windowTraits;

        // provide the calls to set up the vsg::Viewer that will be used to render to the QWindow subclass vsgQt::ViewerWindow
        viewerWindow->initializeCallback = [&](vsgQt::ViewerWindow& vw) {
            // bind the graphics pipeline which should always stay intact
            vsg_scene->addChild(SiegeNodePipeline::BindGraphicsPipeline);

            // always keep this guy below the scene to draw things
            vsg_scene->addChild(vsg_sno);

            auto& window = vw.proxyWindow;
            if (!window) return false;

            auto& viewer = vw.viewer;
            if (!viewer) viewer = vsg::Viewer::create();

            viewer->addWindow(window);

            vsg::ref_ptr<vsg::ProjectionMatrix> perspective = vsg::Perspective::create(30.0, static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height), 0.1, 1000);
            auto lookAt = vsg::LookAt::create(vsg::dvec3(1.0, 15.0, 50.0), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 1.0, 0.0));
            auto viewportState = vsg::ViewportState::create(window->extent2D());
            auto camera = vsg::Camera::create(perspective, lookAt, viewportState);

            // add close handler to respond the close window button and pressing escape
            viewer->addEventHandler(vsg::CloseHandler::create(viewer));

            // add trackball to enable mouse driven camera view control.
            viewer->addEventHandler(vsg::Trackball::create(camera));

            auto commandGraph = vsg::createCommandGraphForView(window, camera, vsg_scene);
            viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

            dynamic_load_and_compile = DynamicLoadAndCompile::create(window, viewportState, viewer->status);

            viewer->compile();

            return true;
        };

        viewerWindow->frameCallback = [&](vsgQt::ViewerWindow& vw) {
            if (!vw.viewer || !vw.viewer->advanceToNextFrame()) return false;

            dynamic_load_and_compile->merge();

            // pass any events into EventHandlers assigned to the Viewer
            vw.viewer->handleEvents();

            vw.viewer->update();

            vw.viewer->recordAndSubmit();

            vw.viewer->present();

            return true;
        };

        setCentralWidget(QWidget::createWindowContainer(viewerWindow, this));

        resize(windowTraits->width, windowTraits->height);
    }

    void MainWindow::currentChanged(const QModelIndex& current, const QModelIndex& previous)
    {
        QFileInfo info(QString(current.data().toString()));

        // setup filters rather than this check
        if (info.suffix() != "sno") return;

        // we need to chop the extension off because OpenSiege internally puts it back
        // maybe it makes sense to be appending full paths in open siege to pass to loaders
        const std::string& nodeFilename = current.data().toString().toStdString();
        std::string file = info.baseName().toStdString();

        spdlog::get("log")->info("clicked item in tree: {}", file);
    }

    void MainWindow::loadNewMap()
    {
        LoadMapDialog dialog(systems.fileSys, this);
        if (dialog.exec() == QDialog::Accepted)
        {
            vsg_sno->children.clear();

            auto t1 = vsg::MatrixTransform::create();
            dynamic_load_and_compile->loadRequest(dialog.getFullPathForSelectedRegion() + ".region", t1, systems.options);
            vsg_sno->addChild(t1);
        }
    }
} // namespace ehb
