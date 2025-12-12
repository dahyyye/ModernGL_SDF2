#include "DgViewer.h"
#include ".\\include\\STB\\stb_image.h"	// 아이콘 로딩용

// 지면 격자 메쉬 생성
void DgScene::createGroundMesh()
{
	// -10부터 10까지 1.0 단위 간격으로 격자선 생성
	for (float x = -10.0f; x <= 10.0f; x += 1.0f)
	{
		// 수직선: (x, 0, -10) ~ (x, 0, 10)
		mGroundVerts.push_back(x);
		mGroundVerts.push_back(0.0f);
		mGroundVerts.push_back(-10.0f);

		mGroundVerts.push_back(x);
		mGroundVerts.push_back(0.0f);
		mGroundVerts.push_back(10.0f);

		// 수평선: (-10, 0, x) ~ (10, 0, x)
		mGroundVerts.push_back(-10.0f);
		mGroundVerts.push_back(0.0f);
		mGroundVerts.push_back(x);

		mGroundVerts.push_back(10.0f);
		mGroundVerts.push_back(0.0f);
		mGroundVerts.push_back(x);
	}
	
	// VAO(Vertex Array Object)와 VBO(Vertex Buffer Object) 생성
	glGenVertexArrays(1, &mGroundVAO);  // VAO 1개 생성
	glGenBuffers(1, &mGroundVBO);       // VBO 1개 생성

	// VAO 바인딩 (이후 설정은 이 VAO에 저장됨)
	glBindVertexArray(mGroundVAO);
	{
		// VBO 바인딩 및 데이터 업로드
		glBindBuffer(GL_ARRAY_BUFFER, mGroundVBO);  // 버퍼 타입 지정

		// 정점 데이터를 GPU 메모리에 복사(변경되지 않으므로 STATIC_DRAW)
		glBufferData(GL_ARRAY_BUFFER, mGroundVerts.size() * sizeof(float), mGroundVerts.data(), GL_STATIC_DRAW);

		// 정점 속성 설정 (location = 0, vec3 위치 좌표)
		glVertexAttribPointer(
			0,                  // layout(location = 0)
			3,                  // vec3: 3개의 float
			GL_FLOAT,           // 데이터 타입
			GL_FALSE,           // 정규화 여부 (정점 위치는 정규화하지 않음)
			3 * sizeof(float),  // stride: 한 점당 3개의 float (12 bytes)
			(void*)0            // 시작 오프셋 (배열 첫 위치부터)
		);
		glEnableVertexAttribArray(0);  // location 0 사용 활성화
	}
	glBindVertexArray(0);   // VAO 언바인딩 (추후 다른 객체 설정에 영향을 주지 않도록)
}

// 바운딩 박스 와이어프레임 버퍼 설정
void DgScene::setupBBoxBuffer()
{
	if (mBBoxBufferInitialized) return;

	// 바운딩 박스 와이어프레임용 정점 (단위 큐브, 12개 엣지 = 24개 정점)
	float bboxVerts[] = {
		// 아래면 4개 엣지
		0, 0, 0,  1, 0, 0,
		1, 0, 0,  1, 0, 1,
		1, 0, 1,  0, 0, 1,
		0, 0, 1,  0, 0, 0,
		// 위면 4개 엣지
		0, 1, 0,  1, 1, 0,
		1, 1, 0,  1, 1, 1,
		1, 1, 1,  0, 1, 1,
		0, 1, 1,  0, 1, 0,
		// 수직 4개 엣지
		0, 0, 0,  0, 1, 0,
		1, 0, 0,  1, 1, 0,
		1, 0, 1,  1, 1, 1,
		0, 0, 1,  0, 1, 1
	};

	glGenVertexArrays(1, &mBBoxVAO);
	glGenBuffers(1, &mBBoxVBO);

	glBindVertexArray(mBBoxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mBBoxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(bboxVerts), bboxVerts, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	mBBoxBufferInitialized = true;
}

// 바운딩 박스 셰이더 로드
void DgScene::loadBBoxShader()
{
	if (mBBoxShader == 0)
	{
		mBBoxShader = load_shaders(".\\shaders\\bbox.vert", ".\\shaders\\bbox.frag");
		if (mBBoxShader == 0)
		{
			std::cerr << "바운딩 박스 셰이더 로드 실패, ground 셰이더 사용" << std::endl;
			mBBoxShader = mShaders[0];  // fallback to ground shader
		}
	}
}

// 구면 좌표 계산
void DgScene::getSphereCoords(double x, double y, float* px, float* py, float* pz)
{
	*px = (2.0f * (float)x - mSceneSize[0]) / mSceneSize[0];
	*py = (-2.0f * (float)y + mSceneSize[1]) / mSceneSize[1];
	float r = (*px) * (*px) + (*py) * (*py);
	if (r >= 1.0f) {
		*px /= std::sqrtf(r);
		*py /= std::sqrtf(r);
		*pz = 0.0;
	}
	else
		*pz = std::sqrtf(1.0f - r);
}

// SceneGL 윈도우 출력
void DgScene::showWindow()
{
	// 오픈 상태가 아니면 리턴한다.
	if (!mOpen)	return;

	// 윈도우 플래그(window flag)를 설정한다.
	static bool no_titlebar = false;
	static bool no_scrollbar = false;
	static bool no_menu = true;
	static bool no_move = false;
	static bool no_resize = false;
	static bool no_collapse = false;
	static bool no_close = true;
	static bool no_nav = false;
	static bool no_background = false;
	static bool no_bring_to_front = false;
	static bool no_docking = false;
	static bool unsaved_document = false;

	ImGuiWindowFlags window_flags = 0;
	if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
	if (no_scrollbar)       window_flags |= ImGuiWindowFlags_NoScrollbar;
	if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
	if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
	if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;
	if (no_collapse)        window_flags |= ImGuiWindowFlags_NoCollapse;
	if (no_nav)             window_flags |= ImGuiWindowFlags_NoNav;
	if (no_background)      window_flags |= ImGuiWindowFlags_NoBackground;
	if (no_bring_to_front)  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (no_docking)         window_flags |= ImGuiWindowFlags_NoDocking;
	if (unsaved_document)   window_flags |= ImGuiWindowFlags_UnsavedDocument;

	// 장면 윈도우를 생성하고, collapsed된 상태라면 바로 리턴한다.
	bool* openPtr = (no_close ? nullptr : &mOpen);
	if (!ImGui::Begin("SceneGL", openPtr, window_flags))
	{
		ImGui::End();
		return;
	}

	// 윈도우 위치 저장 (드래그 선택 좌표 계산용)
	mWindowPos = ImGui::GetWindowPos();

	// 편집 툴바 렌더링 추가
	renderEditToolbar();

	// 마우스 이벤트를 처리
	processMouseEvent();

	// 키보드 이벤트를 처리
	processKeyboardEvent();

	// 장면과 도구를 렌더링
	renderScene();

	// 드래그 선택 박스 렌더링 (ImGui 오버레이)
	renderDragSelectBox();

	// Context 팝업 메뉴를 렌더링
	renderContextPopup();

	ImGui::End();
}

// 아이콘 텍스처 로딩 헬퍼 함수 추가
static GLuint LoadIconTexture(const char* file) {
	int w, h, ch;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* pixels = stbi_load(file, &w, &h, &ch, 0);
	if (!pixels) return 0;

	GLenum fmt = (ch == 4) ? GL_RGBA : (ch == 3) ? GL_RGB : GL_RED;

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(pixels);
	return tex;
}

static inline ImTextureID ToImTexID(GLuint tex) {
	return (ImTextureID)(uintptr_t)tex;
}


// 편집 툴바 렌더링 함수
void DgScene::renderEditToolbar()
{
	// 아이콘 텍스처 로딩 (최초 1회만)
	static GLuint moveIcon = 0;
	static bool iconsLoaded = false;

	if (!iconsLoaded)
	{
		moveIcon = LoadIconTexture(".\\res\\icons\\move.png");
		iconsLoaded = true;
	}

	ImGui::BeginGroup();

	const ImVec2 iconSize(28, 28);

	// 이동 모드 버튼 (move.png 아이콘 사용)
	bool isMoveMode = (mEditMode == EditMode::Move);
	if (isMoveMode)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.5f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.6f, 0.3f, 1.0f));
	}

	if (moveIcon != 0)
	{
		if (ImGui::ImageButton("MoveMode", ToImTexID(moveIcon), iconSize, ImVec2(0, 1), ImVec2(1, 0)))
		{
			// 토글: Move <-> Select
			mEditMode = (mEditMode == EditMode::Move) ? EditMode::Select : EditMode::Move;
		}
	}
	else
	{
		if (ImGui::Button("Move", ImVec2(50, 30)))
		{
			mEditMode = (mEditMode == EditMode::Move) ? EditMode::Select : EditMode::Move;
		}
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Move Tool (Toggle)");
	if (isMoveMode) ImGui::PopStyleColor(2);

	ImGui::EndGroup();
	ImGui::Separator();
}

void DgScene::processMouseEvent()
{
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None))
	{
		// 현재 윈도우의 좌측 상단을 기준(0, 0)으로 마우스 좌표(x, y)를 구한다.
		ImVec2 pos = ImGui::GetMousePos() - ImGui::GetCursorScreenPos();
		int x = (int)pos.x, y = (int)pos.y;
		ImVec2 delta = ImGui::GetIO().MouseDelta;
		ImGuiIO& io = ImGui::GetIO();											// Ctrl 상태 확인

		if (io.KeyCtrl && ImGui::IsMouseClicked(ImGuiMouseButton_Left))			// 왼쪽 버튼을 클릭한 경우
		{
			mStartPos[0] = pos[0];
			mStartPos[1] = pos[1];
		}
		else if (io.KeyCtrl && ImGui::IsMouseDragging(ImGuiMouseButton_Left))	// 왼쪽 버튼으로 드래깅하는 경우
		{
			float px, py, pz, qx, qy, qz;
			getSphereCoords(mStartPos[0], mStartPos[1], &px, &py, &pz);
			getSphereCoords(pos[0], pos[1], &qx, &qy, &qz);
			glm::vec3 rotAxis = glm::cross(glm::vec3(px, py, pz), glm::vec3(qx, qy, qz));
			if (glm::length(rotAxis) > 0.000001f)
			{
				float angle = acos(px * qx + py * qy + pz * qz);
				mRotMat = glm::rotate(glm::mat4(1.0f), angle, glm::normalize(rotAxis)) * mRotMat;
			}
			mStartPos[0] = pos[0];
			mStartPos[1] = pos[1];
		}

		// 이동 모드에서 좌클릭 드래그: 선택된 볼륨 이동
		else if (!io.KeyCtrl && mEditMode == EditMode::Move && hasSelectedVolumes())
		{
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				mIsMoving = true;
				mMoveStartPos = pos;
			}
			else if (mIsMoving && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
			{
				// 마우스 이동량 계산
				float dx = (pos.x - mMoveStartPos.x) * 0.02f;
				float dy = (mMoveStartPos.y - pos.y) * 0.02f;

				// 카메라 회전을 고려하여 월드 좌표로 변환
				glm::vec3 worldDelta = glm::inverse(glm::mat3(mRotMat)) * glm::vec3(dx, dy, 0.0f);

				// 선택된 볼륨들 이동
				moveSelectedVolumes(worldDelta);

				mMoveStartPos = pos;
			}
			else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				mIsMoving = false;
			}
		}

		// 좌클릭 (Ctrl 없이): 드래그 선택 시작
		else if (!io.KeyCtrl && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			mIsDragSelecting = true;
			mDragStartPos = ImGui::GetMousePos();
			mDragEndPos = mDragStartPos;
		}

		// 드래그 선택 중: 끝 위치 업데이트
		else if (!io.KeyCtrl && mIsDragSelecting && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			mDragEndPos = ImGui::GetMousePos();
		}

		else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))		// 클릭했던 왼쪽 버튼을 놓는 경우
		{
			// 드래그 선택 완료: 선택 수행
			if (mIsDragSelecting)
			{
				mDragEndPos = ImGui::GetMousePos();

				// 투영/뷰 행렬 계산
				glm::mat4 projMat = glm::perspective(glm::radians(30.0f), mSceneSize[0] / mSceneSize[1], 1.0f, 1000.0f);
				glm::mat4 viewMat(1.0f);
				viewMat = glm::translate(viewMat, glm::vec3(0.0, 0.0, mZoom));
				viewMat = viewMat * mRotMat;
				viewMat = glm::translate(viewMat, glm::vec3(mPan[0], mPan[1], mPan[2]));

				// 드래그 선택 수행
				performDragSelection(viewMat, projMat);

				mIsDragSelecting = false;
			}
			mStartPos[0] = mStartPos[1] = 0.0;
		}
		else if (io.KeyCtrl && ImGui::IsMouseClicked(ImGuiMouseButton_Middle))	// 중간 버튼을 클릭한 경우
		{
			mStartPos[0] = pos[0];
			mStartPos[1] = pos[1];
		}
		else if (io.KeyCtrl && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))	// 중간 버튼으로 드래깅하는 경우
		{
			float dx = (float)(pos[0] - mStartPos[0]) * 0.01f;
			float dy = (float)(mStartPos[1] - pos[1]) * 0.01f;
			mPan += glm::inverse(glm::mat3(mRotMat)) * glm::vec3(dx, dy, 0.0f);
			mStartPos[0] = pos[0];
			mStartPos[1] = pos[1];
		}
		else if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle))	// 클릭했던 중간 버튼을 놓은 경우
		{
		}
		else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))		// 오른쪽 버튼을 클릭한 경우
		{
		}
		else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))	// 오른쪽 버튼으로 드래깅하는 경우 
		{
		}
		else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))	// 클릭했던 오른쪽 버튼을 놓은 경우
		{
		}
		else if (delta.x != 0.0f || delta.y != 0.0f)				// 그냥 움직이는 경우
		{
		}

		// 장면의 줌인/아웃을 수행한다.
		if (ImGui::GetIO().MouseWheel != 0.0f)
		{
			int dir = (ImGui::GetIO().MouseWheel > 0.0) ? 1 : -1;
			mZoom += (float)dir;
		}
	}
}

// 선택된 볼륨 이동 함수 추가
void DgScene::moveSelectedVolumes(const glm::vec3& delta)
{
	for (DgVolume* pVolume : mSDFList)
	{
		if (pVolume != nullptr && pVolume->mSelected)
		{
			pVolume->translate(delta);
		}
	}
}


// [수정] 선택된 볼륨이 있는지 확인 함수 추가
bool DgScene::hasSelectedVolumes() const
{
	for (DgVolume* v : mSDFList)
	{
		if (v && v->mSelected) return true;
	}
	return false;
}

// 드래그 선택 박스 렌더링
void DgScene::renderDragSelectBox()
{
	if (!mIsDragSelecting) return;

	// 드래그 선택 박스를 ImGui DrawList로 그리기
	ImDrawList* drawList = ImGui::GetForegroundDrawList();

	ImVec2 minPos(std::min(mDragStartPos.x, mDragEndPos.x), std::min(mDragStartPos.y, mDragEndPos.y));
	ImVec2 maxPos(std::max(mDragStartPos.x, mDragEndPos.x), std::max(mDragStartPos.y, mDragEndPos.y));

	// 반투명 채우기
	drawList->AddRectFilled(minPos, maxPos, IM_COL32(100, 180, 120, 50));
	// 테두리
	drawList->AddRect(minPos, maxPos, IM_COL32(80, 160, 100, 255), 0.0f, 0, 1.0f);
}

// 월드 좌표가 스크린 사각형 내에 있는지 확인
bool DgScene::isPointInScreenRect(const glm::vec3& worldPos, const glm::mat4& viewMat, const glm::mat4& projMat,
	const ImVec2& rectMin, const ImVec2& rectMax)
{
	// 월드 좌표를 클립 좌표로 변환
	glm::vec4 clipPos = projMat * viewMat * glm::vec4(worldPos, 1.0f);

	// w가 0 이하면 카메라 뒤에 있음
	if (clipPos.w <= 0.0f) return false;

	// NDC로 변환
	glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

	// NDC를 스크린 좌표로 변환
	float screenX = (ndc.x * 0.5f + 0.5f) * mSceneSize.x;
	float screenY = (1.0f - (ndc.y * 0.5f + 0.5f)) * mSceneSize.y;

	// 윈도우 오프셋 적용 (콘텐츠 영역 기준)
	ImVec2 contentPos = mWindowPos + ImGui::GetStyle().WindowPadding;
	contentPos.y += ImGui::GetFrameHeight(); // 타이틀바 높이

	screenX += contentPos.x;
	screenY += contentPos.y;

	// 사각형 내부에 있는지 확인
	return (screenX >= rectMin.x && screenX <= rectMax.x &&
		screenY >= rectMin.y && screenY <= rectMax.y);
}

// 드래그 선택 수행
void DgScene::performDragSelection(const glm::mat4& viewMat, const glm::mat4& projMat)
{
	// 드래그 영역이 너무 작으면 (클릭만 한 경우) 선택 해제
	float dragDist = glm::length(glm::vec2(mDragEndPos.x - mDragStartPos.x, mDragEndPos.y - mDragStartPos.y));
	if (dragDist < 5.0f)
	{
		clearSelection();
		return;
	}

	// 선택 사각형 계산
	ImVec2 rectMin(std::min(mDragStartPos.x, mDragEndPos.x), std::min(mDragStartPos.y, mDragEndPos.y));
	ImVec2 rectMax(std::max(mDragStartPos.x, mDragEndPos.x), std::max(mDragStartPos.y, mDragEndPos.y));

	// 기존 선택 해제
	clearSelection();

	// 각 볼륨에 대해 중심점이 선택 영역에 있는지 확인
	for (DgVolume* pVolume : mSDFList)
	{
		if (pVolume == nullptr) continue;

		// 이동된 위치 반영
		glm::vec3 center = pVolume->getCenter();
		glm::vec3 minPos = pVolume->getTransformedMin();
		glm::vec3 maxPos = pVolume->getTransformedMax();

		glm::vec3 corners[8] = {
			glm::vec3(minPos.x, minPos.y, minPos.z),
			glm::vec3(maxPos.x, minPos.y, minPos.z),
			glm::vec3(maxPos.x, maxPos.y, minPos.z),
			glm::vec3(minPos.x, maxPos.y, minPos.z),
			glm::vec3(minPos.x, minPos.y, maxPos.z),
			glm::vec3(maxPos.x, minPos.y, maxPos.z),
			glm::vec3(maxPos.x, maxPos.y, maxPos.z),
			glm::vec3(minPos.x, maxPos.y, maxPos.z)
		};

		bool selected = false;

		// 중심점이 선택 영역에 있으면 선택
		if (isPointInScreenRect(center, viewMat, projMat, rectMin, rectMax))
		{
			selected = true;
		}
		else
		{
			// 코너 중 하나라도 선택 영역에 있으면 선택
			for (int i = 0; i < 8; ++i)
			{
				if (isPointInScreenRect(corners[i], viewMat, projMat, rectMin, rectMax))
				{
					selected = true;
					break;
				}
			}
		}

		if (selected)
		{
			pVolume->mSelected = true;
		}
	}
}

// 모든 볼륨 선택 해제
void DgScene::clearSelection()
{
	for (DgVolume* pVolume : mSDFList)
	{
		if (pVolume != nullptr)
		{
			pVolume->mSelected = false;
		}
	}
}

// 선택된 볼륨의 바운딩 박스 렌더링 (와이어프레임)
void DgScene::renderSelectedBoundingBoxes(const glm::mat4& viewMat, const glm::mat4& projMat)
{
	// 바운딩 박스 버퍼 초기화
	setupBBoxBuffer();

	// 바운딩 박스 셰이더 로드
	loadBBoxShader();

	// 바운딩 박스 셰이더 사용
	GLuint shaderProgram = mBBoxShader;
	glUseProgram(shaderProgram);

	// 선 두께 설정
	glLineWidth(2.0f);

	// 깊이 테스트 비활성화 (항상 보이도록)
	glEnable(GL_DEPTH_TEST);

	for (DgVolume* pVolume : mSDFList)
	{
		if (pVolume == nullptr || !pVolume->mSelected) continue;

		// 이동된 위치 반영
		glm::vec3 minPos = pVolume->getTransformedMin();
		glm::vec3 maxPos = pVolume->getTransformedMax();
		glm::vec3 size = maxPos - minPos;

		glm::mat4 modelMat(1.0f);
		modelMat = glm::translate(modelMat, minPos);
		modelMat = glm::scale(modelMat, size);

		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(viewMat));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projMat));

		// 이동 모드일 때 바운딩 박스 색상 변경
		if (mEditMode == EditMode::Move)
			glUniform3f(glGetUniformLocation(shaderProgram, "uColor"), 1.0f, 0.6f, 0.2f);
		else
			glUniform3f(glGetUniformLocation(shaderProgram, "uColor"), 0.85f, 0.4f, 0.35f);

		// 바운딩 박스 와이어프레임 렌더링
		glBindVertexArray(mBBoxVAO);
		glDrawArrays(GL_LINES, 0, 24);
		glBindVertexArray(0);
	}

	// 깊이 테스트 다시 활성화
	glEnable(GL_DEPTH_TEST);
	glLineWidth(1.0f);
	glUseProgram(0);
}

void DgScene::renderScene()
{
	// 현재 윈도우(3D Scene)의 정보를 구하여, 렌더링 버퍼를 갱신한다.
	ImVec2 sceneSize = ImGui::GetContentRegionAvail();
	if (sceneSize[0] != mSceneSize[0] || sceneSize[1] != mSceneSize[1])
	{
		mSceneSize = sceneSize;
		mFrameBuf.rescaleFrameBuffer((int)sceneSize[0], (int)sceneSize[1]);
	}

	// 장면 칼라 버퍼에 렌더링
	mFrameBuf.bind();
	{
		glViewport(0, 0, (GLsizei)mSceneSize[0], (GLsizei)mSceneSize[1]);
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);				

		// 투영 변환 행렬 설정
		glm::mat4 projMat = glm::perspective(glm::radians(30.0f), mSceneSize[0] / mSceneSize[1], 1.0f, 1000.0f);

		// 관측 변환 행렬
		glm::mat4 viewMat(1.0f);													// 단위 행렬 초기화, M = I
		viewMat = glm::translate(viewMat, glm::vec3(0.0, 0.0, mZoom));				// 줌 변환, M = I * T
		viewMat = viewMat * mRotMat;												// 회전 변환, M = I * T * R
		viewMat = glm::translate(viewMat, glm::vec3(mPan[0], mPan[1], mPan[2]));	// Pan 변환, M = I * T * R * Pan

		// 바닥 렌더링
		{
			// 모델링 변환 행렬(단위 행렬)
			glm::mat4 modelMat(1.0f);
			glUseProgram(mShaders[0]);

			// 정점 쉐이더에 파라미터 전달
			glUniformMatrix4fv(glGetUniformLocation(mShaders[0], "uModel"), 1, GL_FALSE, glm::value_ptr(modelMat));
			glUniformMatrix4fv(glGetUniformLocation(mShaders[0], "uView"), 1, GL_FALSE, glm::value_ptr(viewMat));
			glUniformMatrix4fv(glGetUniformLocation(mShaders[0], "uProjection"), 1, GL_FALSE, glm::value_ptr(projMat));

			// 바닥 평면 그리기
			glBindVertexArray(mGroundVAO);
			glDrawArrays(GL_LINES, 0, mGroundVerts.size() / 3);
			glBindVertexArray(0);
			glUseProgram(0);
		}

		// Mesh 렌더링
		for (DgMesh* pMesh : mMeshList)
		{
			// 모델링 변환 행렬(단위 행렬)
			glm::mat4 modelMat(1.0f);

			// 쉐이더 프로그램 설정
			GLuint shaderProgram = pMesh->mShaderId;
			glUseProgram(shaderProgram);

			// 정점 쉐이더에 파라미터 전달
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(modelMat));
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(viewMat));
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projMat));

			// 조명의 속성과 관측 위치 전달
			glm::vec3 viewPos = glm::vec3(glm::inverse(viewMat)[3]);
			glm::vec3 lightPos = glm::vec3(glm::inverse(viewMat)[3]);
			glUniform3fv(glGetUniformLocation(shaderProgram, "uViewPos"), 1, glm::value_ptr(viewPos));
			glUniform3fv(glGetUniformLocation(shaderProgram, "uLightPos"), 1, glm::value_ptr(lightPos));
			glUniform3fv(glGetUniformLocation(shaderProgram, "uLightColor"), 1, glm::value_ptr(glm::vec3(1.0f)));


			// 매 프레임마다 시간 값 계산
			float timeValue = static_cast<float>(glfwGetTime());

			// uTime 위치 얻기
			GLint timeLoc = glGetUniformLocation(shaderProgram, "uTime");

			// 유니폼에 값 전달
			glUseProgram(shaderProgram);
			glUniform1f(timeLoc, timeValue);

			// 모델 렌더링 하기
			pMesh->render();
			glUseProgram(0);
		}

		// SDF 볼륨 렌더링
		for (DgVolume* pVolume : mSDFList)
		{
			if (pVolume == nullptr || pVolume->mTextureID == 0) continue;
			glm::mat4 modelMat(1.0f);
			modelMat = glm::translate(modelMat, pVolume->mPosition);

			GLuint shaderProgram = mShaders[10];
			glUseProgram(shaderProgram);

			// 행렬 유니폼
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(modelMat));
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(viewMat));
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projMat));
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uInvView"), 1, GL_FALSE, glm::value_ptr(glm::inverse(viewMat)));
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uInvProj"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projMat)));
			glUniform2f(glGetUniformLocation(shaderProgram, "uResolution"), mSceneSize[0], mSceneSize[1]);

			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProj"), 1, GL_FALSE, glm::value_ptr(projMat));  // fragment shader용

			// 이동된 위치를 반영하여 uVolumeMin/Max 전달
			glm::vec3 transformedMin = pVolume->getTransformedMin();
			glm::vec3 transformedMax = pVolume->getTransformedMax();
			glUniform3f(glGetUniformLocation(shaderProgram, "uVolumeMin"),
				transformedMin.x, transformedMin.y, transformedMin.z);
			glUniform3f(glGetUniformLocation(shaderProgram, "uVolumeMax"),
				transformedMax.x, transformedMax.y, transformedMax.z);

			// 3D 텍스처 바인딩
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_3D, pVolume->mTextureID);
			glUniform1i(glGetUniformLocation(shaderProgram, "uSDFVolume"), 0);

			// Cull Face 비활성화
			glDisable(GL_CULL_FACE);

			pVolume->mMesh->render();

			glEnable(GL_CULL_FACE);
			glBindTexture(GL_TEXTURE_3D, 0);
			glUseProgram(0);
		}

		// 선택된 볼륨의 바운딩 박스 렌더링
		renderSelectedBoundingBoxes(viewMat, projMat);

		// FPS 렌더링
		renderFps();					
	}
	mFrameBuf.unbind();

	ImTextureID textureID = (void*)(uintptr_t)mFrameBuf.getFrameTexture();
	ImGui::Image(textureID, ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
}

void DgScene::renderFps()
{
	// 출력할 윈도우의 위치와 투명도를 설정한다.
	const float D = 10.0f;
	static int corner = 3;
	float W = ImGui::GetWindowSize().x;
	float H = ImGui::GetWindowSize().y;

	if (corner != -1)
	{
		ImVec2 pos = ImGui::GetWindowPos(), pivot;
		switch (corner)
		{
		case 0: pivot = ImVec2(0.0f, 0.0f); pos += ImVec2(D, D); break;
		case 1: pivot = ImVec2(1.0f, 0.0f); pos += ImVec2(W - D, D); break;
		case 2: pivot = ImVec2(0.0f, 1.0f); pos += ImVec2(D, H - D); break;
		case 3: pivot = ImVec2(1.0f, 1.0f); pos += ImVec2(W - D, H - D); break;
		}
		ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
	}
	ImGui::SetNextWindowBgAlpha(0.35f);

	// 마우스 좌표를 구한다.
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 pos = ImGui::GetMousePos() - ImGui::GetCursorScreenPos();

	// 윈도우를 생성하고 메시지를 출력한다.
	bool open = true;
	bool* p_open = &open;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 7.0f);
	if (ImGui::Begin("Example: Simple overlay", p_open, (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
	{
		ImGui::Text("Rendering Speed: %.1f FPS", io.Framerate);
		ImGui::Separator();
		if (ImGui::IsMousePosValid())
			ImGui::Text("Mouse Position: (%d,%d)", (int)pos.x, (int)pos.y);
		else
			ImGui::Text("Mouse Position: <invalid>");

		// 선택된 볼륨 수 표시
		int selectedCount = 0;
		for (DgVolume* v : mSDFList)
		{
			if (v && v->mSelected) selectedCount++;
		}
		if (selectedCount > 0)
		{
			ImGui::Text("Selected: %d volume(s)", selectedCount);
		}

		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::MenuItem("Custom", NULL, corner == -1)) corner = -1;
			if (ImGui::MenuItem("Top-left", NULL, corner == 0)) corner = 0;
			if (ImGui::MenuItem("Top-right", NULL, corner == 1)) corner = 1;
			if (ImGui::MenuItem("Bottom-left", NULL, corner == 2)) corner = 2;
			if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
			if (p_open && ImGui::MenuItem("Close")) *p_open = false;
			ImGui::EndPopup();
		}
		ImGui::End();
	}
	ImGui::PopStyleVar();
}

void DgScene::processKeyboardEvent()
{
	// 키보드 이벤트를 처리한다.
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None))
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			glfwSetWindowShouldClose(ImGuiManager::instance().mWindow, true);
		}		
	}
}

void DgScene::renderContextPopup()
{
	if (ImGui::BeginPopupContextWindow("SceneContext", ImGuiPopupFlags_MouseButtonRight))
	{
		static const char* kShaderLabel[] = {
			"0: Black",
			"1: Toon", 
			"2: Phong", 
			"3: Texture",
			"4: Effect-1", 
			"5: Effect-2",
			"6: Normals", 
			"7: Fresnel RGB",
			"8: Effect-5", 
			"9: Effect-6"
		};
		const int labelCount = IM_ARRAYSIZE(kShaderLabel);
		const int shaderCount = (int)mShaders.size();
		const int showCount = (shaderCount < labelCount) ? shaderCount : labelCount;

		ImGui::TextUnformatted("Shaders");
		ImGui::Separator();

		if (shaderCount == 0) {
			ImGui::TextDisabled("No shaders loaded");
			ImGui::EndPopup();
			return;
		}
		
		// --- per-mesh ---
		for (size_t i = 0; i < mMeshList.size(); ++i)
		{
			DgMesh* M = mMeshList[i];
			if (!M) continue;

			ImGui::PushID((int)i);
			char menuLabel[128];
			if (!M->mName.empty()) 
				snprintf(menuLabel, sizeof(menuLabel), "%s", M->mName.c_str());
			else                   
				snprintf(menuLabel, sizeof(menuLabel), "Mesh %zu", i);

			if (ImGui::BeginMenu(menuLabel))
			{
				// 1~9 라벨
				for (int s = 0; s < showCount; ++s)
				{
					bool selected = (M->mShaderId == mShaders[s]);
					if (ImGui::MenuItem(kShaderLabel[s], nullptr, selected))
						M->mShaderId = mShaders[s];
				}				
				ImGui::EndMenu();
			}
			ImGui::PopID();
		}

		ImGui::EndPopup();
	}
}

void DgScene::addSDFVolume(DgVolume* volume)
{
	mSDFList.push_back(volume);
}

void DgScene::resetScene()
{
	// 1. 모든 볼륨 삭제
	for (DgVolume* v : mSDFList)
	{
		delete v;  // 소멸자에서 mMesh와 mTextureID도 정리됨
	}
	mSDFList.clear();

	// 2. 카메라 초기화
	mZoom = -45.0f;
	mRotMat = glm::mat4(1.0f);
	mRotMat = glm::rotate(mRotMat, glm::radians(30.0f), glm::vec3(1, 0, 0));  // pitch
	mRotMat = glm::rotate(mRotMat, glm::radians(60.0f), glm::vec3(0, 1, 0));  // yaw
	mPan = glm::vec3(0.0f);

	std::cout << "장면 초기화 완료" << std::endl;
}
