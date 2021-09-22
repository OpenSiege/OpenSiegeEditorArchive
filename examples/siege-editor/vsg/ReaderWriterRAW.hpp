
#pragma once

#include <vsg/core/Inherit.h>
#include <vsg/io/ReaderWriter.h>

namespace ehb
{
    class IFileSys;
    class FileNameMap;
    class ReaderWriterRAW : public vsg::Inherit<vsg::ReaderWriter, ReaderWriterRAW>
    {
    public:
        ReaderWriterRAW(IFileSys& fileSys, FileNameMap& fileNameMap);

        virtual vsg::ref_ptr<vsg::Object> read(const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> = {}) const override;
        virtual vsg::ref_ptr<vsg::Object> read(std::istream& stream, vsg::ref_ptr<const vsg::Options> = {}) const override;

    private:
        IFileSys& fileSys;
        FileNameMap& fileNameMap;
    };
} // namespace ehb
