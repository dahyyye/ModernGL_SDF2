#pragma once
#include "DgMesh.h"
#include <vector>

/*!
 *	\class	DgSDF
 *	\brief	메쉬의 부호거리장을 표현하는 클래스
 */
class DgVolume
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
	double mSpacing[3] = { 0.0, 0.0, 0.0};

	/* !\brief 부호거리장 데이터(격자 샘플별 부호거리 값) */
	std::vector<float> mData;

	GLuint raymarchShader;
public:

	DgVolume();
	DgVolume(DgMesh* mMesh);
	DgVolume(DgVolume& cpy);
	~DgVolume();
	
	void setDimensions(int dimX, int dimY, int dimZ);
	
	/*! #brief 입력 메쉬의 격자 공간을 정의(AABB) */
	void setGridSpace(const DgMesh& mesh, float padding = 0.1f);

	/*! #brief 격자 샘플에 대하여 부호거리 값을 mData에 저장 */
	void computeSDF();

	std::pair<DgFace*, float> findClosestDistanceToMesh(DgMesh* mesh, const glm::vec3& p);



private:
	//float findClosestDistanceToMesh(const glm::vec3& p);
	//float getSign(const glm::vec3& p);
};
