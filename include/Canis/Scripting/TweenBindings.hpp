#pragma once
#include <Canis/Entity.hpp>
#include <functional>
namespace Canis {
class App;
}
namespace Canis::Scripting {
void RegisterTweenBindings(App &, std::function<Entity(uint64_t)>);
}
