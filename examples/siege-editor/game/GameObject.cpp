
#include "GameObject.hpp"

#include <algorithm>
#include "io/Fuel.hpp"

namespace ehb
{
    struct null_deleter
    {
        void operator()(void const*) const
        {
        }
    };

    GameObject::GameObject() : self(this, null_deleter())
    {
    }
}