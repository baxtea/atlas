#ifndef ATLAS_CAMERA_H
#define ATLAS_CAMERA_H

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <string>
#include <math.h>

namespace Atlas {
    class Director {

    public:
        void set_camera();
    };

    class Camera {

        const std::string m_name;

        glm::vec3 m_position;
        glm::quat m_orientation;
        glm::mat4 m_view;

        float m_aspect_ratio;
        bool m_view_is_old;

        bool m_yaw_is_fixed;
        glm::vec3 m_fixed_yaw_axis;
    public:
        Camera(const std::string& name);
        ~Camera() = default;
        const std::string& get_name() const;

        void update_view();
        // Must call update_view() beforehand, since const function
        const glm::mat4& get_view_matrix() const;


        // By default, camera yaws around its local Y axis
        // Can be overridden for an FPS-style camera
        void set_fixed_yaw_axis(bool fixed, const glm::vec3& axis = {0, -1, 0});


        void set_position(const glm::vec3& position);
        const glm::vec3& get_position() const;
        // Uses global coordinates
        void move(const glm::vec3& delta);
        // Uses local coordinates
        void move_relative(const glm::vec3& delta);

        // Relative to camera
        const glm::vec3 get_direction() const;
        // Relative to camera
        const glm::vec3 get_up() const;
        // Relative to camera
        const glm::vec3 get_right() const;
        // Sets the camera's direction vector
        // The up vector will be recalculated based on the current up vector
        // (the roll will remain the same.)
        void set_direction(const glm::vec3& dir);
        // Points the camera at a location in worldspace
        void look_at(const glm::vec3& target);


        void rotate(const glm::quat delta);
        // Rotates the camera around an arbitrary axis
        void rotate(const glm::vec3& axis, float angle_rads);
        // Rolls the camera anticlockwise around its local z axis
        void roll(float rads);
        // Rotates the camera anticlockwise around its local y axis
        void yaw(float rads);
        // Pitches the camera up/down anticlockwise around its local z axis
        void pitch(float rads);
        void set_orientation(const glm::quat orientation);
        void set_orientation(const glm::vec3& axis, float angle_rads);
    };
}

#endif // ATLAS_CAMERA_H