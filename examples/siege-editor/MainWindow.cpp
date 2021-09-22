
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
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <spdlog/spdlog.h>

#include "SiegePipeline.hpp"
#include "cfg/WritableConfig.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace ehb
{
    class LoadMapDialog : public QDialog
    {
        QVBoxLayout* layout;

        QTreeWidget* treeWidget;
        QDialogButtonBox *buttonBox;

    public:

        LoadMapDialog(Systems& systems, QWidget* parent = nullptr) : QDialog(parent)
        {
            // TODO: fix this so its a bit more dynamic
            resize(450, 600);

            layout = new QVBoxLayout(this);

            buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

            treeWidget = new QTreeWidget();
            treeWidget->setColumnCount(2);
            treeWidget->setHeaderLabels(QStringList{ "Name", "Description" });

            layout->addWidget(treeWidget);
            layout->addWidget(buttonBox);

            connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
            connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

            auto maps = systems.fileSys.getDirectoryContents("/world/maps");

            for (const auto& map : maps)
            {
                std::string path = map + "/main.gas";
                if (auto mapdotgas = systems.fileSys.loadGasFile(path))
                {
                    QTreeWidgetItem* item = new QTreeWidgetItem;
                    item->setText(0, stem(map).c_str());

                    treeWidget->addTopLevelItem(item);

                    auto regions = systems.fileSys.getDirectoryContents(map + "/regions");
                    for (const auto& region : regions)
                    {
                        if(auto regionmaindotgas = systems.fileSys.loadGasFile(region + "/main.gas"))                        
                        {
                            QTreeWidgetItem* item2 = new QTreeWidgetItem;
                            item2->setText(0, stem(region).c_str());
                            item2->setText(1, regionmaindotgas->valueAsString("region:description").c_str());
                            item->addChild(item2);
                        }
                    }

                    item->setExpanded(true);
                }
            }

            treeWidget->resizeColumnToContents(0);
            treeWidget->resizeColumnToContents(1);
        }
    };
}

namespace ehb
{

    MainWindow::MainWindow(Systems& systems, QWidget* parent) :
        QMainWindow(parent), systems(systems)
    {
        auto log = spdlog::get("log");

        ui.setupUi(this);

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

        LoadMapDialog dialog(systems, this);
        if (dialog.exec() == QDialog::Accepted)
        {
            log->info("lets load a map");
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
        windowTraits->width = 800;
        windowTraits->height = 600;

        auto* viewerWindow = new vsgQt::ViewerWindow();
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
            auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

            // add close handler to respond the close window button and pressing escape
            viewer->addEventHandler(vsg::CloseHandler::create(viewer));

            // add trackball to enable mouse driven camera view control.
            viewer->addEventHandler(vsg::Trackball::create(camera));

            {
                static std::string siegeNode("t_grs01_houses_generic-a-log");
                //static std::string siegeNode("t_xxx_flr_04x04-v0");

                //if (vsg::ref_ptr<vsg::Group> sno = vsg::read("t_grs01_houses_generic-a-log", siege_options).cast<vsg::Group>(); sno != nullptr)
                if (auto sno = vsg::read("world/maps/multiplayer_world/regions/town_center.region", systems.options).cast<vsg::MatrixTransform>())
                {
                    auto t1 = vsg::MatrixTransform::create();
                    t1->addChild(sno);

                    auto t2 = vsg::MatrixTransform::create();
                    //t2->addChild(sno);

                    // add nodes below the binding pipeline
                    vsg_sno->addChild(t1);
                    //vsg_sno->addChild(t2);

                    //SiegeNodeMesh::connect(t1, 2, t2, 1);
                }
            }

            auto commandGraph = vsg::createCommandGraphForView(window, camera, vsg_scene);
            viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

            viewer->compile();

            return true;
        };

        viewerWindow->frameCallback = [](vsgQt::ViewerWindow& vw) {
            if (!vw.viewer || !vw.viewer->advanceToNextFrame()) return false;

            //vw.viewer->compile();

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
} // namespace ehb
