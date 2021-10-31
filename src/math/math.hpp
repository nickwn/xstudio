#pragma once

#include <type_traits>
#include <Eigen/Dense>
#include <Eigen/Geometry>

static inline Eigen::Vector3f operator/ (const Eigen::Vector3f& a, const Eigen::Vector3f& b)
{
    return Eigen::Vector3f(a.x() / b.x(), a.y() / b.y(), a.z() / b.z());
}

namespace xs
{
namespace mth
{

template<typename T> 
concept is_vec3 = requires(T v)
{
    {v.x()} -> std::same_as<float&>;
    {v.y()} -> std::same_as<float&>;
    {v.z()} -> std::same_as<float&>;
};

template<typename T>
concept is_vec4 = requires(T v)
{
    {v.x()} -> std::same_as<float&>;
    {v.y()} -> std::same_as<float&>;
    {v.z()} -> std::same_as<float&>;
    {v.w()} -> std::same_as<float&>;
};

//using mat = Eigen::Matrix<float, 4, 4,;
constexpr float pi = 3.14159265f;
static inline float to_rad(const float deg)
{
    static constexpr float c = pi / 180.f;
    return deg * c;
}

static inline Eigen::Vector3f vec3f_replicate(const float x)
{
    return Eigen::Vector3f(x, x, x);
}

static inline Eigen::Vector3f vec3f_ones()
{
    return Eigen::Vector3f(1.f, 1.f, 1.f);
}

static inline Eigen::Vector3f vec3f_zeros()
{
    return Eigen::Vector3f(0.f, 0.f, 0.f);
}

/*
static inline Eigen::Quaternionf qmul(const Eigen::Quaternionf& q1, const Eigen::Quaternionf& q2)
{
    const float q1x = q1.x(), q1y = q1.y(), q1z = q1.z(), q1w = q1.w();
    const float q2x = q2.x(), q2y = q2.y(), q2z = q2.z(), q2w = q2.w();

    float x = q1x * q2w + q1y * q2z - q1z * q2y + q1w * q2x;
    float y = -q1x * q2z + q1y * q2w + q1z * q2x + q1w * q2y;
    float z = q1x * q2y - q1y * q2x + q1z * q2w + q1w * q2z;
    float w = -q1x * q2x - q1y * q2y - q1z * q2z + q1w * q2w;
    return Eigen::Quaternionf(w, x, y, z);
}

static inline Eigen::Vector3f vqmul(const Eigen::Quaternionf& q, const Eigen::Vector3f& v)
{
    const Eigen::Vector4f qw = Eigen::Vector4f(q.w(), q.w(), q.w(), q.w());
    Eigen::Vector4f t = q.vec().cross(Eigen::Vector4f(v, 0.f));
    t = add(t, t);
    const __m128 vt0 = madd(qw, t, v);
    const __m128 vt1 = cross_fma(q, t);
    const __m128 rotated = add(vt0, vt1);
    return rotated;
}*/

struct alignas(16) transform
{
    Eigen::Vector3f translation;
    Eigen::Quaternionf rotation;

    inline transform() = default;
	inline transform(const Eigen::Vector3f& translation_, const Eigen::Quaternionf& rotation_) : translation(translation_), rotation(rotation_) {}

    transform operator*(const transform& other) const
    {
        transform res;
        const Eigen::Vector3f c = rotation * other.translation;
        res.translation = c + translation;
        res.rotation = rotation * other.rotation;
        return res;
    }

    Eigen::Vector3f operator*(const Eigen::Vector3f& other) const
    {
        const Eigen::Vector3f rotated = rotation * other;
        return rotated + translation;
    }

    const static transform identity;

};

static inline Eigen::Matrix4f look_at(const Eigen::Vector3f& eye, const Eigen::Vector3f& center, const Eigen::Vector3f& up)
{
    const Eigen::Vector3f f = (center - eye).normalized();
    const Eigen::Vector3f s = up.cross(f).normalized();
    const Eigen::Vector3f u = f.cross(s);

    Eigen::Matrix4f out;
    out << s.x(), s.y(), s.z(), -s.dot(eye),
           u.x(), u.y(), u.z(), -u.dot(eye),
           f.x(), f.y(), f.z(), -f.dot(eye),
           0.f,   0.f,   0.f,   1.f;
    return out;
}

static inline Eigen::Matrix4f perspective(const float fov, const float width, const float height, const float z_near, const float z_far)
{
    const float half_fov = fov * 0.5f;
    const float h = std::cos(half_fov) / std::sin(half_fov);
    const float w = h * height / width;

    Eigen::Matrix4f out;
    out << w, 0.f, 0.f, 0.f,
           0.f, h, 0.f, 0.f,
           0.f, 0.f, z_far / (z_far - z_near), -(z_far * z_near) / (z_far - z_near),
           0.f, 0.f, 1.f, 0.f;
    return out;
}

static inline Eigen::Quaternionf to_quat(const float x, const float y, const float z)
{
    /* const float cr = std::cos(x * .5f), sr = std::sin(x * .5f);
    const float cp = std::cos(y * .5f), sp = std::sin(y * .5f);
    const float cy = std::cos(z * .5f), sy = std::sin(z * .5f); */

    Eigen::Quaternionf q;
    /* q.x() = sr * cp * cy - cr * sp * sy;
    q.y() = cr * sp * cy + sr * cp * sy;
    q.z() = cr * cp * sy - sr * sp * cy;
    q.w() = cr * cp * cy + sr * sp * sy;*/
    q = Eigen::AngleAxisf(z, Eigen::Vector3f::UnitZ()) * Eigen::AngleAxisf(y, Eigen::Vector3f::UnitY()) * Eigen::AngleAxisf(x, Eigen::Vector3f::UnitX());
    return q;
    //return Eigen::Quaternionf::Identity();
}

static inline Eigen::Matrix4f to_mat(const transform& transform)
{
    Eigen::Isometry3f out = Eigen::Translation3f(transform.translation) * transform.rotation;

    /* const float qx = transform.rotation.x(), qy = transform.rotation.y(), qz = transform.rotation.z(), qw = transform.rotation.w();
    const Eigen::Vector4f q_2 = transform.rotation.coeffs().cwiseProduct(transform.rotation.coeffs());

    Eigen::Matrix4f out;
    out(0, 0) = 1.f - 2.f * (q_2.y() + q_2.z());
    out(1, 0) = 2.f * (qx * qy + qz * qw);
    out(2, 0) = 2.f * (qx * qz - qy * qw);
    out(3, 0) = 0.f;
    out(0, 1) = 2.f * (qx * qy - qz * qw);
    out(1, 1) = 1.f - 2.f * (q_2.x() + q_2.z());
    out(2, 1) = 2.f * (qy * qz + qx * qw);
    out(3, 1) = 0.f;
    out(0, 2) = 2.f * (qx * qz + qy * qw);
    out(1, 2) = 2.f * (qy * qz - qx * qw);
    out(2, 2) = 1.f - 2.f * (q_2.x() + q_2.y());
    out(3, 2) = 0.f;
    out(0, 3) = transform.translation.x();
    out(1, 3) = transform.translation.y();
    out(2, 3) = transform.translation.z();
    out(3, 3) = 1.f;*/
    return out.matrix();
}

}
}
