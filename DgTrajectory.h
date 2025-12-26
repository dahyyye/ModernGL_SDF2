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

    void clear() { frames.clear(); }
    size_t size() const { return frames.size(); }
    bool empty() const { return frames.empty(); }

    void addFrame(glm::vec3 pos, glm::quat rot) {
        frames.emplace_back(pos, rot);
    }

    // t (0~1)에서의 변환 행렬 반환
    glm::mat4 getTransformAt(float t) const {
        if (frames.empty()) return glm::mat4(1.0f);
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
};


