#pragma once

#include <memory>
#include <vector>

#include "core/camera_type.h"
#include "core/matrix4.h"
#include "core/quaternion.h"
#include "core/transform.h"
#include "graphics/material.h"
#include "graphics/mesh.h"
#include "graphics/primitive_type.h"
#include "graphics/render_graph/render_node.h"
#include "graphics/skeleton.h"
#include "graphics/texture.h"

namespace iris
{

/**
 * A renderable entity.
 */
class RenderEntity
{
  public:
    /**
     * Construct a RenderEntity.
     *
     * @param mesh
     *   Mesh to render.
     *
     * @param position
     *   Centre of mesh in world space.
     */
    RenderEntity(Mesh mesh, const Vector3 &position);

    /**
     * Construct a RenderEntity.
     *
     * @param mesh
     *   Mesh to render.
     *
     * @param transform
     *   Transform of entity in world space.
     */
    RenderEntity(Mesh mesh, const Transform &transform);

    /**
     * Construct a RenderEntity.
     *
     * @param mesh
     *   Mesh to render.
     *
     * @param transform
     *   Transform of entity in world space.
     *
     * @param skeleton
     *   Skeleton.
     */
    RenderEntity(Mesh mesh, const Transform &transform, Skeleton skeleton);

    /**
     * Construct a RenderEntity.
     *
     * @param meshes
     *   Collection of meshes to render.
     *
     * @param transform
     *   Transform of entity in world space.
     *
     * @param skeleton
     *   Skeleton.
     */
    RenderEntity(
        std::vector<Mesh> meshes,
        const Transform &transform,
        Skeleton skeleton);

    RenderEntity(const RenderEntity &) = delete;
    RenderEntity &operator=(const RenderEntity &) = delete;
    RenderEntity(RenderEntity &&) = default;
    RenderEntity &operator=(RenderEntity &&) = default;

    /**
     * Get position.
     *
     * @returns
     *   Position.
     */
    Vector3 position() const;

    /**
     * Set the position of the RenderEntity.
     *
     * @param position
     *   New position.
     */
    void set_position(const Vector3 &position);

    /**
     * Get orientation.
     *
     * @return
     *   Orientation.
     */
    Quaternion orientation() const;

    /**
     * Set the orientation of the RenderEntity.
     *
     * @param orientation
     *   New orientation.
     */
    void set_orientation(const Quaternion &orientation);

    /**
     * Set the scale of the RenderEntity.
     *
     * @param scale
     *   New scale.
     */
    void set_scale(const Vector3 &scale);

    /**
     * Get the transformation matrix of the RenderEntity.
     *
     * @returns
     *   Transformation matrix.
     */
    Matrix4 transform() const;

    /**
     * Set transformation matrix.
     *
     * @param transform
     *   New transform matrix.
     */
    void set_transform(const Matrix4 &transform);

    /**
     * Get the transformation matrix for the normals of the RenderEntity.
     *
     * @returns
     *   Normal transformation matrix.
     */
    Matrix4 normal_transform() const;

    /**
     * Get all meshes for this entity.
     *
     * @returns
     *   Collection of meshes.
     */
    Mesh *mesh() const;

    void set_mesh(Mesh *mesh);

    /**
     * Returns whether the object should be rendered as a wireframe.
     *
     * @returns
     *   True if should be rendered as a wireframe, false otherwise.
     */
    bool should_render_wireframe() const;

    /**
     * Sets whether the object should be rendered as a wireframe.
     *
     * @param wrireframe
     *   True if should be rendered as a wireframe, false otherwise.
     */
    void set_wireframe(const bool wireframe);

    /**
     * Get primitive type.
     *
     * @returns
     *   Primitive type.
     */
    PrimitiveType primitive_type() const;

    /**
     * Set primitive type.
     *
     * @param type
     *   New primitive type.
     */
    void set_primitive_type(PrimitiveType type);

    /**
     * Get reference to skeleton.
     *
     * @returns
     *   Reference to skeleton.
     */
    Skeleton &skeleton();

    /**
     * Can this entity have shadows rendered on it.
     *
     * @returns
     *   True if shadows should be rendered, false otherwise.
     */
    bool receive_shadow() const;

    /**
     * Set whether this object can have shadows rendered on it.
     *
     * @param receive_shadow
     *   New receive shadow option.
     */
    void set_receive_shadow(bool receive_shadow);

  private:
    /** Mesh to render. */
    Mesh *mesh_;

    /** World space transform. */
    Transform transform_;

    /** Normal transformation matrix. */
    Matrix4 normal_;

    /** Whether the object should be rendered as a wireframe. */
    bool wireframe_;

    /** Primitive type. */
    PrimitiveType primitive_type_;

    /** Skeleton. */
    Skeleton skeleton_;

    /** Should object render shadows. */
    bool receive_shadow_;
};

}
