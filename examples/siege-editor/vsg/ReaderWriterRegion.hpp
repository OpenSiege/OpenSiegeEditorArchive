
#pragma once

#include "io/FileNameMap.hpp"
#include <vsg/io/ReaderWriter.h>
#include "world/SiegeNode.hpp"
#include "vsg/Aspect.hpp"
#include <vsg/nodes/Group.h>
#include <vsg/nodes/MatrixTransform.h>

#include "io/Fuel.hpp"

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

        void apply(vsg::Node& node)
        {
            node.traverse(*this);
        }

        void apply(vsg::MatrixTransform& t)
        {
            // ReaderWriterSiegeNodeList
            // this should be guaranteed - if this even crashes then something went wrong with the setup of the nodes
            if (auto sno = t.children[0].cast<SiegeNodeMesh>())
            {
                uint32_t guid; t.getValue("guid", guid);
                map.emplace(guid, &t);
            }

            t.traverse(*this);
        }
    };


    struct CalculateAndPlaceObjects : public vsg::Visitor
    {
        using vsg::Visitor::apply;

        GuidToXformMap& map;

        CalculateAndPlaceObjects(GuidToXformMap& map) : map(map) {}

        void apply(vsg::Node& node)
        {
            node.traverse(*this);
        }

        void apply(vsg::MatrixTransform& t)
        {
            // ReaderWriterSiegeNodeList
            // this should be guaranteed - if this even crashes then something went wrong with the setup of the nodes
            if (auto aspect = t.children[0].cast<Aspect>())
            {
                // local rotation of the object
                SiegePos pos; t.getValue("position", pos);
                SiegeRot rot; t.getValue("rotation", rot);

                // global rotation of the node this is applied to
                auto gt = map[pos.guid];

                t.matrix = gt->matrix * vsg::dmat4(vsg::translate(pos.pos));
            }

            t.traverse(*this);
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

        void setObjects(vsg::ref_ptr<vsg::Group> objects)
        {
            CalculateAndPlaceObjects visitor(placedNodeXformMap);
            objects->accept(visitor);

            addChild(objects);
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
