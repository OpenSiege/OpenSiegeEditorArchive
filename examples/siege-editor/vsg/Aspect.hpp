
#pragma once

#include <memory>
#include <vsg/nodes/Group.h>

//#include <osgAnimation/Bone>
//#include <osgAnimation/Skeleton>
//#include <osgAnimation/RigGeometry>

// this class emulates the class: Aspect in DS
namespace ehb
{
    class Aspect : public vsg::Inherit<vsg::Group, Aspect>
    {
    public:

        struct Impl;

    public:

        explicit Aspect(std::shared_ptr<Impl> impl);
        //Aspect(const Aspect& aspect, const osg::CopyOp& copyop );

        virtual ~Aspect() = default;

#if 0
        osg::Object* clone(const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY) const override;

        osg::Geometry * geometry(unsigned int index);

        // by default we don't apply the skeleton to the mesh
        void applySkeleton();

        void removeSkeleton();

        void toggleBoundingBox();
#endif

    private:

        friend class ReaderWriterASP;

        std::shared_ptr<Impl> d;

#if 0
        // hold onto our skeleton so we can switch on and off skinning
        // TODO: we probably shouldn't hold this in memory if it's going to just sit here for most nodes
        osg::ref_ptr<osgAnimation::Skeleton> skeleton;

        // hold onto our psuedo root bone
        osg::ref_ptr<osg::MatrixTransform> pseudoRoot;

        // hold easy access to all parts of our rigged geometry
        std::vector<osg::ref_ptr<osgAnimation::RigGeometry>> rigGeometryVec;

        // hold easy access to all source geometry;
        std::vector<osg::ref_ptr<osg::Geometry>> geometryVec;

        // hold easy access to our bones
        std::vector<osgAnimation::Bone*> bones;

        std::vector<osg::ref_ptr<osg::Group>> debugDrawingGroups;

        bool drawingBoundingBox = false;
#endif
    };
}
