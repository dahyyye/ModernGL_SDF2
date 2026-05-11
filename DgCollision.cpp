#include "DgViewer.h"
#include "DgCollision.h"

GLuint DgCollision::sComputeShader = 0;
GLuint DgCollision::sVertexSSBO = 0;
GLuint DgCollision::sResultSSBO = 0;
bool   DgCollision::sInitialized = false;

bool DgCollision::initializeGPU()
{
    if (sInitialized) return true;

    sComputeShader = loadComputeShader(".\\shaders\\collision_detect.comp");
    if (sComputeShader == 0) {
        std::cerr << "collision_detect.comp 로드 실패" << std::endl;
        return false;
    }

    glGenBuffers(1, &sVertexSSBO);
    glGenBuffers(1, &sResultSSBO);
    sInitialized = true;
    return true;
}

CollisionResult DgCollision::detectCollision(DgVolume* sv, DgMesh* obstacle)
{
    CollisionResult result;
    if (!sv || !obstacle || sv->mTextureID == 0) return result;
    if (obstacle->mVerts.empty()) return result;

    // SV 월드 AABB 계산
    glm::vec3 svLocalMin = sv->getLocalMin();
    glm::vec3 svLocalMax = sv->getLocalMax();
    glm::mat4 modelMat = sv->getModelMatrix();

    glm::vec3 svWorldMin(FLT_MAX), svWorldMax(-FLT_MAX);
    glm::vec3 corners[8] = {
        {svLocalMin.x, svLocalMin.y, svLocalMin.z},
        {svLocalMax.x, svLocalMin.y, svLocalMin.z},
        {svLocalMax.x, svLocalMax.y, svLocalMin.z},
        {svLocalMin.x, svLocalMax.y, svLocalMin.z},
        {svLocalMin.x, svLocalMin.y, svLocalMax.z},
        {svLocalMax.x, svLocalMin.y, svLocalMax.z},
        {svLocalMax.x, svLocalMax.y, svLocalMax.z},
        {svLocalMin.x, svLocalMax.y, svLocalMax.z},
    };
    for (auto& c : corners) {
        glm::vec3 wc = glm::vec3(modelMat * glm::vec4(c, 1.0f));
        svWorldMin = glm::min(svWorldMin, wc);
        svWorldMax = glm::max(svWorldMax, wc);
    }

    // 장애물 AABB (이미 월드 좌표)
    glm::vec3 obsMin(FLT_MAX), obsMax(-FLT_MAX);
    for (auto& v : obstacle->mVerts) {
        obsMin = glm::min(obsMin, glm::vec3((float)v.mPos[0], (float)v.mPos[1], (float)v.mPos[2]));
        obsMax = glm::max(obsMax, glm::vec3((float)v.mPos[0], (float)v.mPos[1], (float)v.mPos[2]));
    }

    if (obsMax.x < svWorldMin.x || obsMin.x > svWorldMax.x ||
        obsMax.y < svWorldMin.y || obsMin.y > svWorldMax.y ||
        obsMax.z < svWorldMin.z || obsMin.z > svWorldMax.z)
        return result;

    if (!initializeGPU()) return result;

    int numVerts = (int)obstacle->mVerts.size();

    // 2. 장애물 정점을 SV 로컬 공간으로 변환 (셰이더의 uSvMin/Max가 로컬이므로)
    glm::mat4 invModel = glm::inverse(sv->getModelMatrix());

    std::vector<glm::vec4> vertexData(numVerts);
    for (int i = 0; i < numVerts; ++i) {
        glm::vec3 worldPos(
            (float)obstacle->mVerts[i].mPos[0],
            (float)obstacle->mVerts[i].mPos[1],
            (float)obstacle->mVerts[i].mPos[2]);
        glm::vec3 localPos = glm::vec3(invModel * glm::vec4(worldPos, 1.0f));
        vertexData[i] = glm::vec4(localPos, 0.0f);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sVertexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        numVerts * sizeof(glm::vec4), vertexData.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sVertexSSBO);

    // ── 3. 결과 SSBO ──
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sResultSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        numVerts * sizeof(float), nullptr, GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sResultSSBO);

    // ── 4. SV 텍스처 바인딩 + dispatch ──
    glUseProgram(sComputeShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, sv->mTextureID);
    glUniform1i(glGetUniformLocation(sComputeShader, "uSvSDF"), 0);

    glUniform3f(glGetUniformLocation(sComputeShader, "uSvMin"), svLocalMin.x, svLocalMin.y, svLocalMin.z);
    glUniform3f(glGetUniformLocation(sComputeShader, "uSvMax"), svLocalMax.x, svLocalMax.y, svLocalMax.z);
    glUniform1i(glGetUniformLocation(sComputeShader, "uNumVertices"), numVerts);

    glDispatchCompute((numVerts + 255) / 256, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // ── 5. Readback → 최솟값 찾기 ──
    std::vector<float> sdfData(numVerts);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sResultSSBO);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, numVerts * sizeof(float), sdfData.data());

    float minSDF = 0.0f;
    int minIdx = -1;
    for (int i = 0; i < numVerts; ++i) {
        if (sdfData[i] < minSDF) {
            minSDF = sdfData[i];
            minIdx = i;
        }
    }

    if (minIdx < 0) return result;  // 충돌 없음

    // ── 6. 법선 찾기: minIdx 정점에 연결된 face들의 법선 평균 ──
    glm::vec3 normalSum(0.0f);
    int normalCount = 0;

    for (auto& face : obstacle->mFaces) {
        for (int j = 0; j < 3; ++j) {
            if (face.mVertIdxs[j] == minIdx) {
                auto& n = obstacle->mNormals[face.mNormalIdxs[j]];
                normalSum += glm::vec3((float)n.mDir[0], (float)n.mDir[1], (float)n.mDir[2]);
                normalCount++;
            }
        }
    }

    result.hasCollision = true;
    result.deepestSDF = minSDF;
    result.deepestPoint = glm::vec3(
        (float)obstacle->mVerts[minIdx].mPos[0],
        (float)obstacle->mVerts[minIdx].mPos[1],
        (float)obstacle->mVerts[minIdx].mPos[2]);
    result.normal = (normalCount > 0) ? glm::normalize(normalSum) : glm::vec3(0, 1, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUseProgram(0);
    return result;
}