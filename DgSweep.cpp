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
    // 준비: 그리드 크기 / spacing 읽기
    const int nx = vol->mDim[0];
    const int ny = vol->mDim[1];
    const int nz = vol->mDim[2];

    // mData 레이아웃: idx = x + y*nx + z*nx*ny  (x-major)
    // spacing은 축별로 다를 수 있으므로 각각 읽음
    const float hx = (float)vol->mSpacing[0];
    const float hy = (float)vol->mSpacing[1];
    const float hz = (float)vol->mSpacing[2];

    std::vector<float>& data = vol->mData;
    const int total = nx * ny * nz;

    // ---------------------------------------------------------
    // Step 1. 부호 추출 → unsigned distance grid 초기화
    //
    //   pseudo-SDF는 내부 음수 / 외부 양수를 가짐.
    //   FSM은 unsigned distance를 전파하는 알고리즘이므로
    //   부호를 분리해 저장하고 절댓값으로 작업한다.
    //   (동료 코드의 핵심 누락 부분)
    // ---------------------------------------------------------
    std::vector<float> signGrid(total);
    for (int i = 0; i < total; ++i)
    {
        signGrid[i] = (data[i] >= 0.0f) ? 1.0f : -1.0f;
        data[i] = std::abs(data[i]);
    }

    // ---------------------------------------------------------
    // Step 2. Zero-crossing seed 초기화
    //
    //   FSM의 경계조건 = zero level set 위의 참 거리값.
    //   pseudo-SDF의 절댓값을 그대로 쓰면 부정확한 경계조건이 되므로,
    //   부호가 바뀌는 이웃 쌍을 찾아 선형보간으로
    //   정확한 zero-crossing 거리를 심는다.
    //
    //   보간 공식 (x축 예시):
    //     d = |f(x)| / (|f(x)| + |f(x+1)|) * hx
    //   → 현재 복셀에서 zero crossing까지의 거리
    // ---------------------------------------------------------
    std::vector<float> fsm(total, FLT_MAX);  // FSM 작업 배열

    auto idx = [&](int x, int y, int z) -> int {
        return x + y * nx + z * nx * ny;
        };

    // X 방향 이웃
    for (int z = 0; z < nz; ++z)
        for (int y = 0; y < ny; ++y)
            for (int x = 0; x < nx - 1; ++x)
            {
                int i0 = idx(x, y, z);
                int i1 = idx(x + 1, y, z);
                if (signGrid[i0] != signGrid[i1])  // 부호 변화 = zero crossing
                {
                    float a = std::abs(data[i0]);
                    float b = std::abs(data[i1]);
                    float t = a / (a + b);         // 보간 비율
                    fsm[i0] = std::min(fsm[i0], t * hx);
                    fsm[i1] = std::min(fsm[i1], (1.0f - t) * hx);
                }
            }

    // Y 방향 이웃
    for (int z = 0; z < nz; ++z)
        for (int y = 0; y < ny - 1; ++y)
            for (int x = 0; x < nx; ++x)
            {
                int i0 = idx(x, y, z);
                int i1 = idx(x, y + 1, z);
                if (signGrid[i0] != signGrid[i1])
                {
                    float a = std::abs(data[i0]);
                    float b = std::abs(data[i1]);
                    float t = a / (a + b);
                    fsm[i0] = std::min(fsm[i0], t * hy);
                    fsm[i1] = std::min(fsm[i1], (1.0f - t) * hy);
                }
            }

    // Z 방향 이웃
    for (int z = 0; z < nz - 1; ++z)
        for (int y = 0; y < ny; ++y)
            for (int x = 0; x < nx; ++x)
            {
                int i0 = idx(x, y, z);
                int i1 = idx(x, y, z + 1);
                if (signGrid[i0] != signGrid[i1])
                {
                    float a = std::abs(data[i0]);
                    float b = std::abs(data[i1]);
                    float t = a / (a + b);
                    fsm[i0] = std::min(fsm[i0], t * hz);
                    fsm[i1] = std::min(fsm[i1], (1.0f - t) * hz);
                }
            }

    // ---------------------------------------------------------
    // Step 3. Fast Sweeping (Zhao 2005 — 3D Godunov upwind)
    //
    //   8방향 Gauss-Seidel sweep을 수행한다.
    //   각 방향에서 upwind 이웃 (a, b, c) 을 읽고
    //   Godunov scheme으로 새 거리값을 제안한다:
    //
    //   정렬 후 v1 ≤ v2 ≤ v3 에 대해:
    //     1D: u = v1 + h
    //     2D: u = (v1+v2 + sqrt(2h²-(v1-v2)²)) / 2   (v1+h > v2 일 때)
    //     3D: u = (v1+v2+v3 + sqrt(...)) / 3           (2D sol > v3 일 때)
    //
    //   spacing이 축별로 다르므로 h 대신 hx/hy/hz를 사용하는
    //   anisotropic Godunov 수식을 적용한다.
    // ---------------------------------------------------------

    // anisotropic 3D Godunov solver
    // v[0..2]: 정렬된 upwind 값, h[0..2]: 대응하는 spacing
    // 정렬 시 (값, spacing) 쌍을 함께 정렬해야 함
    auto godunov3D = [](float va, float vb, float vc,
        float ha, float hb, float hc) -> float
        {
            // (값, spacing) 쌍으로 묶어 값 기준 정렬
            struct VS { float v, h; };
            VS s[3] = { {va, ha}, {vb, hb}, {vc, hc} };
            // 버블 정렬 (3개)
            if (s[0].v > s[1].v) std::swap(s[0], s[1]);
            if (s[1].v > s[2].v) std::swap(s[1], s[2]);
            if (s[0].v > s[1].v) std::swap(s[0], s[1]);

            float v1 = s[0].v, h1 = s[0].h;
            float v2 = s[1].v, h2 = s[1].h;
            float v3 = s[2].v, h3 = s[2].h;

            // 1D 시도
            float u = v1 + h1;
            if (u <= v2) return u;

            // 2D 시도: (u-v1)²/h1² + (u-v2)²/h2² = 1
            float A2 = 1.0f / (h1 * h1) + 1.0f / (h2 * h2);
            float B2 = -2.0f * (v1 / (h1 * h1) + v2 / (h2 * h2));
            float C2 = v1 * v1 / (h1 * h1) + v2 * v2 / (h2 * h2) - 1.0f;
            float disc2 = B2 * B2 - 4.0f * A2 * C2;
            if (disc2 >= 0.0f) {
                u = (-B2 + std::sqrt(disc2)) / (2.0f * A2);
                if (u <= v3) return u;
            }

            // 3D: (u-v1)²/h1² + (u-v2)²/h2² + (u-v3)²/h3² = 1
            float A3 = 1.0f / (h1 * h1) + 1.0f / (h2 * h2) + 1.0f / (h3 * h3);
            float B3 = -2.0f * (v1 / (h1 * h1) + v2 / (h2 * h2) + v3 / (h3 * h3));
            float C3 = v1 * v1 / (h1 * h1) + v2 * v2 / (h2 * h2) + v3 * v3 / (h3 * h3) - 1.0f;
            float disc3 = B3 * B3 - 4.0f * A3 * C3;
            if (disc3 >= 0.0f) {
                return (-B3 + std::sqrt(disc3)) / (2.0f * A3);
            }

            // fallback (수치 오차 방어)
            return u;
        };

    // 8방향 sweep
    for (int sweep = 0; sweep < 8; ++sweep)
    {
        int sx = (sweep & 1) ? -1 : 1;
        int sy = (sweep & 2) ? -1 : 1;
        int sz = (sweep & 4) ? -1 : 1;

        int x0 = (sx == 1) ? 0 : nx - 1;
        int y0 = (sy == 1) ? 0 : ny - 1;
        int z0 = (sz == 1) ? 0 : nz - 1;
        int xe = (sx == 1) ? nx : -1;
        int ye = (sy == 1) ? ny : -1;
        int ze = (sz == 1) ? nz : -1;

        for (int z = z0; z != ze; z += sz)
            for (int y = y0; y != ye; y += sy)
                for (int x = x0; x != xe; x += sx)
                {
                    int i = idx(x, y, z);

                    // upwind 이웃 (현재 sweep 방향에서 "이미 업데이트된" 쪽)
                    float va = (x - sx >= 0 && x - sx < nx)
                        ? fsm[idx(x - sx, y, z)] : FLT_MAX;
                    float vb = (y - sy >= 0 && y - sy < ny)
                        ? fsm[idx(x, y - sy, z)] : FLT_MAX;
                    float vc = (z - sz >= 0 && z - sz < nz)
                        ? fsm[idx(x, y, z - sz)] : FLT_MAX;

                    // FLT_MAX 이웃이 있으면 유효한 1D/2D/3D 시도만 할 수 있음
                    // godunov3D 내부에서 정렬 후 처리하므로 그대로 넘겨도 안전
                    // (FLT_MAX + h ≈ FLT_MAX → 자동으로 낮은 차수 선택)
                    float u_new = godunov3D(va, vb, vc, hx, hy, hz);
                    fsm[i] = std::min(fsm[i], u_new);
                }
    }

    // Step 4. 부호 복원 + mData 갱신 + GPU 텍스처 재업로드
    for (int i = 0; i < total; ++i)
    {
        data[i] = signGrid[i] * fsm[i];
    }

    // GPU 텍스처에 반영 (createTexture는 mData를 읽어 재업로드)
    vol->createTexture();

    std::cout << "[FastSweeping] Done. ("
        << nx << "x" << ny << "x" << nz << ")" << std::endl;
}