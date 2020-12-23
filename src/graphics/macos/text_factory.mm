#include "graphics/text_factory.h"

#include <cmath>
#include <memory>
#include <vector>

#import <CoreFoundation/CoreFoundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>

#include "core/exception.h"
#include "graphics/material.h"
#include "graphics/mesh_factory.h"
#include "graphics/render_entity.h"
#include "graphics/render_graph/render_graph.h"
#include "graphics/render_graph/texture_node.h"
#include "graphics/scene.h"
#include "graphics/texture.h"
#include "graphics/texture_factory.h"
#include "log/log.h"
#include "platform/macos/cf_ptr.h"
#include "platform/macos/macos_ios_utility.h"

namespace iris::text_factory
{

std::unique_ptr<Scene> create(
    const std::string &font_name,
    const std::uint32_t size,
    const std::string &text,
    const Vector3 &colour)
{
    // create a CoreFoundation string object from supplied Font name
    const auto font_name_cf = CfPtr<CFStringRef>(::CFStringCreateWithCString(
        kCFAllocatorDefault,
        font_name.c_str(),
        kCFStringEncodingASCII));

    if(!font_name_cf)
    {
        throw Exception("failed to create CF string");
    }

    // create Font object
    CfPtr<CTFontRef> font = CfPtr<CTFontRef>(::CTFontCreateWithName(
        font_name_cf.get(),
        size,
        nullptr));

    if(!font)
    {
        throw Exception("failed to create font");
    }

    // create a device dependant colour space
    CfPtr<CGColorSpaceRef> colour_space = CfPtr<CGColorSpaceRef>(::CGColorSpaceCreateDeviceRGB());

    if(!colour_space)
    {
        throw Exception("failed to create colour space");
    }

    // create a CoreFoundation colour object from supplied colour
    const CGFloat components[] = { colour.x, colour.y, colour.z, 1.0f };
    CfPtr<CGColorRef> font_colour = CfPtr<CGColorRef>(::CGColorCreate(
        colour_space.get(),
        components));

    if(!font_colour)
    {
        throw Exception("failed to create colour");
    }

    std::array<CFStringRef, 2> keys = {{ kCTFontAttributeName, kCTForegroundColorAttributeName }};
    std::array<CFTypeRef, 2> values = {{ font.get(), font_colour.get() }};

    // create string attributes dictionary, containing Font name and colour
    CfPtr<CFDictionaryRef> attributes = CfPtr<CFDictionaryRef>(::CFDictionaryCreate(
        kCFAllocatorDefault,
        reinterpret_cast<const void**>(keys.data()),
        reinterpret_cast<const void**>(values.data()),
        keys.size(),
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks));

    if(!attributes)
    {
        throw Exception("failed to create attributes");
    }

    LOG_DEBUG("font", "creating sprites for string: {}", text);

    // create CoreFoundation string object from supplied text
    const auto text_cf = CfPtr<CFStringRef>(::CFStringCreateWithCString(
        kCFAllocatorDefault,
        text.c_str(),
        kCFStringEncodingASCII));

    // create a CoreFoundation attributed string object
    const auto attr_string = CfPtr<CFAttributedStringRef>(::CFAttributedStringCreate(
        kCFAllocatorDefault,
        text_cf.get(),
        attributes.get()));

    const auto frame_setter = CfPtr<CTFramesetterRef>(
        ::CTFramesetterCreateWithAttributedString(attr_string.get()));

    // calculate minimal size required to render text
    CFRange range;
    const auto rect = ::CTFramesetterSuggestFrameSizeWithConstraints(
        frame_setter.get(),
        CFRangeMake(0, 0),
        nullptr,
        CGSizeMake(CGFLOAT_MAX, CGFLOAT_MAX),
        &range);

    // create a path object to render text
    const auto path = CfPtr<CGPathRef>(::CGPathCreateWithRect(
        CGRectMake(0, 0, std::ceil(rect.width), std::ceil(rect.height)),
        nullptr));

    if(!path)
    {
        throw Exception("failed to create path");
    }

    // create a frame to render text
    const auto frame = CfPtr<CTFrameRef>(::CTFramesetterCreateFrame(
        frame_setter.get(),
        range,
        path.get(),
        nullptr));

    if(!frame)
    {
        throw Exception("failed to create frame");
    }

    // round suggested width and height down to integers, multiply by two for
    // retina displays
    const auto width = static_cast<std::uint32_t>(rect.width) * 2u;
    const auto height = static_cast<std::uint32_t>(rect.height) * 2u;

    // allocate enough space to store RGBA tuples for each pixel
    std::vector<std::uint8_t> pixel_data(width * height * 4);

    const auto bits_per_pixel = 8u;
    const auto bytes_per_row = width * 4u;

    // create a context for rendering text
    const auto context = CfPtr<CGContextRef>(::CGBitmapContextCreateWithData(
        pixel_data.data(),
        width,
        height,
        bits_per_pixel,
        bytes_per_row,
        colour_space.get(),
        kCGImageAlphaPremultipliedLast,
        nullptr,
        nullptr));

    if(!context)
    {
        throw Exception("failed to create context");
    }

    const auto scale = platform::utility::screen_scale();
    LOG_ENGINE_DEBUG("font", "{}", scale);

    // ensure letters are rotated correct way and scaled for screen
    ::CGContextTranslateCTM(context.get(), 0.0f, rect.height * scale);
    ::CGContextScaleCTM(context.get(), scale, -scale);

    // render text, pixel data will be stored in our pixel data buffer
    ::CTFrameDraw(frame.get(), context.get());
    ::CGContextFlush(context.get());

    // create a Texture from the rendered pixel data
    auto *texture = texture_factory::create(pixel_data, width, height, PixelFormat::RGBA);

    RenderGraph render_graph{};
    auto *texture_node = render_graph.create<TextureNode>(texture);
    render_graph.render_node()->set_colour_input(texture_node);

    auto scene = std::make_unique<Scene>();
    scene->create_entity(
        std::move(render_graph),
        mesh_factory::sprite({1.0f}),
        Vector3{1.0f},
        Vector3{ static_cast<float>(width), static_cast<float>(height), 1.0f });

    return scene;
}

}

