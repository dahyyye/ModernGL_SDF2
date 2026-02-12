#include "DgViewer.h"
#include "DgBoolean.h"
#include <algorithm>
#include <cmath>
#include <iostream>

DgVolume* DgBoolean::Boolean(const std::vector<DgVolume*>& volumes, BooleanMode mode, int dim)
{
    clock_t start, finish;
	double duration;
    start = clock();

    if (volumes.size() < 2) return nullptr;

    // 결합된 바운딩 박스 계산
    glm::vec3 combinedMin, combinedMax;
    computeAABB(volumes, combinedMin, combinedMax, mode);

    // 새 볼륨 생성
    DgVolume* result = new DgVolume();
    result->mName = generateName(mode);

    // 해상도 설정
    result->mDim[0] = dim;
    result->mDim[1] = dim;
    result->mDim[2] = dim;

    // 바운딩 박스 설정
    result->mMin = DgPos(combinedMin.x, combinedMin.y, combinedMin.z);
    result->mMax = DgPos(combinedMax.x, combinedMax.y, combinedMax.z);

    // 격자 간격 계산
    glm::vec3 range = combinedMax - combinedMin;
    result->mSpacing[0] = range.x / (dim - 1);
    result->mSpacing[1] = range.y / (dim - 1);
    result->mSpacing[2] = range.z / (dim - 1);

    // SDF 데이터 생성
    int totalSize = dim * dim * dim;
    result->mData.resize(totalSize);

    // 역행렬 계산
    std::vector<glm::mat4> invModels;
    for (DgVolume* vol : volumes) {
        invModels.push_back(glm::inverse(vol->getModelMatrix()));
    }

    for (int k = 0; k < dim; ++k) {
        for (int j = 0; j < dim; ++j) {
            for (int i = 0; i < dim; ++i) {
                // 격자점의 월드 좌표 계산
                glm::vec3 worldPos(
                    combinedMin.x + i * result->mSpacing[0],
                    combinedMin.y + j * result->mSpacing[1],
                    combinedMin.z + k * result->mSpacing[2]
                );

                // 첫 번째 볼륨의 SDF
                float resultSDF = resampleSDF(volumes[0], invModels[0], worldPos);

                // 나머지 볼륨들과 연산
                for (size_t v = 1; v < volumes.size(); ++v) {
                    float sdf = resampleSDF(volumes[v], invModels[v], worldPos);

                    switch (mode) {
                    case BooleanMode::Union:
                        resultSDF = std::min(resultSDF, sdf);
                        break;
                    case BooleanMode::Intersection:
                        resultSDF = std::max(resultSDF, sdf);
                        break;
                    case BooleanMode::Difference:
                        resultSDF = std::max(resultSDF, -sdf);
                        break;
                    }
                }
                int index = i + j * dim + k * dim * dim;
                result->mData[index] = resultSDF;
            }
        }
    }
	finish = clock();
    duration = (double)(finish - start) / CLOCKS_PER_SEC;
    cout << duration << "초" << endl;

    // 텍스처 및 메쉬 생성
    result->createTexture();
    result->mMesh = createBoundingBoxMesh(result->mMin, result->mMax);
    result->mPosition = glm::vec3(0.0f);
    result->mRotation = glm::vec3(0.0f);

    return result;
}

void DgBoolean::computeAABB(const std::vector<DgVolume*>& volumes,
    glm::vec3& combinedMin, glm::vec3& combinedMax,
    BooleanMode mode)
{
    if (volumes.empty()) return;

    // 첫 번째 볼륨의 월드 AABB
    getWorldAABB(volumes[0], combinedMin, combinedMax);

    // 나머지 볼륨들과 결합
    for (size_t i = 1; i < volumes.size(); i++) {
        glm::vec3 volMin, volMax;
        getWorldAABB(volumes[i], volMin, volMax);

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

void DgBoolean::getWorldAABB(DgVolume* vol, glm::vec3& outMin, glm::vec3& outMax)
{
    glm::vec3 localMin = vol->getLocalMin();
    glm::vec3 localMax = vol->getLocalMax();
    glm::mat4 model = vol->getModelMatrix();

    glm::vec3 corners[8] = {
        {localMin.x, localMin.y, localMin.z},
        {localMax.x, localMin.y, localMin.z},
        {localMax.x, localMax.y, localMin.z},
        {localMin.x, localMax.y, localMin.z},
        {localMin.x, localMin.y, localMax.z},
        {localMax.x, localMin.y, localMax.z},
        {localMax.x, localMax.y, localMax.z},
        {localMin.x, localMax.y, localMax.z}
    };

    glm::vec3 transformed = glm::vec3(model * glm::vec4(corners[0], 1.0f));
    outMin = outMax = transformed;

	for (int i = 1; i < 8; i++) { // 8개의 코너를 월드 좌표로 변환 및 크기 계산
        transformed = glm::vec3(model * glm::vec4(corners[i], 1.0f));
        outMin = glm::min(outMin, transformed);
        outMax = glm::max(outMax, transformed);
    }
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

float DgBoolean::resampleSDF(DgVolume* vol, const glm::mat4& invModel, const glm::vec3& worldPos)
{
    // 새로운 바운딩 박스의 월드 좌표 → 로컬좌표
    glm::vec3 localPos = glm::vec3(invModel * glm::vec4(worldPos, 1.0f));

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
