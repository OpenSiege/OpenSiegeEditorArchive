
#include "Aspect.hpp"
#include "AspectImpl.hpp"

#include <algorithm>
#include <functional>

#include <spdlog/spdlog.h>

#include <vsg/io/read.h> // temp
#include <vsg/nodes/VertexIndexDraw.h>
#include <vsg/state/DescriptorSet.h>

#include <vsg/commands/BindIndexBuffer.h>
#include <vsg/commands/BindVertexBuffers.h>
#include <vsg/commands/Commands.h>
#include <vsg/commands/DrawIndexed.h>

namespace ehb
{
    Aspect::Aspect(std::shared_ptr<Impl> impl) :
        d(std::move(impl))
    {
        auto log = spdlog::get("log");

        // log->set_level(spdlog::level::debug);

        log->debug("asp has {} sub meshes", d->subMeshes.size());

        for (const auto& mesh : d->subMeshes)
        {
            log->debug("asp subMesh has {} textures", mesh.textureCount);

            uint32_t f = 0; // track which face the loader is loading across the sub mesh
            for (uint32_t i = 0; i < mesh.textureCount; ++i)
            {
                // create the geometry which will be drawn
                // vsg::ref_ptr<vsg::Geometry> geometry = new vsg::Geometry;

                // textures are stored directly against the mesh
                // TODO: should we store the vsg::Image against AspectImpl?
                const std::string& imageFilename = d->textureNames[i];

                auto vertices = vsg::vec3Array::create(mesh.cornerCount);
                auto normals = vsg::vec3Array::create(mesh.cornerCount);
                auto texCoords = vsg::vec2Array::create(mesh.cornerCount);
                auto colors = vsg::vec3Array::create(mesh.cornerCount); // this is actually 4 colors but im lazy rn

                // this has to match the incoming pipe which was defined by the siege nodes
                auto attributeArrays = vsg::DataList{vertices, colors, texCoords};

#if 0
                    // this is replaced by the attributeArrays call in vsg
                    geometry->setVertexArray(vertices);
                    geometry->setNormalArray(normals, vsg::Array::BIND_PER_VERTEX);
                    geometry->setTexCoordArray(0, texCoords);
                    geometry->setColorArray(colors, vsg::Array::BIND_PER_VERTEX);
#endif

                for (uint32_t cornerCounter = 0; cornerCounter < mesh.cornerCount; ++cornerCounter)
                {
                    const auto& c = mesh.corners[cornerCounter];
                    const auto& w = mesh.wCorners[cornerCounter];

                    (*vertices)[cornerCounter] = w.position;
                    (*texCoords)[cornerCounter] = c.texCoord;
                    (*normals)[cornerCounter] = c.normal;
                    (*colors)[cornerCounter] = vsg::vec3(c.color[0], c.color[1], c.color[2]);
                }

                auto elements = vsg::uintArray::create(mesh.matInfo[i].faceSpan * 3);
                //vsg::ref_ptr<vsg::DrawElementsUInt> elements = new vsg::DrawElementsUInt(GL_TRIANGLES);
                //geometry->addPrimitiveSet(elements);

                ///for (uint32_t fpt = 0; fpt < mesh.matInfo[i].faceSpan; ++fpt)
                for (uint32_t fpt = 0; fpt < mesh.matInfo[i].faceSpan * 3; fpt += 3)
                {
                    (*elements)[fpt] = mesh.faceInfo.cornerIndex.at(f / 3).index[0] + mesh.faceInfo.cornerStart[i];
                    (*elements)[fpt + 1] = mesh.faceInfo.cornerIndex.at(f / 3).index[1] + mesh.faceInfo.cornerStart[i];
                    (*elements)[fpt + 2] = mesh.faceInfo.cornerIndex.at(f / 3).index[2] + mesh.faceInfo.cornerStart[i];

                    f += 3;
                }

                // make sure to calculate our bounding boxes
                //geometry->setInitialBound(geometry->computeBoundingBox());

                // by default we draw static geometry
                //pseudoRoot->addChild(geometry);

                // make sure the skeleton is ready to go if we need it
                //skeleton->addChild(rigGeometry);

                if (auto textureData = vsg::read_cast<vsg::Data>(imageFilename, d->options); textureData != nullptr)
                {
                    auto texture = vsg::DescriptorImage::create(vsg::Sampler::create(), textureData, 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                    assert(texture != nullptr);

                    //! NOTE: should we be accessing the first element of the vector?
                    auto descriptorSet = vsg::DescriptorSet::create(d->pipelineLayout->setLayouts[0], vsg::Descriptors{texture});
                    assert(descriptorSet != nullptr);

                    auto bindDescriptorSets = vsg::BindDescriptorSets::create(VK_PIPELINE_BIND_POINT_GRAPHICS, const_cast<vsg::PipelineLayout*>(d->pipelineLayout), 0, vsg::DescriptorSets{descriptorSet});
                    assert(bindDescriptorSets != nullptr);

                    addChild(bindDescriptorSets);
                }

                auto vid = vsg::VertexIndexDraw::create();
                vid->arrays = attributeArrays;
                vid->indices = elements;
                vid->indexCount = static_cast<uint32_t>(elements->size());
                vid->instanceCount = 1;

                addChild(vid);
            }
        }
    }
} // namespace ehb
