
#include "ReaderWriterRegion.hpp"

#include <vsg/io/read.h>

#include "io/IFileSys.hpp"
#include "world/SiegeNode.hpp"
#include "world/Region.hpp"
#include "vsg/Aspect.hpp"

namespace ehb
{
    ReaderWriterRegion::ReaderWriterRegion(IFileSys& fileSys, FileNameMap& fileNameMap) :
        fileSys(fileSys), fileNameMap(fileNameMap)
    {
        log = spdlog::get("log");
        //log->set_level(spdlog::level::debug);
    }

    vsg::ref_ptr<vsg::Object> ReaderWriterRegion::read(const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options) const
    {
        const std::string simpleFilename = vsg::simpleFilename(filename);

        // check to make sure this is a nodes.gas file
        if (vsg::fileExtension(filename) != "region")
            return {};

        // the below feels a bit hacky but we need to be able to access all the files in the region on load - for now
        // you can think of this loader as a proxy loader for the rest of the elements in a region

        log->info("about to load region with path : {} and simpleFilename {}", filename, simpleFilename);

        auto path = vsg::removeExtension(filename);
        auto maindotgas = path + "/main.gas";
        auto nodesdotgas = path + "/terrain_nodes/nodes.gas";

        InputStream main = fileSys.createInputStream(maindotgas);
        InputStream nodes = fileSys.createInputStream(nodesdotgas);
       
        // default our lookup to maps with difficulties since retail maps have them
        //! TODO: remove when difficulties actually do something
        auto objectspath = path + "/objects/regular";

        // sanity check if we have difficulties. if we don't then adjust the lookup
        if (!fileSys.loadGasFile(objectspath + "/actor.gas"))
        {
            objectspath = path + "/objects";
            log->info("this map does not have difficulties so adjusting path to {}", objectspath);
        }

        auto objectFiles = { "actor.gas", "command.gas", "container.gas", "elevator.gas", "emitter.gas", "generator.gas", "interactive.gas", "inventory.gas", "non_interactive.gas", "special.gas", "test.gas", "trap.gas" };

        auto noninteractivedotgas = objectspath + "/non_interactive.gas";
        auto actordotgas = objectspath + "/actor.gas";

        // objects can fall under difficulty folders - this should probably be a folder check or something
        //if
        InputStream noninteractive = fileSys.createInputStream(noninteractivedotgas);
        InputStream actor = fileSys.createInputStream(actordotgas);

        if (main == nullptr || nodes == nullptr)
        {
            log->critical("main.gas or nodes.gas are missing for region {}", filename);

            return {};
        }

        if (auto region = read_cast<Region>(*main, options))
        {
            if (auto nodeData = vsg::read_cast<vsg::Group>(nodesdotgas, options))
            {
                region->setNodeData(nodeData);

                auto objects = vsg::Group::create();

                { // load all objects
                    for (const auto& file : objectFiles)
                    {
                        if (auto doc = fileSys.loadGasFile(objectspath + "/" + file))
                        {
                            log->debug("loading {}", objectspath + "/" + file);

                            for (const auto& node : doc->eachChild())
                            {
                                if (auto asp = vsg::read_cast<Aspect>("m_i_glb_object-waypoint", options))
                                {
                                    auto t = vsg::MatrixTransform::create();
                                    t->addChild(asp);

                                    // GameObject::onXfer(const FuelBlock& root)
                                    if (auto p = node->child("placement"))
                                    {
                                        auto pos = p->valueAsSiegePos("position");
                                        auto rot = p->valueAsSiegePos("rotation");

                                        t->setValue("position", pos);
                                        t->setValue("rotation", rot);
                                    }

                                    objects->addChild(t);
                                }
                            }
                        }
                    }

                    region->setObjects(objects);
                }

                return region;
            }
        }

        return {};
    }

    vsg::ref_ptr<vsg::Object> ReaderWriterRegion::read(std::istream& stream, vsg::ref_ptr<const vsg::Options> options) const
    {
        if (Fuel doc; doc.load(stream))
        {
            auto region = Region::create();

            region->setValue("guid", doc.valueAsUInt("region:guid"));

            return region;
        }
        else
        {
            log->critical("failed to parse main.gas");
        }

        return {};
    };
} // namespace ehb
