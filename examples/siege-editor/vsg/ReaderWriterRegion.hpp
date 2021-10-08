
#pragma once

#include "io/FileNameMap.hpp"
#include <vsg/io/ReaderWriter.h>
#include <vsg/ReaderWriterSNO.hpp>
#include <vsg/nodes/Group.h>
#include <vsg/nodes/MatrixTransform.h>

#include <spdlog/spdlog.h>

namespace ehb
{
    using GuidToXformMap = std::unordered_map<uint32_t, vsg::ref_ptr<vsg::MatrixTransform>>;

    struct GenerateGlobalGuidToNodeXformMap : public vsg::Visitor
    {
        using vsg::Visitor::apply;

        GuidToXformMap& map;

        GenerateGlobalGuidToNodeXformMap(GuidToXformMap& map) : map(map)
        {

        }

        void apply(vsg::Object& object)
        {
            object.traverse(*this);
        }

        void apply(vsg::Node& node)
        {
            node.traverse(*this);
        }

        void apply(vsg::MatrixTransform& t)
        {
            // ReaderWriterSiegeNodeList
            // this should be guaranteed - if this even crashes then something went wrong with the setup of the nodes
            //
            if (auto sno = t.children[0].cast<SiegeNodeMesh>())
            {
                spdlog::get("log")->info("siege node transform");

                uint32_t guid; t.getValue("guid", guid);
                map.emplace(guid, &t);
            }

            t.traverse(*this);
        }

        void apply(vsg::Group& g)
        {
            g.traverse(*this);
        }
    };

    // hold all region data
    class Region : public vsg::Inherit<vsg::Group, Region>
    {
    public:

        Region() = default;
        ~Region() = default;
        
        void setNodeData(vsg::ref_ptr<vsg::Group> nodes)
        {
            GenerateGlobalGuidToNodeXformMap visitor(placedNodeXformMap);
            nodes->accept(visitor);

            addChild(nodes);
        }

        GuidToXformMap placedNodeXformMap; // holds the final matrix transform against the node guid
    };

    class IFileSys;
    class ReaderWriterRegion : public vsg::Inherit<vsg::ReaderWriter, ReaderWriterRegion>
    {
    public:
        ReaderWriterRegion(IFileSys& fileSys, FileNameMap& fileNameMap);

        virtual vsg::ref_ptr<vsg::Object> read(const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> = {}) const override;
        virtual vsg::ref_ptr<vsg::Object> read(std::istream& stream, vsg::ref_ptr<const vsg::Options> = {}) const override;

    private:
        IFileSys& fileSys;

        // since vsg doesn't have find file callbacks we will this to resolve our filenames
        FileNameMap& fileNameMap;

        std::shared_ptr<spdlog::logger> log;
    };
} // namespace ehb
