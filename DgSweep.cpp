#include "DgViewer.h"
#include "DgSweep.h"
#include "DgBoolean.h"
#include <algorithm>
#include <cfloat>

GLuint DgSweep::sComputeShader = 0;
GLuint DgSweep::sTransformSSBO = 0;
bool DgSweep::sInitialized = false;

GLuint DgSweep::sBrentComputeShader = 0;
GLuint DgSweep::sBrentTransformSSBO = 0;
bool DgSweep::sBrentInitialized = false;

// Brent's method로 선분 위 SDF 최소값 탐색
static float brentMinimize(DgVolume* brush,
    const glm::vec3& p0, const glm::vec3& p1,
    float& outAlpha,
    int maxIter = 20, float tol = 1e-4f)
{
    // SDF 평가 람다
    auto f = [&](float alpha) -> float {
        return DgBoolean::sampleLocalSDF(brush, glm::mix(p0, p1, alpha));
        };

    float a = 0.0f, b = 1.0f;

    // Golden ratio
    const float golden = 0.381966f;  // (3 - sqrt(5)) / 2

    // 초기 내부점: 구간의 golden section 위치
    float x = a + golden * (b - a);
    float w = x, v = x;
    float fx = f(x);
    float fw = fx, fv = fx;

    float d = 0.0f;   // 이전 스텝 크기
    float e = 0.0f;   // 그 이전 스텝 크기

    for (int iter = 0; iter < maxIter; ++iter)
    {
        float mid = 0.5f * (a + b);
        float tol1 = tol * std::abs(x) + 1e-10f;
        float tol2 = 2.0f * tol1;

        // 수렴 확인
        if (std::abs(x - mid) <= (tol2 - 0.5f * (b - a)))
            break;

        bool useParabolic = false;
        float u = 0.0f;

        // 포물선 보간 시도
        if (std::abs(e) > tol1)
        {
            // x, w, v 세 점으로 포물선 피팅
            float r = (x - w) * (fx - fv);
            float q = (x - v) * (fx - fw);
            float p = (x - v) * q - (x - w) * r;
            q = 2.0f * (q - r);

            if (q > 0.0f) p = -p;
            else q = -q;

            float etemp = e;
            e = d;

            // 포물선 스텝이 유효한지 확인
            if (std::abs(p) < std::abs(0.5f * q * etemp)
                && p > q * (a - x)
                && p < q * (b - x))
            {
                // 포물선 스텝 채택
                d = p / q;
                u = x + d;

                // 경계에 너무 가까우면 보정
                if ((u - a) < tol2 || (b - u) < tol2)
                    d = (x < mid) ? tol1 : -tol1;

                useParabolic = true;
            }
        }

        // 포물선 실패 → Golden Section
        if (!useParabolic)
        {
            // x(0.382) < mid(0.5)? → YES → 오른쪽이 넓음
            e = (x < mid) ? (b - x) : (a - x); // e = 1 - 0.382 = 0.618
			d = golden * e;                    // d = 0.382 * 0.618 = 0.236
        }

        // 새 평가점
        if (std::abs(d) >= tol1)
            u = x + d;                         // u = 0.382 + 0.236 = 0.618
        else
            u = x + ((d > 0.0f) ? tol1 : -tol1);

        float fu = f(u);

        // 구간 및 최적점 업데이트
        if (fu <= fx)
        {
            if (u < x) b = x;
            else a = x;

            v = w;  fv = fw;
            w = x;  fw = fx;
            x = u;  fx = fu;
        }
        else
        {
            if (u < x) a = u;
            else b = u;

            if (fu <= fw || w == x)
            {
                v = w;  fv = fw;
                w = u;  fw = fu;
            }
            else if (fu <= fv || v == x || v == w)
            {
                v = u;  fv = fu;
            }
        }
    }

    outAlpha = x;
    return fx;
}

DgVolume* DgSweep::generateBrentCPU(DgVolume* brush,
    const DgTrajectory& trajectory,
    int resolution,
    int samplingSteps)
{
    clock_t start = clock();

    glm::vec3 localMin = brush->getLocalMin();
    glm::vec3 localMax = brush->getLocalMax();
    glm::vec3 localCenter = (localMin + localMax) * 0.5f;

    // 전체 바운딩 박스 계산
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

    DgVolume* result = new DgVolume();
    result->mName = "Swept Volume (Brent)";
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

    // 역변환 사전 계산
    std::vector<glm::mat4> invTransforms(samplingSteps);
    for (int step = 0; step < samplingSteps; ++step)
    {
        float t = (samplingSteps > 1) ? (float)step / (samplingSteps - 1) : 0.0f;
        invTransforms[step] = glm::inverse(trajectory.getTransformAt(t));
    }

    // 통계
    int skipCount = 0;
    int brentCount = 0;

    // --- 메인 루프: 세그먼트 순회 ---
    for (int seg = 0; seg < samplingSteps - 1; ++seg)
    {
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

                    int index = i + j * resolution + k * resolution * resolution;

                    // 역궤적 선분 생성
                    glm::vec3 p0 = glm::vec3(invTransforms[seg] * glm::vec4(worldPos, 1.0f));
                    glm::vec3 p1 = glm::vec3(invTransforms[seg + 1] * glm::vec4(worldPos, 1.0f));

                    // 양 끝점 SDF
                    float sdf0 = DgBoolean::sampleLocalSDF(brush, p0);
                    float sdf1 = DgBoolean::sampleLocalSDF(brush, p1);
                    float segLength = glm::length(p1 - p0);

                    // 외부 판별
                    if (sdf0 > 0.0f && sdf1 > 0.0f && (sdf0 + sdf1) > segLength)
                    {
                        result->mData[index] = std::min(result->mData[index],
                            std::min(sdf0, sdf1));
                        skipCount++;
                        continue;
                    }

                    // Brent's method로 최소값 탐색
                    float bestAlpha;
                    float bestSDF = brentMinimize(brush, p0, p1, bestAlpha);

                    // 양 끝점과도 비교
                    bestSDF = std::min(bestSDF, std::min(sdf0, sdf1));

                    result->mData[index] = std::min(result->mData[index], bestSDF);
                    brentCount++;
                }
            }
        }

        if (seg % 10 == 0)
        {
            std::cout << "Brent: " << (seg * 100 / (samplingSteps - 1)) << "%" << std::endl;
        }
    }

    clock_t finish = clock();
    double duration = (double)(finish - start) / CLOCKS_PER_SEC;

    int totalVoxelSeg = (samplingSteps - 1) * resolution * resolution * resolution;
    std::cout << "Brent Swept Volume: " << duration << " sec" << std::endl;
    std::cout << "  Skipped (Lipschitz): " << skipCount
        << " (" << (100.0 * skipCount / totalVoxelSeg) << "%)" << std::endl;
    std::cout << "  Brent evaluated:     " << brentCount
        << " (" << (100.0 * brentCount / totalVoxelSeg) << "%)" << std::endl;

    result->createTexture();
    result->mMesh = createBoundingBoxMesh(result->mMin, result->mMax);
    result->mPosition = glm::vec3(0.0f);
    result->mRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    return result;
}

DgVolume* DgSweep::generateBrentGPU(DgVolume* brush,
    const DgTrajectory& trajectory,
    int resolution,
    int samplingSteps)
{
    if (!initializeBrentGPU()) return nullptr;

    clock_t start = clock();

    // 바운딩 박스 계산
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

    // 역변환 행렬
    std::vector<glm::mat4> invTransforms(samplingSteps);
    for (int step = 0; step < samplingSteps; ++step)
    {
        float t = (samplingSteps > 1) ? (float)step / (samplingSteps - 1) : 0.0f;
        invTransforms[step] = glm::inverse(trajectory.getTransformAt(t));
    }

    // Compute Shader 실행
    glUseProgram(sBrentComputeShader);

    // SSBO에 변환 행렬 업로드
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sBrentTransformSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        invTransforms.size() * sizeof(glm::mat4),
        invTransforms.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sBrentTransformSSBO);

    // 결과 3D 텍스처 생성
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

    // brush SDF 텍스처 (읽기)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, brush->mTextureID);
    glUniform1i(glGetUniformLocation(sBrentComputeShader, "uBrushSDF"), 0);

    // 결과 텍스처 (쓰기)
    glBindImageTexture(1, resultTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);

    // Uniform 전달
    glUniform3f(glGetUniformLocation(sBrentComputeShader, "uVolumeMin"),
        combinedMin.x, combinedMin.y, combinedMin.z);
    glUniform3f(glGetUniformLocation(sBrentComputeShader, "uVolumeMax"),
        combinedMax.x, combinedMax.y, combinedMax.z);
    glUniform3i(glGetUniformLocation(sBrentComputeShader, "uResolution"),
        resolution, resolution, resolution);

    glUniform3f(glGetUniformLocation(sBrentComputeShader, "uBrushMin"),
        localMin.x, localMin.y, localMin.z);
    glUniform3f(glGetUniformLocation(sBrentComputeShader, "uBrushMax"),
        localMax.x, localMax.y, localMax.z);

    glUniform1i(glGetUniformLocation(sBrentComputeShader, "uSamplingSteps"), samplingSteps);

    // Dispatch
    int numGroups = (resolution + 7) / 8;
    glDispatchCompute(numGroups, numGroups, numGroups);

    // GPU 완료 대기
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // 결과 볼륨 생성
    DgVolume* result = new DgVolume();
    result->mName = "Swept Volume (Brent GPU)";
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

    result->mTextureID = resultTexture;

    result->mMesh = createBoundingBoxMesh(result->mMin, result->mMax);
    result->mPosition = glm::vec3(0.0f);
    result->mRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    clock_t finish = clock();
    double duration = (double)(finish - start) / CLOCKS_PER_SEC;
    std::cout << "Brent GPU Swept Volume: " << duration << " sec" << std::endl;

    glUseProgram(0);
    return result;
}

DgVolume* DgSweep::generateSweptVolume(
    DgVolume* brush,
    const DgTrajectory& trajectory,
    int resolution,
    int samplingSteps,
    bool useGPU)
{
    if (!brush || trajectory.size() < 2) return nullptr;

    if (useGPU) {
        return generateGPU(brush, trajectory, resolution, samplingSteps);
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

bool DgSweep::initializeBrentGPU()
{
    if (sBrentInitialized) return true;

    sBrentComputeShader = loadComputeShader(".\\shaders\\sweeping_brent.comp");
    if (sBrentComputeShader == 0) {
        std::cerr << "Brent Compute Shader 초기화 실패" << std::endl;
        return false;
    }

    glGenBuffers(1, &sBrentTransformSSBO);

    sBrentInitialized = true;
    return true;
}

void DgSweep::fastSweeping(DgVolume* vol)
{
    glm::ivec3 res(vol->mDim[0], vol->mDim[1], vol->mDim[2]);
    float space = (float)vol->mSpacing[0];
    std::vector<float>& grid = vol->mData;
 
    int start[3], end[3], step[3];
    for (int i = 0; i < 8; ++i) {
        step[0] = (i & 1) ? -1 : 1;
        step[1] = (i & 2) ? -1 : 1;
        step[2] = (i & 4) ? -1 : 1;

        start[0] = (step[0] == 1) ? 0 : res.x - 1;
        start[1] = (step[1] == 1) ? 0 : res.y - 1;
        start[2] = (step[2] == 1) ? 0 : res.z - 1;

        end[0] = (step[0] == 1) ? res.x : -1;
        end[1] = (step[1] == 1) ? res.y : -1;
        end[2] = (step[2] == 1) ? res.z : -1;

        for (int z = start[2]; z != end[2]; z += step[2]) {
            for (int y = start[1]; y != end[1]; y += step[1]) {
                for (int x = start[0]; x != end[0]; x += step[0]) {
                    size_t idx = (size_t)z * res.y * res.x + y * res.x + x;

                    float a = (x - step[0] >= 0 && x - step[0] < res.x) ? grid[idx - step[0]] : FLT_MAX;
                    float b = (y - step[1] >= 0 && y - step[1] < res.y) ? grid[idx - (size_t)step[1] * res.x] : FLT_MAX;
                    float c = (z - step[2] >= 0 && z - step[2] < res.z) ? grid[idx - (size_t)step[2] * res.x * res.y] : FLT_MAX;
                    
                    float u_new = grid[idx];
                    float h = space;
                    
                    float v[3] = { a, b, c };
                    std::sort(v, v + 3);
                    float v1 = v[0], v2 = v[1], v3 = v[2];
                    
                    float x_sol = v1 + h;
                    if (x_sol <= v2) {
                        u_new = x_sol;
                    }
                    else {
                        x_sol = (v1 + v2 + sqrt(2.0f * h * h - pow(v1 - v2, 2))) / 2.0f;
                        if (x_sol <= v3) {
                            u_new = x_sol;
                        }
                        else {
                            float b_sum = v1 + v2 + v3;
                            float c_sum = v1 * v1 + v2 * v2 + v3 * v3 - h * h;
                            u_new = (b_sum + sqrt(b_sum * b_sum - 3.0f * c_sum)) / 3.0f;
                        }
                    }
                    grid[idx] = std::min(grid[idx], u_new);
                }
            }
        }
    }
 
    vol->createTexture();
 
    std::cout << "[FastSweeping] Done. ("
        << res.x << "x" << res.y << "x" << res.z << ")" << std::endl;
}