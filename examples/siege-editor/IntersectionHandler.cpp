
#include "IntersectionHandler.hpp"

namespace ehb
{
    void IntersectionHandler::apply(vsg::KeyPressEvent& keyPress)
    {
    }

    void IntersectionHandler::apply(vsg::ButtonPressEvent& buttonPressEvent)
    {
        lastPointerEvent = &buttonPressEvent;

        if (buttonPressEvent.button == 1)
        {
            interesection(buttonPressEvent);
        }
    }

    void IntersectionHandler::apply(vsg::PointerEvent& pointerEvent)
    {
        lastPointerEvent = &pointerEvent;
    }

    void IntersectionHandler::interesection(vsg::PointerEvent& pointerEvent)
    {
        auto intersector = vsg::LineSegmentIntersector::create(*camera, pointerEvent.x, pointerEvent.y);
        scenegraph->accept(*intersector);

        if (verbose) log->info("intersection(({}, {}), {})", pointerEvent.x, pointerEvent.y, intersector->intersections.size());

        if (intersector->intersections.empty()) return;

        // sort the intersectors front to back
        std::sort(intersector->intersections.begin(), intersector->intersections.end(), [](auto lhs, auto rhs) { return lhs.ratio < rhs.ratio; });

        std::stringstream verboseOutput;

        for (auto& intersection : intersector->intersections)
        {
            if (verbose) verboseOutput << "intersection = " << intersection.worldIntersection << " ";

            if (lastIntersection)
            {
                if (verbose) verboseOutput << ", distance from previous intersection = " << vsg::length(intersection.worldIntersection - lastIntersection.worldIntersection);
            }

            if (verbose)
            {
                for (auto& node : intersection.nodePath)
                {
                    verboseOutput << ", " << node->className();
                }

                verboseOutput << ", Arrays[ ";
                for (auto& array : intersection.arrays)
                {
                    verboseOutput << array << " ";
                }
                verboseOutput << "] [";
                for (auto& ir : intersection.indexRatios)
                {
                    verboseOutput << "{" << ir.index << ", " << ir.ratio << "} ";
                }
                verboseOutput << "]";

                verboseOutput << std::endl;

                // spdlog::get("log")->info("{}", verboseOutput.str());
            }
        }

        lastIntersection = intersector->intersections.front();

        auto t1 = vsg::MatrixTransform::create();
        t1->matrix = vsg::translate(lastIntersection.worldIntersection);
        loadandcompile->loadRequest("m_c_gah_fg_pos_a1", t1, options);
        scenegraph->addChild(t1);
    }
} // namespace ehb