#pragma once

#include <cmath>

namespace bsim
{
// Engine-free 3D vector. Units follow Unreal: centimetres, Z up.
struct Vec3
{
    float X = 0.f;
    float Y = 0.f;
    float Z = 0.f;
};

inline Vec3 operator+(const Vec3& A, const Vec3& B) { return {A.X + B.X, A.Y + B.Y, A.Z + B.Z}; }
inline Vec3 operator-(const Vec3& A, const Vec3& B) { return {A.X - B.X, A.Y - B.Y, A.Z - B.Z}; }
inline Vec3 operator-(const Vec3& A) { return {-A.X, -A.Y, -A.Z}; }
inline Vec3 operator*(const Vec3& A, float S) { return {A.X * S, A.Y * S, A.Z * S}; }
inline Vec3 operator*(float S, const Vec3& A) { return A * S; }
inline Vec3 operator/(const Vec3& A, float S) { return {A.X / S, A.Y / S, A.Z / S}; }

inline float Dot(const Vec3& A, const Vec3& B) { return A.X * B.X + A.Y * B.Y + A.Z * B.Z; }
inline Vec3 Cross(const Vec3& A, const Vec3& B)
{
    return {A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X};
}
inline float LengthSq(const Vec3& V) { return Dot(V, V); }
inline float Length(const Vec3& V) { return std::sqrt(LengthSq(V)); }

// Returns the zero vector for (near-)zero input instead of NaN.
inline Vec3 Normalized(const Vec3& V)
{
    const float Len = Length(V);
    return Len > 1e-6f ? V / Len : Vec3{};
}

inline Vec3 Lerp(const Vec3& A, const Vec3& B, float T) { return A + (B - A) * T; }
inline Vec3 Flatten(const Vec3& V) { return {V.X, V.Y, 0.f}; }
} // namespace bsim
