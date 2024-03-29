
#pragma once

#include <spdlog/spdlog.h>

#include <vsg/io/ReaderWriter.h>

namespace ehb
{
    class IFileSys;
    class FileNameMap;
    class ReaderWriterASP : public vsg::Inherit<vsg::ReaderWriter, ReaderWriterASP>
    {
    public:
        ReaderWriterASP(IFileSys& fileSys, FileNameMap& fileNameMap);

        virtual vsg::ref_ptr<vsg::Object> read(const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> = {}) const override;
        virtual vsg::ref_ptr<vsg::Object> read(std::istream& stream, vsg::ref_ptr<const vsg::Options> = {}) const override;

    private:
        IFileSys& fileSys;

        // since vsg doesn't have find file callbacks we will this to resolve our filenames
        FileNameMap& fileNameMap;

        std::shared_ptr<spdlog::logger> log;
    };
} // namespace ehb
