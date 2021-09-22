
#include "SiegePipeline.hpp"

#include <spdlog/spdlog.h>
#include <vsg/all.h>

#include "cfg/WritableConfig.hpp"
#include "io/FileNameMap.hpp"
#include "io/IFileSys.hpp"

#include "vsg/ReaderWriterRAW.hpp"
#include "vsg/ReaderWriterRegion.hpp"
#include "vsg/ReaderWriterSNO.hpp"
#include "vsg/ReaderWriterSiegeNodeList.hpp"

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

    void Systems::init()
    {
        fileSys.init(config);
        fileNameMap.init(fileSys);

        options->readerWriters = {

            ReaderWriterRAW::create(fileSys, fileNameMap),
            ReaderWriterSNO::create(fileSys, fileNameMap),
            ReaderWriterSiegeNodeList::create(fileSys, fileNameMap),
            ReaderWriterRegion::create(fileSys, fileNameMap)

        };

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

        vsg::GraphicsPipelineStates pipelineStates{
            vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
            vsg::InputAssemblyState::create(),
            vsg::RasterizationState::create(),
            vsg::MultisampleState::create(),
            vsg::ColorBlendState::create(),
            vsg::DepthStencilState::create()};

        PipelineLayout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);
        GraphicsPipeline = vsg::GraphicsPipeline::create(PipelineLayout, vsg::ShaderStages{vertexShader, fragmentShader}, pipelineStates);
        BindGraphicsPipeline = vsg::BindGraphicsPipeline::create(GraphicsPipeline);
    }
} // namespace ehb