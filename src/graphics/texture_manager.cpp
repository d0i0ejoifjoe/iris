////////////////////////////////////////////////////////////////////////////////
//         Distributed under the Boost Software License, Version 1.0.         //
//            (See accompanying file LICENSE or copy at                       //
//                 https://www.boost.org/LICENSE_1_0.txt)                     //
////////////////////////////////////////////////////////////////////////////////

#include "graphics/texture_manager.h"

#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "core/auto_release.h"
#include "core/data_buffer.h"
#include "core/error_handling.h"
#include "core/resource_loader.h"
#include "graphics/cube_map.h"
#include "graphics/texture.h"
#include "graphics/texture_usage.h"

namespace
{

/**
 * Load an image from a data buffer.
 *
 * @param data
 *   Image data.
 *
 * @returns
 *   Tuple of <data, width, height, number of channels>.
 */
std::tuple<iris::DataBuffer, std::uint32_t, std::uint32_t> parse_image(
    const iris::DataBuffer &data,
    bool flip_on_load = true)
{
    int width = 0;
    int height = 0;
    int num_channels = 0;

    // ensure that images are flipped along the y axis when loaded, this is so
    // they work with what the graphics api treats as the origin
    ::stbi_set_flip_vertically_on_load(flip_on_load);

    // load image using stb library
    iris::AutoRelease<::stbi_uc *, nullptr> raw_data(
        ::stbi_load_from_memory(
            reinterpret_cast<const stbi_uc *>(data.data()),
            static_cast<int>(data.size()),
            &width,
            &height,
            &num_channels,
            0),
        ::stbi_image_free);

    iris::ensure(raw_data && (num_channels != 0), "failed to load image");

    // calculate the total number of bytes needed for the raw data
    const auto size = width * height * num_channels;

    static constexpr auto output_channels = 4u;

    // create buffer big enough for RGBA data
    iris::DataBuffer padded_data{width * height * output_channels};

    // we only store image data as RGBA in the engine, so extend the data if
    // we have less than four channels

    auto dst_ptr = padded_data.data();
    auto *src_ptr = reinterpret_cast<const std::byte *>(raw_data.get());
    const auto *end_ptr = reinterpret_cast<const std::byte *>(raw_data.get() + size);

    while (src_ptr != end_ptr)
    {
        // default pixel value (black with alpha)
        // this allows us to memcpy over the data we do have and leaves the
        // correct defaults if we have less than four channels
        std::byte rgba[] = {std::byte{0x0}, std::byte{0x0}, std::byte{0x0}, std::byte{0xff}};

        memcpy(rgba, src_ptr, num_channels);
        memcpy(dst_ptr, rgba, output_channels);

        dst_ptr += output_channels;
        src_ptr += num_channels;
    }

    return std::make_tuple(
        std::move(padded_data), static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
}

/**
 * Create data for a texture which is just one colour.
 *
 * @param colour
 *   Colour of texture.
 *
 * @param width
 *   Width of texture to create.
 *
 * @param height
 *   Height of texture to create.
 *
 * @returns
 *   Texture data.
 */
iris::DataBuffer create_texture_data(const iris::Colour &colour, std::uint32_t width, std::uint32_t height)
{
    const std::byte colour_bytes[] = {
        static_cast<std::byte>(colour.r * 0xff),
        static_cast<std::byte>(colour.g * 0xff),
        static_cast<std::byte>(colour.b * 0xff),
        static_cast<std::byte>(colour.a * 0xff)};

    iris::DataBuffer colour_data(width * height * 4u);
    auto *cursor = colour_data.data();

    for (auto i = 0u; i < width * height; ++i)
    {
        std::memcpy(cursor, colour_bytes, sizeof(colour_bytes));
        cursor += 4u;
    }

    return colour_data;
}

/**
 * Create data for a texture which is a vertical gradient.
 *
 * @param start
 *   Bottom colour.
 *
 * @param end
 *   Top colour.
 *
 * @param width
 *   Width of texture to create.
 *
 * @param height
 *   Height of texture to create.
 *
 * @returns
 *   Texture data.
 */
iris::DataBuffer create_texture_data(
    const iris::Colour &start,
    const iris::Colour &end,
    std::uint32_t width,
    std::uint32_t height)
{
    iris::DataBuffer colour_data(width * height * 4u);
    auto *cursor = colour_data.data();

    for (auto y = 0u; y < height; ++y)
    {
        const auto row_colour = iris::Colour::lerp(start, end, static_cast<float>(y) / static_cast<float>(height));

        const std::byte colour_bytes[] = {
            static_cast<std::byte>(row_colour.r * 0xff),
            static_cast<std::byte>(row_colour.g * 0xff),
            static_cast<std::byte>(row_colour.b * 0xff),
            static_cast<std::byte>(row_colour.a * 0xff)};

        for (auto x = 0u; x < width; ++x)
        {
            std::memcpy(cursor, colour_bytes, sizeof(colour_bytes));
            cursor += 4u;
        }
    }

    return colour_data;
}

}

namespace iris
{

Texture *TextureManager::load(const std::string &resource, TextureUsage usage)
{
    expect((usage == TextureUsage::IMAGE) || (usage == TextureUsage::DATA), "can only load IMAGE or DATA from file");

    // check if texture has been loaded before, if not then load it
    if (!loaded_textures_.contains(resource))
    {
        const auto file_data = ResourceLoader::instance().load(resource);
        auto [data, width, height] = parse_image(file_data);

        auto texture = do_create(data, width, height, usage);

        loaded_textures_[resource] = {1u, std::move(texture)};
    }
    else
    {
        ++loaded_textures_[resource].ref_count;
    }

    return loaded_textures_[resource].asset.get();
}

CubeMap *TextureManager::load(
    const std::string &right_resource,
    const std::string &left_resource,
    const std::string &top_resource,
    const std::string &bottom_resource,
    const std::string &back_resource,
    const std::string &front_resource)
{
    std::stringstream strm{};
    strm << right_resource << left_resource << top_resource << bottom_resource << back_resource << front_resource;

    const auto resource = strm.str();

    if (!loaded_cube_maps_.contains(resource))
    {
        const std::vector<std::tuple<iris::DataBuffer, std::uint32_t, std::uint32_t>> parsed_sides = {
            parse_image(ResourceLoader::instance().load(right_resource), false),
            parse_image(ResourceLoader::instance().load(left_resource), false),
            parse_image(ResourceLoader::instance().load(top_resource), false),
            parse_image(ResourceLoader::instance().load(bottom_resource), false),
            parse_image(ResourceLoader::instance().load(back_resource), false),
            parse_image(ResourceLoader::instance().load(front_resource), false)};

        const auto width = std::get<1>(parsed_sides.front());
        const auto height = std::get<2>(parsed_sides.front());

        ensure(
            std::all_of(
                std::cbegin(parsed_sides) + 1u,
                std::cend(parsed_sides),
                [width, height](const auto &side)
                { return (std::get<1>(side) == width) && (std::get<2>(side) == height); }),
            "cube map images must all have the same dimensions");

        auto cube_map = do_create(
            std::get<0>(parsed_sides[0]),
            std::get<0>(parsed_sides[1]),
            std::get<0>(parsed_sides[2]),
            std::get<0>(parsed_sides[3]),
            std::get<0>(parsed_sides[4]),
            std::get<0>(parsed_sides[5]),
            width,
            height);

        loaded_cube_maps_[resource] = {1u, std::move(cube_map)};
    }
    else
    {
        ++loaded_cube_maps_[resource].ref_count;
    }

    return loaded_cube_maps_[resource].asset.get();
}

Texture *TextureManager::create(const DataBuffer &data, std::uint32_t width, std::uint32_t height, TextureUsage usage)
{
    static std::uint32_t counter = 0u;

    // create a unique name for the in-memory texture
    std::stringstream strm;
    strm << "!" << counter;
    ++counter;

    const auto resource = strm.str();

    auto texture = do_create(data, width, height, usage);

    loaded_textures_[resource] = {1u, std::move(texture)};

    return loaded_textures_[resource].asset.get();
}

CubeMap *TextureManager::create(
    const DataBuffer &right_data,
    const DataBuffer &left_data,
    const DataBuffer &top_data,
    const DataBuffer &bottom_data,
    const DataBuffer &near_data,
    const DataBuffer &far_data,
    std::uint32_t width,
    std::uint32_t height)
{
    static std::uint32_t counter = 0u;

    // create a unique name for the in-memory texture
    std::stringstream strm;
    strm << "!" << counter;
    ++counter;

    const auto resource = strm.str();

    auto cube_map = do_create(right_data, left_data, top_data, bottom_data, near_data, far_data, width, height);

    loaded_cube_maps_[resource] = {1u, std::move(cube_map)};

    return loaded_cube_maps_[resource].asset.get();
}

CubeMap *TextureManager::create(const Colour &start, const Colour &end, std::uint32_t width, std::uint32_t height)
{
    const auto top = create_texture_data(start, width, height);
    const auto bottom = create_texture_data(end, width, height);
    const auto side = create_texture_data(start, end, width, height);

    return create(side, side, top, bottom, side, side, width, height);
}

void TextureManager::unload(Texture *texture)
{
    // allow for implementation specific unloading logic
    destroy(texture);

    // don't unload the static blank texture!
    if (texture != blank())
    {
        // find the texture that we want to unload
        auto loaded = std::find_if(
            std::begin(loaded_textures_),
            std::end(loaded_textures_),
            [texture](const auto &element) { return element.second.asset.get() == texture; });

        expect(loaded != std::cend(loaded_textures_), "texture has not been loaded");

        // decrement reference count and, if 0, unload
        --loaded->second.ref_count;
        if (loaded->second.ref_count == 0u)
        {
            loaded_textures_.erase(loaded);
        }
    }
}

void TextureManager::unload(CubeMap *cube_map)
{
    // allow for implementation specific unloading logic
    destroy(cube_map);

    // find the texture that we want to unload
    auto loaded = std::find_if(
        std::begin(loaded_cube_maps_),
        std::end(loaded_cube_maps_),
        [cube_map](const auto &element) { return element.second.asset.get() == cube_map; });

    expect(loaded != std::cend(loaded_cube_maps_), "cube_map has not been loaded");

    // decrement reference count and, if 0, unload
    --loaded->second.ref_count;
    if (loaded->second.ref_count == 0u)
    {
        loaded_cube_maps_.erase(loaded);
    }
}

Texture *TextureManager::blank()
{
    static Texture *texture =
        create({std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff}}, 1u, 1u, TextureUsage::IMAGE);

    return texture;
}

void TextureManager::destroy(Texture *)
{
    // by default do nothing
}

void TextureManager::destroy(CubeMap *)
{
    // by default do nothing
}

}
