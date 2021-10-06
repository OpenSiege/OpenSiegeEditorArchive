
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

        // std::unordered_map<vsg::ref_ptr<vsg::Group>, std::vector<InstanceData>> instanceData;

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

                        // since our data cache is enabled, we should be getting the same pointer back every time we load this node
                        // from here we can check for the instance data that should be directly associated with it
                        // on the first load we have to make sure we actually create and store the instance data
#if 0
                        vsg::ref_ptr<InstanceData> instanceData (mesh->getObject<InstanceData>("InstanceData"));
                        if (instanceData == nullptr)
                        {
                            instanceData = InstanceData::create();
                            mesh->setObject("InstanceData", instanceData);

                            uniqueMeshes.emplace(mesh.cast<SiegeNodeMesh>());
                        }
#endif

                        auto xform = vsg::MatrixTransform::create();

#if 0
                        xform->setValue("bounds_camera", node->valueAsBool("bounds_camera"));
                        xform->setValue("camera_fade", node->valueAsBool("camera_fade"));
                        xform->setValue<uint32_t>("guid", node->valueAsUInt("guid"));
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

#if 1
            // loop all the nodes loaded
            for (auto const& [nodeGuid, xform] : nodeMap)
            {
                std::string mapName;
                xform->children[0]->getValue("name", mapName);
                uint32_t guid;
                xform->getValue("guid", guid);

                // now find a matching node from the first pass and the unique nodes
                for (auto const& mesh : uniqueMeshes)
                {
                    std::string name;
                    mesh->getValue("name", name);
                    if (mapName == name)
                    {
                        // now store the instance information
                        mesh->getObject<InstanceData>("InstanceData")->matrices.emplace_back(xform->matrix);

                        vsg::vec4 tmpPos(xform->matrix[3]);
                        vsg::dquat quat;
                        auto m = xform->matrix;

#    if 0
                        auto fourXSquaredMinus1 = xform->matrix[0][0] - xform->matrix[1][1] - xform->matrix[2][2];
                        auto fourYSquaredMinus1 = xform->matrix[1][1] - xform->matrix[0][0] - xform->matrix[2][2];
                        auto fourZSquaredMinus1 = xform->matrix[2][2] - xform->matrix[0][0] - xform->matrix[1][1];
                        auto fourWSquaredMinus1 = xform->matrix[0][0] + xform->matrix[1][1] + xform->matrix[2][2];

                        int biggestIndex = 0;
                        double fourBiggestSquaredMinus1 = fourWSquaredMinus1;
                        if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
                        {
                            fourBiggestSquaredMinus1 = fourXSquaredMinus1;
                            biggestIndex = 1;
                        }
                        if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
                        {
                            fourBiggestSquaredMinus1 = fourYSquaredMinus1;
                            biggestIndex = 2;
                        }
                        if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
                        {
                            fourBiggestSquaredMinus1 = fourZSquaredMinus1;
                            biggestIndex = 3;
                        }

                        double biggestVal = sqrt(fourBiggestSquaredMinus1 + static_cast<double>(1)) * static_cast<double>(0.5);
                        double mult = static_cast<double>(0.25) / biggestVal;

                        auto m = xform->matrix;

                        switch (biggestIndex)
                        {
                        case 0:
                            quat.set(biggestVal, (m[1][2] - m[2][1]) * mult, (m[2][0] - m[0][2]) * mult, (m[0][1] - m[1][0]) * mult);
                            break;
                        case 1:
                            quat.set((m[1][2] - m[2][1]) * mult, biggestVal, (m[0][1] + m[1][0]) * mult, (m[2][0] + m[0][2]) * mult);
                            break;
                        case 2:
                            quat.set((m[2][0] - m[0][2]) * mult, (m[0][1] + m[1][0]) * mult, biggestVal, (m[1][2] + m[2][1]) * mult);
                            break;
                        case 3:
                            quat.set((m[0][1] - m[1][0]) * mult, (m[2][0] + m[0][2]) * mult, (m[1][2] + m[2][1]) * mult, biggestVal);
                            break;
                        default: // Silence a -Wswitch-default warning in GCC. Should never actually get here. Assert is just for sanity.
                            assert(false);
                        }
#    endif

                        // https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
                        double tr = m[0][0] + m[1][1] + m[2][2];
                        if (tr > 0)
                        {
                            double S = sqrt(tr + 1.0) * 2;
                            quat.set((m[2][1] - m[1][2]) / S, (m[0][2] - m[2][0]) / S, (m[1][0] - m[0][1]) / S, 0.25 * S);
                        }
                        else if ((m[0][0] > m[1][1]) && (m[0][0] > m[2][2]))
                        {
                            double S = sqrt(1.0 + m[0][0] - m[1][1] - m[2][2]) * 2; // S=4*qx
                            quat.set(0.25 * S, (m[0][1] + m[1][0]) / S, (m[0][2] + m[2][0]) / S, (m[2][1] - m[1][2]) / S);
                        }
                        else if (m[1][1] > m[2][2])
                        {
                            double S = sqrt(1.0 + m[1][1] - m[0][0] - m[2][2]) * 2; // S=4*qy
                            quat.set((m[0][1] + m[1][0]) / S, 0.25 * S, (m[1][2] + m[2][1]) / S, (m[0][2] - m[2][0]) / S);
                        }
                        else
                        {
                            double S = sqrt(1.0 + m[2][2] - m[0][0] - m[1][1]) * 2; // S=4*qz
                            quat.set((m[0][2] + m[2][0]) / S, (m[1][2] + m[2][1]) / S, 0.25 * S, (m[1][0] - m[0][1]) / S);
                        }

                        // constexpr t_mat4<T> mat4_cast(const t_quat<T>& q)

                        // matrix = translate(cp.position) * scale(cp.scale) * mat4_cast(cp.rotation);

                        vsg::dvec3 yup(tmpPos[0], tmpPos[1], tmpPos[2]);
                        mesh->getObject<InstanceData>("InstanceData")->shader.emplace_back(InstanceData::Data{vsg::dvec3(tmpPos[0], tmpPos[1], tmpPos[2]), quat});

                        //mesh->getObject<InstanceData>("InstanceData")->data.push_back(yup);

                        int foo = 55;
                    }
                }
            }

            for (auto const& mesh : uniqueMeshes)
            {
                std::string name;
                mesh->getValue("name", name);
                // log->info("{} has {} instances", name, mesh->getObject<InstanceData>("InstanceData")->matrices.size());
            }
#endif
            log->info("region loaded with {} nodes, targetGuid: 0x{:x}", group->children.size(), targetGuid);

            return group;
        }

        return vsg::ref_ptr<vsg::Object>();
    };
} // namespace ehb
