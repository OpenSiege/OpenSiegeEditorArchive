
#pragma once

#include <memory>
#include <vsg/nodes/Group.h>

// this class emulates the class: Aspect in DS
namespace ehb
{
    class Aspect : public vsg::Inherit<vsg::Group, Aspect>
    {
    public:
        struct Impl;

    public:
        explicit Aspect(std::shared_ptr<Impl> impl);

        virtual ~Aspect() = default;

    private:
        friend class ReaderWriterASP;

        std::shared_ptr<Impl> d;
    };
} // namespace ehb
