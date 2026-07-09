#pragma once
#include "DgViewer.h"

// 궤적 프레임: 위치 + 회전
struct DgTrajectoryFrame
{
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    DgTrajectoryFrame()
        : position(0.0f), rotation(1.0f, 0.0f, 0.0f, 0.0f), scale(1.0f) {
    }

    DgTrajectoryFrame(glm::vec3 pos, glm::quat rot, glm::vec3 scl = glm::vec3(1.0f))
        : position(pos), rotation(rot), scale(scl) {
    }
};

class DgTrajectory
{
public:
    std::vector<DgTrajectoryFrame> frames;      // 렌더링/시각화용 샘플링된 프레임
    std::vector<DgTrajectoryFrame> keyframes;   // 사용자가 정의한 키프레임
    bool mIsLinear = false; // true면 키프레임 사이를 직선 보간

    void clear() {
        frames.clear();
        keyframes.clear();
        mIsLinear = false;
    }

    size_t size() const { return frames.size(); }
    bool empty() const { return frames.empty(); }

    /*!
     *  \brief  Catmull-Rom 곡선에서 t에 해당하는 위치 계산
     *  \param  kfs  키프레임 배열
     *  \param  seg  세그먼트 인덱스 (keyframes[seg] ~ keyframes[seg+1] 사이)
     *  \param  lt   세그먼트 내 로컬 t [0,1]
     *  \return Catmull-Rom 보간된 위치
     *  \note   경계에서는 phantom point(반사점)로 자연스러운 곡선 유지
     *          키프레임이 2개(선형 궤적)이면 수식이 자동으로 선형 보간으로 단순화됨
     */
    static glm::vec3 catmullRomEval(const std::vector<DgTrajectoryFrame>& kfs, int seg, float lt)
    {
        int N = (int)kfs.size();

        // 경계 phantom point: 존재하지 않는 이웃을 반사점으로 대체
        glm::vec3 k00 = (seg == 0)
            ? (2.0f * kfs[0].position - kfs[1].position)
            : kfs[seg - 1].position;

        glm::vec3 k0 = kfs[seg].position;
        glm::vec3 k1 = kfs[seg + 1].position;

        glm::vec3 k2 = (seg + 2 < N)
            ? kfs[seg + 2].position
            : (2.0f * kfs[N - 1].position - kfs[N - 2].position);

        // Catmull-Rom 공식 (Cubic Hermite 형태)
        float lt2 = lt * lt;
        float lt3 = lt2 * lt;

		return 0.5f * ((2.0f * k0)  // 상수항, 키프레임 k0 위치
			+ (-k00 + k1) * lt      // 1차항, 접선 방향
			+ (2.0f * k00 - 5.0f * k0 + 4.0f * k1 - k2) * lt2  // 2차항, 곡률을 만듦
            + (-k00 + 3.0f * k0 - 3.0f * k1 + k2) * lt3);      // 3차항, 변곡을 만듦
    }

    /*!
 *  \brief  t-구간 [t0, t1] 안에서 Catmull-Rom 곡선이 chord(직선)로부터
 *          벗어나는 최대 거리(deviation)를 계산
 *  \param  t0, t1  전체 트라젝토리 기준 [0,1] 파라미터 구간
 *  \param  N       구간 내부 샘플 수 (기본 8)
 *  \return 구간 내 최대 이탈 거리
 *  \note   GPU의 catmullRomPos(t)와 동일한 seg/lt 계산을 거쳐야
 *          CPU-GPU 간 위치가 정확히 일치함
 */
 // 수정
    float computeSegmentDeviation(float t0, float t1, int N = 8) const
    {
        int numSegs = (int)keyframes.size() - 1;
        if (numSegs <= 0) return 0.0f;

        auto positionAt = [&](float t) -> glm::vec3 {
            float scaled = glm::clamp(t, 0.0f, 1.0f) * numSegs;
            int   seg = glm::clamp((int)scaled, 0, numSegs - 1);
            float lt = scaled - (float)seg;
            return catmullRomEval(keyframes, seg, lt);
            };

        // [t0, t1] 구간에서 scale이 가장 작아지는 값 (로컬 공간에서 deviation이 가장 커지는 지점)
        auto minScaleAt = [&](float t) -> float {
            float scaled = glm::clamp(t, 0.0f, 1.0f) * numSegs;
            int   seg = glm::clamp((int)scaled, 0, numSegs - 1);
            float lt = scaled - (float)seg;
            glm::vec3 scl = glm::mix(keyframes[seg].scale, keyframes[seg + 1].scale, lt);
            return std::min({ scl.x, scl.y, scl.z });
            };

        glm::vec3 p0 = positionAt(t0);
        glm::vec3 p1 = positionAt(t1);
        float worstScale = std::min(minScaleAt(t0), minScaleAt(t1));

        float maxDev = 0.0f;
        for (int i = 1; i < N; ++i)
        {
            float alpha = (float)i / (float)N;
            glm::vec3 curvePos = positionAt(glm::mix(t0, t1, alpha));
            glm::vec3 chordPos = glm::mix(p0, p1, alpha);
            maxDev = std::max(maxDev, glm::length(curvePos - chordPos));
            worstScale = std::min(worstScale, minScaleAt(glm::mix(t0, t1, alpha)));
        }
        worstScale = std::max(worstScale, 1e-4f); // 0으로 나누기 방지
        return maxDev / worstScale;               // 로컬 공간 기준으로 정규화
    }

    /*!
     *  \brief  t (0~1) 에서의 변환 행렬 반환
     *  \param  t   0~1 범위의 궤적 파라미터
     *  \return t에서의 위치 + 회전을 담은 4x4 변환 행렬
     */
    glm::mat4 getTransformAt(float t) const
    {
		int numSegs = (int)keyframes.size() - 1;    // 세그먼트 수
		t = glm::clamp(t, 0.0f, 1.0f);              // t를 [0,1]로 클램프
        
		float scaled = t * numSegs;                 // t를 세그먼트 수로 스케일링
		int   seg = glm::clamp((int)scaled, 0, numSegs - 1); // 세그먼트 인덱스 계산 및 클램프
		float lt = scaled - (float)seg;             // 세그먼트 내 로컬 t 계산

        glm::vec3 pos;
		if (mIsLinear)  // 직선 궤적이면 키프레임 사이를 선형 보간
            pos = glm::mix(keyframes[seg].position, keyframes[seg + 1].position, lt);
		else            // 곡선 궤적이면 Catmull-Rom 보간
            pos = catmullRomEval(keyframes, seg, lt);

		// 회전은 항상 slerp로 보간 (선형 궤적이든 곡선 궤적이든)
        glm::quat rot = glm::slerp(keyframes[seg].rotation, keyframes[seg + 1].rotation, lt);
        
        // scale lerp 보간 
        glm::vec3 scl = glm::mix(keyframes[seg].scale, keyframes[seg + 1].scale, lt);

        // 변환 행렬 반환: T = Trans * Rot * Scale
        return glm::translate(glm::mat4(1.0f), pos)
            * glm::mat4_cast(rot)
            * glm::scale(glm::mat4(1.0f), scl);
    }

    /*!
     *  \brief  선형 궤적 생성
     *  \param  startPos  시작 위치
     *  \param  endPos    끝 위치
     */
    void generateLinear(const glm::vec3& center)
    {
        clear();
        mIsLinear = true;  // 직선 플래그 설정
        glm::quat baseRot(1.0f, 0.0f, 0.0f, 0.0f);
        keyframes.emplace_back(center, baseRot);
        keyframes.emplace_back(center + glm::vec3(50.0f, 0.0f, 0.0f), baseRot);
        rebuild();
    }

    /*!
     *  \brief  Catmull-Rom 곡선 궤적 생성
     *  \param  center      곡선의 시작 위치
     *  \param  numSamples  frames 샘플링 수 (기본값 64)
     */
    void generateCurve(const glm::vec3& center)
    {
        clear();
        glm::quat baseRot(1.0f, 0.0f, 0.0f, 0.0f);
        keyframes.emplace_back(center, baseRot);
        keyframes.emplace_back(center + glm::vec3(5.0f, 0.0f, 4.0f), baseRot);
        keyframes.emplace_back(center + glm::vec3(13.0f, 0.0f, -3.0f), baseRot);
        keyframes.emplace_back(center + glm::vec3(20.0f, 0.0f, 0.0f), baseRot);

        rebuild();
    }

    /*!
     *  \brief  keyframes로부터 frames를 Catmull-Rom으로 샘플링하여 재생성
     *  \param  numSamples  frames 샘플링 수 (기본값 64)
     */
    void rebuild(int numSamples = 64)
    {
        int numSegs = (int)keyframes.size() - 1;
        if (numSegs <= 0) return;
        frames.clear();

        for (int i = 0; i < numSamples; ++i)
        {
            float t = (float)i / (numSamples - 1);
            float scaled = t * numSegs;
            int   seg = glm::clamp((int)scaled, 0, numSegs - 1);
            float lt = scaled - (float)seg;

            glm::vec3 pos;

            if (mIsLinear)
                // 직선: 키프레임 사이를 선형 보간
                pos = glm::mix(keyframes[seg].position, keyframes[seg + 1].position, lt);
            else
                // 곡선: Catmull-Rom 보간
                pos = catmullRomEval(keyframes, seg, lt);

			// 회전은 항상 slerp로 보간 (선형 궤적이든 곡선 궤적이든)
            glm::quat rot = glm::slerp(keyframes[seg].rotation, keyframes[seg + 1].rotation, lt);

            // scale lerp 보간
            glm::vec3 scl = glm::mix(keyframes[seg].scale, keyframes[seg + 1].scale, lt);

            frames.emplace_back(pos, rot, scl);
        }
    }

    /*!
     *  \brief  파일에 저장 (keyframes만 저장)
     */
    void saveToFile(const char* filename) const
    {
        std::ofstream f(filename);
        f << keyframes.size() << "\n";
        for (auto& kf : keyframes)
            f << kf.position.x << " " << kf.position.y << " " << kf.position.z << " "
            << kf.rotation.w << " " << kf.rotation.x << " " << kf.rotation.y << " " << kf.rotation.z << " "
            << kf.scale.x << " " << kf.scale.y << " " << kf.scale.z << "\n";
        std::cout << filename << " 저장 완료" << std::endl;
    }

    /*!
     *  \brief  파일에서 로드 후 rebuild() 호출
     */
    void loadFromFile(const char* filename)
    {
        std::ifstream f(filename);
        int n; f >> n;
        keyframes.clear();
        for (int i = 0; i < n; ++i) {
            glm::vec3 pos; glm::quat rot; glm::vec3 scl;
            f >> pos.x >> pos.y >> pos.z >> rot.w >> rot.x >> rot.y >> rot.z >> scl.x >> scl.y >> scl.z;
            keyframes.emplace_back(pos, rot, scl);
        }
        rebuild();
    }
};