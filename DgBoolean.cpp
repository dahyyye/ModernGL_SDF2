#include "DgViewer.h"
#include "DgBoolean.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// ===================================================================
//  GPU 정적 변수
// ===================================================================
GLuint DgBoolean::sComputeShader = 0;
bool   DgBoolean::sInitialized = false;


// ===================================================================
//  메인 진입점 (GPU)
// ===================================================================
DgVolume* DgBoolean::Boolean(const std::vector<DgVolume*>& volumes, BooleanMode mode, int dim)
{
    if (volumes.size() < 2) return nullptr;
    return BooleanGPU(volumes, mode, dim);
}


// ===================================================================
//  GPU 초기화
// ===================================================================
bool DgBoolean::initializeGPU()
{
    if (sInitialized) return true;

    sComputeShader = loadComputeShader(".\\shaders\\boolean.comp");
    if (sComputeShader == 0) {
        std::cerr << "Boolean Compute Shader 초기화 실패" << std::endl;
        return false;
    }

    sInitialized = true;
    return true;
}


// ===================================================================
//  GPU Boolean: 2-입력 이항 연산 (핵심)
//
//  이 함수가 실제로 Compute Shader를 디스패치하는 단위.
//  volA와 volB 각각의 3D 텍스처를 바인딩하고,
//  결과 볼륨의 모든 복셀을 병렬 계산한다.
// ===================================================================
DgVolume* DgBoolean::booleanGPU_pair(
    DgVolume* volA, DgVolume* volB,
    BooleanMode mode, int dim,
    const glm::vec3& combinedMin, const glm::vec3& combinedMax)
{
    glUseProgram(sComputeShader);

    // isotropic voxel 기준 dim 계산
    glm::vec3 range = combinedMax - combinedMin;
    float maxRange = std::max({ range.x, range.y, range.z });
    float cellSize = maxRange / (dim - 1);
    int minDim = dim;
    int dimX = std::max(minDim, (int)std::round(range.x / cellSize) + 1);
    int dimY = std::max(minDim, (int)std::round(range.y / cellSize) + 1);
    int dimZ = std::max(minDim, (int)std::round(range.z / cellSize) + 1);

    // 결과 3D 텍스처 생성 (isotropic dim 적용)
    GLuint resultTexture;
    glGenTextures(1, &resultTexture);
    glBindTexture(GL_TEXTURE_3D, resultTexture);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F,
        dimX, dimY, dimZ,
        0, GL_RED, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // 볼륨 A: 텍스처 슬롯 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, volA->mTextureID);
    glUniform1i(glGetUniformLocation(sComputeShader, "uSdfA"), 0);

    // 볼륨 B: 텍스처 슬롯 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, volB->mTextureID);
    glUniform1i(glGetUniformLocation(sComputeShader, "uSdfB"), 1);

    // 결과 텍스처: 이미지 슬롯 2 (쓰기)
    glBindImageTexture(2, resultTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);

    // Uniform 전달
    glUniform3f(glGetUniformLocation(sComputeShader, "uResultMin"),
        combinedMin.x, combinedMin.y, combinedMin.z);
    glUniform3f(glGetUniformLocation(sComputeShader, "uResultMax"),
        combinedMax.x, combinedMax.y, combinedMax.z);
    // isotropic dim을 셰이더에 전달
    glUniform3i(glGetUniformLocation(sComputeShader, "uResolution"),
        dimX, dimY, dimZ);

    glm::vec3 minA = volA->getLocalMin();
    glm::vec3 maxA = volA->getLocalMax();
    glm::mat4 invModelA = glm::inverse(volA->getModelMatrix());
    glUniform3f(glGetUniformLocation(sComputeShader, "uMinA"), minA.x, minA.y, minA.z);
    glUniform3f(glGetUniformLocation(sComputeShader, "uMaxA"), maxA.x, maxA.y, maxA.z);
    glUniformMatrix4fv(glGetUniformLocation(sComputeShader, "uInvModelA"),
        1, GL_FALSE, glm::value_ptr(invModelA));

    glm::vec3 minB = volB->getLocalMin();
    glm::vec3 maxB = volB->getLocalMax();
    glm::mat4 invModelB = glm::inverse(volB->getModelMatrix());
    glUniform3f(glGetUniformLocation(sComputeShader, "uMinB"), minB.x, minB.y, minB.z);
    glUniform3f(glGetUniformLocation(sComputeShader, "uMaxB"), maxB.x, maxB.y, maxB.z);
    glUniformMatrix4fv(glGetUniformLocation(sComputeShader, "uInvModelB"),
        1, GL_FALSE, glm::value_ptr(invModelB));

    // Boolean 모드
    int modeInt = 0;
    if (mode == BooleanMode::Intersection) modeInt = 1;
    else if (mode == BooleanMode::Difference) modeInt = 2;
    glUniform1i(glGetUniformLocation(sComputeShader, "uBooleanMode"), modeInt);

    // 각 축 dim에 맞춰 워크그룹 수 계산
    int numGroupsX = (dimX + 7) / 8;
    int numGroupsY = (dimY + 7) / 8;
    int numGroupsZ = (dimZ + 7) / 8;
    glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);

    // GPU 완료 대기
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // 결과 볼륨 생성 (createResultVolume 경유로 isotropic dim 일관 적용)
    DgVolume* result = DgVolume::createResultVolume(
        generateName(mode) + " (GPU)", dim, combinedMin, combinedMax);

    // GPU → CPU 복사
    int totalSize = dimX * dimY * dimZ;
    result->mData.resize(totalSize);
    glBindTexture(GL_TEXTURE_3D, resultTexture);
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RED, GL_FLOAT, result->mData.data());

    result->mTextureID = resultTexture;
    result->mPosition = glm::vec3(0.0f);
    result->mRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    glUseProgram(0);
    return result;
}


// ===================================================================
//  GPU Boolean: 체이닝 (3개 이상 볼륨 지원)
//
//  A ∪ B ∪ C → (A ∪ B) = temp → temp ∪ C = result
//  이항 연산을 순차 적용하는 방식.
//  각 중간 결과는 이미 텍스처를 갖고 있으므로 다음 단계의 입력으로 즉시 사용 가능.
// ===================================================================
DgVolume* DgBoolean::BooleanGPU(const std::vector<DgVolume*>& volumes, BooleanMode mode, int dim)
{
    if (!initializeGPU()) return nullptr;

    clock_t start = clock();

    // 전체 AABB 계산 (CPU에서 미리)
    glm::vec3 combinedMin, combinedMax;
    computeAABB(volumes, combinedMin, combinedMax, mode);

    // --- 2개일 때: 단일 디스패치 ---
    if (volumes.size() == 2)
    {
        DgVolume* result = booleanGPU_pair(
            volumes[0], volumes[1], mode, dim,
            combinedMin, combinedMax);

        clock_t finish = clock();
        double duration = (double)(finish - start) / CLOCKS_PER_SEC;
        std::cout << "Boolean GPU: " << duration << "초" << std::endl;

        return result;
    }

    // --- 3개 이상일 때: 순차 체이닝 ---
    //
    //  Difference의 경우: A - B - C = (A - B) - C
    //  각 단계의 AABB는 전체 통합 AABB를 그대로 사용.
    //  (중간 결과의 AABB를 다시 계산하면 더 타이트하지만,
    //   GPU 디스패치 비용 대비 이득이 미미하므로 통합 AABB 재활용)

    DgVolume* accumulated = booleanGPU_pair(
        volumes[0], volumes[1], mode, dim,
        combinedMin, combinedMax);

    for (size_t i = 2; i < volumes.size(); ++i)
    {
        DgVolume* next = booleanGPU_pair(
            accumulated, volumes[i], mode, dim,
            combinedMin, combinedMax);

        // 중간 결과 정리
        delete accumulated;
        accumulated = next;
    }

    clock_t finish = clock();
    double duration = (double)(finish - start) / CLOCKS_PER_SEC;
    std::cout << "Boolean GPU (" << volumes.size() << "개 볼륨): " << duration << "초" << std::endl;

    return accumulated;
}

void DgBoolean::computeAABB(const std::vector<DgVolume*>& volumes,
    glm::vec3& combinedMin, glm::vec3& combinedMax,
    BooleanMode mode)
{
    if (volumes.empty()) return;

    // 첫 번째 볼륨의 월드 AABB
    volumes[0]->getWorldAABB(combinedMin, combinedMax);

    // 나머지 볼륨들과 결합
    for (size_t i = 1; i < volumes.size(); i++) {
        glm::vec3 volMin, volMax;
        volumes[i]->getWorldAABB(volMin, volMax);
        switch (mode) {
        case BooleanMode::Union:
            combinedMin = glm::min(combinedMin, volMin);
            combinedMax = glm::max(combinedMax, volMax);
            break;

        case BooleanMode::Intersection:
            combinedMin = glm::max(combinedMin, volMin);
            combinedMax = glm::min(combinedMax, volMax);
            break;

        case BooleanMode::Difference:
            break;
        }
    }

    // 패딩 추가
    glm::vec3 size = combinedMax - combinedMin;
    float padding = glm::max(size.x, glm::max(size.y, size.z)) * 0.05f;
    combinedMin -= glm::vec3(padding);
    combinedMax += glm::vec3(padding);
}

std::string DgBoolean::generateName(BooleanMode mode)
{

    switch (mode) {
    case BooleanMode::Union:        return "union";
    case BooleanMode::Intersection: return "intersection";
    case BooleanMode::Difference:   return "difference";
    }
    return "boolean";
}

float DgBoolean::sampleLocalSDF(DgVolume* vol, const glm::vec3& localPos)
{
    // 원래 볼륨의 크기를 가져옴 (로컬 -> uvw 변환용)
    glm::vec3 volMin = vol->getLocalMin();
    glm::vec3 volMax = vol->getLocalMax();
    glm::vec3 range = volMax - volMin;

    // 0으로 나누기 방지 (range가 너무 작으면 나눗셈에서 오류남)
    if (range.x < 0.0001f || range.y < 0.0001f || range.z < 0.0001f) {
        return 1.0f;
    }

    // UVW 좌표 계산하면 각 x, y, z가 기존 로컬 좌표의 범위를 [0,1]로 정규화시켜줌
    glm::vec3 uvw = (localPos - volMin) / range;

    // 범위 밖이면 경계까지의 거리를 더해서 반환
    float outsideDist = 0.0f;

    if (uvw.x < 0.0f || uvw.x > 1.0f ||
        uvw.y < 0.0f || uvw.y > 1.0f ||
        uvw.z < 0.0f || uvw.z > 1.0f)
    {
        // 경계까지의 거리 계산
        glm::vec3 clamped = glm::clamp(uvw, glm::vec3(0.0f), glm::vec3(1.0f));
        glm::vec3 diff = (uvw - clamped) * range;  // 월드 단위로 변환
        outsideDist = glm::length(diff);

        uvw = clamped;
    }

    float sdfValue = trilinearInterpolate(vol->mData.data(), vol->mDim[0], vol->mDim[1], vol->mDim[2], uvw);

    return sdfValue + outsideDist;
}

float DgBoolean::resampleSDF(DgVolume* vol, const glm::mat4& invModel, const glm::vec3& worldPos)
{
    // 새로운 바운딩 박스의 월드 좌표 → 로컬좌표
    glm::vec3 localPos = glm::vec3(invModel * glm::vec4(worldPos, 1.0f));

    return sampleLocalSDF(vol, localPos);
}

float DgBoolean::trilinearInterpolate(const float* data,
    int dimX, int dimY, int dimZ,
    const glm::vec3& uvw)
{
	// UVW 좌표를 격자 인덱스로 변환
    float fx = uvw.x * (dimX - 1);
    float fy = uvw.y * (dimY - 1);
    float fz = uvw.z * (dimZ - 1);

    //주변 8개 격자점의 정수 인덱스 찾기
    int x0 = std::max(0, std::min((int)std::floor(fx), dimX - 1));
    int y0 = std::max(0, std::min((int)std::floor(fy), dimY - 1));
    int z0 = std::max(0, std::min((int)std::floor(fz), dimZ - 1));

    int x1 = std::min(x0 + 1, dimX - 1);
    int y1 = std::min(y0 + 1, dimY - 1);
    int z1 = std::min(z0 + 1, dimZ - 1);

    //보간 가중치 계산
    float tx = fx - std::floor(fx);
    float ty = fy - std::floor(fy);
    float tz = fz - std::floor(fz);

	// 8개 격자점의 값 가져오기
    int sliceXY = dimX * dimY;

    float c000 = data[x0 + y0 * dimX + z0 * sliceXY];
    float c100 = data[x1 + y0 * dimX + z0 * sliceXY];
    float c010 = data[x0 + y1 * dimX + z0 * sliceXY];
    float c110 = data[x1 + y1 * dimX + z0 * sliceXY];
    float c001 = data[x0 + y0 * dimX + z1 * sliceXY];
    float c101 = data[x1 + y0 * dimX + z1 * sliceXY];
    float c011 = data[x0 + y1 * dimX + z1 * sliceXY];
    float c111 = data[x1 + y1 * dimX + z1 * sliceXY];

    // X축 보간 (8개 → 4개)
    float c00 = c000 * (1 - tx) + c100 * tx;
    float c01 = c001 * (1 - tx) + c101 * tx;
    float c10 = c010 * (1 - tx) + c110 * tx;
    float c11 = c011 * (1 - tx) + c111 * tx;

    // Y축 보간 (4개 → 2개)
    float c0 = c00 * (1 - ty) + c10 * ty;
    float c1 = c01 * (1 - ty) + c11 * ty;

    // Z축 보간 (2개 → 1개)
    return c0 * (1 - tz) + c1 * tz;
}
