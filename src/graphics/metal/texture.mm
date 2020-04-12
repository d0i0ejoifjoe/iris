#include "texture.hpp"

#include <any>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#import <Metal/Metal.h>

#include "exception.hpp"
#include "utility.hpp"
#include "log.hpp"

namespace
{

/**
 * Helper method to convert number of channels into a metal type.
 *
 * @param num_channels
 *   The number of channels in an image.
 *
 * @returns
 *   A metal type representing the number of channels.
 */
MTLPixelFormat channels_to_format(const std::uint32_t num_channels)
{
    auto format = MTLPixelFormatR8Unorm;

    switch(num_channels)
    {
        case 1:
            format = MTLPixelFormatR8Unorm;
            break;
        case 4:
            format = MTLPixelFormatRGBA8Unorm;
            break;
        default:
            throw eng::exception("incorrect number of channels");
    }

    return format;
}

/**
 * Helper function to create an metal texture from data.
 *
 * @param data
 *   Raw data of image.
 *
 * @param width
 *   Width of image.
 *
 * @param height
 *   Height of image.
 *
 * @param num_channels
 *   Number of channels.
 *
 * @returns
 *   Handle to texture.
 */
id<MTLTexture> create_texture(
    const std::vector<std::uint8_t> &data,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t num_channels)
{
    // sanity check we have enough data
    const auto expected_size = width * height * num_channels;
    if(data.size() != expected_size)
    {
        throw eng::exception("incorrect data size");
    }

    const auto format = channels_to_format(num_channels);

    // create metal texture descriptor
    auto *texture_descriptor =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format
                                                           width:width
                                                          height:height
                                                       mipmapped:YES];

    // get metal device handle
    static const auto *device =
        ::CGDirectDisplayCopyCurrentMetalDevice(::CGMainDisplayID());

    // create new texture
    auto texture = [device newTextureWithDescriptor:texture_descriptor];

    auto region = MTLRegionMake2D(0, 0, width, height);
    const auto bytes_per_row = width * num_channels;

    // set image data for texture
    [texture replaceRegion:region
               mipmapLevel:0
                 withBytes:data.data()
               bytesPerRow:bytes_per_row];

    return texture;
}

}

namespace eng
{

/**
 * Struct containing implementation specific data.
 */
struct texture::implementation final
{
    /** Simple constructor which takes a value for each member. */
    implementation(id<MTLTexture> texture)
        : texture(texture)
    { }

    /** Default */
    implementation() = default;
    ~implementation() = default;
    implementation(const implementation&) = default;
    implementation& operator=(const implementation&) = default;
    implementation(implementation&&) = default;
    implementation& operator=(implementation&&) = default;

    /** Metal handle for texture. */
    id<MTLTexture> texture;
};

texture::texture(const std::filesystem::path &path)
    : data_(),
      width_(0u),
      height_(0u),
      num_channels_(0u),
      impl_(nullptr)
{
    const auto [data, width, height, num_channels] =
        graphics::utility::load_image(path);

    const auto texture = create_texture(data, width, height, num_channels);
    impl_ = std::make_unique<implementation>(texture);

    LOG_ENGINE_INFO("texture", "loaded from file");
}


texture::texture(
    const std::vector<std::uint8_t> &data,
    const std::uint32_t width,
    const std::uint32_t height,
    const std::uint32_t num_channels)
    : data_(data),
      width_(width),
      height_(height),
      num_channels_(num_channels),
      impl_(nullptr)
{
    const auto texture = create_texture(data, width, height, num_channels);
    impl_ = std::make_unique<implementation>(texture);

    LOG_ENGINE_INFO("texture", "loaded from data");
}

/** Default. */
texture::~texture() = default;
texture::texture(texture&&) = default;
texture& texture::operator=(texture&&) = default;

std::vector<std::uint8_t> texture::data() const noexcept
{
    return data_;
}

std::uint32_t texture::width() const noexcept
{
    return width_;
}

std::uint32_t texture::height() const noexcept
{
    return height_;
}

std::uint32_t texture::num_channels() const noexcept
{
    return num_channels_;
}

std::any texture::native_handle() const
{
    return impl_->texture;
}

texture texture::blank()
{
    return{ { 0xFF, 0xFF, 0xFF, 0xFF }, 1u, 1u, 4u };
}

}

