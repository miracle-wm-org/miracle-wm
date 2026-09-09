/**
Copyright (C) 2025  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef MIRACLE_STUB_RENDERABLE_H
#define MIRACLE_STUB_RENDERABLE_H

#include <mir/graphics/buffer.h>
#include <mir/graphics/renderable.h>
#include <mir/renderer/sw/pixel_source.h>

namespace miracle
{
namespace test
{

    /// The bare minimum buffer needed to tessellate: everything but the size is
    /// unreachable from the geometry under test.
    class StubBuffer : public mir::graphics::Buffer
    {
    public:
        explicit StubBuffer(mir::geometry::Size size) :
            size_ { size }
        {
        }

        auto id() const -> mir::graphics::BufferID override { return mir::graphics::BufferID { 1 }; }
        auto size() const -> mir::geometry::Size override { return size_; }
        auto pixel_format() const -> MirPixelFormat override { return mir_pixel_format_argb_8888; }
        auto native_buffer_base() -> mir::graphics::NativeBufferBase* override { return nullptr; }
        auto map_readable() const -> std::unique_ptr<mir::renderer::software::Mapping<std::byte const>> override
        {
            return nullptr;
        }

    private:
        mir::geometry::Size size_;
    };

    /// A renderable with just enough shape for the tessellation geometry: where it
    /// sits on screen, how big its buffer is, and which part of that buffer it draws.
    class StubRenderable : public mir::graphics::Renderable
    {
    public:
        StubRenderable(mir::geometry::Rectangle position, mir::geometry::Size buffer_size) :
            position_ {
                position
        },
            buffer_ { std::make_shared<StubBuffer>(buffer_size) },
            src_bounds_ { { 0, 0 }, { buffer_size.width.as_value(), buffer_size.height.as_value() } }
        {
        }

        /// A renderable that draws only part of its buffer. Clients that submit a
        /// buffer bigger than the region they actually draw need this to be separable
        /// from the screen position.
        StubRenderable(
            mir::geometry::Rectangle position,
            mir::geometry::Size buffer_size,
            mir::geometry::RectangleD src_bounds) :
            position_ { position },
            buffer_ { std::make_shared<StubBuffer>(buffer_size) },
            src_bounds_ { src_bounds }
        {
        }

        ID id() const override { return this; }
        std::shared_ptr<mir::graphics::Buffer> buffer() const override { return buffer_; }
        mir::geometry::Rectangle screen_position() const override { return position_; }
        mir::geometry::RectangleD src_bounds() const override { return src_bounds_; }
        std::optional<mir::geometry::Rectangle> clip_area() const override { return std::nullopt; }
        float alpha() const override { return 1.f; }
        glm::mat4 transformation() const override { return glm::mat4(1.f); }
        MirOrientation orientation() const override { return mir_orientation_normal; }
        MirMirrorMode mirror_mode() const override { return mir_mirror_mode_none; }
        bool shaped() const override { return false; }
        auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override { return std::nullopt; }
        auto opaque_region() const -> std::optional<mir::geometry::Rectangles> override { return std::nullopt; }

    private:
        mir::geometry::Rectangle position_;
        std::shared_ptr<StubBuffer> buffer_;
        mir::geometry::RectangleD src_bounds_;
    };

} // namespace test
} // namespace miracle

#endif // MIRACLE_STUB_RENDERABLE_H
