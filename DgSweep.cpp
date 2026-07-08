#include "DgViewer.h"
#include "DgSweep.h"
#include "DgBoolean.h"
#include <algorithm>
#include <cfloat>
#include <chrono>

GLuint DgSweep::sComputeShader = 0;
GLuint DgSweep::sTransformSSBO = 0;
bool DgSweep::sInitialized = false;

GLuint DgSweep::sBrentComputeShader = 0;
GLuint DgSweep::sBrentTransformSSBO = 0;
GLuint DgSweep::sBrentDevSSBO = 0;
GLuint DgSweep::sBrentCounterSSBO = 0;
bool DgSweep::sBrentInitialized = false;

// Brent's method로 선분 위 SDF 최소값 탐색
static float brentMinimize(DgVolume* brush,
    const glm::vec3& p0, const glm::vec3& p1,
    float& outAlpha,
    int maxIter = 20, float tol = 1e-4f)
{
    auto f = [&](float alpha) -> float {
        return DgBoolean::sampleLocalSDF(brush, glm::mix(p0, p1, alpha));
        };

    float a = 0.0f, b = 1.0f;
    const float golden = 0.381966f;

    float x = a + golden * (b - a);
    float w = x, v = x;
    float fx = f(x);
    float fw = fx, fv = fx;

    float d = 0.0f;
    float e = 0.0f;

    for (int iter = 0; iter < maxIter; ++iter)
    {
        float mid = 0.5f * (a + b);
        float tol1 = tol * std::abs(x) + 1e-10f;
        float tol2 = 2.0f * tol1;

        if (std::abs(x - mid) <= (tol2 - 0.5f * (b - a)))
            break;

        bool useParabolic = false;
        float u = 0.0f;

        if (std::abs(e) > tol1)
        {
            float r = (x - w) * (fx - fv);
            float q = (x - v) * (fx - fw);
            float p = (x - v) * q - (x - w) * r;
            q = 2.0f * (q - r);

            if (q > 0.0f) p = -p;
            else q = -q;

            float etemp = e;
            e = d;

            if (std::abs(p) < std::abs(0.5f * q * etemp)
                && p > q * (a - x)
                && p < q * (b - x))
            {
                d = p / q;
                u = x + d;

                if ((u - a) < tol2 || (b - u) < tol2)
                    d = (x < mid) ? tol1 : -tol1;

                useParabolic = true;
            }
        }

        if (!useParabolic)
        {
            e = (x < mid) ? (b - x) : (a - x);
            d = golden * e;
        }

        if (std::abs(d) >= tol1)
            u = x + d;
        else
            u = x + ((d > 0.0f) ? tol1 : -tol1);

        float fu = f(u);

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

    std::vector<glm::mat4> invTransforms(samplingSteps);

    glm::vec3 combinedMin(FLT_MAX), combinedMax(-FLT_MAX);
    for (int step = 0; step < samplingSteps; ++step)
    {
        float t = (samplingSteps > 1) ? (float)step / (samplingSteps - 1) : 0.0f;
        glm::mat4 transform = trajectory.getTransformAt(t);
        glm::vec3 worldCenter = glm::vec3(transform * glm::vec4(localCenter, 1.0f));
        combinedMin = glm::min(combinedMin, worldCenter);
        combinedMax = glm::max(combinedMax, worldCenter);
        invTransforms[step] = glm::inverse(transform);
    }

    float radius = glm::length(localMax - localCenter);
    float maxScaleFactor = 1.0f;
    for (const auto& kf : trajectory.keyframes) {
        float s = std::max({ kf.scale.x, kf.scale.y, kf.scale.z });
        maxScaleFactor = std::max(maxScaleFactor, s);
    }
    combinedMin -= glm::vec3(radius * maxScaleFactor);
    combinedMax += glm::vec3(radius * maxScaleFactor);

    DgVolume* result = DgVolume::createResultVolume("Swept Volume (Brent)", resolution, combinedMin, combinedMax);

    int totalSize = resolution * resolution * resolution;
    result->mData.resize(totalSize, FLT_MAX);

    int skipCount = 0;
    int brentCount = 0;

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

                    glm::vec3 p0 = glm::vec3(invTransforms[seg] * glm::vec4(worldPos, 1.0f));
                    glm::vec3 p1 = glm::vec3(invTransforms[seg + 1] * glm::vec4(worldPos, 1.0f));

                    float sdf0 = DgBoolean::sampleLocalSDF(brush, p0);
                    float sdf1 = DgBoolean::sampleLocalSDF(brush, p1);
                    float segLength = glm::length(p1 - p0);

                    if (sdf0 > 0.0f && sdf1 > 0.0f && (sdf0 + sdf1) > segLength * 1.5f)
                    {
                        result->mData[index] = std::min(result->mData[index],
                            std::min(sdf0, sdf1));
                        skipCount++;
                        continue;
                    }

                    float bestAlpha;
                    float bestSDF = brentMinimize(brush, p0, p1, bestAlpha);
                    bestSDF = std::min(bestSDF, std::min(sdf0, sdf1));
                    result->mData[index] = std::min(result->mData[index], bestSDF);
                    brentCount++;
                }
            }
        }

        if (seg % 10 == 0)
            std::cout << "Brent: " << (seg * 100 / (samplingSteps - 1)) << "%" << std::endl;
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
    return result;
}

DgVolume* DgSweep::generateBrentGPU(DgVolume* brush,
    const DgTrajectory& trajectory,
    int resolution,
    int samplingSteps,
    bool skipReadback)
{
    if (!initializeBrentGPU()) return nullptr;

    auto cpuStart = std::chrono::high_resolution_clock::now();

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

    float maxScaleFactor = 1.0f;
    for (const auto& kf : trajectory.keyframes) {
        float s = std::max({ kf.scale.x, kf.scale.y, kf.scale.z });
        maxScaleFactor = std::max(maxScaleFactor, s);
    }
    combinedMin -= glm::vec3(radius * maxScaleFactor);
    combinedMax += glm::vec3(radius * maxScaleFactor);

    struct GPUKeyFrame {
        glm::vec4 position;
        glm::vec4 rotation;
        glm::vec4 scale;
    };

    int numKeyframes = (int)trajectory.keyframes.size();
    int numSegs = numKeyframes - 1;
    std::vector<GPUKeyFrame> gpuKFs(numKeyframes);

    for (int i = 0; i < numKeyframes; ++i) {
        gpuKFs[i].position = glm::vec4(trajectory.keyframes[i].position, 0.0f);
        glm::quat q = trajectory.keyframes[i].rotation;
        gpuKFs[i].rotation = glm::vec4(q.x, q.y, q.z, q.w);
        gpuKFs[i].scale = glm::vec4(trajectory.keyframes[i].scale, 0.0f);
    }

    const int M = 5;  // 셰이더 main()의 M과 반드시 일치해야 함
    int numSamplingSegs = samplingSteps - 1;
    int numSubSegs = numSamplingSegs * M;
    std::vector<float> devs(numSubSegs);

    for (int seg = 0; seg < numSamplingSegs; ++seg)
    {
        float segT0 = (float)seg / (float)numSamplingSegs;
        float segT1 = (float)(seg + 1) / (float)numSamplingSegs;

        for (int i = 0; i < M; ++i)
        {
            float subT0 = glm::mix(segT0, segT1, (float)i / (float)M);
            float subT1 = glm::mix(segT0, segT1, (float)(i + 1) / (float)M);
            devs[seg * M + i] = trajectory.computeSegmentDeviation(subT0, subT1);
        }
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sBrentTransformSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        gpuKFs.size() * sizeof(GPUKeyFrame),
        gpuKFs.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sBrentTransformSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sBrentDevSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        devs.size() * sizeof(float),
        devs.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, sBrentDevSSBO);

    glUseProgram(sBrentComputeShader);

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

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, brush->mTextureID);
    glUniform1i(glGetUniformLocation(sBrentComputeShader, "uBrushSDF"), 0);

    glBindImageTexture(1, resultTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);

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
    glUniform1i(glGetUniformLocation(sBrentComputeShader, "uNumSegments"), numSegs);
    glUniform1i(glGetUniformLocation(sBrentComputeShader, "uMaxBrentIter"), skipReadback ? 6 : 10);

    int numGroups = (resolution + 7) / 8;

    // 수정
    unsigned int zeros[2] = { 0, 0 };   // [0]=brentCallCount, [1]=outsideBoxCount
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sBrentCounterSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int) * 2, zeros, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, sBrentCounterSSBO);

    glDispatchCompute(numGroups, numGroups, numGroups);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    unsigned int counters[2] = { 0, 0 };
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sBrentCounterSSBO);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned int) * 2, counters);

    int totalVoxels = resolution * resolution * resolution;
    std::cout << "[BRENT CALL COUNT] total=" << counters[0]
        << ", voxel당 평균=" << (double)counters[0] / totalVoxels << std::endl;
    std::cout << "[OUTSIDE BOX COUNT] total=" << counters[1]
        << ", voxel당 평균=" << (double)counters[1] / totalVoxels
        << " (" << (100.0 * counters[1] / (totalVoxels * (double)samplingSteps)) << "% of all samples)"
        << std::endl;

    DgVolume* result = DgVolume::createResultVolume("Swept Volume (Brent GPU)", resolution, combinedMin, combinedMax);

    if (!skipReadback) {
        int totalSize = resolution * resolution * resolution;
        result->mData.resize(totalSize);
        glBindTexture(GL_TEXTURE_3D, resultTexture);
        glGetTexImage(GL_TEXTURE_3D, 0, GL_RED, GL_FLOAT, result->mData.data());

        {
            int rx = result->mDim[0];
            int ry = result->mDim[1];
            int rz = result->mDim[2];
            float maxJump = 0.0f;
            int bigJumpCount = 0;
            auto idx = [rx, ry](int x, int y, int z) { return x + rx * (y + ry * z); };

            for (int z = 1; z < rz - 1; ++z)
                for (int y = 1; y < ry - 1; ++y)
                    for (int x = 1; x < rx - 1; ++x)
                    {
                        float c = result->mData[idx(x, y, z)];
                        if (std::abs(c) > 2.0f) continue; // 표면 근처만 체크

                        float dxp = std::abs(result->mData[idx(x + 1, y, z)] - c);
                        float dxm = std::abs(result->mData[idx(x - 1, y, z)] - c);
                        float dyp = std::abs(result->mData[idx(x, y + 1, z)] - c);
                        float dym = std::abs(result->mData[idx(x, y - 1, z)] - c);
                        float dzp = std::abs(result->mData[idx(x, y, z + 1)] - c);
                        float dzm = std::abs(result->mData[idx(x, y, z - 1)] - c);

                        float localMax = std::max({ dxp, dxm, dyp, dym, dzp, dzm });
                        if (localMax > maxJump) maxJump = localMax;
                        if (localMax > 0.05f) bigJumpCount++;
                    }

            std::cout << "[JITTER CHECK] maxJump=" << maxJump
                << ", bigJumpCount(>0.05)=" << bigJumpCount << std::endl;
        }
    }

    result->mTextureID = resultTexture;

    glFinish();
    auto cpuEnd = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(cpuEnd - cpuStart).count();
    std::cout << "[BrentGPU] "
        << (skipReadback ? "Preview" : "Full")
        << " | res=" << resolution
        << " | steps=" << samplingSteps
        << " | time=" << ms << " ms" << std::endl;

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

    glm::vec3 localMin = brush->getLocalMin();
    glm::vec3 localMax = brush->getLocalMax();
    glm::vec3 localCenter = (localMin + localMax) * 0.5f;

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
    float maxScaleFactor = 1.0f;
    for (const auto& kf : trajectory.keyframes) {
        float s = std::max({ kf.scale.x, kf.scale.y, kf.scale.z });
        maxScaleFactor = std::max(maxScaleFactor, s);
    }
    combinedMin -= glm::vec3(radius * maxScaleFactor);
    combinedMax += glm::vec3(radius * maxScaleFactor);

    DgVolume* result = DgVolume::createResultVolume("Swept Volume (CPU)", resolution, combinedMin, combinedMax);

    int totalSize = resolution * resolution * resolution;
    result->mData.resize(totalSize, FLT_MAX);

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

    result->createTexture();
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

    float maxScaleFactor = 1.0f;
    for (const auto& kf : trajectory.keyframes) {
        float s = std::max({ kf.scale.x, kf.scale.y, kf.scale.z });
        maxScaleFactor = std::max(maxScaleFactor, s);
    }
    combinedMin -= glm::vec3(radius * maxScaleFactor);
    combinedMax += glm::vec3(radius * maxScaleFactor);

    std::vector<glm::mat4> invTransforms(samplingSteps);
    for (int step = 0; step < samplingSteps; ++step)
    {
        float t = (samplingSteps > 1) ? (float)step / (samplingSteps - 1) : 0.0f;
        glm::mat4 transform = trajectory.getTransformAt(t);
        invTransforms[step] = glm::inverse(transform);
    }

    glUseProgram(sComputeShader);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sTransformSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        invTransforms.size() * sizeof(glm::mat4),
        invTransforms.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sTransformSSBO);

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

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, brush->mTextureID);
    glUniform1i(glGetUniformLocation(sComputeShader, "uBrushSDF"), 0);

    glBindImageTexture(1, resultTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);

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

    int numGroups = (resolution + 7) / 8;
    glDispatchCompute(numGroups, numGroups, numGroups);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    DgVolume* result = DgVolume::createResultVolume("Swept Volume (GPU)", resolution, combinedMin, combinedMax);

    int totalSize = resolution * resolution * resolution;
    result->mData.resize(totalSize);
    glBindTexture(GL_TEXTURE_3D, resultTexture);
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RED, GL_FLOAT, result->mData.data());

    result->mTextureID = resultTexture;

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

    sComputeShader = loadComputeShader(".\\shaders\\sweeping.comp");
    if (sComputeShader == 0) {
        std::cerr << "Sweep Compute Shader 초기화 실패" << std::endl;
        return false;
    }

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
    glGenBuffers(1, &sBrentDevSSBO);
    glGenBuffers(1, &sBrentCounterSSBO);

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