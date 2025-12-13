#include "DgViewer.h"

DgVolume::DgVolume()
{
	mMesh = nullptr;
	mName = "volume";
	
	mDim[0] = 0;
	mDim[1] = 0;
	mDim[2] = 0;

	mMin.mPos[0] = 0;
	mMin.mPos[1] = 0;
	mMin.mPos[2] = 0;

	mMax.mPos[0] = 0;
	mMax.mPos[1] = 0;
	mMax.mPos[2] = 0;

	mSpacing[0] = 0;
	mSpacing[1] = 0;
	mSpacing[2] = 0;

	mPosition = glm::vec3(0.0f);
	mRotation = glm::vec3(0.0f);
}

DgVolume::DgVolume(DgMesh* mesh)
{
	mMesh = mesh;
	mName = "volume";
	mPosition = glm::vec3(0.0f);
	mRotation = glm::vec3(0.0f);
}

DgVolume::DgVolume(DgVolume& cpy)
{
	mMesh = cpy.mMesh;
	mName = cpy.mName;

	mDim[0] = cpy.mDim[0];
	mDim[1] = cpy.mDim[1];
	mDim[2] = cpy.mDim[2];

	mMin.mPos[0] = cpy.mMin.mPos[0];
	mMin.mPos[1] = cpy.mMin.mPos[1];
	mMin.mPos[2] = cpy.mMin.mPos[2];

	mMax.mPos[0] = cpy.mMax.mPos[0];
	mMax.mPos[1] = cpy.mMax.mPos[1];
	mMax.mPos[2] = cpy.mMax.mPos[2];

	mSpacing[0] = cpy.mSpacing[0];
	mSpacing[1] = cpy.mSpacing[1];
	mSpacing[2] = cpy.mSpacing[2];

	mPosition = cpy.mPosition;
	mRotation = cpy.mRotation;
}
DgVolume::~DgVolume()
{
	if (mMesh != nullptr) {
		delete mMesh;
	}

	if (mTextureID != 0) {
        glDeleteTextures(1, &mTextureID);
    }
}
void DgVolume::setDimensions(int dimX, int dimY, int dimZ)
{
	mDim[0] = dimX;
	mDim[1] = dimY;
	mDim[2] = dimZ;
}

/*!
*	@brief	입력 메쉬의 격자 공간을 정의(AABB)
*
*	@param	DgMesh& mesh	입력 받은 메쉬
*	@param	padding[in]		격자 공간에 추가할 패딩 비율 (기본값: 0.1f)
*
*/
void DgVolume::setGridSpace(const DgMesh& mesh, float padding)
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
void DgVolume::computeSDF()
{

	int N_X = mDim[0];
	int N_Y = mDim[1];
	int N_Z = mDim[2];

	// 1) mData 벡터 크기 설정
	mData.resize(mDim[0] * mDim[1] * mDim[2], std::numeric_limits<float>::max());

	for (int k = 0; k < N_Z; k++)
	{
		for (int j = 0; j < N_Y; j++)
		{
			for (int i = 0; i < N_X; i++) 
			{
				// 2) 격자 샘플의 실제 좌표 계산
				glm::vec3 p;
				p.x = mMin.mPos[0] + i * mSpacing[0];
				p.y = mMin.mPos[1] + j * mSpacing[1];
				p.z = mMin.mPos[2] + k * mSpacing[2];

				// 3) 메쉬와 샘플좌표의 부호거리 계산
				auto distance = findClosestDistanceToMesh(mMesh, p);

				// 4) mData에 부호거리 저장
				int index = i + j * N_X + k * N_X * N_Y;
				mData[index] = distance.second;
			}
		}
	}

	// 디버그 출력
	for (int k = 0; k < N_Z; k++) {
		for (int j = 0; j < N_Y; j++) {
			for (int i = 0; i < N_X; i++) {

				// 4) mData에 부호거리 저장
				int index = i + j * N_X + k * N_X * N_Y;

				std::cout << mData[index] << " ";
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}

/*!
*	@brief	메쉬와 점 p 간의 최단 거리와 그 거리를 갖는 삼각형을 반환
*
*	@param	DgMesh* mesh	입력 받은 메쉬
*	@param	vec3& p			입력 받은 점의 좌표
*
*/
std::pair<DgFace*, float> DgVolume::findClosestDistanceToMesh(DgMesh* mesh, const glm::vec3& p)
{
	std::vector<float>	distances;
	std::vector<int>	faceIndices;
	std::vector<bool>	isInside;

	for (int i = 0; i < mesh->mFaces.size(); ++i)
	{
		// 삼각형의 세 정점 좌표
		glm::vec3 v0(mesh->mVerts[mesh->mFaces[i].mVertIdxs[0]].mPos[0],	
					 mesh->mVerts[mesh->mFaces[i].mVertIdxs[0]].mPos[1],
					 mesh->mVerts[mesh->mFaces[i].mVertIdxs[0]].mPos[2]);
		glm::vec3 v1(mesh->mVerts[mesh->mFaces[i].mVertIdxs[1]].mPos[0],
					 mesh->mVerts[mesh->mFaces[i].mVertIdxs[1]].mPos[1],
					 mesh->mVerts[mesh->mFaces[i].mVertIdxs[1]].mPos[2]);
		glm::vec3 v2(mesh->mVerts[mesh->mFaces[i].mVertIdxs[2]].mPos[0],
					 mesh->mVerts[mesh->mFaces[i].mVertIdxs[2]].mPos[1],
					 mesh->mVerts[mesh->mFaces[i].mVertIdxs[2]].mPos[2]);
		
		glm::vec3 n = glm::cross(v1 - v0, v2 - v0);						// 삼각형 법선 벡터
		n = glm::normalize(n);

		double d = glm::dot(n * -1.0f, v0);								// 평면 방정식의 d 값 계산

		double dist = n[0] * (p.x) + n[1] * (p.y) + n[2] * (p.z) + d;	// 점 p에서 평면까지의 거리 계산

		if(dist < 0)
			isInside.push_back(true);
		else
			isInside.push_back(false);

		dist = std::abs(dist);
		double denom = glm::length(n);
		dist = dist / denom;
		distances.push_back(static_cast<float>(dist));
		faceIndices.push_back(i);
	}

	int minIndex = std::min_element(distances.begin(), distances.end()) - distances.begin();

	return std::make_pair(&mesh->mFaces[minIndex], isInside[minIndex] ? distances[minIndex] * -1.0 : distances[minIndex]);
	
	// (추후) BVH를 활용
}

/*!
*   @brief  VTI 파일에서 SDF 데이터를 로드하여 mData에 저장
*
*   @param  filename    VTI 파일 경로
*   @return 로드 성공 여부
*/
bool DgVolume::loadFromVTI(const char* filename)
{
	// 1) VTI 파일 읽기
	vtkSmartPointer<vtkXMLImageDataReader> reader =
		vtkSmartPointer<vtkXMLImageDataReader>::New();

	if (!reader->CanReadFile(filename)) {
		std::cerr << "VTI 파일을 읽을 수 없습니다: " << filename << std::endl;
		return false;
	}

	reader->SetFileName(filename);
	reader->Update();

	vtkSmartPointer<vtkImageData> imageData = reader->GetOutput();
	if (!imageData) {
		std::cerr << "VTI 데이터 로드 실패: " << filename << std::endl;
		return false;
	}

	// 2) 차원 정보 추출
	int dims[3];
	imageData->GetDimensions(dims);
	mDim[0] = dims[0];
	mDim[1] = dims[1];
	mDim[2] = dims[2];

	// 3) 원점과 간격 추출
	double origin[3];
	double spacing[3];
	imageData->GetOrigin(origin);
	imageData->GetSpacing(spacing);

	mSpacing[0] = spacing[0];
	mSpacing[1] = spacing[1];
	mSpacing[2] = spacing[2];

	// 4) 볼륨 경계 계산 (min = origin, max = origin + (dim-1)*spacing)
	mMin.mPos[0] = origin[0];
	mMin.mPos[1] = origin[1];
	mMin.mPos[2] = origin[2];

	mMax.mPos[0] = origin[0] + (dims[0] - 1) * spacing[0];
	mMax.mPos[1] = origin[1] + (dims[1] - 1) * spacing[1];
	mMax.mPos[2] = origin[2] + (dims[2] - 1) * spacing[2];

	// 5) 스칼라 데이터를 mData에 복사
	vtkDataArray* scalars = imageData->GetPointData()->GetScalars();
	if (!scalars) {
		std::cerr << "스칼라 데이터가 없습니다: " << filename << std::endl;
		return false;
	}

	int totalSize = dims[0] * dims[1] * dims[2];
	mData.resize(totalSize);

	for (int i = 0; i < totalSize; ++i) {
		mData[i] = static_cast<float>(scalars->GetTuple1(i));
	}

	//=========================================================================
	// 디버깅: mData 통계 출력
	//=========================================================================
	float minVal = mData[0];
	float maxVal = mData[0];
	int negCount = 0;   // 음수 개수 (내부)
	int posCount = 0;   // 양수 개수 (외부)
	int zeroCount = 0;  // 0에 가까운 값 (표면)

	for (int i = 0; i < totalSize; ++i) {
		float v = mData[i];
		if (v < minVal) minVal = v;
		if (v > maxVal) maxVal = v;

		if (v < -0.001f) negCount++;
		else if (v > 0.001f) posCount++;
		else zeroCount++;
	}

	std::cout << "========== mData 디버깅 ==========" << std::endl;
	std::cout << "  총 복셀 수: " << totalSize << std::endl;
	std::cout << "  최소값: " << minVal << std::endl;
	std::cout << "  최대값: " << maxVal << std::endl;
	std::cout << "  음수 개수 (내부): " << negCount << " (" << (100.0f * negCount / totalSize) << "%)" << std::endl;
	std::cout << "  양수 개수 (외부): " << posCount << " (" << (100.0f * posCount / totalSize) << "%)" << std::endl;
	std::cout << "  표면 근처 (|v|<0.001): " << zeroCount << std::endl;
	std::cout << "==================================" << std::endl;

	// 중앙 슬라이스 몇 개 값 출력
	int midZ = dims[2] / 2;
	int midY = dims[1] / 2;
	std::cout << "  중앙 슬라이스 (z=" << midZ << ", y=" << midY << ") 값들:" << std::endl;
	std::cout << "  ";
	for (int x = 0; x < dims[0]; x += dims[0] / 8) {
		int idx = x + midY * dims[0] + midZ * dims[0] * dims[1];
		std::cout << mData[idx] << " ";
	}
	std::cout << std::endl;

	return true;
}

void DgVolume::createTexture()
{
	// 기존 텍스처가 있으면 삭제
	if (mTextureID != 0) {
		glDeleteTextures(1, &mTextureID);
	}

	// 새 텍스처 생성
	glGenTextures(1, &mTextureID);
	glBindTexture(GL_TEXTURE_3D, mTextureID);

	// 데이터 업로드
	glTexImage3D(
		GL_TEXTURE_3D,
		0,
		GL_R32F,
		mDim[0], mDim[1], mDim[2],
		0,
		GL_RED,
		GL_FLOAT,
		mData.data()
	);

	// 텍스처 파라미터 설정
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_3D, 0);

	std::cout << "볼륨 텍스처 생성 완료! ID: " << mTextureID << std::endl;
}