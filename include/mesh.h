#ifndef ATLAS_MESH_H
#define ATLAS_MESH_H

#include <memory>
#include "wrappers/device.h"
#include "wrappers/buffer.h"

namespace Atlas {

    // Contains vertices, indices. maybe normals, tex coords, ...bone weights?
    // Does not handle textures or materials
    class StaticMesh {
        struct Hidden {};
        StaticMesh() = default;
        std::shared_ptr<Anvil::Buffer> m_vertex_buffer, m_index_buffer;
    public:
        explicit StaticMesh(Hidden) {};
        ~StaticMesh() = default;

        // TODO: Implement like all of these
        static std::shared_ptr<StaticMesh> create_from_data(void *data, uint64_t size, VkPrimitiveTopology topology);
        static std::shared_ptr<StaticMesh> create_from_file(const std::string &filename);  // Use assimp?
        struct create_primitive final {
            virtual ~create_primitive() = 0; // make uninstantiable
            static std::shared_ptr<StaticMesh> box(float width, float height, float depth);
            static std::shared_ptr<StaticMesh> cylinder(float radius, float height, uint32_t radial_segments);
            static std::shared_ptr<StaticMesh> cone(float radius, float height, uint32_t radial_segments);
            static std::shared_ptr<StaticMesh> sphere(float radius, uint32_t radial_segments);
            static std::shared_ptr<StaticMesh> tube(float radius, float height, float thickness, uint32_t radial_segments);
            static std::shared_ptr<StaticMesh> torus(float radius_main, float radius_cross_section, uint32_t radial_segments_main, uint32_t radial_segments_cross_section);
        };
    };

    class SkeletalMesh {
        // TODO
    };
}

#endif // ATLAS_MESH_H