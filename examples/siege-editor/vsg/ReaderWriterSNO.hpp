
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
} // namespace ehb
