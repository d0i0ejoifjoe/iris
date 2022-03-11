////////////////////////////////////////////////////////////////////////////////
//         Distributed under the Boost Software License, Version 1.0.         //
//            (See accompanying file LICENSE or copy at                       //
//                 https://www.boost.org/LICENSE_1_0.txt)                     //
////////////////////////////////////////////////////////////////////////////////

#include "graphics/mesh_manager.h"

#include <cmath>
#include <cstdint>
#include <queue>
#include <sstream>
#include <stack>
#include <vector>

#include "core/colour.h"
#include "core/error_handling.h"
#include "core/resource_loader.h"
#include "core/transform.h"
#include "core/vector3.h"
#include "graphics/bone.h"
#include "graphics/mesh.h"
#include "graphics/mesh_loader.h"
#include "graphics/skeleton.h"
#include "graphics/texture.h"
#include "graphics/vertex_data.h"
#include "log/log.h"

namespace iris
{

MeshManager::MeshManager()
    : loaded_meshes_()
    , loaded_skeletons_()
{
}

const Mesh *MeshManager::sprite(const Colour &colour)
{
    // create a unique for this mesh
    std::stringstream strm{};
    strm << "!sprite" << colour;
    const auto id = strm.str();

    if (loaded_meshes_.count(id) == 0u)
    {
        std::vector<VertexData> vertices{
            {{-1.0, 1.0, 0.0f}, {}, colour, {0.0f, 1.0f, 0.0f}},
            {{1.0, 1.0, 0.0f}, {}, colour, {1.0f, 1.0f, 0.0f}},
            {{1.0, -1.0, 0.0f}, {}, colour, {1.0f, 0.0f, 0.0f}},
            {{-1.0, -1.0, 0.0f}, {}, colour, {0.0f, 0.0f, 0.0f}}};

        std::vector<std::uint32_t> indices{0, 2, 1, 3, 2, 0};

        loaded_meshes_[id] = create_mesh(vertices, indices);
    }

    return loaded_meshes_[id].get();
}

const Mesh *MeshManager::cube(const Colour &colour)
{
    // create a unique for this mesh
    std::stringstream strm{};
    strm << "!cube" << colour;

    const auto id = strm.str();

    if (loaded_meshes_.count(id) == 0u)
    {
        loaded_meshes_[id] = unique_cube(colour);
    }

    return loaded_meshes_[id].get();
}

std::unique_ptr<Mesh> MeshManager::unique_cube(const Colour &colour)
{
    std::vector<VertexData> vertices{
        {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, colour, {0.0f, 0.0f, 0.0f}},
        {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, colour, {1.0f, 0.0f, 0.0f}},
        {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, colour, {1.0f, 1.0f, 0.0f}},
        {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, colour, {0.0f, 1.0f, 0.0f}},
        {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, colour, {0.0f, 0.0f, 0.0f}},
        {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, colour, {1.0f, 0.0f, 0.0f}},
        {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, colour, {1.0f, 1.0f, 0.0f}},
        {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, colour, {0.0f, 1.0f, 0.0f}},
        {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, colour, {0.0f, 0.0f, 0.0f}},
        {{-1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, colour, {1.0f, 0.0f, 0.0f}},
        {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, colour, {1.0f, 1.0f, 0.0f}},
        {{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, colour, {0.0f, 1.0f, 0.0f}},
        {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, colour, {0.0f, 0.0f, 0.0f}},
        {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, colour, {1.0f, 0.0f, 0.0f}},
        {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, colour, {1.0f, 1.0f, 0.0f}},
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, colour, {0.0f, 1.0f, 0.0f}},
        {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, colour, {0.0f, 0.0f, 0.0f}},
        {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, colour, {1.0f, 0.0f, 0.0f}},
        {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, colour, {1.0f, 1.0f, 0.0f}},
        {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, colour, {0.0f, 1.0f, 0.0f}},
        {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, colour, {0.0f, 0.0f, 0.0f}},
        {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, colour, {1.0f, 0.0f, 0.0f}},
        {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, colour, {1.0f, 1.0f, 0.0f}},
        {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, colour, {0.0f, 1.0f, 0.0f}}};

    std::vector<std::uint32_t> indices{0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
                                       12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23};

    return create_mesh(vertices, indices);
}

const Mesh *MeshManager::plane(const Colour &colour, std::uint32_t divisions)
{
    expect(divisions != 0, "divisions must be >= 0");

    // create a unique for this mesh
    std::stringstream strm{};
    strm << "!plane" << colour << ":" << divisions;
    const auto id = strm.str();

    if (loaded_meshes_.count(id) == 0u)
    {
        std::vector<VertexData> vertices(static_cast<std::size_t>(std::pow(divisions + 1u, 2u)));

        const Vector3 normal{0.0f, 0.0f, 1.0f};
        const Vector3 tangent{1.0f, 0.0f, 0.0f};
        const Vector3 bitangent{0.0f, 1.0f, 0.0f};

        const auto width = 1.0f / static_cast<float>(divisions);

        for (auto y = 0u; y <= divisions; ++y)
        {
            for (auto x = 0u; x <= divisions; ++x)
            {
                vertices[(y * (divisions + 1u)) + x] = {
                    {(x * width) - 0.5f, (y * width) - 0.5f, 0.0f},
                    normal,
                    colour,
                    {(x * width) * divisions, (1.0f - (y * width)) * divisions, 0.0f},
                    tangent,
                    bitangent};
            }
        }

        std::vector<std::uint32_t> indices{};

        for (auto y = 0u; y < divisions; ++y)
        {
            for (auto x = 0u; x < divisions; ++x)
            {
                indices.emplace_back((y * (divisions + 1u) + x));
                indices.emplace_back(((y + 1) * (divisions + 1u) + x));
                indices.emplace_back((y * (divisions + 1u) + x + 1u));
                indices.emplace_back((y * (divisions + 1u) + x + 1u));
                indices.emplace_back(((y + 1) * (divisions + 1u) + x));
                indices.emplace_back(((y + 1) * (divisions + 1u) + x + 1u));
            }
        }

        loaded_meshes_[id] = create_mesh(vertices, indices);
    }

    return loaded_meshes_[id].get();
}

const Mesh *MeshManager::heightmap(const Colour &colour, const Texture *height_image)
{
    ensure(height_image->width() == height_image->height(), "height_image must be square");

    // create a unique for this mesh
    std::stringstream strm{};
    strm << "!height_map" << colour << ":" << height_image;
    const auto id = strm.str();

    const auto divisions = height_image->width();
    const auto &height_data = height_image->data();

    if (loaded_meshes_.count(id) == 0u)
    {
        std::vector<VertexData> vertices(static_cast<std::size_t>(std::pow(divisions, 2u)));

        const Vector3 tangent{1.0f, 0.0f, 0.0f};
        const Vector3 bitangent{0.0f, 1.0f, 0.0f};

        const auto width = 1.0f / static_cast<float>(divisions);

        // lambda to get adjacent points (clamped to edges)
        const auto get_adjacent =
            [&height_data,
             divisions,
             width](std::uint32_t x, std::uint32_t z, std::int32_t offset_x, std::int32_t offset_z) -> Vector3 {
            auto adj_x = x;
            if (x != 0u && offset_x == -1)
            {
                --adj_x;
            }
            else if (x != (divisions - 1u) && offset_x == 1)
            {
                ++adj_x;
            }

            auto adj_z = z;
            if (z != 0u && offset_z == -1)
            {
                --adj_z;
            }
            else if (z != (divisions - 1u) && offset_z == 1)
            {
                ++adj_z;
            }

            const auto raw_y = height_data[((adj_z * divisions) + adj_x) * 4u];
            const auto y = static_cast<float>(raw_y) / 255.0f;

            return {(adj_x * width) - 0.5f, y, (adj_z * width) - 0.5f};
        };

        for (auto z = 0u; z < divisions; ++z)
        {
            for (auto x = 0u; x < divisions; ++x)
            {
                const auto right = get_adjacent(x, z, 1, 0);
                const auto left = get_adjacent(x, z, -1, 0);
                const auto top = get_adjacent(x, z, 0, -1);
                const auto bottom = get_adjacent(x, z, 0, 1);

                const auto raw_y = height_data[((z * divisions) + x) * 4u];
                const auto y = static_cast<float>(raw_y) / 255.0f;

                vertices[(z * (divisions)) + x] = {
                    {(x * width) - 0.5f, y, (z * width) - 0.5f},
                    Vector3::normalise(Vector3::cross((right - left), (top - bottom))),
                    colour,
                    {(x * width) * 30.0f, (1.0f - (z * width)) * 30.0f, 0.0f},
                    tangent,
                    bitangent};
            }
        }

        std::vector<std::uint32_t> indices{};

        for (auto z = 0u; z < divisions - 1u; ++z)
        {
            for (auto x = 0u; x < divisions - 1u; ++x)
            {
                indices.emplace_back((z * (divisions) + x));
                indices.emplace_back(((z + 1) * (divisions) + x));
                indices.emplace_back((z * (divisions) + x + 1u));
                indices.emplace_back((z * (divisions) + x + 1u));
                indices.emplace_back(((z + 1) * (divisions) + x));
                indices.emplace_back(((z + 1) * (divisions) + x + 1u));
            }
        }

        loaded_meshes_[id] = create_mesh(vertices, indices);
    }

    return loaded_meshes_[id].get();
}

const Mesh *MeshManager::quad(
    const Colour &colour,
    const Vector3 &lower_left,
    const Vector3 &lower_right,
    const Vector3 &upper_left,
    const Vector3 &upper_right)
{
    // create a unique for this mesh
    std::stringstream strm{};
    strm << "!quad" << colour << ":" << lower_left << ":" << lower_right << ":" << upper_left << ":" << upper_right;
    const auto id = strm.str();

    if (loaded_meshes_.count(id) == 0u)
    {
        std::vector<VertexData> vertices{
            {upper_left, {}, colour, {0.0f, 1.0f, 0.0f}},
            {upper_right, {}, colour, {1.0f, 1.0f, 0.0f}},
            {lower_right, {}, colour, {1.0f, 0.0f, 0.0f}},
            {lower_left, {}, colour, {0.0f, 0.0f, 0.0f}}};

        std::vector<std::uint32_t> indices{0, 2, 1, 3, 2, 0};

        loaded_meshes_[id] = create_mesh(vertices, indices);
    }

    return loaded_meshes_[id].get();
}

const Mesh *MeshManager::load_mesh(const std::string &mesh_file)
{
    if (loaded_meshes_.count(mesh_file) == 0u)
    {
        const auto &[vertices, indices, skeleton, animations] = mesh_loader::load(mesh_file);
        loaded_meshes_[mesh_file] = create_mesh(vertices, indices);
        loaded_skeletons_[mesh_file] = skeleton;
        loaded_animations_[mesh_file] = animations;
    }

    return loaded_meshes_[mesh_file].get();
}

Skeleton MeshManager::load_skeleton(const std::string &mesh_file)
{
    load_mesh(mesh_file);
    return loaded_skeletons_[mesh_file];
}

std::vector<Animation> MeshManager::load_animations(const std::string &mesh_file)
{
    load_mesh(mesh_file);
    return loaded_animations_[mesh_file];
}

}
