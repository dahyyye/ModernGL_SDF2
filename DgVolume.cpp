#include "DgViewer.h"

/*!
*	@brief	입력 메쉬의 격자 공간을 정의(AABB)
*
*	@param	DgMesh& mesh	입력 받은 메쉬
*	@param	padding[in]		격자 공간에 추가할 패딩 비율 (기본값: 0.1f)
*
*/
void DgSDF::setGridSpace(const DgMesh& mesh, float padding)
{
	// 입력 메쉬의 AABB 계산
	DgPos minPos(mesh.mVerts[0].mPos[0], mesh.mVerts[0].mPos[1], mesh.mVerts[0].mPos[2]);
	DgPos maxPos(mesh.mVerts[0].mPos[0], mesh.mVerts[0].mPos[1], mesh.mVerts[0].mPos[2]);
	for (const DgVertex& v : mesh.mVerts) {
		if (v.mPos[0] < minPos.mPos[0]) minPos.mPos[0] = v.mPos[0];
		if (v.mPos[1] < minPos.mPos[1]) minPos.mPos[1] = v.mPos[1];
		if (v.mPos[2] < minPos.mPos[2]) minPos.mPos[2] = v.mPos[2];
		if (v.mPos[0] > maxPos.mPos[0]) maxPos.mPos[0] = v.mPos[0];
		if (v.mPos[1] > maxPos.mPos[1]) maxPos.mPos[1] = v.mPos[1];
		if (v.mPos[2] > maxPos.mPos[2]) maxPos.mPos[2] = v.mPos[2];
	}
	// 패딩 적용
	double paddingX = (maxPos.mPos[0] - minPos.mPos[0]) * padding;
	double paddingY = (maxPos.mPos[1] - minPos.mPos[1]) * padding;
	double paddingZ = (maxPos.mPos[2] - minPos.mPos[2]) * padding;

	// 격자 공간 설정
	mMin = DgPos(minPos.mPos[0] - paddingX, minPos.mPos[1] - paddingY, minPos.mPos[2] - paddingZ);
	mMax = DgPos(maxPos.mPos[0] + paddingX, maxPos.mPos[1] + paddingY, maxPos.mPos[2] + paddingZ);

	// 격자 해상도에 따라 격자 간격 계산
	mSpacing[0] = (mMax.mPos[0] - mMin.mPos[0]) / (mDim[0] - 1);
	mSpacing[1] = (mMax.mPos[1] - mMin.mPos[1]) / (mDim[1] - 1); 
	mSpacing[2] = (mMax.mPos[2] - mMin.mPos[2]) / (mDim[2] - 1);
}

/*!
*	@brief	격자 샘플에 대하여 부호거리 값을 mData에 저장
*
*/
void DgSDF::computeSDF() {

	// 1) mData 벡터 크기 설정
	mData.resize(mDim[0] * mDim[1] * mDim[2], std::numeric_limits<float>::max());

	// 2) 격자 샘플의 실제 좌표 계산
	// 3) 메쉬와 샘플좌표의 부호거리 계산
	// 4) mData에 부호거리 저장
}