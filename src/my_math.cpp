#include "my_math.h"
#include <math.h>

#if (GLM_COMPILER & GLM_COMPILER_VC15) && (_MSC_VER < 1910)
#error vc15
#endif

using namespace Atlas;
GLM_CONSTEXPR_CTOR glm::vec3 VEC3_0(0,0,0);

glm::quat Math::get_rotation_to(const glm::vec3& u, const glm::vec3& v, const glm::vec3& fallback_axis) {
    // From Sam Hocevar's article Quaternion from two vectors: the final version'
    // http://sam.hocevar.net/blog/category/maths/

    // Build a unit quaternion representing the rotation from u to v.
    // The input vectors need not be normalized.
    float norm_u_norm_v = std::sqrt(glm::dot(u, u) * glm::dot(v, v));
    float real_part = norm_u_norm_v + dot(u, v);
    glm::vec3 axis;
    if (real_part < 1.e-6f * norm_u_norm_v)
    {
        /* If u and v are exactly opposite, rotate 180 degrees
         * around an arbitrary orthogonal axis. Axis normalisation
         * can happen later, when we normalise the quaternion.
         */
        real_part = 0.0f;
        axis = (fallback_axis != VEC3_0) ? fallback_axis
             : ( abs(u.x) > abs(u.z) ? glm::vec3(-u.y, u.x, 0.0f)
             : glm::vec3(0.0f, -u.z, u.y) );
    }
    else
    {
        // Otherwise, build quaternion the standard way.
        axis = glm::cross(u, v);
    }
    return glm::normalize(glm::quat(real_part, axis));
}

glm::mat3 Math::mat3_from_axes(const std::array<glm::vec3, 3> xyz) {
    glm::mat3 rot_from_axes;

    for (size_t col = 0; col < 3; ++col) {
        rot_from_axes[0][col] = xyz[col].x;
        rot_from_axes[1][col] = xyz[col].y;
        rot_from_axes[2][col] = xyz[col].z;
    }

    return rot_from_axes;
}

std::array<glm::vec3, 3> Math::axes_from_mat3(const glm::mat3& mat) {
    std::array<glm::vec3, 3> axes;

    for (size_t col = 0; col < 3; ++col) {
        axes[col].x = mat[0][col];
        axes[col].y = mat[1][col];
        axes[col].z = mat[2][col];
    }

    return axes;
}