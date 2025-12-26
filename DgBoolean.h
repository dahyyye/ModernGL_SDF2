#pragma once

#include "DgViewer.h"

enum class BooleanMode {
	Union,
	Intersection,
	Difference
};

class DgBoolean
{
public:
	/*!
	 *  \brief  메인 Boolean 연산 함수
	 *
	 *  \param[in]  volumes     Boolean 연산할 볼륨들의 배열 (최소 2개)
	 *  \param[in]  mode        연산 모드 (Union, Intersection, Difference)
	 *  \param[in]  dim         결과 볼륨의 격자 해상도 (dim x dim x dim)
	 *
	 *  \return     새로 생성된 결과 볼륨 (nullptr: 실패)
	 */
	static DgVolume* Boolean(const std::vector<DgVolume*>& volumes, BooleanMode mode, int dim);
	
	/*!
	 *  \brief  월드 좌표에서 볼륨의 SDF 값 샘플링
	 *
	 *  \param[in]  vol         샘플링할 볼륨
	 *  \param[in]  invModel    볼륨 모델 행렬의 역행렬 (월드→로컬 변환용)
	 *  \param[in]  worldPos    샘플링할 월드 좌표
	 *
	 *  \return     해당 위치의 SDF 값 (범위 밖이면 경계값 + 거리)
	 */
	static float resampleSDF(DgVolume* vol, const glm::mat4& invModel, const glm::vec3& worldPos);

	/*!
	 *  \brief  삼선형 보간으로 SDF 값 계산
	 *
	 *  \param[in]  data    SDF 데이터 배열
	 *  \param[in]  dimX    X축 격자 해상도
	 *  \param[in]  dimY    Y축 격자 해상도
	 *  \param[in]  dimZ    Z축 격자 해상도
	 *  \param[in]  uvw     정규화된 텍스처 좌표 [0,1]^3
	 *
	 *  \return     보간된 SDF 값
	 */
	static float trilinearInterpolate(const float* data, int dimX, int dimY, int dimZ, const glm::vec3& uvw);

private:

	/*!
	 *  \brief  여러 볼륨의 결합된 바운딩 박스 계산
	 *
	 *  \param[in]  volumes         입력 볼륨들의 배열
	 *  \param[out] combinedMin     결합된 AABB 최소점
	 *  \param[out] combinedMax     결합된 AABB 최대점
	 *  \param[in]  mode            연산 모드 (Union/Intersection/Difference에 따라 다르게 계산)
	 */
	static void computeAABB(const std::vector<DgVolume*>& volumes, 
		glm::vec3& combinedMin, 
		glm::vec3& combinedMax,
		BooleanMode mode);

	/*!
	 *  \brief  단일 볼륨의 월드 공간 AABB 계산 (이동+회전 반영)
	 *
	 *  \param[in]  vol         입력 볼륨
	 *  \param[out] outMin      월드 공간 AABB 최소점
	 *  \param[out] outMax      월드 공간 AABB 최대점
	 */
	static void getWorldAABB(DgVolume* vol, glm::vec3& outMin, glm::vec3& outMax);

	/*!
	 *  \brief  결과 볼륨 이름 생성 (union1, intersection2 등)
	 *
	 *  \param[in]  mode    연산 모드
	 *
	 *  \return     생성된 이름 문자열
	 */
	static std::string generateName(BooleanMode mode);

	
};