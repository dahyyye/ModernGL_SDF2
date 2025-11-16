#include "SdfUtils.h"

namespace SdfUtils
{

    // 구
    float sdSphere(glm::vec3 p, float radius) {
        return glm::length(p) - radius;
    }

    // 박스
    float sdBox(glm::vec3 p, glm::vec3 halfExtent)
    {
        glm::vec3 q = glm::abs(p) - halfExtent;
        return glm::length(glm::max(q, 0.0f)) + glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.0f);
    }

    // 도넛 // t.x: 중심 원의 반지름, t.y: 관 단면 반지름
    float sdTorus(glm::vec3 p, glm::vec2 t)
    {
        // GLSL의 p.xz -> C++의 glm::vec2(p.x, p.z)
        glm::vec2 q = glm::vec2(glm::length(glm::vec2(p.x, p.z)) - t.x, p.y);
        return glm::length(q) - t.y;
    }

    // 모서리 둥근 박스 
    float sdRoundBox(glm::vec3 p, glm::vec3 halfExtent, float radius)
    {
        // C++(GLM)에서는 vec3와 float를 바로 더할 수 있습니다.
        glm::vec3 q = glm::abs(p) - halfExtent + radius;
        return glm::length(glm::max(q, 0.0f)) + glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.0f) - radius;
    }

    // 프레임 박스 // e: 프레임 두께
    float sdBoxFrame(glm::vec3 p, glm::vec3 halfExtent, float e)
    {
        p = glm::abs(p) - halfExtent;
        glm::vec3 q = glm::abs(p + e) - e;

        // GLSL의 min(a,b) -> C++의 glm::min(a,b) 또는 std::min(a,b)
        // 여기서는 glm:: 함수를 통일성 있게 사용합니다.
        using glm::max;
        using glm::min;

        return min(min(
            glm::length(max(glm::vec3(p.x, q.y, q.z), 0.0f)) + min(max(p.x, max(q.y, q.z)), 0.0f),
            glm::length(max(glm::vec3(q.x, p.y, q.z), 0.0f)) + min(max(q.x, max(p.y, q.z)), 0.0f)),
            glm::length(max(glm::vec3(q.x, q.y, p.z), 0.0f)) + min(max(q.x, max(q.y, p.z)), 0.0f));
    }

    // 절단 도넛 // sc: (관 단면 비율.x, 관 단면 비율.y), ra: 중심 원 반지름, rb: 관 단면 반지름
    float sdCappedTorus(glm::vec3 p, glm::vec2 sc, float ra, float rb)
    {
        p.x = glm::abs(p.x);
        glm::vec2 p_xy(p.x, p.y); // GLSL의 p.xy
        float k = (sc.y * p.x > sc.x * p.y) ? glm::dot(p_xy, sc) : glm::length(p_xy);
        return glm::sqrt(glm::dot(p, p) + ra * ra - 2.0f * ra * k) - rb;
    }

    // 링 // le: 링의 절반 길이, r1: 중심 원 반지름, r2: 관 단면 반지름
    float sdLink(glm::vec3 p, float le, float r1, float r2)
    {
        glm::vec3 q = glm::vec3(p.x, glm::max(glm::abs(p.y) - le, 0.0f), p.z);
        glm::vec2 q_xy(q.x, q.y); // GLSL의 q.xy
        glm::vec2 q_xz_r1(glm::length(q_xy) - r1, q.z); // GLSL의 vec2(length(q.xy)-r1,q.z)
        return glm::length(q_xz_r1) - r2;
    }

    // 유한 원기둥 // h: 높이/2
    float sdCylinder(glm::vec3 p, float h, float radius)
    {
        glm::vec2 p_xz(p.x, p.z); // GLSL의 p.xz
        glm::vec2 d = glm::abs(glm::vec2(glm::length(p_xz), p.y)) - glm::vec2(radius, h);
        return glm::min(glm::max(d.x, d.y), 0.0f) + glm::length(glm::max(d, 0.0f));
    }

    // 무한 원기둥 // c: (중심.x, 중심.z, 반지름)
    float sdInfCylinder(glm::vec3 p, glm::vec3 c) {
        glm::vec2 p_xz(p.x, p.z); // GLSL의 p.xz
        glm::vec2 c_xy(c.x, c.y); // GLSL의 c.xy
        return glm::length(p_xz - c_xy) - c.z;
    }

    // 유한 원뿔 // c: (sinθ, cosθ)
    float sdCone(glm::vec3 p, glm::vec2 c, float h)
    {
        glm::vec2 q = h * glm::vec2(c.x / c.y, -1.0f);
        glm::vec2 w = glm::vec2(glm::length(glm::vec2(p.x, p.z)), p.y); // GLSL의 length(p.xz)
        glm::vec2 a = w - q * glm::clamp(glm::dot(w, q) / glm::dot(q, q), 0.0f, 1.0f);
        glm::vec2 b = w - q * glm::vec2(glm::clamp(w.x / q.x, 0.0f, 1.0f), 1.0f);
        float k = glm::sign(q.y);
        float d = glm::min(glm::dot(a, a), glm::dot(b, b));
        float s = glm::max(k * (w.x * q.y - w.y * q.x), k * (w.y - q.y));
        return glm::sqrt(d) * glm::sign(s);
    }

    // 무한 원뿔 // c: (sinθ, cosθ)
    float sdInfCone(glm::vec3 p, glm::vec2 c)
    {
        glm::vec2 q = glm::vec2(glm::length(glm::vec2(p.x, p.z)), -p.y);
        float d = glm::length(q - c * glm::max(glm::dot(q, c), 0.0f));
        return d * ((q.x * c.y - q.y * c.x < 0.0f) ? -1.0f : 1.0f);
    }

} 