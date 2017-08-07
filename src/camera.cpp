#include "camera.h"
#include "glm/gtc/matrix_transform.hpp"
#include "my_math.h"
#include <iostream>
#include <array>
#define _USE_MATH_DEFINES // Tells <math.h> to define M_PI and whatnot
#include <math.h>

using namespace Atlas;

constexpr glm::vec3 UNIT_X(1, 0, 0);
constexpr glm::vec3 UNIT_Y(0, 1, 0);
constexpr glm::vec3 UNIT_Z(0, 0, 1);
constexpr glm::vec3 VEC3_0(0, 0, 0);


Camera::Camera(const std::string& name) : m_name(name), m_view_is_old(true) {
    // m_view will be calculated before access
    // m_position initializes to origin
    // m_orientation initializes to identity (no rotation)
}

const std::string& Camera::get_name() const {
    return m_name;
}

void Camera::update_view() {
    // m_view = glm::translate( glm::mat4_cast(m_orientation), m_position);

    // View matrix is:
    //
    //  [ Lx  Uy  Dz  Tx  ]
    //  [ Lx  Uy  Dz  Ty  ]
    //  [ Lx  Uy  Dz  Tz  ]
    //  [ 0   0   0   1   ]
    //

    if (m_view_is_old) {
        glm::mat3 rot = glm::mat3_cast(m_orientation);

        // Make the translation relative to new axes
        glm::mat3 rotT = glm::transpose(rot);
        glm::vec3 trans = -rotT * m_position;

        m_view = glm::mat4(); // Reset to identity
        m_view = rotT; // Fills upper 3x3
        m_view[0][3] = trans.x;
        m_view[1][3] = trans.y;
        m_view[2][3] = trans.z;

        m_view_is_old = false;
    }
}

const glm::mat4& Camera::get_view_matrix() const {
    if (m_view_is_old)
        std::cerr << "[!] Camera " << m_name << " providing stale view matrix!\n";
    
    return m_view;
}


void Camera::set_fixed_yaw_axis(bool fixed, const glm::vec3& axis) {
    m_yaw_is_fixed = fixed;
    m_fixed_yaw_axis = glm::normalize(axis);
}


void Camera::set_position(const glm::vec3& p) {
    m_position = p;
    m_view_is_old = true;
}
const glm::vec3& Camera::get_position() const {
    return m_position;
}
void Camera::move(const glm::vec3& delta) {
    m_position += delta;
    m_view_is_old = true;
}
void Camera::move_relative(const glm::vec3& delta) {
    // Transform the vector to align with the camera's local axes
    glm::vec3 transform = m_orientation * delta;

    m_position += transform;
    m_view_is_old = true;
}

// Relative to camera
const glm::vec3 Camera::get_direction() const {
    // Z points out of screen
    return m_orientation * -UNIT_Z;
}
// Relative to camera
const glm::vec3 Camera::get_up() const {
    // Y axis points down in Vulkan, but that's dumb
    // so we're doing those transforms right before being passed to shaders
    return m_orientation * UNIT_Y;
}
// Relative to camera
const glm::vec3 Camera::get_right() const {
    return m_orientation * UNIT_X;
}
// Sets the camera's direction vector
// The up vector will be recalculated based on the current up vector
// (the roll will remain the same.)
void Camera::set_direction(const glm::vec3& dir) {
    // Do nothing if zero vector
    if (dir != glm::vec3(0, 0, 0)) {
        // x, y, z
        std::array<glm::vec3, 3> axes;
        // Reverse direction of vector since Y and Z are upside-down
        axes[2] = -dir;
        axes[2] = glm::normalize(axes[2]);


        if (m_yaw_is_fixed) {

            // Guaranteed to be normalized
            axes[0] = glm::cross(m_fixed_yaw_axis, axes[2]);
            axes[1] = glm::cross(axes[2], axes[0]);

            glm::mat3 rot_from_axes = Math::mat3_from_axes(axes);
            m_orientation = glm::quat_cast(rot_from_axes);
        }
        else {
            // Store reversed dir vector
            glm::vec3 z_adjust_vec = axes[2];
            // Convert from orientation to axes
            glm::mat3 orientation_mat = glm::mat3_cast(m_orientation);
            axes = Math::axes_from_mat3(orientation_mat);

            glm::quat rotation;
            // Calc squared distance between reversed dir and current orientation's z axis
            glm::vec3 diff = axes[2] + z_adjust_vec;
            if ( glm::dot(diff, diff) < 0.00005f ) {
                // 180-degree turn (infinite possible rotation axes)
                // Default to yaw (use current up)
                rotation = glm::angleAxis(float(M_PI), axes[1]);
            }
            else {
                // Derive shortest arc to new direction
                rotation = Math::get_rotation_to(axes[2], z_adjust_vec);
            }

            m_orientation = rotation * m_orientation;
        }

        m_view_is_old = true;
    }


}
// Points the camera at a location in worldspace
void Camera::look_at(const glm::vec3& target) {
    update_view();
    set_direction(target - m_position);
}


void Camera::rotate(const glm::quat delta) {
    // Order of multiplication is important
    glm::quat dres = delta * m_orientation;
    // Normalize delta to avoid cumulative problems with precision
    m_orientation = glm::normalize(dres);

    m_view_is_old = true;
}
// Rotates the camera around an arbitrary axis
void Camera::rotate(const glm::vec3& axis, float angle_rads) {
    rotate(glm::angleAxis(angle_rads, axis));
}


// Rolls the camera anticlockwise around its local z axis
void Camera::roll(float rads) {
    glm::vec3 axis = m_orientation * UNIT_Z;
    rotate(axis, rads);
}
// Rotates the camera anticlockwise around specified y axis
// See set_fixed_yaw_axis()
void Camera::yaw(float rads) {
    glm::vec3 axis;
    if (m_yaw_is_fixed) {
        axis = m_fixed_yaw_axis;
    }
    else {
        axis = m_orientation * UNIT_Y;
    }

    rotate(axis, rads);
}
// Pitches the camera up/down anticlockwise around its local x axis
void Camera::pitch(float rads) {
    glm::vec3 axis = m_orientation * UNIT_X;
    rotate(axis, rads);
}


void Camera::set_orientation(const glm::quat orientation) {
    m_orientation = glm::normalize(orientation);
    m_view_is_old = true;
}
void Camera::set_orientation(const glm::vec3& axis, float angle_rads) {
    set_orientation(glm::angleAxis(angle_rads, axis));
}