#pragma once
#include "DgViewer.h"

class DgVolume;
class DgMesh;

struct CollisionResult {
    bool hasCollision = false;
    glm::vec3 deepestPoint;     // 가장 깊이 침투한 정점 좌표
    float deepestSDF = 0.0f;    // 그 정점의 SDF (음수)
    glm::vec3 normal;           // 그 정점의 법선 (정규화)
};

class DgCollision {
public:
    static CollisionResult detectCollision(DgVolume* sv, DgMesh* obstacle);

private:
    static GLuint sComputeShader;
    static GLuint sVertexSSBO;
    static GLuint sResultSSBO;
    static bool   sInitialized;

    static bool initializeGPU();
};