#pragma once
#include "DgViewer.h"


namespace SdfUtils
{
    // 구
    float sdSphere(glm::vec3 p, float radius);

    // 박스
    float sdBox(glm::vec3 p, glm::vec3 halfExtent);

    // 도넛
    float sdTorus(glm::vec3 p, glm::vec2 t);

    // 모서리 둥근 박스
    float sdRoundBox(glm::vec3 p, glm::vec3 halfExtent, float radius);

    // 프레임 박스
    float sdBoxFrame(glm::vec3 p, glm::vec3 halfExtent, float e);

    // 절단 도넛
    float sdCappedTorus(glm::vec3 p, glm::vec2 sc, float ra, float rb);

    // 링
    float sdLink(glm::vec3 p, float le, float r1, float r2);

    // 유한 원기둥
    float sdCylinder(glm::vec3 p, float h, float radius);

    // 무한 원기둥
    float sdInfCylinder(glm::vec3 p, glm::vec3 c);

    // 유한 원뿔
    float sdCone(glm::vec3 p, glm::vec2 c, float h);

    // 무한 원뿔
    float sdInfCone(glm::vec3 p, glm::vec2 c);

} // namespace SdfUtils