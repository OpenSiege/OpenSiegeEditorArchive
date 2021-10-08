
#pragma once

#include <string>

#if 0
#    include <vsg/io/Options.h>
#    include <vsg/state/GraphicsPipeline.h>
#    include <vsg/state/PipelineLayout.h>
#endif

#include <vsg/all.h>

#include "io/FileNameMap.hpp"
#include "io/LocalFileSys.hpp"

#include "game/ContentDb.hpp"

namespace ehb
{
    extern const std::string vertexPushConstantsSource;
    extern const std::string fragmentPushConstantsSource;

    class WritableConfig;
    class IFileSys;
    class FileNameMap;
    class ContentDb;

    class SiegeNodePipeline
    {
    public:
        static void SetupPipeline();

        inline static vsg::ref_ptr<vsg::PipelineLayout> PipelineLayout;
        inline static vsg::ref_ptr<vsg::GraphicsPipeline> GraphicsPipeline;
        inline static vsg::ref_ptr<vsg::BindGraphicsPipeline> BindGraphicsPipeline;
    };

    class SiegeNodeMeshGUIDDatabase : public vsg::Inherit<vsg::Object, SiegeNodeMeshGUIDDatabase>
    {
    public:
        SiegeNodeMeshGUIDDatabase(IFileSys& fileSys);

        const std::string& resolveFileName(const std::string& filename) const;

    private:
        virtual ~SiegeNodeMeshGUIDDatabase() = default;

        IFileSys& fileSys;

        std::unordered_map<std::string, std::string> keyMap;
    };

    class Systems
    {
    public:
        Systems(WritableConfig& config) :
            config(config) {}

        void init();

        WritableConfig& config;
        LocalFileSys fileSys; // temp
        FileNameMap fileNameMap;
        ContentDb contentDb;

        vsg::ref_ptr<SiegeNodeMeshGUIDDatabase> nodeMeshGuidDb;

        vsg::ref_ptr<vsg::Options> options = vsg::Options::create();
    };

    class DynamicLoadAndCompile : public vsg::Inherit<vsg::Object, DynamicLoadAndCompile>
    {
    public:
        vsg::ref_ptr<vsg::ActivityStatus> status;

        vsg::ref_ptr<vsg::OperationThreads> loadThreads;
        vsg::ref_ptr<vsg::OperationThreads> compileThreads;
        vsg::ref_ptr<vsg::OperationQueue> mergeQueue;

        std::mutex mutex_compileTraversals;
        std::list<vsg::ref_ptr<vsg::CompileTraversal>> compileTraversals;

        // window related settings used to set up the CompileTraversal
        vsg::ref_ptr<vsg::Window> window;
        vsg::ref_ptr<vsg::ViewportState> viewport;
        vsg::BufferPreferences buildPreferences;

        DynamicLoadAndCompile(vsg::ref_ptr<vsg::Window> in_window, vsg::ref_ptr<vsg::ViewportState> in_viewport, vsg::ref_ptr<vsg::ActivityStatus> in_status = vsg::ActivityStatus::create());

        struct Request : public vsg::Inherit<vsg::Object, Request>
        {
            Request(const vsg::Path& in_filename, vsg::ref_ptr<vsg::Group> in_attachmentPoint, vsg::ref_ptr<vsg::Options> in_options) :
                filename(in_filename),
                attachmentPoint(in_attachmentPoint),
                options(in_options) {}

            vsg::Path filename;
            vsg::ref_ptr<vsg::Group> attachmentPoint;
            vsg::ref_ptr<vsg::Options> options;
            vsg::ref_ptr<vsg::Node> loaded;
        };

        struct LoadOperation : public vsg::Inherit<vsg::Operation, LoadOperation>
        {
            LoadOperation(vsg::ref_ptr<Request> in_request, vsg::observer_ptr<DynamicLoadAndCompile> in_dlac) :
                request(in_request),
                dlac(in_dlac) {}

            vsg::ref_ptr<Request> request;
            vsg::observer_ptr<DynamicLoadAndCompile> dlac;

            void run() override;
        };

        struct CompileOperation : public vsg::Inherit<vsg::Operation, CompileOperation>
        {
            CompileOperation(vsg::ref_ptr<Request> in_request, vsg::observer_ptr<DynamicLoadAndCompile> in_dlac) :
                request(in_request),
                dlac(in_dlac) {}

            vsg::ref_ptr<Request> request;
            vsg::observer_ptr<DynamicLoadAndCompile> dlac;

            void run() override;
        };

        struct MergeOperation : public vsg::Inherit<vsg::Operation, MergeOperation>
        {
            MergeOperation(vsg::ref_ptr<Request> in_request, vsg::observer_ptr<DynamicLoadAndCompile> in_dlac) :
                request(in_request),
                dlac(in_dlac) {}

            vsg::ref_ptr<Request> request;
            vsg::observer_ptr<DynamicLoadAndCompile> dlac;

            void run() override;
        };

        void loadRequest(const vsg::Path& filename, vsg::ref_ptr<vsg::Group> attachmentPoint, vsg::ref_ptr<vsg::Options> options);

        void compileRequest(vsg::ref_ptr<Request> request);

        void mergeRequest(vsg::ref_ptr<Request> request);

        vsg::ref_ptr<vsg::CompileTraversal> takeCompileTraversal();

        void addCompileTraversal(vsg::ref_ptr<vsg::CompileTraversal> ct);

        void merge();
    };
} // namespace ehb