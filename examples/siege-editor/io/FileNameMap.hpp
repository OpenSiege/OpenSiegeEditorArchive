
#pragma once

#include <string>
#include <unordered_map>
// #include <osgDB/Callbacks>

// NOTE: this class violates several of my design decisions, but the code was already written and we can come back around to clean it up later
namespace ehb
{
    class IFileSys;
    // class FileNameMap : public osgDB::FindFileCallback
    class FileNameMap
    {
    public:
        FileNameMap() = default;
        virtual ~FileNameMap() = default;

        void init(IFileSys& fileSys);

        // std::string findDataFile(const std::string & filename, const osgDB::Options * options, osgDB::CaseSensitivity caseSensitivity) override;
        std::string findDataFile(const std::string& filename);

    private:
        std::unordered_map<std::string, std::string> keyMap;
    };
} // namespace ehb
