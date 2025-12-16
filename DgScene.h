#pragma once
#include "DgViewer.h"
#include "DgVolume.h"

enum class EditMode {
	Select,		// 선택 모드 (기본)
	Move,		// 이동 모드
	Rotate		// 회전 모드
};

class DgScene
{
public:
	bool mOpen;
	ImVec2 mSceneSize;
	DgFrmBuffer mFrameBuf;

	std::vector<float> mGroundVerts;
	GLuint mGroundVAO;
	GLuint mGroundVBO;

	std::vector<DgMesh*> mMeshList;
	std::vector<GLuint> mShaders;

	// 화면 조작을 위한 변수
	float mZoom;
	ImVec2 mStartPos;
	glm::mat4 mRotMat;
	glm::vec3 mPan;

	// 드래그 선택을 위한 변수
	bool mIsDragSelecting;			// 드래그 선택 중인지
	ImVec2 mDragStartPos;			// 드래그 시작 위치 (스크린 좌표)
	ImVec2 mDragEndPos;				// 드래그 현재/끝 위치 (스크린 좌표)
	ImVec2 mWindowPos;				// SceneGL 윈도우 위치

	// 바운딩 박스 렌더링을 위한 변수
	GLuint mBBoxVAO;
	GLuint mBBoxVBO;
	GLuint mBBoxShader;				// 바운딩 박스 전용 셰이더
	bool mBBoxBufferInitialized;

	// 편집 모드 관련 변수
	EditMode mEditMode = EditMode::Select;	// 현재 편집 모드
	bool mIsMoving = false;					// 이동 드래그 중인지
	ImVec2 mMoveStartPos;					// 이동 시작 마우스 위치

private:
	std::vector<DgVolume*> mSDFList; //DgVolume 객체 관리 리스트

	DgScene()
	{
		mOpen = true;
		mZoom = -45.0f;
		mStartPos = ImVec2(0.0f, 0.0f);
		mRotMat = glm::mat4(1.0f);
		mRotMat = glm::rotate(mRotMat, glm::radians(30.0f), glm::vec3(1, 0, 0)); // pitch
		mRotMat = glm::rotate(mRotMat, glm::radians(60.0f), glm::vec3(0, 1, 0)); // yaw
		mPan = glm::vec3(0.0f);

		// 드래그 선택 초기화
		mIsDragSelecting = false;
		mDragStartPos = ImVec2(0.0f, 0.0f);
		mDragEndPos = ImVec2(0.0f, 0.0f);
		mWindowPos = ImVec2(0.0f, 0.0f);

		// 바운딩 박스 버퍼 초기화
		mBBoxVAO = 0;
		mBBoxVBO = 0;
		mBBoxShader = 0;
		mBBoxBufferInitialized = false;
	}
	~DgScene()
	{
		for (DgVolume* v : mSDFList)
			delete v;

		glDeleteVertexArrays(1, &mGroundVAO);
		glDeleteBuffers(1, &mGroundVBO);

		// 바운딩 박스 버퍼 삭제
		if (mBBoxBufferInitialized) {
			glDeleteVertexArrays(1, &mBBoxVAO);
			glDeleteBuffers(1, &mBBoxVBO);
		}

		// 바운딩 박스 셰이더 삭제
		if (mBBoxShader != 0) {
			glDeleteProgram(mBBoxShader);
		}

		for (DgMesh* m : mMeshList)
			delete m;
		
		for (GLuint id : mShaders)
			glDeleteProgram(id);
	}
	

public:
	static DgScene& instance() {
		static DgScene _inst;
		return _inst;
	}

	void setOpen(bool v) {
		mOpen = v;
	}

	bool isOpen() const {
		return mOpen;
	}

	std::vector<DgVolume*>& getSDFList() { return mSDFList; }

	void initOpenGL() {				// OpenGL 초기 설정
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}

	void createGroundMesh();													// 바닥 평면 메쉬 생성
	void getSphereCoords(double x, double y, float* px, float* py, float* pz);	// 구면 좌표 계산
	void showWindow();															// SceneGL 윈도우 출력
	void renderScene();															// 장면 렌더링
	void renderFps();															// FPS 렌더링
	void renderContextPopup();													// 컨텍스트 팝업 렌더링
	void processMouseEvent();													// 마우스 이벤트 처리
	void processKeyboardEvent();												// 키보드 이벤트 처리
	void addSDFVolume(DgVolume* volume);										// SDF 볼륨 추가
	void resetScene();															// 장면 초기화

	// 드래그 선택 관련 함수
	void renderDragSelectBox();														// 드래그 선택 박스 렌더링
	void performDragSelection(const glm::mat4& viewMat, const glm::mat4& projMat);	// 드래그 선택 수행
	bool isPointInScreenRect(const glm::vec3& worldPos, const glm::mat4& viewMat, const glm::mat4& projMat,
		const ImVec2& rectMin, const ImVec2& rectMax);								// 포인트가 스크린 사각형 내에 있는지 확인
	void clearSelection();															// 선택 해제

	// 바운딩 박스 렌더링 관련 함수
	void setupBBoxBuffer();															// 바운딩 박스 버퍼 설정
	void loadBBoxShader();															// 바운딩 박스 셰이더 로드
	void renderSelectedBoundingBoxes(const glm::mat4& viewMat, const glm::mat4& projMat);	// 선택된 볼륨의 바운딩 박스 렌더링

	// SDF 리스트 접근자
	const std::vector<DgVolume*>& getSDFList() const { return mSDFList; }

	// 편집 모드 관련 함수
	void setEditMode(EditMode mode) { mEditMode = mode; }
	EditMode getEditMode() const { return mEditMode; }
	void moveSelectedVolumes(const glm::vec3& delta);		// 선택된 볼륨 이동
	void rotateSelectedVolumes(const glm::vec3& delta);		// 선택된 볼륨 회전
	void renderEditToolbar();								// 편집 툴바 렌더링
	bool hasSelectedVolumes() const;						// 선택된 볼륨 있는지 확인
};

