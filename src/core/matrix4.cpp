#include "core/matrix4.h"

#include <cmath>
#include <iostream>

#include "core/quaternion.h"
#include "core/real.h"
#include "core/vector3.h"

namespace eng
{

Matrix4::Matrix4()
    : elements_({{ 1.0f, 0.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 0.0f, 0.0f,
                   0.0f, 0.0f, 1.0f, 0.0f,
                   0.0f, 0.0f, 0.0f, 1.0f  }})
{ }

Matrix4::Matrix4(const std::array<real, 16> &elements)
    : elements_(elements)
{ }

Matrix4::Matrix4(const Quaternion &q)
    : Matrix4()
{
    elements_[0] = 1.0f - 2.0f * q.y * q.y - 2.0f * q.z * q.z;
    elements_[1] = 2.0f * q.x * q.y - 2.0f * q.z * q.w;
    elements_[2] = 2.0f * q.x * q.z + 2.0f * q.y * q.w;

    elements_[4] = 2.0f * q.x * q.y + 2.0f * q.z * q.w;
    elements_[5] = 1.0f - 2.0f * q.x * q.x  - 2.0f * q.z * q.z;
    elements_[6] = 2.0f * q.y * q.z - 2.0f * q.x * q.w;

    elements_[8] = 2.0f * q.x * q.z - 2.0f * q.y * q.w;
    elements_[9] = 2.0f * q.y * q.z + 2.0f * q.x * q.w;
    elements_[10] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.y * q.y;
}

Matrix4::Matrix4(const Quaternion &q, const Vector3 &p)
    : Matrix4(q)
{
    elements_[3u] = p.x;
    elements_[7u] = p.y;
    elements_[11u] = p.z;
}

Matrix4 Matrix4::make_orthographic_projection(real width, real height, real depth)
{
    Matrix4 m;
    
    const auto right = width;
    const auto left = -right;
    const auto top = height;
    const auto bottom = -top;
    const auto far = depth;
    const auto near = -far;

    m.elements_ = {{
        2.0f / (right - left), 0.0f, 0.0f, -(right + left) / (right - left),
        0.0f, 2.0f / (top - bottom), 0.0f, -(top + bottom) / (top - bottom),
        0.0f, 0.0f, -2.0f / (far - near), -(far + near) / (far - near),
        0.0f, 0.0f, 0.0f, 1.0f
    }};

    return m;
}

Matrix4 Matrix4::make_perspective_projection(
    real fov,
    real width,
    real height,
    real near,
    real far)
{
    Matrix4 m;

    const auto aspect_ratio = width / height;
    const auto t = std::tan(fov / 2.0f) * near;
    const auto b = -t;
    const auto r = t * aspect_ratio;
    const auto l = b * aspect_ratio;

    m.elements_ = {{
        (2.0f * near) / (r - l), 0.0f, (r + l) / (r - l), 0.0f,
        0.0f, (2.0f * near) / (t - b), (t + b) / (t - b), 0.0f,
        0.0f, 0.0f, -(far + near) / (far - near), -(2.0f * far * near) / (far - near),
        0.0f, 0.0f, -1.0f, 0.0f
    }};

    return m;
}

Matrix4 Matrix4::make_look_at(
    const Vector3 &eye,
    const Vector3 &look_at,
    const Vector3 &up)
{
    const auto f = Vector3::normalise(look_at - eye);
    const auto up_normalised = Vector3::normalise(up);

    const auto s = Vector3::cross(f, up_normalised).normalise();
    const auto u = Vector3::cross(s, f).normalise();

    Matrix4 m;

    m.elements_ = {{
        s.x, s.y, s.z, 0.0f,
        u.x, u.y, u.z, 0.0f,
        -f.x, -f.y, -f.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }};

    return m * make_translate(-eye);
}

Matrix4 Matrix4::make_scale(const Vector3 &scale)
{
    Matrix4 m;

    m.elements_ =
    {{
         scale.x, 0.0f, 0.0f, 0.0f,
         0.0f, scale.y, 0.0f, 0.0f,
         0.0f, 0.0f, scale.z, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f
    }};

    return m;
}

Matrix4 Matrix4::make_translate(const Vector3 &translate)
{
    Matrix4 m;

    m.elements_ =
    {{
         1.0f, 0.0f, 0.0f, translate.x,
         0.0f, 1.0f, 0.0f, translate.y,
         0.0f, 0.0f, 1.0f, translate.z,
         0.0f, 0.0f, 0.0f, 1.0f
    }};

    return m;
}

Matrix4 Matrix4::invert(const Matrix4 &m)
{
    Matrix4 inv{ };

    inv[0] = m[5]  * m[10] * m[15] -
             m[5]  * m[11] * m[14] -
             m[9]  * m[6]  * m[15] +
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] -
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] +
              m[4]  * m[11] * m[14] +
              m[8]  * m[6]  * m[15] -
              m[8]  * m[7]  * m[14] -
              m[12] * m[6]  * m[11] +
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] -
             m[4]  * m[11] * m[13] -
             m[8]  * m[5] * m[15] +
             m[8]  * m[7] * m[13] +
             m[12] * m[5] * m[11] -
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] +
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] -
               m[8]  * m[6] * m[13] -
               m[12] * m[5] * m[10] +
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] +
              m[1]  * m[11] * m[14] +
              m[9]  * m[2] * m[15] -
              m[9]  * m[3] * m[14] -
              m[13] * m[2] * m[11] +
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] -
             m[0]  * m[11] * m[14] -
             m[8]  * m[2] * m[15] +
             m[8]  * m[3] * m[14] +
             m[12] * m[2] * m[11] -
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] +
              m[0]  * m[11] * m[13] +
              m[8]  * m[1] * m[15] -
              m[8]  * m[3] * m[13] -
              m[12] * m[1] * m[11] +
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] -
              m[0]  * m[10] * m[13] -
              m[8]  * m[1] * m[14] +
              m[8]  * m[2] * m[13] +
              m[12] * m[1] * m[10] -
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] -
             m[1]  * m[7] * m[14] -
             m[5]  * m[2] * m[15] +
             m[5]  * m[3] * m[14] +
             m[13] * m[2] * m[7] -
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] +
              m[0]  * m[7] * m[14] +
              m[4]  * m[2] * m[15] -
              m[4]  * m[3] * m[14] -
              m[12] * m[2] * m[7] +
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] -
              m[0]  * m[7] * m[13] -
              m[4]  * m[1] * m[15] +
              m[4]  * m[3] * m[13] +
              m[12] * m[1] * m[7] -
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] +
               m[0]  * m[6] * m[13] +
               m[4]  * m[1] * m[14] -
               m[4]  * m[2] * m[13] -
               m[12] * m[1] * m[6] +
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] +
              m[1] * m[7] * m[10] +
              m[5] * m[2] * m[11] -
              m[5] * m[3] * m[10] -
              m[9] * m[2] * m[7] +
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] -
             m[0] * m[7] * m[10] -
             m[4] * m[2] * m[11] +
             m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] -
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] +
               m[0] * m[7] * m[9] +
               m[4] * m[1] * m[11] -
               m[4] * m[3] * m[9] -
               m[8] * m[1] * m[7] +
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] -
              m[0] * m[6] * m[9] -
              m[4] * m[1] * m[10] +
              m[4] * m[2] * m[9] +
              m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    auto det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if(det != 0.0f)
    {
        det = 1.0 / det;

        for(auto i = 0; i < 16; i++)
        {
            inv[i] *= det;
        }
    }

    return inv;
}

Matrix4 Matrix4::transpose(const Matrix4 &matrix)
{
    auto m{ matrix };

    std::swap(m[1], m[4]);
    std::swap(m[2], m[8]);
    std::swap(m[3], m[12]);
    std::swap(m[6], m[9]);
    std::swap(m[7], m[13]);
    std::swap(m[11], m[14]);

    return m;
}

Matrix4& Matrix4::operator*=(const Matrix4 &matrix)
{
    const auto e = elements_;

    const auto calculate_cell = [&e, &matrix](
        std::size_t row_num,
        std::size_t col_num)
    {
        return (e[row_num + 0u] * matrix[col_num +  0u]) +
               (e[row_num + 1u] * matrix[col_num +  4u]) +
               (e[row_num + 2u] * matrix[col_num +  8u]) +
               (e[row_num + 3u] * matrix[col_num + 12u]);
    };

    elements_[0u] = calculate_cell(0u, 0u);
    elements_[1u] = calculate_cell(0u, 1u);
    elements_[2u] = calculate_cell(0u, 2u);
    elements_[3u] = calculate_cell(0u, 3u);

    elements_[4u] = calculate_cell(4u, 0u);
    elements_[5u] = calculate_cell(4u, 1u);
    elements_[6u] = calculate_cell(4u, 2u);
    elements_[7u] = calculate_cell(4u, 3u);

    elements_[8u]  = calculate_cell(8u, 0u);
    elements_[9u]  = calculate_cell(8u, 1u);
    elements_[10u] = calculate_cell(8u, 2u);
    elements_[11u] = calculate_cell(8u, 3u);

    elements_[12u] = calculate_cell(12u, 0u);
    elements_[13u] = calculate_cell(12u, 1u);
    elements_[14u] = calculate_cell(12u, 2u);
    elements_[15u] = calculate_cell(12u, 3u);

    return *this;
}

Matrix4 Matrix4::operator*(const Matrix4 &matrix) const
{
    return Matrix4(*this) *= matrix;
}

Vector3 Matrix4::operator*(const Vector3 &vector) const
{
    return {
        vector.x * elements_[0] +
        vector.y * elements_[1] +
        vector.z * elements_[2] +
        elements_[3],

        vector.x * elements_[4] +
        vector.y * elements_[5] +
        vector.z * elements_[6] +
        elements_[7],

        vector.x * elements_[8] +
        vector.y * elements_[9] +
        vector.z * elements_[10] +
        elements_[11],
    };
}

real& Matrix4::operator[](const std::size_t index)
{
    return elements_[index];
}

real Matrix4::operator[](const std::size_t index) const
{
    return elements_[index];
}

bool Matrix4::operator==(const Matrix4 &other) const
{
    return (elements_[0] == other[0]) &&
           (elements_[1] == other[1]) &&
           (elements_[2] == other[2]) &&
           (elements_[3] == other[3]) &&
           (elements_[4] == other[4]) &&
           (elements_[5] == other[5]) &&
           (elements_[6] == other[6]) &&
           (elements_[7] == other[7]) &&
           (elements_[8] == other[8]) &&
           (elements_[9] == other[9]) &&
           (elements_[10] == other[10]) &&
           (elements_[11] == other[11]) &&
           (elements_[12] == other[12]) &&
           (elements_[13] == other[13]) &&
           (elements_[14] == other[14]) &&
           (elements_[15] == other[15]);
}

bool Matrix4::operator!=(const Matrix4 &other) const
{
    return !(*this == other);
}

const real* Matrix4::data() const
{
    return elements_.data();
}

Vector3 Matrix4::column(const std::size_t index) const
{
    return{ elements_[index], elements_[index + 4u], elements_[index + 8u] };
}

std::ostream& operator<<(std::ostream &out, const Matrix4 &m)
{
    out << m[0] << " " << m[1] << " " <<  m[2] << " " << m[3] << std::endl;
    out << m[4] << " " << m[5] << " " <<  m[6] << " " << m[7] << std::endl;
    out << m[8] << " " << m[9] << " " <<  m[10] << " " << m[11] << std::endl;
    out << m[12] << " " << m[13] << " " <<  m[14] << " " << m[15];

    return out;
}

}

