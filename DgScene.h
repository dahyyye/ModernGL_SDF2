#pragma once
#include "DgViewer.h"
#include "DgVolume.h"
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

private:
	GLuint mSDFID = 0; //볼륨 텍스처 ID
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
	}
	~DgScene()
	{
		if (mSDFID != 0)
			glDeleteTextures(1, &mSDFID);
		for (DgVolume* v : mSDFList)
			delete v;

		glDeleteVertexArrays(1, &mGroundVAO);
		glDeleteBuffers(1, &mGroundVBO);

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

	void initOpenGL() {						// OpenGL 초기 설정
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}

	void createGroundMesh();				// 바닥 평면 메쉬 생성
	void getSphereCoords(double x, double y, float* px, float* py, float* pz);		// 구면 좌표 계산
	void showWindow();																// SceneGL 윈도우 출력
	void renderScene();																// 장면 렌더링
	void renderFps();																// FPS 렌더링
	void renderContextPopup();														// 컨텍스트 팝업 렌더링
	void processMouseEvent();														// 마우스 이벤트 처리
	void processKeyboardEvent();	// 키보드 이벤트 처리
	void createSDF(const DgVolume& volume);
	void addSDFVolume(DgVolume* volume);
};
