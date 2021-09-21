
#include "MainWindow.hpp"

#include "vsg/ReaderWriterRAW.hpp"
#include "vsg/ReaderWriterSNO.hpp"
#include "vsg/ReaderWriterSiegeNodeList.hpp"
#include "vsg/ReaderWriterRegion.hpp"

#include "SiegePipeline.hpp"

// work-around for weird VK_NO_PROTOTYPES issue with Qt
#include <vsg/vk/CommandPool.h>
#include <vsg/vk/Fence.h>
#include <vsg/all.h>

#include <vsg/viewer/Viewer.h>
#include <vsgQt/ViewerWindow.h>

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

        auto windowTraits = vsg::WindowTraits::create();
        windowTraits->windowTitle = "Open Siege Editor";
        windowTraits->debugLayer = true;
        windowTraits->apiDumpLayer = false;
        windowTraits->width = 800;
        windowTraits->height = 600;

        auto* viewerWindow = new vsgQt::ViewerWindow();
        viewerWindow->traits = windowTraits;

        auto log = spdlog::get("log");

        // provide the calls to set up the vsg::Viewer that will be used to render to the QWindow subclass vsgQt::ViewerWindow
        viewerWindow->initializeCallback = [&](vsgQt::ViewerWindow& vw) {

            fileSys.init(config);
            fileNameMap.init(fileSys);

            siege_options->readerWriters = { ReaderWriterRAW::create(fileSys, fileNameMap),
                              ReaderWriterSNO::create(fileSys, fileNameMap),
                              ReaderWriterSiegeNodeList::create(fileSys, fileNameMap),
                              ReaderWriterRegion::create(fileSys, fileNameMap)
            };

            vsg::ref_ptr<vsg::ShaderStage> vertexShader = vsg::ShaderStage::create(VK_SHADER_STAGE_VERTEX_BIT, "main", vertexPushConstantsSource);
            vsg::ref_ptr<vsg::ShaderStage> fragmentShader = vsg::ShaderStage::create(VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragmentPushConstantsSource);

            if (!vertexShader || !fragmentShader)
            {
                log->error("Could not create shaders.");

                return false;
            }

            // set up graphics pipeline
            vsg::DescriptorSetLayoutBindings descriptorBindings
            {
                {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr} // { binding, descriptorTpe, descriptorCount, stageFlags, pImmutableSamplers}
            };

            vsg::DescriptorSetLayouts descriptorSetLayouts{ vsg::DescriptorSetLayout::create(descriptorBindings) };

            vsg::PushConstantRanges pushConstantRanges
            {
                {VK_SHADER_STAGE_VERTEX_BIT, 0, 128} // projection view, and model matrices, actual push constant calls automatically provided by the VSG's DispatchTraversal
            };

            vsg::VertexInputState::Bindings vertexBindingsDescriptions
            {
                VkVertexInputBindingDescription{0, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // vertex data
                VkVertexInputBindingDescription{1, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // colour data
                VkVertexInputBindingDescription{2, sizeof(vsg::vec2), VK_VERTEX_INPUT_RATE_VERTEX}  // tex coord data
            };

            vsg::VertexInputState::Attributes vertexAttributeDescriptions
            {
                VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, // vertex data
                VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0}, // colour data
                VkVertexInputAttributeDescription{2, 2, VK_FORMAT_R32G32_SFLOAT, 0},    // tex coord data
            };

            vsg::GraphicsPipelineStates pipelineStates
            {
                vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
                vsg::InputAssemblyState::create(),
                vsg::RasterizationState::create(),
                vsg::MultisampleState::create(),
                vsg::ColorBlendState::create(),
                vsg::DepthStencilState::create()
            };

            auto pipelineLayout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);
            auto graphicsPipeline = vsg::GraphicsPipeline::create(pipelineLayout, vsg::ShaderStages{ vertexShader, fragmentShader }, pipelineStates);
            auto bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(graphicsPipeline);

            siege_options->setObject("graphics_pipeline", bindGraphicsPipeline);
            siege_options->setObject("layout", bindGraphicsPipeline->pipeline->layout);

            // bind the graphics pipeline which should always stay intact
            vsg_scene->addChild(bindGraphicsPipeline);

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
                if (auto sno = vsg::read("world/maps/multiplayer_world/regions/town_center.region", siege_options).cast<vsg::MatrixTransform>())
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
            viewer->assignRecordAndSubmitTaskAndPresentation({ commandGraph });

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
}
