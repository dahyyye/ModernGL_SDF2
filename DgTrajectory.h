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
	std::vector<DgTrajectoryFrame> frames;          // 균일 샘플링된 최종 궤적 프레임
	std::vector<DgTrajectoryFrame> keyframes;       // 사용자가 지정한 키프레임
	std::vector<DgTrajectoryFrame> controlPoints;   // 베지어 곡선 제어점
	bool mIsLinear = false;                         // 선형 궤적 여부

    /*!
     *  \brief  모든 데이터 초기화
     */
    void clear() { 
        frames.clear(); 
        keyframes.clear();  
        controlPoints.clear(); 
        mIsLinear = false;
     }

	size_t size() const { return frames.size(); }   // 샘플링된 프레임 수 반환
	bool empty() const { return frames.empty(); }   // 프레임이 없는지 여부 반환

    /*!
     *  \brief  t (0~1) 에서의 변환 행렬 반환
     *  \param  t   0~1 범위의 궤적 파라미터
     *  \return t에서의 위치 + 회전을 담은 4x4 변환 행렬
     *  \note   controlPoints가 채워져 있어야 유효한 결과를 반환한다.
     *          rebuild() 호출 이후에 사용할 것.
     */
    glm::mat4 getTransformAt(float t) const {
        
		int numSegs = (int)controlPoints.size() / 4;

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
     *  \brief  직선 궤적 생성
     *  \param  startPos  시작 위치
     *  \param  endPos    끝 위치
     *  \note   mIsLinear = true로 설정되어 keyframe 추가 시에도 직선을 유지한다.
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
     *  \brief  Catmull-Rom 곡선 궤적 생성
     *  \param  center      궤적의 시작 위치
     *  \param  numSamples  frames 샘플링 수 (기본값 64, 렌더링용)
     */
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

    /*!
     *  \brief  3차 Bezier 곡선 위의 점 계산
     *  \param  p0  시작점
     *  \param  p1  제어점 1
     *  \param  p2  제어점 2
     *  \param  p3  끝점
     *  \param  t   0~1 범위의 파라미터
     *  \return t에서의 Bezier 곡선 위 위치
     */
    static glm::vec3 cubicBezier(const glm::vec3& p0, const glm::vec3& p1,
        const glm::vec3& p2, const glm::vec3& p3, float t)
    {
        float u = 1.0f - t;
        return u * u * u * p0 + 3.0f * u * u * t * p1 + 3.0f * u * t * t * p2 + t * t * t * p3;
    }

    /*!
     *  \brief  keyframes로부터 controlPoints와 frames를 재생성
     *  \param  numSamples  frames 샘플링 수 (기본값 64)
     *  \note   keyframes 변경 후 반드시 호출해야 세 표현이 동기화된다.
     */
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

    /*!
     *  \brief  궤적을 파일에 저장 (keyframes만 저장)
     *  \param  filename  저장할 파일 경로
     */
    void saveToFile(const char* filename) const
    {
        std::ofstream f(filename);
        f << keyframes.size() << "\n";
        for (auto& kf : keyframes)
            f << kf.position.x << " " << kf.position.y << " " << kf.position.z << " "
            << kf.rotation.w << " " << kf.rotation.x << " " << kf.rotation.y << " " << kf.rotation.z << "\n";
        std::cout << filename << " 저장 완료" << std::endl;
    }

    /*!
     *  \brief  파일에서 궤적 로드 후 rebuild() 실행
     *  \param  filename  로드할 파일 경로
     */
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

    /*!
     *  \brief  keyframes를 Cubic Bezier 제어점(controlPoints)으로 변환
     *
     *  mIsLinear == true 인 경우:
     *    각 세그먼트의 제어점을 p0, p0+1/3*(p1-p0), p0+2/3*(p1-p0), p1 로 설정하여
     *    cubicBezier 평가 시 직선이 되도록 한다.
     *
     *  mIsLinear == false 인 경우:
     *    Catmull-Rom → Cubic Bezier 변환 공식을 적용한다.
     *    경계 세그먼트에서는 phantom point를 사용하여 자연스러운 접선을 유지한다.
     *      p1 = k0 + (k1 - k00) / 6
     *      p2 = k1 - (k2  - k0) / 6
     */
    void catmullRomToSegments()
    {
        controlPoints.clear();
        int N = (int)keyframes.size();
        if (N < 2) return;

        for (int i = 0; i < N - 1; ++i)
        {
            // i=0 이면 이전 정보가 없어서 임의로 만들어줌
            glm::vec3 k00 = (i == 0) ? (2.0f * keyframes[0].position - keyframes[1].position)
                : keyframes[i - 1].position;

            glm::vec3 k0 = keyframes[i].position;
            glm::vec3 k1 = keyframes[i + 1].position;

			// 마지막 세그먼트면 다음 정보가 없어서 임의로 만들어줌
            glm::vec3 k2 = (i + 2 < N) ? keyframes[i + 2].position
                : (2.0f * keyframes[N - 1].position - keyframes[N - 2].position);

            // 베지어 제어점 계산
            glm::vec3 p0 = k0;
            glm::vec3 p1 = k0 + (k1 - k00) / 6.0f;
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

        if (mIsLinear)   // 직선 제어점 생성
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
    }
};


