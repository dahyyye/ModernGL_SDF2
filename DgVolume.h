#pragma once
#include "DgMesh.h"
#include <vector>

// VTK 사용
#include <vtkSmartPointer.h>
#include <vtkXMLImageDataReader.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

/*!
 *	\class	DgVolume
 *	\brief	메쉬의 부호거리장을 표현하는 클래스
 */
class DgVolume
{
public:
	/*! \brief SDF의 기본 메쉬 */
	DgMesh* mMesh = nullptr;

	/*! \brief 볼륨 이름 (sphere, bunny 등) */
	std::string mName;

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
	
	/* 볼륨의 텍스쳐 id */
	GLuint mTextureID = 0;

	/* 선택 상태 */
	bool mSelected = false;

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

	/*! #brief 메쉬와 점 p 간의 최단 거리와 그 거리를 갖는 삼각형을 반환 */
	std::pair<DgFace*, float> findClosestDistanceToMesh(DgMesh* mesh, const glm::vec3& p);

	/*! #brief VTI 로드 함수 */
	bool loadFromVTI(const char* filename);

	/*! #brief 텍스쳐 생성 함수 */
	void createTexture();

	/*! #brief 볼륨의 중심점 반환 */
	glm::vec3 getCenter() const {
		return glm::vec3(
			(mMin.mPos[0] + mMax.mPos[0]) * 0.5f,
			(mMin.mPos[1] + mMax.mPos[1]) * 0.5f,
			(mMin.mPos[2] + mMax.mPos[2]) * 0.5f
		);
	}

private:
};
