#pragma once
#include <glm/glm.hpp>
#include <string>

namespace nova {
struct Transform {
    glm::vec3 position{0}; glm::vec3 rotation{0}; glm::vec3 scale{1.f,1.f,1.f};
};
struct Renderable {
    int mesh = 0; int material = 0;
};
struct Name { std::string value; };
}
