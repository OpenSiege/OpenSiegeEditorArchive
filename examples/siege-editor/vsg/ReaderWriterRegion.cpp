
#include "ReaderWriterRegion.hpp"
#include "io/IFileSys.hpp"
#include "vsg/ReaderWriterSNO.hpp"
#include "vsg/Aspect.hpp"

// i'm lazy
#include "vsg/all.h"

namespace ehb
{
    ReaderWriterRegion::ReaderWriterRegion(IFileSys& fileSys, FileNameMap& fileNameMap) :
        fileSys(fileSys), fileNameMap(fileNameMap)
    {
        log = spdlog::get("log");
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
        auto noninteractivedotgas = path + "/objects/regular/non_interactive.gas";
        auto actordotgas = path + "/objects/actor.gas";

        InputStream main = fileSys.createInputStream(maindotgas);
        InputStream nodes = fileSys.createInputStream(nodesdotgas);
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

                { // load up actors as a test

                    if (actor != nullptr)
                    {
                        if (Fuel doc; doc.load(*actor))
                        {
                            spdlog::get("log")->info("loading {}", actordotgas);

                            for (const auto& node : doc.eachChild())
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

                                        t->matrix = vsg::translate(pos.pos);
                                    }

                                    region->addChild(t);
                                }
                            }
                        }
                    }
                    else
                    {
                        spdlog::get("log")->warn("{} is unavailable for this map", actordotgas);
                    }
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
