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
    int samplingSteps,
    bool useGPU,
    bool useSegment)
{
    if (!brush || trajectory.size() < 2) return nullptr;

    if (useGPU) {
        return generateGPU(brush, trajectory, resolution, samplingSteps);
    }
    else if (useSegment) {
        return generateSegmentCPU(brush, trajectory, resolution, samplingSteps);
    }
    else {
        return generateCPU(brush, trajectory, resolution, samplingSteps);
    }
}

// CPU 기반 스탬핑 방식
DgVolume* DgSweep::generateCPU(DgVolume* brush,
    const DgTrajectory& trajectory,
    int resolution,
    int samplingSteps)
{
    clock_t start = clock();

    // 브러시 로컬 정보
    glm::vec3 localMin = brush->getLocalMin();
    glm::vec3 localMax = brush->getLocalMax();
    glm::vec3 localCenter = (localMin + localMax) * 0.5f;
    
    // 궤적 중심점들의 AABB 계산
    glm::vec3 combinedMin(FLT_MAX), combinedMax(-FLT_MAX);

    for (int step = 0; step < samplingSteps; ++step)
    {
        float t = (samplingSteps > 1) ? (float)step / (samplingSteps - 1) : 0.0f;
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
    for (int step = 0; step < samplingSteps; ++step)
    {
        float t = (samplingSteps > 1) ? (float)step / (samplingSteps - 1) : 0.0f;
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
            std::cout << "Stamping: " << (step * 100 / samplingSteps) << "%" << std::endl;
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
    int samplingSteps)
{
    if (!initializeGPU()) return nullptr;

    clock_t start = clock();

    // 1. 바운딩 박스 계산
    glm::vec3 localMin = brush->getLocalMin();
    glm::vec3 localMax = brush->getLocalMax();
    glm::vec3 localCenter = (localMin + localMax) * 0.5f;
    float radius = glm::length(localMax - localCenter);

    glm::vec3 combinedMin(FLT_MAX), combinedMax(-FLT_MAX);

    for (int step = 0; step < samplingSteps; ++step)
    {
        float t = (samplingSteps > 1) ? (float)step / (samplingSteps - 1) : 0.0f;
        glm::mat4 transform = trajectory.getTransformAt(t);
        glm::vec3 worldCenter = glm::vec3(transform * glm::vec4(localCenter, 1.0f));

        combinedMin = glm::min(combinedMin, worldCenter);
        combinedMax = glm::max(combinedMax, worldCenter);
    }

    combinedMin -= glm::vec3(radius);
    combinedMax += glm::vec3(radius);

    // 변환 행렬
    std::vector<glm::mat4> invTransforms(samplingSteps);
    for (int step = 0; step < samplingSteps; ++step)
    {
        float t = (samplingSteps > 1) ? (float)step / (samplingSteps - 1) : 0.0f;
        glm::mat4 transform = trajectory.getTransformAt(t);
        invTransforms[step] = glm::inverse(transform);
    }

    // Compute Shader 실행
    glUseProgram(sComputeShader);

	// SSBO에 변환 행렬 업로드
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sTransformSSBO);         // SSBO 바인딩
    glBufferData(GL_SHADER_STORAGE_BUFFER, 
		invTransforms.size() * sizeof(glm::mat4),       //크기: 행렬 개수 * 행렬 크기
        invTransforms.data(),                           // CPU 메모리 주소
		GL_DYNAMIC_DRAW);                               // 사용 빈도: 동적 업데이트
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sTransformSSBO);  // 바인딩 포인트 2에 연결

    // 결과 3D 텍스처 생성
    GLuint resultTexture;
    glGenTextures(1, &resultTexture);
    glBindTexture(GL_TEXTURE_3D, resultTexture);    // 작업 대상으로 지정

	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F,         // 빈 3D 텍스처 생성
        resolution, resolution, resolution,
        0, GL_RED, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);       // 텍스처 샘플링 설정
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);       //텍스처 호출 시 주변 값을 보간해서 반환하도록 함
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

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

    glUniform1i(glGetUniformLocation(sComputeShader, "uSamplingSteps"), samplingSteps);

    // Dispatch
	int numGroups = (resolution + 7) / 8;       // 나누어떨어지지 않을 때를 대비해 올림처리
	glDispatchCompute(numGroups, numGroups, numGroups); // 워크 그룹 동시 실행

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

    // GPU → CPU 복사
    int totalSize = resolution * resolution * resolution;

    result->mData.resize(totalSize);
    glBindTexture(GL_TEXTURE_3D, resultTexture);
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RED, GL_FLOAT, result->mData.data());

    // 텍스처 ID 직접 사용     
    result->mTextureID = resultTexture;

    // 박스 메쉬 생성
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

DgVolume* DgSweep::generateSegmentCPU(DgVolume* brush,
    const DgTrajectory& trajectory,
    int resolution,
    int samplingSteps,
    int lamda)
{
    clock_t start = clock();

    //브러시 로컬 정보
    glm::vec3 localMin = brush->getLocalMin();
    glm::vec3 localMax = brush->getLocalMax();
    glm::vec3 localCenter = (localMin + localMax) * 0.5f;

	// 결과 볼륨의 바운딩 박스 계산(AABB)
    glm::vec3 combinedMin(FLT_MAX), combinedMax(-FLT_MAX);
    for (int step = 0; step < samplingSteps; ++step)
    {
        float t = (samplingSteps > 1) ? (float)step / (samplingSteps - 1) : 0.0f;
        glm::mat4 transform = trajectory.getTransformAt(t);
        glm::vec3 worldCenter = glm::vec3(transform * glm::vec4(localCenter, 1.0f));
        combinedMin = glm::min(combinedMin, worldCenter);
        combinedMax = glm::max(combinedMax, worldCenter);
    }

    float radius = glm::length(localMax - localCenter);
    combinedMin -= glm::vec3(radius);
    combinedMax += glm::vec3(radius);

	// 결과 볼륨 생성
    DgVolume* result = new DgVolume();
    result->mName = "Swept Volume (Segment)";
    result->mDim[0] = resolution;
    result->mDim[1] = resolution;
    result->mDim[2] = resolution;

    result->mMin = DgPos(combinedMin.x, combinedMin.y, combinedMin.z);
    result->mMax = DgPos(combinedMax.x, combinedMax.y, combinedMax.z);

    glm::vec3 range = combinedMax - combinedMin;
    result->mSpacing[0] = range.x / (resolution - 1);
    result->mSpacing[1] = range.y / (resolution - 1);
    result->mSpacing[2] = range.z / (resolution - 1);

    int totalSize = resolution * resolution * resolution;
    result->mData.resize(totalSize, FLT_MAX);

    // 역변환 행렬 미리 계산
    std::vector<glm::mat4> invTransforms(samplingSteps);
    for (int step = 0; step < samplingSteps; ++step)
    {
        float t = (samplingSteps > 1) ? (float)step / (samplingSteps - 1) : 0.0f;
        invTransforms[step] = glm::inverse(trajectory.getTransformAt(t));
    }

    // Segment-Based Stamping
    for (int seg = 0; seg < samplingSteps - 1; ++seg)
    {
        for (int k = 0; k < resolution; ++k)
        {
            for (int j = 0; j < resolution; ++j)
            {
                for (int i = 0; i < resolution; ++i)
                {
					glm::vec3 worldPos( // 격자 샘플의 월드 좌표(쿼리 점)
                        combinedMin.x + i * result->mSpacing[0],
                        combinedMin.y + j * result->mSpacing[1],
                        combinedMin.z + k * result->mSpacing[2]
                    );

					// 선분의 양 끝점 p0, p1 계산
                    glm::vec3 p0 = glm::vec3(invTransforms[seg] * glm::vec4(worldPos, 1.0f));
                    glm::vec3 p1 = glm::vec3(invTransforms[seg + 1] * glm::vec4(worldPos, 1.0f));

                    int index = i + j * resolution + k * resolution * resolution;

					// 선분을 따라 lamda+1 개의 샘플링 점에서 SDF 평가
                    for (int s = 0; s <= lamda; ++s)
                    {
                        float alpha = (float)s / lamda;
                        glm::vec3 p = glm::mix(p0, p1, alpha);

                        float sdf = DgBoolean::sampleLocalSDF(brush, p);
                        result->mData[index] = std::min(result->mData[index], sdf);
                    }
                }
            }
        }

        if (seg % 10 == 0)
            std::cout << "Segment: " << (seg * 100 / (samplingSteps - 1)) << "%" << std::endl;
    }

    clock_t finish = clock();
    double duration = (double)(finish - start) / CLOCKS_PER_SEC;
    std::cout << "Segment Swept Volume 완료: " << duration << "초" << std::endl;

	// 후처리(텍스처 및 볼륨 생성)
    result->createTexture();
    result->mMesh = createBoundingBoxMesh(result->mMin, result->mMax);
    result->mPosition = glm::vec3(0.0f);
    result->mRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    return result;
}

DgVolume* DgSweep::generateTestCPU(DgVolume* brush,
    const DgTrajectory& trajectory,
    int resolution,
    int samplingSteps)
{
    clock_t start = clock();

	// 브러시 로컬 정보
    glm::vec3 localMin = brush->getLocalMin();
    glm::vec3 localMax = brush->getLocalMax();
    glm::vec3 localCenter = (localMin + localMax) * 0.5f;

	// 결과 볼륨의 바운딩 박스 계산(AABB)
    glm::vec3 combinedMin(FLT_MAX), combinedMax(-FLT_MAX);
    for (int step = 0; step < samplingSteps; ++step)
    {
        float t = (samplingSteps > 1) ? (float)step / (samplingSteps - 1) : 0.0f;
        glm::mat4 transform = trajectory.getTransformAt(t);
        glm::vec3 worldCenter = glm::vec3(transform * glm::vec4(localCenter, 1.0f));
        combinedMin = glm::min(combinedMin, worldCenter);
        combinedMax = glm::max(combinedMax, worldCenter);
    }

    float radius = glm::length(localMax - localCenter);
    combinedMin -= glm::vec3(radius);
    combinedMax += glm::vec3(radius);

	// 결과 볼륨 생성
    DgVolume* result = new DgVolume();
    result->mName = "Swept Volume (Test)";
    result->mDim[0] = resolution;
    result->mDim[1] = resolution;
    result->mDim[2] = resolution;

    result->mMin = DgPos(combinedMin.x, combinedMin.y, combinedMin.z);
    result->mMax = DgPos(combinedMax.x, combinedMax.y, combinedMax.z);

    glm::vec3 range = combinedMax - combinedMin;
    result->mSpacing[0] = range.x / (resolution - 1);
    result->mSpacing[1] = range.y / (resolution - 1);
    result->mSpacing[2] = range.z / (resolution - 1);

    int totalSize = resolution * resolution * resolution;
    result->mData.resize(totalSize, FLT_MAX);

    // 순변환 + 역변환 행렬 사전 계산
    std::vector<glm::mat4> transforms(samplingSteps);
    std::vector<glm::mat4> invTransforms(samplingSteps);
    for (int step = 0; step < samplingSteps; ++step)
    {
        float t = (samplingSteps > 1) ? (float)step / (samplingSteps - 1) : 0.0f;
        transforms[step] = trajectory.getTransformAt(t);
        invTransforms[step] = glm::inverse(transforms[step]);
    }

    glm::vec3 brushCenter = localCenter;

    for (int seg = 0; seg < samplingSteps - 1; ++seg)
    {
        // 궤적 선분계산 : brush 중심의 월드 좌표 양 끝점
        glm::vec3 c0 = glm::vec3(transforms[seg] * glm::vec4(brushCenter, 1.0f));
        glm::vec3 c1 = glm::vec3(transforms[seg + 1] * glm::vec4(brushCenter, 1.0f));

        // 
        glm::vec3 segDir = c1 - c0;
        float segLenSq = glm::dot(segDir, segDir);

        for (int k = 0; k < resolution; ++k)
        {
            for (int j = 0; j < resolution; ++j)
            {
                for (int i = 0; i < resolution; ++i)
                {
					glm::vec3 worldPos( // 격자 샘플의 월드 좌표(쿼리 점)
                        combinedMin.x + i * result->mSpacing[0],
                        combinedMin.y + j * result->mSpacing[1],
                        combinedMin.z + k * result->mSpacing[2]
                    );

                    // 1. 복셀에서 궤적 선분까지의 최근접점 → alpha
                    float alpha = 0.0f;
                    if (segLenSq > 0.00001f)
                        // dot : worldPos에서 선분 위로 정사영한 위치, clamp(0, 1)로 선분 밖으로 나가지 않게
                        alpha = glm::clamp(glm::dot(worldPos - c0, segDir) / segLenSq, 0.0f, 1.0f);

                    // 2. alpha로 역변환 보간 → 로컬 좌표 구함
                    glm::vec3 p0 = glm::vec3(invTransforms[seg] * glm::vec4(worldPos, 1.0f));
                    glm::vec3 p1 = glm::vec3(invTransforms[seg + 1] * glm::vec4(worldPos, 1.0f));
                    glm::vec3 localPos = glm::mix(p0, p1, alpha);
                    // mix 함수는 두 값을 세 번째 인자(가중치)를 기반으로 선형 보간하여 섞는 함수

                    float sdfClosest = DgBoolean::sampleLocalSDF(brush, localPos);

                    // 양 끝점에서의 SDF (stamp 위치 보장)
                    float sdfStart = DgBoolean::sampleLocalSDF(brush, p0);
                    float sdfEnd = DgBoolean::sampleLocalSDF(brush, p1);

                    // 셋 중 최소값
                    float sdf = std::min({ sdfClosest, sdfStart, sdfEnd });

                    int index = i + j * resolution + k * resolution * resolution;
                    result->mData[index] = std::min(result->mData[index], sdf);
                }
            }
        }

        if (seg % 10 == 0)
            std::cout << "Test: " << (seg * 100 / (samplingSteps - 1)) << "%" << std::endl;
    }

    clock_t finish = clock();
    double duration = (double)(finish - start) / CLOCKS_PER_SEC;
    std::cout << "Test Swept Volume: " << duration << "초" << std::endl;

    result->createTexture();
    result->mMesh = createBoundingBoxMesh(result->mMin, result->mMax);
    result->mPosition = glm::vec3(0.0f);
    result->mRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    return result;
}