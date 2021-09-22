
#pragma once

#include <vsg/core/Inherit.h>
#include <vsg/io/ReaderWriter.h>
#include <vsg/nodes/Group.h>

using dquatArray = vsg::Array<vsg::dquat>;
template<>
constexpr const char* vsg::type_name<dquatArray>() noexcept
{
    return "vsg::dquatArray";
}

//! temp spot
namespace ehb
{
    // the general goal is to attach one of these to each region load and hold it in memory for later
    class InstanceData : public vsg::Inherit<vsg::Object, InstanceData>
    {

    public:
        struct Data
        {
            vsg::dvec3 pos;
            vsg::dquat rot;
        };

        vsg::ref_ptr<vsg::dvec3Array> pos;
        vsg::ref_ptr<dquatArray> rot;

        InstanceData()
        {
            pos = vsg::dvec3Array::create();
            rot = dquatArray::create();
            //data = vsg::Array<Data>::create();
        }

        // should there be an array of instance data?
        // this should get populated by each loop and then it should be bound to the buffers?
        //vsg::ref_ptr<vsg::Array<Data>> data;

        vsg::DataList data;

        std::vector<vsg::dmat4> matrices;
        std::vector<Data> shader;
        uint32_t texIndex = 0;

    protected:
        virtual ~InstanceData() = default;
    };
} // namespace ehb

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
