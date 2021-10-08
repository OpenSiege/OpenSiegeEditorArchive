
#include "ObjectDb.hpp"

#include "ContentDb.hpp"

#if 0
#include "AspectComponent.hpp"
#include "BodyComponent.hpp"
#include "MindComponent.hpp"
#include "PlacementComponent.hpp"
#include "TestComponent.hpp"
#endif

namespace ehb
{
#if 0
    void ObjectDb::update(double deltaTime)
    {
        // TODO: purge objects marked for deletion

        for (const auto& entry : db)
        {
            auto& object = entry.second;

            object->onUpdate(deltaTime);
        }
    }
#endif

    GameObject* ObjectDb::cloneGameObject(const std::string& name)
    {
        static uint32_t id = 1;

        if (const auto tmpl = contentDb.getGameObjectTmpl(name))
        {
            auto object = new GameObject();
            {
                object->TemplateName = name;

                //object->onXfer(*tmpl);
            }

            db.emplace(id++, object);

            return object;
        }

        return nullptr;
    }

    void ObjectDb::markForDeletion(GameObject* object)
    {
        q.emplace(object);
    }
}