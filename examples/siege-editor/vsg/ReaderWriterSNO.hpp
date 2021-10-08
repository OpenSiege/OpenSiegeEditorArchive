
#pragma once

#include <vsg/core/Inherit.h>
#include <vsg/io/ReaderWriter.h>
#include <vsg/nodes/Group.h>

namespace ehb
{
    class IFileSys;
    class FileNameMap;
    class ReaderWriterSNO : public vsg::Inherit<vsg::ReaderWriter, ReaderWriterSNO>
    {
    public:
        ReaderWriterSNO(IFileSys& fileSys, FileNameMap& fileNameMap);

        virtual vsg::ref_ptr<vsg::Object> read(const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> = {}) const override;
        virtual vsg::ref_ptr<vsg::Object> read(std::istream& stream, vsg::ref_ptr<const vsg::Options> = {}) const override;

    private:
        IFileSys& fileSys;

        // since vsg doesn't have find file callbacks we will this to resolve our filenames
        FileNameMap& fileNameMap;
    };

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
} // namespace ehb
