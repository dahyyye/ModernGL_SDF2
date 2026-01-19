#include "DgViewer.h"
#include "DgSweep.h"
#include "DgBoolean.h"
#include <algorithm>
#include <cfloat>

GLuint DgSweep::sComputeShader = 0;
GLuint DgSweep::sTransformSSBO = 0;
bool DgSweep::sInitialized = false;

DgVolume* DgSweep::generateSweptVolume(
    DgVolume* brush,
    const DgTrajectory& trajectory,
    int resolution,
    int timeSteps,
    bool useGPU)
{
    if (!brush || trajectory.size() < 2) return nullptr;

    if (useGPU) {
        return generateGPU(brush, trajectory, resolution, timeSteps);
    }
    else {
        return generateCPU(brush, trajectory, resolution, timeSteps);
    }
}

// CPU 기반 스탬핑 방식
DgVolume* DgSweep::generateCPU(DgVolume* brush,
    const DgTrajectory& trajectory,
    int resolution,
    int timeSteps)
{
    clock_t start = clock();

    // 브러시 로컬 정보
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
    result->mName = "Swept Volume (CPU)";
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

// GPU 기반 스탬핑 방식
DgVolume* DgSweep::generateGPU(DgVolume* brush,
    const DgTrajectory& trajectory,
    int resolution,
    int timeSteps)
{
    if (!initializeGPU()) return nullptr;

    clock_t start = clock();

    // 1. 바운딩 박스 계산
    glm::vec3 localMin = brush->getLocalMin();
    glm::vec3 localMax = brush->getLocalMax();
    glm::vec3 localCenter = (localMin + localMax) * 0.5f;
    float radius = glm::length(localMax - localCenter);

    glm::vec3 combinedMin(FLT_MAX), combinedMax(-FLT_MAX);

    for (int step = 0; step <= timeSteps; ++step)
    {
        float t = (float)step / timeSteps;
        glm::mat4 transform = trajectory.getTransformAt(t);
        glm::vec3 worldCenter = glm::vec3(transform * glm::vec4(localCenter, 1.0f));

        combinedMin = glm::min(combinedMin, worldCenter);
        combinedMax = glm::max(combinedMax, worldCenter);
    }

    combinedMin -= glm::vec3(radius);
    combinedMax += glm::vec3(radius);

    // 2. 변환 행렬 → SSBO 업로드
    std::vector<glm::mat4> invTransforms(timeSteps + 1);
    for (int step = 0; step <= timeSteps; ++step)
    {
        float t = (float)step / timeSteps;
        glm::mat4 transform = trajectory.getTransformAt(t);
        invTransforms[step] = glm::inverse(transform);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sTransformSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, invTransforms.size() * sizeof(glm::mat4),
        invTransforms.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sTransformSSBO);

    // 3. 결과 3D 텍스처 생성
    GLuint resultTexture;
    glGenTextures(1, &resultTexture);
    glBindTexture(GL_TEXTURE_3D, resultTexture);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F,
        resolution, resolution, resolution,
        0, GL_RED, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // 4. Compute Shader 실행
    glUseProgram(sComputeShader);

    // 브러시 SDF 텍스처 (읽기)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, brush->mTextureID);
    glUniform1i(glGetUniformLocation(sComputeShader, "uBrushSDF"), 0);

    // 결과 텍스처 (쓰기)
    glBindImageTexture(1, resultTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);

    // Uniform 전달
    glUniform3f(glGetUniformLocation(sComputeShader, "uVolumeMin"),
        combinedMin.x, combinedMin.y, combinedMin.z);
    glUniform3f(glGetUniformLocation(sComputeShader, "uVolumeMax"),
        combinedMax.x, combinedMax.y, combinedMax.z);
    glUniform3i(glGetUniformLocation(sComputeShader, "uResolution"),
        resolution, resolution, resolution);

    glUniform3f(glGetUniformLocation(sComputeShader, "uBrushMin"),
        localMin.x, localMin.y, localMin.z);
    glUniform3f(glGetUniformLocation(sComputeShader, "uBrushMax"),
        localMax.x, localMax.y, localMax.z);

    glUniform1i(glGetUniformLocation(sComputeShader, "uTimeSteps"), timeSteps);

    // Dispatch
    int numGroups = (resolution + 7) / 8;
    glDispatchCompute(numGroups, numGroups, numGroups);

    // GPU 완료 대기
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // 5. 결과 볼륨 생성
    DgVolume* result = new DgVolume();
    result->mName = "Swept Volume (GPU)";
    result->mDim[0] = resolution;
    result->mDim[1] = resolution;
    result->mDim[2] = resolution;

    result->mMin = DgPos(combinedMin.x, combinedMin.y, combinedMin.z);
    result->mMax = DgPos(combinedMax.x, combinedMax.y, combinedMax.z);

    glm::vec3 range = combinedMax - combinedMin;
    result->mSpacing[0] = range.x / (resolution - 1);
    result->mSpacing[1] = range.y / (resolution - 1);
    result->mSpacing[2] = range.z / (resolution - 1);

    // GPU → CPU 복사 (비교용)
    int totalSize = resolution * resolution * resolution;
    result->mData.resize(totalSize);
    glBindTexture(GL_TEXTURE_3D, resultTexture);
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RED, GL_FLOAT, result->mData.data());

    // 텍스처 ID 직접 사용     
    result->mTextureID = resultTexture;

    // 메쉬 생성
    result->mMesh = createBoundingBoxMesh(result->mMin, result->mMax);
    result->mPosition = glm::vec3(0.0f);
    result->mRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    clock_t finish = clock();
    double duration = (double)(finish - start) / CLOCKS_PER_SEC;
    std::cout << "GPU Swept Volume 완료: " << duration << "초" << std::endl;

    glUseProgram(0);
    return result;
}

// GPU 초기화
bool DgSweep::initializeGPU()
{
    if (sInitialized) return true;

    // Compute Shader 로드
    sComputeShader = loadComputeShader(".\\shaders\\sweeping.comp");
    if (sComputeShader == 0) {
        std::cerr << "Sweep Compute Shader 초기화 실패" << std::endl;
        return false;
    }

    // SSBO 생성
    glGenBuffers(1, &sTransformSSBO);

    sInitialized = true;
    return true;
}