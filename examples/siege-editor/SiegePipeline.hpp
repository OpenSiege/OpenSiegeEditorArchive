
#pragma once

#include <string>

#include <vsg/io/Options.h>
#include <vsg/state/PipelineLayout.h>
#include <vsg/state/GraphicsPipeline.h>

#include "io/LocalFileSys.hpp"
#include "io/FileNameMap.hpp"

namespace ehb
{
    extern const std::string vertexPushConstantsSource;
    extern const std::string fragmentPushConstantsSource;

    class WritableConfig;
    class IFileSys;
    class FileNameMap;

    class SiegeNodePipeline
    {
    public:

        static void SetupPipeline();

        inline static vsg::ref_ptr<vsg::PipelineLayout> PipelineLayout;
        inline static vsg::ref_ptr<vsg::GraphicsPipeline> GraphicsPipeline;
        inline static vsg::ref_ptr<vsg::BindGraphicsPipeline> BindGraphicsPipeline;
    };

    class Systems
    {
    public:

        Systems(WritableConfig& config) : config(config) {}

        void init();

        WritableConfig& config;
        LocalFileSys fileSys; // temp
        FileNameMap fileNameMap;

        vsg::ref_ptr<vsg::Options> options = vsg::Options::create();
    };
}