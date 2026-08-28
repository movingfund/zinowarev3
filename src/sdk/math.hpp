#pragma once
#include <cmath>
#include <cstdint>
#include <array>

struct Vector2 {
    float x, y;

    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}

    float Length() const { return std::sqrt(x * x + y * y); }
    float Dist(const Vector2& o) const { return (*this - o).Length(); }
    Vector2 operator-(const Vector2& o) const { return {x - o.x, y - o.y}; }
    Vector2 operator+(const Vector2& o) const { return {x + o.x, y + o.y}; }
};

struct Vector3 {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vector3 operator/(float s) const { return {x / s, y / s, z / s}; }

    float  Dot(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vector3 Cross(const Vector3& o) const {
        return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x };
    }
    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    Vector3 Normalized() const {
        float len = Length();
        if (len < 0.0001f) return {};
        return *this / len;
    }
};

struct Matrix4x4 {
    float m[16]{};

    float& operator[](int i) { return m[i]; }
    const float& operator[](int i) const { return m[i]; }

    bool WorldToScreen(const Vector3& world, Vector2& screen, int width, int height) const {
        float w = m[3] * world.x + m[7] * world.y + m[11] * world.z + m[15];
        if (w < 0.001f) return false;

        float invW = 1.0f / w;
        float ndcX = (m[0] * world.x + m[4] * world.y + m[8]  * world.z + m[12]) * invW;
        float ndcY = (m[1] * world.x + m[5] * world.y + m[9]  * world.z + m[13]) * invW;

        screen.x = (ndcX + 1.0f) * 0.5f * width;
        screen.y = (1.0f - ndcY) * 0.5f * height;
        return true;
    }
};

struct RobloxCFrame {
    float rotation[9]{};
    float position[3]{};

    Vector3 GetPosition() const { return {position[0], position[1], position[2]}; }
    void    SetPosition(const Vector3& p) { position[0] = p.x; position[1] = p.y; position[2] = p.z; }

    static RobloxCFrame LookAt(const Vector3& from, const Vector3& to) {
        Vector3 forward = (to - from).Normalized();
        Vector3 worldUp = {0, 1, 0};

        if (std::abs(forward.y) > 0.999f)
            worldUp = {0, 0, forward.y > 0 ? -1.0f : 1.0f};

        Vector3 right = worldUp.Cross(forward).Normalized();
        Vector3 up    = forward.Cross(right).Normalized();

        RobloxCFrame cf;
        cf.rotation[0] = right.x;   cf.rotation[3] = up.x;   cf.rotation[6] = -forward.x;
        cf.rotation[1] = right.y;   cf.rotation[4] = up.y;   cf.rotation[7] = -forward.y;
        cf.rotation[2] = right.z;   cf.rotation[5] = up.z;   cf.rotation[8] = -forward.z;
        cf.SetPosition(from);
        return cf;
    }
};
