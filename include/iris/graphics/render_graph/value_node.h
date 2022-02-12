////////////////////////////////////////////////////////////////////////////////
//         Distributed under the Boost Software License, Version 1.0.         //
//            (See accompanying file LICENSE or copy at                       //
//                 https://www.boost.org/LICENSE_1_0.txt)                     //
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <string_view>
#include <typeindex>
#include <typeinfo>

#include "graphics/render_graph/node.h"
#include "graphics/render_graph/shader_compiler.h"

namespace iris
{
/**
 * Implementation of Node which provides access to a constant value. See
 * compiler.h for supported types.
 */
template <class T>
class ValueNode : public Node
{
  public:
    /**
     * Create a new ValueNode.
     *
     * @param value
     *   Value to provide access to.
     */
    ValueNode(const T &value)
        : value_(value)
    {
    }

    ~ValueNode() override = default;

    /**
     * Accept a compiler visitor.
     *
     * @param compiler
     *   Compiler to accept.
     */
    void accept(ShaderCompiler &compiler) const override
    {
        return compiler.visit(*this);
    }

    /**
     * Get value.
     *
     * @returns
     *   Value.
     */
    T value() const
    {
        return value_;
    }

    /**
     * Compute hash of node.
     *
     * @return
     *   Hash of node.
     */
    std::size_t hash() const override
    {
        return combine_hash(value_, std::string_view{"value_node"}, std::type_index(typeid(T)));
    }

  private:
    /** Value. */
    T value_;
};
}
