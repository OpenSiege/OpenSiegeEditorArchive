
#include "ReaderWriterRAW.hpp"

//#include <fstream>
#include "io/FileNameMap.hpp"
#include "io/IFileSys.hpp"
#include <vsg/core/Array2D.h>
#include <vsg/nodes/Group.h>

namespace ehb
{
    static constexpr uint32_t RAW_MAGIC = 0x52617069;
    static constexpr uint32_t RAW_FORMAT_8888 = 0x38383838;

    ReaderWriterRAW::ReaderWriterRAW(IFileSys& fileSys, FileNameMap& fileNameMap) :
        fileSys(fileSys), fileNameMap(fileNameMap)
    {
    }

    vsg::ref_ptr<vsg::Object> ReaderWriterRAW::read(const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options) const
    {
        if (auto fullFilePath = fileNameMap.findDataFile(filename); !fullFilePath.empty())
        {
            if (auto file = fileSys.createInputStream(fullFilePath + ".raw"); file != nullptr)
            {
                return read(*file, options);
            }
        }

        return vsg::ref_ptr<vsg::Object>();
    }

    vsg::ref_ptr<vsg::Object> ReaderWriterRAW::read(std::istream& stream, vsg::ref_ptr<const vsg::Options>) const
    {
        uint32_t magic = 0, format = 0;
        uint16_t flags = 0, surfaceCount = 0, width = 0, height = 0;

        stream.read((char*)&magic, sizeof(uint32_t));
        stream.read((char*)&format, sizeof(uint32_t));
        stream.read((char*)&flags, sizeof(uint16_t));
        stream.read((char*)&surfaceCount, sizeof(uint16_t));
        stream.read((char*)&width, sizeof(uint16_t));
        stream.read((char*)&height, sizeof(uint16_t));

        if (magic != RAW_MAGIC)
        {
            // this isn't a raw file
            return {};
        }

        // TODO: handle the different possible formats available here
        if (format != RAW_FORMAT_8888)
        {
            // this is an unsupported format
            return {};
        }

        // with the 8888 format each pixel is 4 bytes
        const uint32_t size = static_cast<uint32_t>(width) * static_cast<uint32_t>(height) * 4;

        auto image = vsg::ubvec4Array2D::create(width, height);

        // set the format to BGRA so we don't have to swizzle
        image->setFormat(VK_FORMAT_B8G8R8A8_UNORM);

        stream.read(static_cast<char*>(image->dataPointer()), sizeof(uint8_t) * size);

        return image;
    }
} // namespace ehb
