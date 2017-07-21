#ifndef ATLAS_MATH_H
#define ATLAS_MATH_H

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <array>

namespace Atlas {
    namespace Math {
        glm::quat get_rotation_to(const glm::vec3& src, const glm::vec3& dest, const glm::vec3& fallback_axis = glm::vec3(0,0,0));

        glm::mat3 mat3_from_axes(const std::array<glm::vec3, 3> xyz);
        std::array<glm::vec3, 3> axes_from_mat3(const glm::mat3& mat);
    }
}

#endif // ATLAS_MATH_H