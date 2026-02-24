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
    std::vector<DgTrajectoryFrame> controlPoints;   // 컨트롤 포인트

    void clear() { frames.clear(); controlPoints.clear(); }
    size_t size() const { return frames.size(); }
    bool empty() const { return frames.empty(); }

    void addFrame(glm::vec3 pos, glm::quat rot) {
        frames.emplace_back(pos, rot);
    }

    // t (0~1)에서의 변환 행렬 반환
    glm::mat4 getTransformAt(float t) const {
        if (frames.empty()) return glm::mat4(1.0f);

        // 컨트롤 포인트가 있으면 베지어 곡선 직접 계산
        if (controlPoints.size() >= 4) {
            t = glm::clamp(t, 0.0f, 1.0f);
            glm::vec3 pos = cubicBezier(
                controlPoints[0].position, controlPoints[1].position,
                controlPoints[2].position, controlPoints[3].position, t);
            glm::quat rot = glm::slerp(controlPoints[0].rotation, controlPoints[3].rotation, t);
            return glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot);
        }

        if (frames.size() == 1) {
            return glm::translate(glm::mat4(1.0f), frames[0].position)
                * glm::mat4_cast(frames[0].rotation); // 이동 * 회전
        }

        // t를 프레임 인덱스로 변환
        t = glm::clamp(t, 0.0f, 1.0f);
        float idx = t * (frames.size() - 1);
        int i0 = (int)floor(idx);
        int i1 = glm::min(i0 + 1, (int)frames.size() - 1);
        float alpha = idx - i0;

        // 선형 보간 (위치) + SLERP (회전)
        glm::vec3 pos = glm::mix(frames[i0].position, frames[i1].position, alpha);
        glm::quat rot = glm::slerp(frames[i0].rotation, frames[i1].rotation, alpha);

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
        glm::quat baseRot(1.0f, 0.0f, 0.0f, 0.0f);
        addFrame(startPos, baseRot);
        addFrame(endPos, baseRot);
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

    // 컨트롤 포인트 4개로 베지어 곡선 생성
    void generateCurve(const glm::vec3& center, int numSamples = 64)
    {
        clear();
        glm::quat baseRot(1.0f, 0.0f, 0.0f, 0.0f);

        // 컨트롤 포인트 4개 (S자 형태)
        glm::vec3 p0 = center;
        glm::vec3 p1 = center + glm::vec3(3.0f, 0.0f, 4.0f);
        glm::vec3 p2 = center + glm::vec3(7.0f, 0.0f, -4.0f);
        glm::vec3 p3 = center + glm::vec3(20.0f, 0.0f, 0.0f);

		// 4개 컨트롤 포인트 저장 
        controlPoints.emplace_back(p0, baseRot);
        controlPoints.emplace_back(p1, baseRot);
        controlPoints.emplace_back(p2, baseRot);
        controlPoints.emplace_back(p3, baseRot);

        rebuild(numSamples);
    }

    // controlPoints로부터 frames 재생성
    void rebuild(int numSamples = 64)
    {
        if (controlPoints.size() < 4) return;
        frames.clear();

        glm::quat baseRot(1.0f, 0.0f, 0.0f, 0.0f);
        for (int i = 0; i < numSamples; ++i)
        {
            float t = (float)i / (numSamples - 1);
            glm::vec3 pos = cubicBezier(
                controlPoints[0].position, controlPoints[1].position,
                controlPoints[2].position, controlPoints[3].position, t);
            frames.emplace_back(pos, baseRot);
        }
    }
};


