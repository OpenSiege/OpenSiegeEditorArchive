
#include "ReaderWriterSiegeNodeList.hpp"
#include "io/IFileSys.hpp"
#include "vsg/ReaderWriterSNO.hpp"

#include "SiegePipeline.hpp"

// i'm lazy
#include "vsg/all.h"

namespace ehb
{
    ReaderWriterSiegeNodeList::ReaderWriterSiegeNodeList(IFileSys& fileSys, FileNameMap& fileNameMap) :
        fileSys(fileSys), fileNameMap(fileNameMap)
    {
        log = spdlog::get("log");
    }

    vsg::ref_ptr<vsg::Object> ReaderWriterSiegeNodeList::read(const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options) const
    {
        const std::string simpleFilename = vsg::simpleFilename(filename);

        // check to make sure this is a nodes.gas file
        if (simpleFilename != "nodes")
            return {};

        log->info("about to build region with path : {} and simpleFilename {}", filename, simpleFilename);

        InputStream stream = fileSys.createInputStream(filename);

        return read(*stream, options);
    }

    vsg::ref_ptr<vsg::Object> ReaderWriterSiegeNodeList::read(std::istream& stream, vsg::ref_ptr<const vsg::Options> options) const
    {
        const auto nodeMeshGuidDb = options->getObject<SiegeNodeMeshGUIDDatabase>("SiegeNodeMeshGuidDatabase");

        if (nodeMeshGuidDb == nullptr)
        {
            log->critical("Options passed to ReaderWriterSiegeNodeList don't contain a valid SiegeNodeMeshGUIDDatabase. Bye.");

            return {};
        }

        struct DoorEntry
        {
            uint32_t id;
            uint32_t farDoor;
            uint32_t farGuid;
        };

        std::unordered_multimap<uint32_t, DoorEntry> doorMap;
        std::unordered_map<uint32_t, vsg::MatrixTransform*> nodeMap;
        std::set<uint32_t> completeSet;

        // there are way to many of these containers -.-
        std::set<vsg::ref_ptr<SiegeNodeMesh>> uniqueMeshes;

        if (Fuel doc; doc.load(stream))
        {
            auto group = vsg::Group::create();

            for (const auto node : doc.eachChildOf("siege_node_list"))
            {
                const uint32_t nodeGuid = node->valueAsUInt("guid");
                // const uint64_t meshGuid = node->valueAsUInt("mesh_guid");
                const std::string meshGuid = convertToLowerCase(node->valueOf("mesh_guid"));

                const std::string& texSetAbbr = node->valueOf("texsetabbr");

                // log->info("nodeGuid: {}, meshGuid: {}, texSetAbbr: '{}'", nodeGuid, meshGuid, texSetAbbr);

                for (const auto child : node->eachChild())
                {
                    DoorEntry e;

                    e.id = child->valueAsInt("id");
                    e.farDoor = child->valueAsInt("fardoor");
                    // NOTE: explicitly not using valueAsUInt because of 64bit value
                    e.farGuid = std::stoul(child->valueOf("farguid"), nullptr, 16);

                    doorMap.emplace(nodeGuid, std::move(e));
                }

                const std::string meshFileName = nodeMeshGuidDb->resolveFileName(meshGuid);

                if (meshFileName != meshGuid)
                {
                    // this is pretty messy because the auxiliary object seems to be intended to be unique
                    // so we have to reassign the pipeline to our local options
                    // we are doing this because of things like texture set abbrvs that are related to this one load
                    auto localOptions = vsg::Options::create(*options);
                    localOptions->setValue("texsetabbr", texSetAbbr);

                    // can't just set the object because of const
                    //localOptions->setObject("PipelineLayout", options->getObject("PipelineLayout")->cast<vsg::PipelineLayout>());

                    // we might as well just reference the pipeline layout at this point rather than passing it around
                    localOptions->setObject("PipelineLayout", SiegeNodePipeline::PipelineLayout);

                    if (auto mesh = vsg::read(meshFileName, localOptions).cast<vsg::Group>(); mesh != nullptr)
                    //if (auto mesh = vsg::read(meshFileName, options).cast<vsg::Group>(); mesh != nullptr)
                    {
                        // temp
                        mesh->setValue("name", vsg::simpleFilename(meshFileName));

                        auto xform = vsg::MatrixTransform::create();

#if 0
                        xform->setValue("bounds_camera", node->valueAsBool("bounds_camera"));
                        xform->setValue("camera_fade", node->valueAsBool("camera_fade"));
#endif
                        xform->setValue<uint32_t>("guid", node->valueAsUInt("guid"));
#if 0
                        xform->setValue<uint32_t>("nodelevel", node->valueAsUInt("nodelevel"));
                        xform->setValue<uint32_t>("nodeobject", node->valueAsUInt("nodeobject"));
                        xform->setValue<uint32_t>("nodesection", node->valueAsUInt("nodesection"));
                        xform->setValue("occludes_camera", node->valueAsBool("occludes_camera"));
                        xform->setValue("occludes_light", node->valueAsBool("occludes_light"));
#endif

                        group->addChild(xform);
                        xform->addChild(mesh);

                        nodeMap.emplace(nodeGuid, xform);
                    }
                }
                else
                {
                    log->error("mesh guid {} is not listed in {}", meshGuid, "/world/global/siege_nodes");
                }
            }

            log->info("there are {} unique meshes in this region", uniqueMeshes.size());

            // now position it all
            const uint32_t targetGuid = doc.valueAsUInt("siege_node_list:targetnode");

            std::function<void(const uint32_t)> func;

            func = [&func, &doorMap, &nodeMap, &completeSet](const uint32_t guid) {
                if (completeSet.insert(guid).second)
                {
                    auto targetNode = nodeMap[guid];

                    const auto range = doorMap.equal_range(guid);

                    for (auto entry = range.first; entry != range.second; ++entry)
                    {
                        auto connectNode = nodeMap[entry->second.farGuid];
                        auto siegeNodeMesh = connectNode->children[0].cast<SiegeNodeMesh>(); // grab our siege mesh since thats where our cache is

                        SiegeNodeMesh::connect(targetNode, entry->second.id, connectNode, entry->second.farDoor);

                        // if we setup the matrix data here we get very inflated values, i think because of just how we are looping?

                        if (completeSet.count(entry->second.farGuid) == 0)
                        {
                            func(entry->second.farGuid);
                        }
                    }
                }
            };

            func(targetGuid);

            log->info("region loaded with {} nodes, targetGuid: 0x{:x}", group->children.size(), targetGuid);

            return group;
        }

        return vsg::ref_ptr<vsg::Object>();
    };
} // namespace ehb
