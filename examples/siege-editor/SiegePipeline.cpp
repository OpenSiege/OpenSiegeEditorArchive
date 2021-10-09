
#include "SiegePipeline.hpp"

#include <spdlog/spdlog.h>

#include "cfg/WritableConfig.hpp"
#include "io/FileNameMap.hpp"
#include "io/IFileSys.hpp"

#include "vsg/ReaderWriterASP.hpp"
#include "vsg/ReaderWriterRAW.hpp"
#include "vsg/ReaderWriterRegion.hpp"
#include "vsg/ReaderWriterSNO.hpp"
#include "vsg/ReaderWriterSiegeNodeList.hpp"
#include "vsg/io/stream.h"
#include <spdlog/fmt/ostr.h>

namespace ehb
{
    const std::string vertexPushConstantsSource = R"(#version 450
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

    const std::string fragmentPushConstantsSource = R"(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord);
})";

    SiegeNodeMeshGUIDDatabase::SiegeNodeMeshGUIDDatabase(IFileSys& fileSys) :
        fileSys(fileSys)
    {
        auto log = spdlog::get("log");

        static const std::string directory = "/world/global/siege_nodes";

        fileSys.eachGasFile(directory, [&](const std::string& filename, auto doc) {
            for (auto root : doc->eachChild())
            {
                for (auto node : root->eachChild())
                {
                    const auto itr = keyMap.emplace(node->valueOf("guid"), convertToLowerCase(node->valueOf("filename")));

                    if (itr.second != true)
                    {
                        log->error("duplicate mesh mapping found: tried to insert {} for guid {}, but found filename {} there already", node->valueOf("filename"), node->valueOf("guid"), itr.first->second);
                    }
                }
            }
        });

        const std::string mapsFolder = "/world/maps/";
        for (const auto& mapPath : fileSys.getDirectoryContents(mapsFolder)) // each map folder
        {
            const std::string regionsPath = mapPath + "/regions";

            for (const auto& regionPath : fileSys.getDirectoryContents(regionsPath))
            {
                const std::string node_mesh_index_dot_gas = regionPath + "/index/node_mesh_index.gas";

                if (auto stream = fileSys.createInputStream(node_mesh_index_dot_gas))
                {
                    if (Fuel doc; doc.load(*stream))
                    {
                        // log->info("handling non-global mesh guids @ {}", node_mesh_index_dot_gas);

                        for (const auto& entry : doc.child("node_mesh_index")->eachAttribute())
                        {
                            const auto itr = keyMap.emplace(entry.name, convertToLowerCase(entry.value));

                            if (itr.second != true)
                            {
                                log->error("duplicate mesh mapping found: tried to insert {} for guid {}, but found filename {} there already", entry.value, entry.name, itr.first->second);
                            }
                        }
                    }
                }
            }
        }

        log->info("{} loaded nodes {} into its mappings", __func__, keyMap.size());
    }

    const std::string& SiegeNodeMeshGUIDDatabase::resolveFileName(const std::string& filename) const
    {
        const auto itr = keyMap.find(filename);

        return itr != keyMap.end() ? itr->second : filename;
    }

    void Systems::init()
    {
        fileSys.init(config);
        fileNameMap.init(fileSys);
        contentDb.init(fileSys);
        objectDb.reset(new ObjectDb(contentDb));

        nodeMeshGuidDb = SiegeNodeMeshGUIDDatabase::create(fileSys);
        options->setObject("SiegeNodeMeshGuidDatabase", nodeMeshGuidDb);

        options->readerWriters = {

            ReaderWriterRAW::create(fileSys, fileNameMap),
            ReaderWriterSNO::create(fileSys, fileNameMap),
            ReaderWriterSiegeNodeList::create(fileSys, fileNameMap),
            ReaderWriterRegion::create(fileSys, fileNameMap, contentDb),
            ReaderWriterASP::create(fileSys, fileNameMap)

        };

        options->objectCache = vsg::ObjectCache::create();

        SiegeNodePipeline::SetupPipeline();

        // we currently have two ways to access this variable
        // the first is via options that get passed around
        // the second is via the static variable - which should only be accessed and not written to so should be thread safe?
        options->setObject("PipelineLayout", SiegeNodePipeline::PipelineLayout);
    }

    void SiegeNodePipeline::SetupPipeline()
    {
        vsg::ref_ptr<vsg::ShaderStage> vertexShader = vsg::ShaderStage::create(VK_SHADER_STAGE_VERTEX_BIT, "main", vertexPushConstantsSource);
        vsg::ref_ptr<vsg::ShaderStage> fragmentShader = vsg::ShaderStage::create(VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragmentPushConstantsSource);

        if (!vertexShader || !fragmentShader)
        {
            spdlog::get("log")->critical("Could not create shaders.");

            return;
        }

        // set up graphics pipeline
        vsg::DescriptorSetLayoutBindings descriptorBindings{
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr} // { binding, descriptorTpe, descriptorCount, stageFlags, pImmutableSamplers}
        };

        vsg::DescriptorSetLayouts descriptorSetLayouts{vsg::DescriptorSetLayout::create(descriptorBindings)};

        vsg::PushConstantRanges pushConstantRanges{
            {VK_SHADER_STAGE_VERTEX_BIT, 0, 128} // projection view, and model matrices, actual push constant calls automatically provided by the VSG's DispatchTraversal
        };

        vsg::VertexInputState::Bindings vertexBindingsDescriptions{
            VkVertexInputBindingDescription{0, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // vertex data
            VkVertexInputBindingDescription{1, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // colour data
            VkVertexInputBindingDescription{2, sizeof(vsg::vec2), VK_VERTEX_INPUT_RATE_VERTEX}  // tex coord data
        };

        vsg::VertexInputState::Attributes vertexAttributeDescriptions{
            VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, // vertex data
            VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0}, // colour data
            VkVertexInputAttributeDescription{2, 2, VK_FORMAT_R32G32_SFLOAT, 0},    // tex coord data
        };

        auto colorBlendState = vsg::ColorBlendState::create();
        colorBlendState->attachments = vsg::ColorBlendState::ColorBlendAttachments{
            {true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_SUBTRACT, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT} };

        vsg::GraphicsPipelineStates pipelineStates{
            vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
            vsg::InputAssemblyState::create(),
            vsg::RasterizationState::create(),
            vsg::MultisampleState::create(),
            colorBlendState,
            vsg::DepthStencilState::create()};

        PipelineLayout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);
        GraphicsPipeline = vsg::GraphicsPipeline::create(PipelineLayout, vsg::ShaderStages{vertexShader, fragmentShader}, pipelineStates);
        BindGraphicsPipeline = vsg::BindGraphicsPipeline::create(GraphicsPipeline);
    }

    DynamicLoadAndCompile::DynamicLoadAndCompile(vsg::ref_ptr<vsg::Window> in_window, vsg::ref_ptr<vsg::ViewportState> in_viewport, vsg::ref_ptr<vsg::ActivityStatus> in_status) :
        status(in_status),
        window(in_window),
        viewport(in_viewport)
    {
        loadThreads = vsg::OperationThreads::create(12, status);
        compileThreads = vsg::OperationThreads::create(1, status);
        mergeQueue = vsg::OperationQueue::create(status);
    }

    void DynamicLoadAndCompile::loadRequest(const vsg::Path& filename, vsg::ref_ptr<vsg::Group> attachmentPoint, vsg::ref_ptr<vsg::Options> options)
    {
        auto request = Request::create(filename, attachmentPoint, options);
        loadThreads->add(LoadOperation::create(request, vsg::observer_ptr<DynamicLoadAndCompile>(this)));
    }

    void DynamicLoadAndCompile::compileRequest(vsg::ref_ptr<Request> request)
    {
        compileThreads->add(CompileOperation::create(request, vsg::observer_ptr<DynamicLoadAndCompile>(this)));
    }

    void DynamicLoadAndCompile::mergeRequest(vsg::ref_ptr<Request> request)
    {
        mergeQueue->add(MergeOperation::create(request, vsg::observer_ptr<DynamicLoadAndCompile>(this)));
    }

    vsg::ref_ptr<vsg::CompileTraversal> DynamicLoadAndCompile::takeCompileTraversal()
    {
        auto log = spdlog::get("log");

        {
            std::scoped_lock lock(mutex_compileTraversals);
            if (!compileTraversals.empty())
            {
                auto ct = compileTraversals.front();
                compileTraversals.erase(compileTraversals.begin());
                log->info("takeCompileTraversal() reusing");
                return ct;
            }
        }

        log->info("takeCompileTraversal() creating a new CompileTraversal");
        auto ct = vsg::CompileTraversal::create(window, viewport, buildPreferences);

        return ct;
    }

    void DynamicLoadAndCompile::addCompileTraversal(vsg::ref_ptr<vsg::CompileTraversal> ct)
    {
        spdlog::get("log")->info("addCompileTraversal({})", ct);
        std::scoped_lock lock(mutex_compileTraversals);
        compileTraversals.push_back(ct);
    }

    void DynamicLoadAndCompile::merge()
    {
        vsg::ref_ptr<vsg::Operation> operation;
        while (operation = mergeQueue->take())
        {
            operation->run();
        }
    }

    void DynamicLoadAndCompile::LoadOperation::run()
    {
        vsg::ref_ptr<DynamicLoadAndCompile> dynamicLoadAndCompile(dlac);
        if (!dynamicLoadAndCompile) return;

        auto log = spdlog::get("log");

        log->info("Loading {}", request->filename);

        if (auto node = vsg::read_cast<vsg::Node>(request->filename, request->options); node)
        {
            request->loaded = node;

            log->info("Loaded {}", request->filename);

            dynamicLoadAndCompile->compileRequest(request);
        }
    }

    void DynamicLoadAndCompile::CompileOperation::run()
    {
        vsg::ref_ptr<DynamicLoadAndCompile> dynamicLoadAndCompile(dlac);
        if (!dynamicLoadAndCompile) return;

        auto log = spdlog::get("log");

        if (request->loaded)
        {
            log->info("Compiling {}", request->filename);

            auto compileTraversal = dynamicLoadAndCompile->takeCompileTraversal();

            vsg::CollectDescriptorStats collectStats;
            request->loaded->accept(collectStats);

            auto maxSets = collectStats.computeNumDescriptorSets();
            auto descriptorPoolSizes = collectStats.computeDescriptorPoolSizes();

            // brute force allocation of new DescrptorPool for this subgraph, TODO : need to preallocate large DescriptorPool for multiple loaded subgraphs
            if (descriptorPoolSizes.size() > 0) compileTraversal->context.descriptorPool = vsg::DescriptorPool::create(compileTraversal->context.device, maxSets, descriptorPoolSizes);

            request->loaded->accept(*compileTraversal);

            log->info("Finished compile traversal {}", request->filename);

            compileTraversal->context.record(); // records and submits to queue

            compileTraversal->context.waitForCompletion();

            log->info("Finished waiting for compile {}", request->filename);

            dynamicLoadAndCompile->mergeRequest(request);

            dynamicLoadAndCompile->addCompileTraversal(compileTraversal);
        }
    }

    void DynamicLoadAndCompile::MergeOperation::run()
    {
        spdlog::get("log")->info("Merging {}", request->filename);

        //vsg::write(request->loaded, "testascii.vsgt");
        //vsg::write(request->loaded, "testbinary.vsgb");

        // clear out an already loaded region
        if (request->attachmentPoint->children.size() != 0)
        {
            request->attachmentPoint->children.clear();
        }

        request->attachmentPoint->addChild(request->loaded);
    }
} // namespace ehb