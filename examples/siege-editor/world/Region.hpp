
#pragma once

#include <unordered_map>

#include <vsg/core/Visitor.h>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/maths/transform.h>

#include "io/Fuel.hpp"
#include "world/SiegeNode.hpp"
#include "vsg/Aspect.hpp"

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
}