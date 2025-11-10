#pragma once
#include "DgMesh.h"
#include <vector>

/*!
 *	\class	DgSDF
 *	\brief	메쉬의 부호거리장을 표현하는 클래스
 */
class DgSDF
{
public:
	/*! \brief SDF의 기본 메쉬 */
	DgMesh* mMesh = nullptr;

	/*! \brief 격자 해상도 */
	int mDim[3] = { 0, 0, 0 };

	/*! \brief 격자 공간 최소점 */
	DgPos mMin;

	/*! \brief 격자 공간 최대점 */
	DgPos mMax;

	/*! \brief 격자 간격(해상도) */
	double mSpacing[3] = { 64.0, 64.0, 64.0 };

	std::vector<float> mData;
public:
	/*! #brief 입력 메쉬의 bounding box(AABB) 계산 */
	void setGridSpace(const DgMesh& mesh, float padding = 0.1f);

	/*! #brief 격자 샘플에 대하여 부호거리 값을 mData에 저장 */
	void computeSDF();
};
