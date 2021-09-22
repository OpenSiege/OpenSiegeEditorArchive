
#pragma once

#include <string>

#include <vsg/state/PipelineLayout.h>
#include <vsg/state/GraphicsPipeline.h>

namespace ehb
{
    extern const std::string vertexPushConstantsSource;
    extern const std::string fragmentPushConstantsSource;

    class SiegeNodePipeline
    {
    public:

        static void SetupPipeline();

        inline static vsg::ref_ptr<vsg::PipelineLayout> PipelineLayout;
        inline static vsg::ref_ptr<vsg::GraphicsPipeline> GraphicsPipeline;
        inline static vsg::ref_ptr<vsg::BindGraphicsPipeline> BindGraphicsPipeline;
    };
}