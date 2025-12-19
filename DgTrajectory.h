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

    /*glm::mat4 getMatrix() const {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), position);
        return m * glm::mat4_cast(rotation);
    }*/
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

    //// t=0~1 사이 값으로 보간된 프레임 반환 (Swept Volume용)
    //    DgTrajectoryFrame interpolate(float t) const {
    //    if (frames.empty()) return DgTrajectoryFrame();
    //    if (frames.size() == 1 || t <= 0.0f) return frames.front();
    //    if (t >= 1.0f) return frames.back();

    //    float scaled = t * (frames.size() - 1);
    //    size_t i0 = (size_t)scaled;
    //    size_t i1 = std::min(i0 + 1, frames.size() - 1);
    //    float local = scaled - (float)i0;

    //    DgTrajectoryFrame result;
    //    result.position = glm::mix(frames[i0].position, frames[i1].position, local);
    //    result.rotation = glm::slerp(frames[i0].rotation, frames[i1].rotation, local);
    //    return result;
    //}
};


