
#pragma once

#include "Fuel.hpp"
#include <algorithm>
#include <functional>
#include <istream>
#include <memory>
#include <set>
#include <string>

#ifdef WIN32
#    include <filesystem>
namespace fs = std::filesystem;
#else
#    include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace ehb
{
    typedef std::set<std::string> FileList;
    typedef std::unique_ptr<std::istream> InputStream;

    class IConfig;
    class IFileSys
    {
    public:
        virtual ~IFileSys() = default;

        virtual void init(IConfig& config) = 0;

        virtual InputStream createInputStream(const std::string& filename) = 0;

        virtual FileList getFiles() const = 0;
        virtual FileList getDirectoryContents(const std::string& directory) const = 0;

        std::unique_ptr<Fuel> loadGasFile(const std::string& file);

        void eachGasFile(const std::string& directory, std::function<void(const std::string&, std::unique_ptr<Fuel>)> func);
    };

    inline std::string convertToLowerCase(const std::string& str)
    {
        std::string lowcase_str(str);
        std::transform(std::begin(lowcase_str), std::end(lowcase_str), std::begin(lowcase_str), ::tolower);
        return lowcase_str;
    }

    inline std::string getSimpleFileName(const std::string& fileName)
    {
        fs::path path(fileName);
        return path.filename().string();
    }

    inline std::string stem(const std::string& fileName)
    {
        fs::path path(fileName);
        return path.stem().string();
    }

    // the standard returns an extension as ".ext"
    // osg returns an extension as "ext"
    // we will elect to use the standard
    inline std::string getFileExtension(const std::string& fileName)
    {
        fs::path path(fileName);
        return path.extension().string();
    }

    inline std::string getLowerCaseFileExtension(const std::string& filename)
    {
        return convertToLowerCase(getFileExtension(filename));
    }

    inline std::unique_ptr<Fuel> IFileSys::loadGasFile(const std::string& file)
    {
        if (auto stream = createInputStream(file))
        {
            if (auto doc = std::make_unique<Fuel>(); doc->load(*stream))
            {
                return doc;
            }
        }

        return { };
    }

    inline void IFileSys::eachGasFile(const std::string& directory, std::function<void(const std::string&, std::unique_ptr<Fuel>)> func)
    {
        for (const auto& filename : getFiles())
        {
            if (getLowerCaseFileExtension(filename) == ".gas")
            {
                if (filename.find(directory) == 0)
                {
                    if (auto stream = createInputStream(filename))
                    {
                        if (auto doc = std::make_unique<Fuel>(); doc->load(*stream))
                        {
                            func(filename, std::move(doc));
                        }
                        else
                        {
                            // log->error("{}: could not parse", filename);
                        }
                    }
                    else
                    {
                        // log->error("{}: could not create input stream", filename);
                    }
                }
            }
        }
    }
} // namespace ehb
