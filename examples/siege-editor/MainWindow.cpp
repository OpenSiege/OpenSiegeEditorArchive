
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
    class IntersectionHandler : public vsg::Inherit<vsg::Visitor, IntersectionHandler>
    {
    public:
        vsg::ref_ptr<vsg::Builder> builder;
        vsg::ref_ptr<vsg::Options> options;
        vsg::ref_ptr<vsg::Camera> camera;
        vsg::ref_ptr<vsg::Group> scenegraph;
        vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel;
        double scale = 1.0;
        bool verbose = true;

        IntersectionHandler(vsg::ref_ptr<vsg::Builder> in_builder, vsg::ref_ptr<vsg::Camera> in_camera, vsg::ref_ptr<vsg::Group> in_scenegraph, vsg::ref_ptr<vsg::EllipsoidModel> in_ellipsoidModel, double in_scale, vsg::ref_ptr<vsg::Options> in_options) :
            builder(in_builder),
            options(in_options),
            camera(in_camera),
            scenegraph(in_scenegraph),
            ellipsoidModel(in_ellipsoidModel),
            scale(in_scale)
        {
            if (scale > 10.0) scale = 10.0;
        }

        void apply(vsg::KeyPressEvent& keyPress) override
        {
            if (lastPointerEvent)
            {
                interesection(*lastPointerEvent);
                if (!lastIntersection) return;

                vsg::GeometryInfo info;
                info.position = vsg::vec3(lastIntersection.worldIntersection);
                info.dx.set(scale, 0.0f, 0.0f);
                info.dy.set(0.0f, scale, 0.0f);
                info.dz.set(0.0f, 0.0f, scale);

                // info.image = vsg::read_cast<vsg::Data>("textures/lz.vsgb", options);

                if (keyPress.keyBase == 'b')
                {
                    scenegraph->addChild(builder->createBox(info));
                }
                else if (keyPress.keyBase == 'q')
                {
                    scenegraph->addChild(builder->createQuad(info));
                }
                else if (keyPress.keyBase == 'c')
                {
                    scenegraph->addChild(builder->createCylinder(info));
                }
                else if (keyPress.keyBase == 'p')
                {
                    scenegraph->addChild(builder->createCapsule(info));
                }
                else if (keyPress.keyBase == 's')
                {
                    scenegraph->addChild(builder->createSphere(info));
                }
                else if (keyPress.keyBase == 'n')
                {
                    scenegraph->addChild(builder->createCone(info));
                }

            }
        }

        void apply(vsg::ButtonPressEvent& buttonPressEvent) override
        {
            lastPointerEvent = &buttonPressEvent;

            if (buttonPressEvent.button == 1)
            {
                interesection(buttonPressEvent);
            }
        }

        void apply(vsg::PointerEvent& pointerEvent) override
        {
            lastPointerEvent = &pointerEvent;
        }

        void interesection(vsg::PointerEvent& pointerEvent)
        {
            auto intersector = vsg::LineSegmentIntersector::create(*camera, pointerEvent.x, pointerEvent.y);
            scenegraph->accept(*intersector);

            if (verbose) std::cout << "interesection(" << pointerEvent.x << ", " << pointerEvent.y << ") " << intersector->intersections.size() << ")" << std::endl;

            if (intersector->intersections.empty()) return;

            // sort the intersectors front to back
            std::sort(intersector->intersections.begin(), intersector->intersections.end(), [](auto lhs, auto rhs) { return lhs.ratio < rhs.ratio; });

            for (auto& intersection : intersector->intersections)
            {
                if (verbose) std::cout << "intersection = " << intersection.worldIntersection << " ";

                if (ellipsoidModel)
                {
                    std::cout.precision(10);
                    auto location = ellipsoidModel->convertECEFToLatLongAltitude(intersection.worldIntersection);
                    if (verbose) std::cout << " lat = " << location[0] << ", long = " << location[1] << ", height = " << location[2];
                }

                if (lastIntersection)
                {
                    if (verbose) std::cout << ", distance from previous intersection = " << vsg::length(intersection.worldIntersection - lastIntersection.worldIntersection);
                }

                if (verbose)
                {
                    for (auto& node : intersection.nodePath)
                    {
                        std::cout << ", " << node->className();
                    }

                    std::cout << ", Arrays[ ";
                    for (auto& array : intersection.arrays)
                    {
                        std::cout << array << " ";
                    }
                    std::cout << "] [";
                    for (auto& ir : intersection.indexRatios)
                    {
                        std::cout << "{" << ir.index << ", " << ir.ratio << "} ";
                    }
                    std::cout << "]";

                    std::cout << std::endl;
                }
            }

            lastIntersection = intersector->intersections.front();
        }

    protected:
        vsg::ref_ptr<vsg::PointerEvent> lastPointerEvent;
        vsg::LineSegmentIntersector::Intersection lastIntersection;
    };

}

namespace ehb
{

    MainWindow::MainWindow(Systems& systems, QWidget* parent) :
        QMainWindow(parent), systems(systems)
    {
        auto log = spdlog::get("log");

        ui.setupUi(this);

        readSettings();

        auto windowTraits = vsg::WindowTraits::create();
        windowTraits->windowTitle = "Open Siege Editor";
        windowTraits->debugLayer = false;
        windowTraits->apiDumpLayer = false;

        // since we are using QSettings make sure we just read inthe size of the main window
        windowTraits->width = size().width();
        windowTraits->height = size().height();

        viewerWindow = new vsgQt::ViewerWindow();
        viewerWindow->traits = windowTraits;

//#if QT_HAS_VULKAN_SUPPORT
        // if required set the QWindow's SurfaceType to QSurface::VulkanSurface.
        viewerWindow->setSurfaceType(QSurface::VulkanSurface);
//#endif

        auto widget = QWidget::createWindowContainer(viewerWindow, this);
        setCentralWidget(widget);

        ui.actionSave->setDisabled(true);
        ui.actionSave_As->setDisabled(true);

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

        // provide the calls to set up the vsg::Viewer that will be used to render to the QWindow subclass vsgQt::ViewerWindow
        viewerWindow->initializeCallback = [&](vsgQt::ViewerWindow& vw, uint32_t width, uint32_t height) {

            auto& window = vw.windowAdapter;
            if (!window) return false;

            auto& viewer = vw.viewer;
            if (!viewer) viewer = vsg::Viewer::create();

            viewer->addWindow(window);

            vsg::ref_ptr<vsg::ProjectionMatrix> perspective = vsg::Perspective::create(30.0, static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height), 0.1, 10000);
            auto lookAt = vsg::LookAt::create(vsg::dvec3(1.0, 15.0, 50.0), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 1.0, 0.0));
            auto viewportState = vsg::ViewportState::create(window->extent2D());
            auto camera = vsg::Camera::create(perspective, lookAt, viewportState);

            // bind the graphics pipeline which should always stay intact
            vsg_scene->add(SiegeNodePipeline::BindGraphicsPipeline);

            // always keep this guy below the scene to draw things
            vsg_scene->addChild(vsg_sno);

            // add close handler to respond the close window button and pressing escape
            viewer->addEventHandler(vsg::CloseHandler::create(viewer));

            vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel;
            auto builder = vsg::Builder::create(); builder->setup(window, camera->viewportState);
            viewer->addEventHandler(IntersectionHandler::create(builder, camera, vsg_scene, ellipsoidModel, 10000.0f, systems.options));

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

            vw.viewer->compile();

            dynamic_load_and_compile->merge();

            // pass any events into EventHandlers assigned to the Viewer
            vw.viewer->handleEvents();

            vw.viewer->update();

            vw.viewer->recordAndSubmit();

            vw.viewer->present();

            return true;
        };

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
