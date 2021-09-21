#include <vsg/all.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

#include <vsgQt/ViewerWindow.h>

#include "MainWindow.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "cfg/WritableConfig.hpp"

#include "io/LocalFileSys.hpp"
#include "io/FileNameMap.hpp"

#include "vsg/ReaderWriterRAW.hpp"
#include "vsg/ReaderWriterSNO.hpp"
#include "vsg/ReaderWriterSiegeNodeList.hpp"
#include "vsg/ReaderWriterRegion.hpp"

#include "ui_editor.h"

#include <QFileSystemModel>
#include <QModelIndex>
#include <QFileInfo>
#include <QInputDialog>

namespace ehb
{
    struct RenderingState
    {
        // need options to make sure we can call properly loading of rendering objects
        vsg::ref_ptr<vsg::Options> siege_options = vsg::Options::create();

        // top of the graph that contains the binding pipeline
        vsg::ref_ptr<vsg::Group> vsg_scene = vsg::Group::create();

        // contains our siege nodes, you can pop children off this guy and add to it to render nodes
        vsg::ref_ptr<vsg::Group> vsg_sno = vsg::Group::create();
    };

    auto replace_child = [](vsg::Group* group, vsg::ref_ptr<vsg::Node> previous, vsg::ref_ptr<vsg::Node> replacement)
    {
        for (auto& child : group->children)
        {
            if (child == previous) child = replacement;
        }
    };

    std::vector<vsg::ref_ptr<vsg::Group>> pending;

    std::string vert_PushConstants = R"(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants {
    mat4 projection;
    mat4 modelview;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = (pc.projection * pc.modelview) * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}

)";

    std::string frag_PushConstants = R"(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord);
})";

    RenderingState state;

}

int main(int argc, char* argv[])
{
    auto log = spdlog::stdout_color_mt("log");

    using namespace ehb;

    WritableConfig config(argc, argv);
    config.dump("log");

    auto windowTraits = vsg::WindowTraits::create();
    windowTraits->windowTitle = "Open Siege Editor";
    windowTraits->debugLayer = false;
    windowTraits->apiDumpLayer = false;
    windowTraits->width = 800;
    windowTraits->height = 600;

    QApplication application(argc, argv);

    //QMainWindow* mainWindow = new QMainWindow();
    ehb::MainWindow* mainWindow = new ehb::MainWindow(config);

    auto* viewerWindow = new vsgQt::ViewerWindow();
    viewerWindow->traits = windowTraits;

    LocalFileSys fileSys;
    FileNameMap fileNameMap;

    // provide the calls to set up the vsg::Viewer that will be used to render to the QWindow subclass vsgQt::ViewerWindow
    viewerWindow->initializeCallback = [&](vsgQt::ViewerWindow& vw) {

        fileSys.init(config);
        fileNameMap.init(fileSys);

        state.siege_options->readerWriters = { ReaderWriterRAW::create(fileSys, fileNameMap),
                          ReaderWriterSNO::create(fileSys, fileNameMap),
                          ReaderWriterSiegeNodeList::create(fileSys, fileNameMap),
                          ReaderWriterRegion::create(fileSys, fileNameMap)
        };

        vsg::ref_ptr<vsg::ShaderStage> vertexShader = vsg::ShaderStage::create(VK_SHADER_STAGE_VERTEX_BIT, "main", vert_PushConstants);
        vsg::ref_ptr<vsg::ShaderStage> fragmentShader = vsg::ShaderStage::create(VK_SHADER_STAGE_FRAGMENT_BIT, "main", frag_PushConstants);

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

        state.siege_options->setObject("graphics_pipeline", bindGraphicsPipeline);
        state.siege_options->setObject("layout", bindGraphicsPipeline->pipeline->layout);

        // bind the graphics pipeline which should always stay intact
        state.vsg_scene->addChild(bindGraphicsPipeline);

        // always keep this guy below the scene to draw things
        state.vsg_scene->addChild(state.vsg_sno);

        auto& window = vw.proxyWindow;
        if (!window) return false;

        auto& viewer = vw.viewer;
        if (!viewer) viewer = vsg::Viewer::create();

        viewer->addWindow(window);

        vsg::ref_ptr<vsg::ProjectionMatrix> perspective = vsg::Perspective::create( 30.0, static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height), 0.1, 1000);
        auto lookAt = vsg::LookAt::create(vsg::dvec3(1.0, 15.0, 50.0), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 1.0, 0.0));
        auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

        // add close handler to respond the close window button and pressing escape
        viewer->addEventHandler(vsg::CloseHandler::create(viewer));

        // add trackball to enable mouse driven camera view control.
        viewer->addEventHandler(vsg::Trackball::create(camera));

        {
            static std::string siegeNode("t_grs01_houses_generic-a-log");
            //static std::string siegeNode("t_xxx_flr_04x04-v0");

            //if (vsg::ref_ptr<vsg::Group> sno = vsg::read("t_grs01_houses_generic-a-log", state.siege_options).cast<vsg::Group>(); sno != nullptr)
            if (auto sno= vsg::read("world/maps/multiplayer_world/regions/town_center.region", state.siege_options).cast<vsg::MatrixTransform>())
            {
                auto t1 = vsg::MatrixTransform::create();
                t1->addChild(sno);

                auto t2 = vsg::MatrixTransform::create();
                //t2->addChild(sno);

                // add nodes below the binding pipeline
                state.vsg_sno->addChild(t1);
                //vsg_sno->addChild(t2);

                //SiegeNodeMesh::connect(t1, 2, t2, 1);
            }
        }

        auto commandGraph = vsg::createCommandGraphForView(window, camera, state.vsg_scene);
        viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

        viewer->compile();

        return true;
    };

    // provide the calls to invokve the vsg::Viewer to render a frame.
    viewerWindow->frameCallback = [](vsgQt::ViewerWindow& vw) {

        if (!vw.viewer || !vw.viewer->advanceToNextFrame()) return false;

        //vw.viewer->compile();

        // pass any events into EventHandlers assigned to the Viewer
        vw.viewer->handleEvents();

        if (!pending.empty())
        {
            state.vsg_sno->children.clear();
            state.vsg_sno->addChild(pending.back());
            pending.clear();

            vw.viewer->compile();
        }

        vw.viewer->update();

        vw.viewer->recordAndSubmit();

        vw.viewer->present();

        return true;
    };

    bool ok;
    QString bitsDir = QInputDialog::getText(mainWindow, "Bits directory pathing", "Path", QLineEdit::Normal, config.getString("bits", "").c_str(), &ok);

    config.setString("bits", bitsDir.toStdString());

    auto widget = QWidget::createWindowContainer(viewerWindow, mainWindow);

    mainWindow->setCentralWidget(widget);
    mainWindow->resize(windowTraits->width, windowTraits->height);

    mainWindow->show();

    int value = application.exec();
    state.vsg_scene->unref();
    return value;
}

