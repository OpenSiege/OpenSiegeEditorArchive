
#pragma once

#include <vector>

#include <vsg/core/Inherit.h>
#include <vsg/maths/mat4.h>
#include <vsg/nodes/Group.h>
#include <vsg/nodes/MatrixTransform.h>

namespace ehb
{
    class SiegeNodeMesh : public vsg::Inherit<vsg::Group, SiegeNodeMesh>
    {
        friend class ReaderWriterSNO;

    public:
        explicit SiegeNodeMesh() = default;

        static void connect(vsg::MatrixTransform* targetNode, uint32_t targetDoor, vsg::MatrixTransform* connectNode, uint32_t connectDoor);

        static void connect(vsg::MatrixTransform* targetRegion, vsg::MatrixTransform* targetNode, uint32_t targetDoor, vsg::MatrixTransform* connectRegion, vsg::MatrixTransform* connectNode, uint32_t connectDoor);

    protected:
        virtual ~SiegeNodeMesh() = default;

    private:
        std::vector<std::pair<uint32_t, vsg::dmat4>> doorXform;
    };
}