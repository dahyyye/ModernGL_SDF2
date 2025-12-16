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

	/*! \brief 볼륨 위치 */
	glm::vec3 mPosition = glm::vec3(0.0f);

	/*! \brief 볼륨 회전 (오일러 각도, 라디안) */
	glm::vec3 mRotation = glm::vec3(0.0f);

public:

	DgVolume();
	DgVolume(DgMesh* mMesh);
	DgVolume(DgVolume& cpy);
	~DgVolume();
	
	void setDimensions(int dimX, int dimY, int dimZ);
	
	/*! \brief 입력 메쉬의 격자 공간을 정의(AABB) */
	void setGridSpace(const DgMesh& mesh, float padding = 0.1f);

	/*! \brief 격자 샘플에 대하여 부호거리 값을 mData에 저장 */
	void computeSDF();

	/*! \brief 메쉬와 점 p 간의 최단 거리와 그 거리를 갖는 삼각형을 반환 */
	std::pair<DgFace*, float> findClosestDistanceToMesh(DgMesh* mesh, const glm::vec3& p);

	/*! \brief VTI 로드 함수 */
	bool loadFromVTI(const char* filename);

	/*! \brief 텍스쳐 생성 함수 */
	void createTexture();

	/*! \brief 볼륨의 중심점 반환 (위치 포함) */
	glm::vec3 getCenter() const {
		glm::vec3 localCenter(
			(mMin.mPos[0] + mMax.mPos[0]) * 0.5f,
			(mMin.mPos[1] + mMax.mPos[1]) * 0.5f,
			(mMin.mPos[2] + mMax.mPos[2]) * 0.5f
		);
		return glm::vec3(getModelMatrix() * glm::vec4(localCenter, 1.0f));
	}

	/*! \brief 로컬 최소점 반환 */
	glm::vec3 getLocalMin() const {
		return glm::vec3(mMin.mPos[0], mMin.mPos[1], mMin.mPos[2]);
	}

	/*! \brief 로컬 최대점 반환 */
	glm::vec3 getLocalMax() const {
		return glm::vec3(mMax.mPos[0], mMax.mPos[1], mMax.mPos[2]);
	}

	/*! \brief 이동된 최소점 반환 */
	glm::vec3 getTransformedMin() const {
		return glm::vec3(
			mMin.mPos[0] + mPosition.x,
			mMin.mPos[1] + mPosition.y,
			mMin.mPos[2] + mPosition.z
		);
	}

	/*! \brief 이동된 최대점 반환 */
	glm::vec3 getTransformedMax() const {
		return glm::vec3(
			mMax.mPos[0] + mPosition.x,
			mMax.mPos[1] + mPosition.y,
			mMax.mPos[2] + mPosition.z
		);
	}

	/*! \brief 모델 행렬 반환 (이동 + 회전) */
	glm::mat4 getModelMatrix() const {
		glm::mat4 model(1.0f);

		// 볼륨 중심 계산
		glm::vec3 center(
			(mMin.mPos[0] + mMax.mPos[0]) * 0.5f,
			(mMin.mPos[1] + mMax.mPos[1]) * 0.5f,
			(mMin.mPos[2] + mMax.mPos[2]) * 0.5f
		);

		// 이동 적용
		model = glm::translate(model, mPosition);

		// 중심으로 이동 → 회전 → 원래 위치로
		model = glm::translate(model, center);
		model = glm::rotate(model, mRotation.y, glm::vec3(0, 1, 0));  // Y축 회전
		model = glm::rotate(model, mRotation.x, glm::vec3(1, 0, 0));  // X축 회전
		model = glm::rotate(model, mRotation.z, glm::vec3(0, 0, 1));  // Z축 회전
		model = glm::translate(model, -center);

		return model;
	}

	/*! \brief 위치 이동 */
	void translate(const glm::vec3& delta) {
		mPosition += delta;
	}

	/*! \brief 회전 적용 (라디안) */
	void rotate(const glm::vec3& deltaRadians) {
		mRotation += deltaRadians;
	}

private:
};
