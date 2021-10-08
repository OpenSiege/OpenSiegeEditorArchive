
#include "ReaderWriterSNO.hpp"

#include <cassert>
#include <iostream>

#include <vsg/io/read.h>

#include <vsg/maths/quat.h>
#include <vsg/maths/transform.h>

#include <vsg/nodes/MatrixTransform.h>
#include <vsg/nodes/VertexIndexDraw.h>

#include <vsg/state/DescriptorImage.h>
#include <vsg/state/DescriptorSet.h>
#include <vsg/traversals/ComputeBounds.h>

#include "io/FileNameMap.hpp"
#include "io/LocalFileSys.hpp"

#include "world/SiegeNode.hpp"

namespace ehb
{
    static constexpr uint32_t SNO_MAGIC = 0x444F4E53;

    ReaderWriterSNO::ReaderWriterSNO(IFileSys& fileSys, FileNameMap& fileNameMap) :
        fileSys(fileSys), fileNameMap(fileNameMap)
    {
    }

    vsg::ref_ptr<vsg::Object> ReaderWriterSNO::read(const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options) const
    {
        if (auto fullFilePath = fileNameMap.findDataFile(filename); !fullFilePath.empty())
        {
            if (auto file = fileSys.createInputStream(fullFilePath + ".sno"); file != nullptr)
            {
                return read(*file, options);
            }
        }

        return {};
    }

    vsg::ref_ptr<vsg::Object> ReaderWriterSNO::read(std::istream& stream, vsg::ref_ptr<const vsg::Options> options) const
    {
        uint32_t magic, version, unk1;

        stream.read((char*)&magic, sizeof(uint32_t));
        stream.read((char*)&version, sizeof(uint32_t));
        stream.read((char*)&unk1, sizeof(uint32_t));

        if (magic != SNO_MAGIC)
            return {};

        uint32_t doorCount = 0, spotCount = 0, cornerCount = 0, faceCount = 0, textureCount = 0;
        float minX = 0.0f, minY = 0.0f, minZ = 0.0f, maxX = 0.0f, maxY = 0.0f, maxZ = 0.0f;
        float unk2 = 0.0f, unk3 = 0.0f, unk4 = 0.0f;
        uint32_t unk5 = 0, unk6 = 0, unk7 = 0, unk8 = 0;
        float checksum = 0.0f;

        stream.read((char*)&doorCount, sizeof(uint32_t));
        stream.read((char*)&spotCount, sizeof(uint32_t));
        stream.read((char*)&cornerCount, sizeof(uint32_t));
        stream.read((char*)&faceCount, sizeof(uint32_t));
        stream.read((char*)&textureCount, sizeof(uint32_t));
        stream.read((char*)&minX, sizeof(float));
        stream.read((char*)&minY, sizeof(float));
        stream.read((char*)&minZ, sizeof(float));
        stream.read((char*)&maxX, sizeof(float));
        stream.read((char*)&maxY, sizeof(float));
        stream.read((char*)&maxZ, sizeof(float));
        stream.read((char*)&unk2, sizeof(float));
        stream.read((char*)&unk3, sizeof(float));
        stream.read((char*)&unk4, sizeof(float));
        stream.read((char*)&unk5, sizeof(uint32_t));
        stream.read((char*)&unk6, sizeof(uint32_t));
        stream.read((char*)&unk7, sizeof(uint32_t));
        stream.read((char*)&unk8, sizeof(uint32_t));
        stream.read((char*)&checksum, sizeof(float));

        // construct the actual mesh node
        vsg::ref_ptr<SiegeNodeMesh> group = SiegeNodeMesh::create();

        // read door data
        for (uint32_t index = 0; index < doorCount; index++)
        {
            int32_t id = 0, count = 0;
            float a00 = 0.0f, a01 = 0.0f, a02 = 0.0f, a10 = 0.0f, a11 = 0.0f, a12 = 0.0f, a20 = 0.0f, a21 = 0.0f, a22 = 0.0f, x = 0.0f, y = 0.0f, z = 0.0f;

            stream.read((char*)&id, sizeof(int32_t));
            stream.read((char*)&x, sizeof(float));
            stream.read((char*)&y, sizeof(float));
            stream.read((char*)&z, sizeof(float));
            stream.read((char*)&a00, sizeof(float));
            stream.read((char*)&a01, sizeof(float));
            stream.read((char*)&a02, sizeof(float));
            stream.read((char*)&a10, sizeof(float));
            stream.read((char*)&a11, sizeof(float));
            stream.read((char*)&a12, sizeof(float));
            stream.read((char*)&a20, sizeof(float));
            stream.read((char*)&a21, sizeof(float));
            stream.read((char*)&a22, sizeof(float));
            stream.read((char*)&count, sizeof(int32_t));

            stream.seekg(count * 4, std::ios::cur);

            /*
             * this is pretty straight forward but just documenting that osg and
             * dungeon siege node transformation matrices ARE compatible so no funky
             * modifications are required here
             * TODO: THE ABOVE WAS FOR OSG, MIGHT BE DIFFERENT FOR VSG
             */
            vsg::dmat4 xform;

#if 0
            xform(0, 0) = a00;
            xform(0, 1) = a01;
            xform(0, 2) = a02;
            xform(1, 0) = a10;
            xform(1, 1) = a11;
            xform(1, 2) = a12;
            xform(2, 0) = a20;
            xform(2, 1) = a21;
            xform(2, 2) = a22;
            xform(3, 0) = x;
            xform(3, 1) = y;
            xform(3, 2) = z;
#else
            xform(0, 0) = a00;
            xform(0, 1) = a01;
            xform(0, 2) = a02;
            xform(1, 0) = a10;
            xform(1, 1) = a11;
            xform(1, 2) = a12;
            xform(2, 0) = a20;
            xform(2, 1) = a21;
            xform(2, 2) = a22;
            xform(3, 0) = x;
            xform(3, 1) = y;
            xform(3, 2) = z;
#endif

            group->doorXform.emplace_back(id, std::move(xform));
        }

        // read spot data
        for (uint32_t index = 0; index < spotCount; index++)
        {
            std::string tmp;

            // rot, pos, string?
            stream.seekg(44, std::ios_base::cur);
            std::getline(stream, tmp, '\0');
        }

        // create vertex data per entire mesh
        auto vertices = vsg::vec3Array::create(cornerCount);
        auto normals = vsg::vec3Array::create(cornerCount);
        auto colors = vsg::vec4Array::create(cornerCount);
        auto tcoords = vsg::vec2Array::create(cornerCount);

        // read in our vertex data
        for (uint32_t index = 0; index < cornerCount; index++)
        {
            float x = 0.0f, y = 0.0f, z = 0.0f, nX = 0.0f, nY = 0.0f, nZ = 0.0f, tX = 0.0f, tY = 0.0f;
            uint8_t r = 0, g = 0, b = 0, a = 0;

            stream.read((char*)&x, sizeof(float));
            stream.read((char*)&y, sizeof(float));
            stream.read((char*)&z, sizeof(float));

            stream.read((char*)&nX, sizeof(float));
            stream.read((char*)&nY, sizeof(float));
            stream.read((char*)&nZ, sizeof(float));

            // this is swizzled
            stream.read((char*)&r, sizeof(uint8_t));
            stream.read((char*)&b, sizeof(uint8_t));
            stream.read((char*)&g, sizeof(uint8_t));
            stream.read((char*)&a, sizeof(uint8_t));

            stream.read((char*)&tX, sizeof(float));
            stream.read((char*)&tY, sizeof(float));

            (*vertices)[index].set(x, y, z);
            (*normals)[index].set(nX, nY, nZ);
            (*colors)[index].set(r, g, b, a);
            (*tcoords)[index].set(tX, tY);
        }

        for (uint32_t index = 0; index < textureCount; index++)
        {
            std::string textureName;
            uint32_t start, span, count;

            // the textureName here is associated with the material name on export - this matches a texture name
            std::getline(stream, textureName, '\0');

            stream.read((char*)&start, sizeof(uint32_t));
            stream.read((char*)&span, sizeof(uint32_t));
            stream.read((char*)&count, sizeof(uint32_t));

            auto attributeArrays = vsg::DataList{vertices, colors, tcoords};

            auto indicies = vsg::ushortArray::create(count);

            for (uint32_t j = 0; j < count; ++j)
            {
                uint16_t value;

                stream.read((char*)&value, sizeof(uint16_t));
                (*indicies)[j] = start + value;
            }

            // a texSetAbbr such as grs01
            std::string texSetAbbr;

            if (options != nullptr)
            {
                options->getValue("texsetabbr", texSetAbbr);

                if (!texSetAbbr.empty())
                {
                    // if the material / textureName contains xxx then its generic and we are about to replace it
                    if (const auto itr = textureName.find("_xxx_"); itr != std::string::npos)
                    {
                        textureName.replace(itr + 1, 3, texSetAbbr);
                    }
                }
            }

#if 0
            if (fileSys.loadGasFile(textureName + ".gas"))
            {
                spdlog::get("log")->info("loading {}", textureName + ".gas");
            }

            if (auto fullFilePath = fileNameMap.findDataFile(textureName); !fullFilePath.empty())
            {
                if (auto file = fileSys.createInputStream(fullFilePath + ".gas"); file != nullptr)
                {
                    spdlog::get("log")->info("test");
                }
            }
#endif

            if (auto layout = options->getObject<vsg::PipelineLayout>("PipelineLayout"); layout != nullptr)
            {
                if (auto textureData = vsg::read(textureName, options).cast<vsg::Data>(); textureData != nullptr)
                {
                    auto texture = vsg::DescriptorImage::create(vsg::Sampler::create(), textureData, 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                    assert(texture != nullptr);

                    //! NOTE: should we be accessing the first element of the vector?
                    auto descriptorSet = vsg::DescriptorSet::create(layout->setLayouts[0], vsg::Descriptors{texture});
                    assert(descriptorSet != nullptr);

                    auto bindDescriptorSets = vsg::BindDescriptorSets::create(VK_PIPELINE_BIND_POINT_GRAPHICS, const_cast<vsg::PipelineLayout*>(layout), 0, vsg::DescriptorSets{descriptorSet});
                    assert(bindDescriptorSets != nullptr);

                    group->addChild(bindDescriptorSets);
                }
            }
            else
            {
                std::cout << "No layout passed via options. We are bailing because the pipeline requires them" << std::endl;

                return {};
            }

            auto vid = vsg::VertexIndexDraw::create();
            vid->arrays = attributeArrays;
            vid->indices = indicies;
            vid->indexCount = static_cast<uint32_t>(indicies->size());
            vid->instanceCount = 1;

            vsg::ComputeBounds computeBounds;
            vid->accept(computeBounds);

            // taken from Intersector.cpp#138
            if (computeBounds.bounds.valid())
            {
                // manually calculate sphere so its done on load rather than by the graph later
                vsg::sphere bound;
                bound.center = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
                bound.radius = vsg::length(computeBounds.bounds.max - computeBounds.bounds.min) * 0.5;

                vid->setValue("bound", bound);
            }

            group->addChild(vid);
        }

        return group;
    };

} // namespace ehb