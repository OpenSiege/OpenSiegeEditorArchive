
#pragma once

#include <spdlog/spdlog.h>

#include <vsg/core/Inherit.h>
#include <vsg/core/Visitor.h>

#include <vsg/viewer/Camera.h>
#include <vsg/nodes/Group.h>
#include <vsg/traversals/LineSegmentIntersector.h>

#include "SiegePipeline.hpp"

namespace ehb
{
    class IntersectionHandler : public vsg::Inherit<vsg::Visitor, IntersectionHandler>
    {
    public:
        vsg::ref_ptr<vsg::Options> options;
        vsg::ref_ptr<vsg::Camera> camera;
        vsg::ref_ptr<vsg::Group> scenegraph;
        vsg::ref_ptr<DynamicLoadAndCompile> loadandcompile;
        bool verbose = true;

        std::shared_ptr<spdlog::logger> log;

        IntersectionHandler(vsg::ref_ptr<vsg::Camera> in_camera, vsg::ref_ptr<vsg::Group> in_scenegraph, vsg::ref_ptr<vsg::Options> in_options, vsg::ref_ptr<DynamicLoadAndCompile> lac) :
            options(in_options),
            camera(in_camera),
            scenegraph(in_scenegraph),
            loadandcompile(lac)
        {

            log = spdlog::get("log");
        }

        void apply(vsg::KeyPressEvent& keyPress) override;

        void apply(vsg::ButtonPressEvent& buttonPressEvent) override;

        void apply(vsg::PointerEvent& pointerEvent) override;

        void interesection(vsg::PointerEvent& pointerEvent);

    protected:
        vsg::ref_ptr<vsg::PointerEvent> lastPointerEvent;
        vsg::LineSegmentIntersector::Intersection lastIntersection;
    };
}