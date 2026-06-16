#include "DgViewer.h"
#include <vtkXMLImageDataWriter.h>
#include <vtkFloatArray.h>

DgVolume::DgVolume()
{
	mMesh = nullptr;
	mName = "volume";
}

DgVolume::DgVolume(DgMesh* mesh)
{
	mMesh = mesh;
	mName = "volume";
	mPosition = glm::vec3(0.0f);
	mRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
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
	mScale = cpy.mScale;      
	mSelected = cpy.mSelected;  

	mData = cpy.mData;
	mTextureID = 0;
	mOffset = cpy.mOffset;

	mIsSweptVolume = cpy.mIsSweptVolume;
	mSweepResolution = cpy.mSweepResolution;
	mSweepTimeSteps = cpy.mSweepTimeSteps;
	mSweepMethod = cpy.mSweepMethod;

	mSourceTrajectory = nullptr; 
	mBrushVolume = nullptr;  
}

DgVolume::~DgVolume()
{
	if (mMesh != nullptr) {
		delete mMesh;
	}

	if (mTextureID != 0) {
        glDeleteTextures(1, &mTextureID);
    }

	if (mSourceTrajectory) { delete mSourceTrajectory; mSourceTrajectory = nullptr; }
	if (mBrushVolume) { delete mBrushVolume;      mBrushVolume = nullptr; }
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
	if (auto* fa = vtkFloatArray::SafeDownCast(scalars)) {
		float* ptr = fa->GetPointer(0);
		std::copy(ptr, ptr + totalSize, mData.data());
	}
	else {
		for (int i = 0; i < totalSize; ++i)
			mData[i] = static_cast<float>(scalars->GetTuple1(i));
	}

	return true;
}

/*!
* @brief	텍스쳐 생성 함수
* 
*/
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

	std::cout << "볼륨 텍스처 생성 완료 ID: " << mTextureID << std::endl;
}

bool DgVolume::saveToVTI(const char* filename)
{
	auto imageData = vtkSmartPointer<vtkImageData>::New();
	imageData->SetDimensions(mDim[0], mDim[1], mDim[2]);
	imageData->SetSpacing(mSpacing[0], mSpacing[1], mSpacing[2]);
	imageData->SetOrigin(mMin.mPos[0], mMin.mPos[1], mMin.mPos[2]);

	auto array = vtkSmartPointer<vtkFloatArray>::New();
	array->SetName("SDF");
	array->SetNumberOfValues(mData.size());
	for (size_t i = 0; i < mData.size(); ++i)
		array->SetValue(i, mData[i]);
	imageData->GetPointData()->SetScalars(array);

	auto writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
	writer->SetFileName(filename);
	writer->SetInputData(imageData);

	std::cout << filename << " 저장 완료" << std::endl;

	return writer->Write() != 0;
}

DgVolume* DgVolume::createResultVolume(const std::string& name,
	int resolution, const glm::vec3& minPos, const glm::vec3& maxPos)
{
	DgVolume* vol = new DgVolume();
	vol->mName = name;
	vol->mDim[0] = resolution;
	vol->mDim[1] = resolution;
	vol->mDim[2] = resolution;

	vol->mMin = DgPos(minPos.x, minPos.y, minPos.z);
	vol->mMax = DgPos(maxPos.x, maxPos.y, maxPos.z);

	glm::vec3 range = maxPos - minPos;
	vol->mSpacing[0] = range.x / (resolution - 1);
	vol->mSpacing[1] = range.y / (resolution - 1);
	vol->mSpacing[2] = range.z / (resolution - 1);

	vol->mMesh = createBoundingBoxMesh(vol->mMin, vol->mMax);

	return vol;
}

void DgVolume::getWorldAABB(glm::vec3& outMin, glm::vec3& outMax) const
{
	glm::vec3 localMin = getLocalMin();
	glm::vec3 localMax = getLocalMax();
	glm::mat4 model = getModelMatrix();

	glm::vec3 corners[8] = {
		{localMin.x, localMin.y, localMin.z},
		{localMax.x, localMin.y, localMin.z},
		{localMax.x, localMax.y, localMin.z},
		{localMin.x, localMax.y, localMin.z},
		{localMin.x, localMin.y, localMax.z},
		{localMax.x, localMin.y, localMax.z},
		{localMax.x, localMax.y, localMax.z},
		{localMin.x, localMax.y, localMax.z}
	};

	outMin = outMax = glm::vec3(model * glm::vec4(corners[0], 1.0f));
	for (int i = 1; i < 8; ++i) {
		glm::vec3 transformed = glm::vec3(model * glm::vec4(corners[i], 1.0f));
		outMin = glm::min(outMin, transformed);
		outMax = glm::max(outMax, transformed);
	}
}