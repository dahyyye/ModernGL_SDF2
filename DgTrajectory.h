#pragma once
#include "DgViewer.h"

// 궤적 프레임: 위치 + 회전
struct DgTrajectoryFrame
{
    glm::vec3 position;
    glm::quat rotation;

    DgTrajectoryFrame()
        : position(0.0f), rotation(1.0f, 0.0f, 0.0f, 0.0f) {
    }

    DgTrajectoryFrame(glm::vec3 pos, glm::quat rot)
        : position(pos), rotation(rot) {
    }
};

class DgTrajectory
{
public:
    std::vector<DgTrajectoryFrame> frames;
    std::vector<DgTrajectoryFrame> keyframes; 
    std::vector<DgTrajectoryFrame> controlPoints; 
	bool mIsLinear = false; // 선형 궤적 여부

    void clear() { 
        frames.clear(); 
        keyframes.clear();  
        controlPoints.clear(); 
        mIsLinear = false;
     }

    size_t size() const { return frames.size(); }
    bool empty() const { return frames.empty(); }

    void addFrame(glm::vec3 pos, glm::quat rot) {
        frames.emplace_back(pos, rot);
    }

    // t (0~1)에서의 변환 행렬 반환
    glm::mat4 getTransformAt(float t) const {
        
		int numSegs = (int)controlPoints.size() / 4;

        if (numSegs == 0)
        {
            if (frames.empty()) return glm::mat4(1.0f);
            if (frames.size() == 1)
                return glm::translate(glm::mat4(1.0f), frames[0].position) * glm::mat4_cast(frames[0].rotation);

            t = glm::clamp(t, 0.0f, 1.0f);
            float idx = t * (frames.size() - 1);
            int   i0 = (int)floor(idx);
            int   i1 = glm::min(i0 + 1, (int)frames.size() - 1);
            float alpha = idx - i0;
            glm::vec3 pos = glm::mix(frames[i0].position, frames[i1].position, alpha);
            glm::quat rot = glm::slerp(frames[i0].rotation, frames[i1].rotation, alpha);
            return glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot);
        }

        // 세그먼트 인덱싱
        t = glm::clamp(t, 0.0f, 1.0f);
        float scaled = t * numSegs;
        int   seg = glm::clamp((int)scaled, 0, numSegs - 1);
        float lt = scaled - (float)seg;
        int   base = seg * 4;

        glm::vec3 pos = cubicBezier(
            controlPoints[base + 0].position, controlPoints[base + 1].position,
            controlPoints[base + 2].position, controlPoints[base + 3].position, lt);
        glm::quat rot = glm::slerp(
            controlPoints[base + 0].rotation,
            controlPoints[base + 3].rotation, lt);

        return glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot);
    }

    /*!
     *  \brief  직선 궤적 생성 (시작점, 끝점 2개만 저장)
     *  \param  startPos  시작 위치
     *  \param  endPos    끝 위치
     */
    void generateLinear(const glm::vec3& startPos, const glm::vec3& endPos)
    {
        clear();
        mIsLinear = true; 
        glm::quat baseRot(1.0f, 0.0f, 0.0f, 0.0f);
        keyframes.emplace_back(startPos, baseRot);
        keyframes.emplace_back(endPos, baseRot);
        rebuild();
    }

    /*!
	 *  \brief  4개 컨트롤 포인트로 S자 형태의 베지어 곡선 생성
	 *  \param  p0  시작 위치
	 *  \param  p1  컨트롤 포인트 1
	 *  \param  p2  컨트롤 포인트 2
	 *  \param  p3  끝 위치
     *  \param  t   0~1 사이의 보간 인자
     * 
     *  \return t에서의 위치
     * 
	 *  \note   회전은 선형 보간으로 고정 (baseRot)
    */
    static glm::vec3 cubicBezier(const glm::vec3& p0, const glm::vec3& p1,
        const glm::vec3& p2, const glm::vec3& p3, float t)
    {
        float u = 1.0f - t;
        return u * u * u * p0 + 3.0f * u * u * t * p1 + 3.0f * u * t * t * p2 + t * t * t * p3;
    }

    // 곡선 생성
    void generateCurve(const glm::vec3& center, int numSamples = 64)
    {
        clear();
        glm::quat baseRot(1.0f, 0.0f, 0.0f, 0.0f);

        keyframes.emplace_back(center, baseRot);
        keyframes.emplace_back(center + glm::vec3(5.0f, 0.0f, 4.0f), baseRot);
        keyframes.emplace_back(center + glm::vec3(13.0f, 0.0f, -3.0f), baseRot);
        keyframes.emplace_back(center + glm::vec3(20.0f, 0.0f, 0.0f), baseRot);

        rebuild(numSamples);
    }

    // controlPoints로부터 frames 재생성
    void rebuild(int numSamples = 64)
    {
        catmullRomToSegments();            // keyframes → controlPoints 자동 계산
        int numSegs = (int)controlPoints.size() / 4;
        if (numSegs == 0) return;
        frames.clear();

        for (int i = 0; i < numSamples; ++i)
        {
            float t = (float)i / (numSamples - 1);
            float scaled = t * numSegs;
            int   seg = glm::clamp((int)scaled, 0, numSegs - 1);
            float lt = scaled - (float)seg;     // 해당 세그먼트 내 로컬 t
            int   base = seg * 4;

            glm::vec3 pos = cubicBezier(
                controlPoints[base + 0].position, controlPoints[base + 1].position,
                controlPoints[base + 2].position, controlPoints[base + 3].position, lt);
            glm::quat rot = glm::slerp(
                controlPoints[base + 0].rotation,
                controlPoints[base + 3].rotation, lt);
            frames.emplace_back(pos, rot);
        }
    }

    void saveToFile(const char* filename) const
    {
        std::ofstream f(filename);
        f << keyframes.size() << "\n";
        for (auto& kf : keyframes)
            f << kf.position.x << " " << kf.position.y << " " << kf.position.z << " "
            << kf.rotation.w << " " << kf.rotation.x << " " << kf.rotation.y << " " << kf.rotation.z << "\n";
        std::cout << filename << " 저장 완료" << std::endl;
    }

    void loadFromFile(const char* filename)
    {
        std::ifstream f(filename);
        int n; f >> n;
        keyframes.clear();
        for (int i = 0; i < n; ++i) {
            glm::vec3 pos; glm::quat rot;
            f >> pos.x >> pos.y >> pos.z >> rot.w >> rot.x >> rot.y >> rot.z;
            keyframes.emplace_back(pos, rot);
        }
        rebuild();  // keyframes → controlPoints → frames 순으로 자동 계산
    }

    void catmullRomToSegments()
    {
        controlPoints.clear();
        int N = (int)keyframes.size();
        if (N < 2) return;

        if (mIsLinear)   // ← 추가: 직선 제어점 생성
        {
            for (int i = 0; i < N - 1; ++i)
            {
                glm::vec3 k0 = keyframes[i].position;
                glm::vec3 k1 = keyframes[i + 1].position;
                glm::quat r0 = keyframes[i].rotation;
                glm::quat r1 = keyframes[i + 1].rotation;

                controlPoints.emplace_back(k0, r0);
                controlPoints.emplace_back(k0 + (k1 - k0) / 3.0f, glm::slerp(r0, r1, 1.0f / 3.0f));
                controlPoints.emplace_back(k0 + (k1 - k0) * 2.0f / 3.0f, glm::slerp(r0, r1, 2.0f / 3.0f));
                controlPoints.emplace_back(k1, r1);
            }
            return;
        }

        for (int i = 0; i < N - 1; ++i)
        {
            // Ki-1: 없으면 phantom
            glm::vec3 km1 = (i == 0)
                ? (2.0f * keyframes[0].position - keyframes[1].position)
                : keyframes[i - 1].position;

            glm::vec3 k0 = keyframes[i].position;
            glm::vec3 k1 = keyframes[i + 1].position;

            // Ki+2: 없으면 phantom
            glm::vec3 k2 = (i + 2 < N)
                ? keyframes[i + 2].position
                : (2.0f * keyframes[N - 1].position - keyframes[N - 2].position);

            // 베지어 제어점 계산
            glm::vec3 p0 = k0;
            glm::vec3 p1 = k0 + (k1 - km1) / 6.0f;
            glm::vec3 p2 = k1 - (k2 - k0) / 6.0f;
            glm::vec3 p3 = k1;

            // 회전: 이 세그먼트의 시작/끝 키프레임 회전
            glm::quat r0 = keyframes[i].rotation;
            glm::quat r3 = keyframes[i + 1].rotation;

            // 베지어 내부 제어점 회전은 1/3, 2/3 보간
            glm::quat r1 = glm::slerp(r0, r3, 1.0f / 3.0f);
            glm::quat r2 = glm::slerp(r0, r3, 2.0f / 3.0f);

            controlPoints.emplace_back(p0, r0);
            controlPoints.emplace_back(p1, r1);
            controlPoints.emplace_back(p2, r2);
            controlPoints.emplace_back(p3, r3);
        }
    }
};


