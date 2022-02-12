////////////////////////////////////////////////////////////////////////////////
//         Distributed under the Boost Software License, Version 1.0.         //
//            (See accompanying file LICENSE or copy at                       //
//                 https://www.boost.org/LICENSE_1_0.txt)                     //
////////////////////////////////////////////////////////////////////////////////

#include "graphics/render_graph/conditional_node.h"

#include <cstddef>

#include "graphics/render_graph/node.h"
#include "graphics/render_graph/shader_compiler.h"

namespace iris
{

ConditionalNode::ConditionalNode(
    Node *input_value1,
    Node *input_value2,
    Node *output_value1,
    Node *output_value2,
    ConditionalOperator conditional_operator)
    : input_value1_(input_value1)
    , input_value2_(input_value2)
    , output_value1_(output_value1)
    , output_value2_(output_value2)
    , conditional_operator_(conditional_operator)
{
}

void ConditionalNode::accept(ShaderCompiler &compiler) const
{
    compiler.visit(*this);
}

Node *ConditionalNode::input_value1() const
{
    return input_value1_;
}

Node *ConditionalNode::input_value2() const
{
    return input_value2_;
}

Node *ConditionalNode::output_value1() const
{
    return output_value1_;
}

Node *ConditionalNode::output_value2() const
{
    return output_value2_;
}

ConditionalOperator ConditionalNode::conditional_operator() const
{
    return conditional_operator_;
}

std::size_t ConditionalNode::hash() const
{
    return combine_hash(
        input_value1_, input_value2_, output_value1_, output_value2_, conditional_operator_, "conditional_node");
}

}
