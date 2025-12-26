#include "DgViewer.h"
#include "DgSweep.h"
#include "DgBoolean.h"
#include <algorithm>
#include <cfloat>

DgVolume* DgSweep::generateSweptVolume(
    DgVolume* brush,
    const DgTrajectory& trajectory,
    int resolution,
    int timeSteps)
{
    if (!brush || trajectory.size() < 2) return nullptr;

    clock_t start = clock();

    // 브러시 로컹 정보
    glm::vec3 localMin = brush->getLocalMin();
    glm::vec3 localMax = brush->getLocalMax();
    glm::vec3 localCenter = (localMin + localMax) * 0.5f;
    
    // 궤적 중심점들의 AABB 계산
    glm::vec3 combinedMin(FLT_MAX), combinedMax(-FLT_MAX);

    for (int step = 0; step <= timeSteps; ++step)
    {
        float t = (float)step / timeSteps;
        glm::mat4 transform = trajectory.getTransformAt(t);
        glm::vec3 worldCenter = glm::vec3(transform * glm::vec4(localCenter, 1.0f));

        combinedMin = glm::min(combinedMin, worldCenter);
        combinedMax = glm::max(combinedMax, worldCenter);
    }

    // 반경만큼 패딩
    float radius = glm::length(localMax - localCenter);

    combinedMin -= glm::vec3(radius);
    combinedMax += glm::vec3(radius);

    // 결과 볼륨 생성
    DgVolume* result = new DgVolume();
    result->mName = "Swept Volume";
    result->mDim[0] = resolution;
    result->mDim[1] = resolution;
    result->mDim[2] = resolution;

    result->mMin = DgPos(combinedMin.x, combinedMin.y, combinedMin.z);
    result->mMax = DgPos(combinedMax.x, combinedMax.y, combinedMax.z);

    glm::vec3 range = combinedMax - combinedMin;
    result->mSpacing[0] = range.x / (resolution - 1);
    result->mSpacing[1] = range.y / (resolution - 1);
    result->mSpacing[2] = range.z / (resolution - 1);

    // SDF 초기화
    int totalSize = resolution * resolution * resolution;
    result->mData.resize(totalSize, FLT_MAX);

    // 스탬핑
    for (int step = 0; step <= timeSteps; ++step)
    {
        float t = (float)step / timeSteps;
        glm::mat4 transform = trajectory.getTransformAt(t);
        glm::mat4 invTransform = glm::inverse(transform);

        for (int k = 0; k < resolution; ++k)
        {
            for (int j = 0; j < resolution; ++j)
            {
                for (int i = 0; i < resolution; ++i)
                {
                    glm::vec3 worldPos(
                        combinedMin.x + i * result->mSpacing[0],
                        combinedMin.y + j * result->mSpacing[1],
                        combinedMin.z + k * result->mSpacing[2]
                    );

                    float sdf = DgBoolean::resampleSDF(brush, invTransform, worldPos);

                    int index = i + j * resolution + k * resolution * resolution;
                    result->mData[index] = std::min(result->mData[index], sdf);
                }
            }
        }

        if (step % 10 == 0)
            std::cout << "Stamping: " << (step * 100 / timeSteps) << "%" << std::endl;
    }

    clock_t finish = clock();
    double duration = (double)(finish - start) / CLOCKS_PER_SEC;
    std::cout << "Swept Volume 생성 완료: " << duration << "초" << std::endl;

    // 텍스처 및 볼륨 생성
    result->createTexture();
    result->mMesh = createBoundingBoxMesh(result->mMin, result->mMax);
    result->mPosition = glm::vec3(0.0f);
    result->mRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    return result;
}