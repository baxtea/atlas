#include "mesh.h"
//#include "atlas.h"
#include "glm/glm.hpp"
#include <array>

using namespace Atlas;
/*
std::shared_ptr<StaticMesh> create_from_data() {

}

std::shared_ptr<StaticMesh> StaticMesh::create_primitive::box(float width, float height, float depth) {
    auto mesh = std::make_shared<StaticMesh>(Hidden());
    
    const float half_x = width/2;
    const float half_y = height/2;
    const float half_z = depth/2;

    const std::vector<glm::vec3> vertices = {
        // Bottom face -- starting back right, counter-clockwise
        {half_x, half_y, half_z},
        {-half_x, half_y, half_z},
        {half_x, half_y, -half_z},
        {-half_x, half_y, -half_z},
        // Top face -- starting back right, counter-clockwise
        {half_x, -half_y, half_z},
        {-half_x, -half_y, half_z},
        {half_x, -half_y, -half_z},
        {-half_x, -half_y, -half_z}
    };
    mesh->m_vertex_buffer = Anvil::Buffer::create_nonsparse(g_device, 
        vertices.size() * sizeof(decltype(vertices)::value_type), // total size of buffer
        Anvil::QUEUE_FAMILY_GRAPHICS_BIT, // which queues can access this buffer
        VK_SHARING_MODE_EXCLUSIVE, // only access from one queue at a time, since only one queue has permissions in the first place
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // this is a vertex buffer
        Anvil::MEMORY_FEATURE_FLAG_DEVICE_LOCAL, // memory features
        vertices.data() // data to initialize buffer with
    );
    if (!mesh->m_vertex_buffer) {
        fprintf(stderr, "[!] Failed to create vertex buffer for box primitive\n");
        return nullptr;
    }

    constexpr std::array<uint32_t, 36> indices = {
        2, 1, 3,   3, 1, 0, // Bottom face
        6, 2, 7,   7, 2, 3, // Front face
        5, 1, 6,   6, 1, 2, // Left face
        4, 0, 5,   5, 0, 1, // Back face
        7, 3, 4,   4, 3, 0, // Right face
        5, 6, 4,   4, 6, 3  // Top face
    };
    mesh->m_index_buffer = Anvil::Buffer::create_nonsparse(g_device, 
        indices.size() * sizeof(decltype(indices)::value_type), // total size of buffer
        Anvil::QUEUE_FAMILY_GRAPHICS_BIT, // which queues can access this buffer
        VK_SHARING_MODE_EXCLUSIVE, // only access from one queue at a time, since only one queue has permissions in the first place
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, // this is a vertex buffer
        Anvil::MEMORY_FEATURE_FLAG_DEVICE_LOCAL, // memory features
        indices.data() // data to initialize buffer with
    );
    if (!mesh->m_index_buffer) {
        fprintf(stderr, "[!] Failed to create index buffer for box primitive\n");
        return nullptr;
    }
    
    return mesh;
}
*/