////////////////////////////////////////////////////////////////////////////////
//         Distributed under the Boost Software License, Version 1.0.         //
//            (See accompanying file LICENSE or copy at                       //
//                 https://www.boost.org/LICENSE_1_0.txt)                     //
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <memory>

#include "graphics/render_graph/node.h"

namespace iris
{
class ShaderCompiler;

/**
 * Implementation of Node which calculates the sine of the input node.
 */
class SinNode : public Node
{
  public:
    /**
     * Create a new SinNode.
     *
     * @param input_node
     *   Node to sine.
     */
    SinNode(Node *input_node);

    ~SinNode() override = default;

    /**
     * Accept a compiler visitor.
     *
     * @param compiler
     *   Compiler to accept.
     */
    void accept(ShaderCompiler &compiler) const override;

    /**
     * Get input node.
     *
     * @returns
     *   Input node.
     */
    Node *input_node() const;

    /**
     * Compute hash of node.
     *
     * @return
     *   Hash of node.
     */
    std::size_t hash() const override;

  private:
    /** Input node. */
    Node *input_node_;
};
}
