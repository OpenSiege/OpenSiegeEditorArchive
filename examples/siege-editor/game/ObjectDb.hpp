
#pragma once

#include <string>
#include <map>
#include <memory>
#include <queue>
#include "GameObject.hpp"

namespace ehb
{
    class ContentDb;
    class ObjectDb
    {
    public:

        ObjectDb() = default;

        void init(ContentDb& contentDb);

        // not needed for right now
        // void update(double deltaTime);

        GameObject* cloneGameObject(const std::string& name);

        void markForDeletion(GameObject* object);

    private:

        ContentDb& contentDb;
        std::map<uint32_t, std::unique_ptr<GameObject>> db;
        std::queue<GameObject*> q; // TODO: store id instead
    };
}