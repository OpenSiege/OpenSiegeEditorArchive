
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace vsg
{
    class Group;
}

namespace ehb
{
    class FuelBlock;
    class GameObject final
    {
    public:

        GameObject();
        ~GameObject() = default;

        std::weak_ptr<GameObject> safeRef() const;

        //void onUpdate(double deltaTime);
        //void onXfer(const FuelBlock& root);

        void addToSceneGraph(vsg::Group& root);
        void removeFromSceneGraph(vsg::Group& root);

    public:

        std::string TemplateName = "";

    private:

        std::shared_ptr<GameObject> self;
    };

}