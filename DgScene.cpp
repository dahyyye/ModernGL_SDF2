#include "DgViewer.h"

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

	ImGuiWindowFlags window_flags = 0;

	if (!ImGui::Begin("SceneGL", nullptr, window_flags))
	{
		ImGui::End();
		return;
	}

	ImGuizmo::BeginFrame();

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

// 편집 툴바 렌더링 함수
void DgScene::renderEditToolbar()
{
	// 아이콘 텍스처 로딩 (최초 1회만)
	static GLuint moveIcon = 0;
	static GLuint rotateIcon = 0;
	static GLuint scaleIcon = 0;
	static bool iconsLoaded = false;

	if (!iconsLoaded)
	{
		moveIcon = DgUtil::loadTexture2D(".\\res\\icons\\move.png");
		rotateIcon = DgUtil::loadTexture2D(".\\res\\icons\\rotation.png");
		scaleIcon = DgUtil::loadTexture2D(".\\res\\icons\\scale.png");
		iconsLoaded = true;
	}

	ImGui::BeginGroup();

	const ImVec2 iconSize(28, 28);

	// Move 버튼
	bool isMoveMode = (mEditMode == EditMode::Move);
	if (isMoveMode) { ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.5f, 0.2f, 1.0f)); ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.6f, 0.3f, 1.0f)); }
	if (moveIcon != 0) {
		if (ImGui::ImageButton("MoveMode", DgUtil::toImTextureID(moveIcon), iconSize, ImVec2(0, 1), ImVec2(1, 0)))
			mEditMode = (mEditMode == EditMode::Move) ? EditMode::Select : EditMode::Move;
	}
	else {
		if (ImGui::Button("Move", ImVec2(50, 30)))
			mEditMode = (mEditMode == EditMode::Move) ? EditMode::Select : EditMode::Move;
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Move Tool (Toggle)");
	if (isMoveMode) ImGui::PopStyleColor(2);

	ImGui::SameLine();

	// Rotate 버튼
	bool isRotateMode = (mEditMode == EditMode::Rotate);
	if (isRotateMode) { ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.9f, 1.0f)); ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 1.0f, 1.0f)); }
	if (rotateIcon != 0) {
		if (ImGui::ImageButton("RotateMode", DgUtil::toImTextureID(rotateIcon), iconSize, ImVec2(0, 1), ImVec2(1, 0)))
			mEditMode = (mEditMode == EditMode::Rotate) ? EditMode::Select : EditMode::Rotate;
	}
	else {
		if (ImGui::Button("Rotate", ImVec2(50, 30)))
			mEditMode = (mEditMode == EditMode::Rotate) ? EditMode::Select : EditMode::Rotate;
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate Tool (Toggle)");
	if (isRotateMode) ImGui::PopStyleColor(2);

	ImGui::SameLine();

	// Scale 버튼
	bool isScaleMode = (mEditMode == EditMode::Scale);
	if (isScaleMode) { ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.4f, 1.0f)); ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.9f, 0.5f, 1.0f)); }
	if (scaleIcon != 0) {
		if (ImGui::ImageButton("ScaleMode", DgUtil::toImTextureID(scaleIcon), iconSize, ImVec2(0, 1), ImVec2(1, 0)))
			mEditMode = (mEditMode == EditMode::Scale) ? EditMode::Select : EditMode::Scale;
	}
	else {
		if (ImGui::Button("Scale", ImVec2(50, 30)))
			mEditMode = (mEditMode == EditMode::Scale) ? EditMode::Select : EditMode::Scale;
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale Tool (Toggle)");
	if (isScaleMode) ImGui::PopStyleColor(2);

	ImGui::EndGroup();
	ImGui::Separator();
}

// 마우스 이벤트 처리
void DgScene::processMouseEvent()
{
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None))
	{
		if (ImGuizmo::IsUsing()) return;    // ← 이 줄 추가

		// 키프레임 클릭 선택
		if (mEditMode != EditMode::Trajectory
			&& mEditMode != EditMode::Select
			&& mSelectedSweptVolume != nullptr
			&& mSelectedSweptVolume->mSourceTrajectory != nullptr
			&& (int)mSelectedSweptVolume->mSourceTrajectory->keyframes.size() >= 2)
		{
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)
				&& !ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
			{
				auto& srcTraj = *mSelectedSweptVolume->mSourceTrajectory;
				ImVec2 mouse = ImGui::GetMousePos();
				const float kPickRadius = 2.5f;
				float bestDist = kPickRadius;
				int   bestIdx = -1;
				glm::mat4 modelMat = mSelectedSweptVolume->getModelMatrix();
				for (int i = 0; i < (int)srcTraj.keyframes.size(); ++i)
				{
					// 키프레임 로컬 좌표 → 볼륨 모델 변환 적용 → 월드 좌표
					glm::vec3 cp = glm::vec3(modelMat * glm::vec4(srcTraj.keyframes[i].position, 1.0f));
					glm::vec3 mw = mouseToWorld(mouse, cp.y);
					float dist = sqrtf(powf(mw.x - cp.x, 2) + powf(mw.z - cp.z, 2));
					if (dist < bestDist) { bestDist = dist; bestIdx = i; }
				}
				if (bestIdx >= 0) {
					mSelectedKeyframeIdx = bestIdx;
					return;
				}
			}
		}

		if (mEditMode == EditMode::Trajectory) return;

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

		// 좌클릭 (Ctrl 없이): 드래그 선택 시작
		else if (!io.KeyCtrl && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
			&& !ImGuizmo::IsOver())
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

		else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
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

// 선택된 볼륨 이동 함수
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

// 선택된 볼륨 회전 함수
void DgScene::rotateSelectedVolumes(const glm::vec3& delta)
{
	for (DgVolume* pVolume : mSDFList)
	{
		if (pVolume != nullptr && pVolume->mSelected)
		{
			pVolume->rotate(delta);
		}
	}
}

// 선택된 볼륨이 있는지 확인하는 함수
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
	mSelectedSweptVolume = nullptr;
	mSelectedKeyframeIdx = -1;
	for (DgVolume* v : mSDFList) {
		if (v && v->mSelected && v->mIsSweptVolume
			&& v->mSourceTrajectory != nullptr
			&& (int)v->mSourceTrajectory->keyframes.size() >= 2) {
			mSelectedSweptVolume = v;
			break;
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
	mSelectedSweptVolume = nullptr;
	mSelectedKeyframeIdx = -1;
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

		/// 회전 포함된 모델 행렬 사용
		glm::vec3 localMin = pVolume->getLocalMin();
		glm::vec3 localMax = pVolume->getLocalMax();
		glm::vec3 size = localMax - localMin;

		// 로컬 바운딩박스를 위한 스케일/이동 행렬
		glm::mat4 bboxLocal(1.0f);
		bboxLocal = glm::translate(bboxLocal, localMin);
		bboxLocal = glm::scale(bboxLocal, size);

		// 볼륨의 모델 행렬 (이동 + 회전)을 적용
		glm::mat4 modelMat = pVolume->getModelMatrix() * bboxLocal;

		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(viewMat));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projMat));

		// 모드별 바운딩 박스 색상 변경
		if (mEditMode == EditMode::Move)
			glUniform3f(glGetUniformLocation(shaderProgram, "uColor"), 1.0f, 0.6f, 0.2f);   // 주황색
		else if (mEditMode == EditMode::Rotate)
			glUniform3f(glGetUniformLocation(shaderProgram, "uColor"), 0.2f, 0.7f, 0.9f);   // 하늘색
		else
			glUniform3f(glGetUniformLocation(shaderProgram, "uColor"), 0.85f, 0.4f, 0.35f); // 빨간색

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

// 장면 렌더링
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
			glm::vec3 lightPos = viewPos;
			glUniform3fv(glGetUniformLocation(shaderProgram, "uViewPos"), 1, glm::value_ptr(viewPos));
			glUniform3fv(glGetUniformLocation(shaderProgram, "uLightPos"), 1, glm::value_ptr(lightPos));
			glUniform3fv(glGetUniformLocation(shaderProgram, "uLightColor"), 1, glm::value_ptr(glm::vec3(1.0f)));


			// 매 프레임마다 시간 값 계산
			float timeValue = static_cast<float>(glfwGetTime());

			// uTime 위치 얻기
			GLint timeLoc = glGetUniformLocation(shaderProgram, "uTime");

			// 유니폼에 값 전달
			glUniform1f(timeLoc, timeValue);

			// 모델 렌더링 하기
			pMesh->render();
			glUseProgram(0);
		}

		glm::mat4 invViewMat = glm::inverse(viewMat);
		glm::mat4 invProjMat = glm::inverse(projMat);

		// 선택된 키프레임 위치에 브러시 볼륨 분홍색 프리뷰
		if (mSelectedSweptVolume != nullptr
			&& mSelectedSweptVolume->mBrushVolume != nullptr
			&& mSelectedSweptVolume->mSourceTrajectory != nullptr
			&& mSelectedKeyframeIdx >= 0
			&& mSelectedKeyframeIdx < (int)mSelectedSweptVolume->mSourceTrajectory->keyframes.size())
		{
			// 선택된 키프레임의 위치/회전 정보
			auto& kf = mSelectedSweptVolume->mSourceTrajectory->keyframes[mSelectedKeyframeIdx];
			DgVolume* brush = mSelectedSweptVolume->mBrushVolume;

			// 키프레임의 position + rotation으로 모델 행렬 구성
			// → 브러시를 해당 키프레임 위치/자세로 배치
			glm::mat4 kfModel = mSelectedSweptVolume->getModelMatrix()
				* glm::translate(glm::mat4(1.0f), kf.position)
				* glm::mat4_cast(kf.rotation);
			glm::mat4 kfModelInv = glm::inverse(kfModel);

			// 레이마칭 셰이더 설정 (기존 SDF 볼륨 렌더링과 동일한 파이프라인)
			GLuint sp = mShaders[10];
			glUseProgram(sp);

			// 행렬 유니폼 전달
			glUniformMatrix4fv(glGetUniformLocation(sp, "uModel"), 1, GL_FALSE, glm::value_ptr(kfModel));
			glUniformMatrix4fv(glGetUniformLocation(sp, "uView"), 1, GL_FALSE, glm::value_ptr(viewMat));
			glUniformMatrix4fv(glGetUniformLocation(sp, "uProjection"), 1, GL_FALSE, glm::value_ptr(projMat));
			glUniformMatrix4fv(glGetUniformLocation(sp, "uInvView"), 1, GL_FALSE, glm::value_ptr(invViewMat));
			glUniformMatrix4fv(glGetUniformLocation(sp, "uInvProj"), 1, GL_FALSE, glm::value_ptr(invProjMat));
			glUniform2f(glGetUniformLocation(sp, "uResolution"), mSceneSize[0], mSceneSize[1]);
			glUniformMatrix4fv(glGetUniformLocation(sp, "uProj"), 1, GL_FALSE, glm::value_ptr(projMat));
			glUniformMatrix4fv(glGetUniformLocation(sp, "uModelInverse"), 1, GL_FALSE, glm::value_ptr(kfModelInv));
			glUniform1f(glGetUniformLocation(sp, "uOffset"), brush->mOffset);

			// 브러시 볼륨의 로컬 AABB 전달 (레이마칭 범위 지정)
			glm::vec3 bMin = brush->getLocalMin();
			glm::vec3 bMax = brush->getLocalMax();
			glUniform3f(glGetUniformLocation(sp, "uVolumeMin"), bMin.x, bMin.y, bMin.z);
			glUniform3f(glGetUniformLocation(sp, "uVolumeMax"), bMax.x, bMax.y, bMax.z);

			// 프리뷰 색상: 분홍색
			glUniform3f(glGetUniformLocation(sp, "uBaseColor"), 1.0f, 0.5f, 0.7f);
			glUniform1f(glGetUniformLocation(sp, "uAlpha"), 1.0f);  // 반투명

			// 브러시의 3D SDF 텍스처 바인딩
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_3D, brush->mTextureID);
			glUniform1i(glGetUniformLocation(sp, "uSDFVolume"), 0);

			// 브러시 복사본에 프록시 메시가 없으면 AABB 바운딩 박스 메시 생성
			if (!brush->mMesh)
				brush->mMesh = createBoundingBoxMesh(brush->mMin, brush->mMax);

			// Depth Test 비활성화: 스윕 볼륨에 가려지지 않고 전체 브러시가 보이도록
			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST);
			brush->mMesh->render();

			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);

			glBindTexture(GL_TEXTURE_3D, 0);
			glUseProgram(0);
		}

		// SDF 볼륨 렌더링
		for (DgVolume* pVolume : mSDFList)
		{
			if (pVolume == nullptr || pVolume->mTextureID == 0) continue;

			glm::mat4 modelMat = pVolume->getModelMatrix();
			glm::mat4 modelInverse = glm::inverse(modelMat);

			GLuint shaderProgram = mShaders[10];
			glUseProgram(shaderProgram);

			// 행렬 유니폼
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(modelMat));
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(viewMat));
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projMat));
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uInvView"), 1, GL_FALSE, glm::value_ptr(invViewMat));
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uInvProj"), 1, GL_FALSE, glm::value_ptr(invProjMat));
			glUniform2f(glGetUniformLocation(shaderProgram, "uResolution"), mSceneSize[0], mSceneSize[1]);

			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProj"), 1, GL_FALSE, glm::value_ptr(projMat));  // fragment shader용
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uModelInverse"), 1, GL_FALSE, glm::value_ptr(modelInverse));
			glUniform1f(glGetUniformLocation(shaderProgram, "uOffset"), pVolume->mOffset);		// DgScene.cpp의 SDF 볼륨 렌더링 부분에 추가
			glUniform3f(glGetUniformLocation(shaderProgram, "uBaseColor"), 0.6f, 0.6f, 0.6f);   // 볼륨 기본 색상 (회색)

			// 키프레임 선택 중이면 기존 볼륨을 반투명으로
			bool kfSelected = (mSelectedKeyframeIdx >= 0 && mSelectedSweptVolume != nullptr);
			glUniform1f(glGetUniformLocation(shaderProgram, "uAlpha"), kfSelected ? 0.5f : 1.0f);

			// 이동된 위치를 반영하여 uVolumeMin/Max 전달
			glm::vec3 localMin = pVolume->getLocalMin();
			glm::vec3 localMax = pVolume->getLocalMax();
			glUniform3f(glGetUniformLocation(shaderProgram, "uVolumeMin"),
				localMin.x, localMin.y, localMin.z);
			glUniform3f(glGetUniformLocation(shaderProgram, "uVolumeMax"),
				localMax.x, localMax.y, localMax.z);

			// 3D 텍스처 바인딩
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_3D, pVolume->mTextureID);
			glUniform1i(glGetUniformLocation(shaderProgram, "uSDFVolume"), 0);

			// Cull Face 비활성화
			if (kfSelected) {
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			glDisable(GL_CULL_FACE);

			pVolume->mMesh->render();

			glEnable(GL_CULL_FACE);
			if (kfSelected) {
				glDisable(GL_BLEND);
			}

			glBindTexture(GL_TEXTURE_3D, 0);
			glUseProgram(0);
		}

		// 선택된 볼륨의 바운딩 박스 렌더링
		renderSelectedBoundingBoxes(viewMat, projMat);

		// 궤적 시각화
		renderTrajectory(viewMat, projMat);  

		renderSweptVolumeTrajectory(viewMat, projMat);

		// FPS 렌더링
		renderFps();					
	}
	mFrameBuf.unbind();

	ImTextureID textureID = (void*)(uintptr_t)mFrameBuf.getFrameTexture();
	ImVec2 sceneImagePos = ImGui::GetCursorScreenPos();
	ImGui::Image(textureID, ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
	
	// ── 키프레임 기즈모 ──
	bool keyframeGizmoActive = false;
	if (mSelectedSweptVolume != nullptr
		&& mSelectedSweptVolume->mSourceTrajectory != nullptr
		&& mSelectedKeyframeIdx >= 0
		&& mSelectedKeyframeIdx < (int)mSelectedSweptVolume->mSourceTrajectory->keyframes.size()
		&& mEditMode != EditMode::Select && mEditMode != EditMode::Trajectory)
	{
		keyframeGizmoActive = true;
		auto& kf = mSelectedSweptVolume->mSourceTrajectory->keyframes[mSelectedKeyframeIdx];
		glm::mat4 projMat = glm::perspective(glm::radians(30.0f), mSceneSize.x / mSceneSize.y, 1.0f, 1000.0f);
		glm::mat4 viewMat(1.0f);
		viewMat = glm::translate(viewMat, glm::vec3(0.0f, 0.0f, mZoom));
		viewMat = viewMat * mRotMat;
		viewMat = glm::translate(viewMat, glm::vec3(mPan[0], mPan[1], mPan[2]));
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
		ImGuizmo::SetRect(sceneImagePos.x, sceneImagePos.y, mSceneSize.x, mSceneSize.y);
		ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
		if (mEditMode == EditMode::Rotate) op = ImGuizmo::ROTATE;
		if (mEditMode == EditMode::Scale)  op = ImGuizmo::SCALE;

		// 볼륨 모델 행렬 (이동/회전 반영)
		glm::mat4 volModel = mSelectedSweptVolume->getModelMatrix();
		glm::mat4 volModelInv = glm::inverse(volModel);

		// 로컬 키프레임 → 볼륨 모델 변환 적용 → 월드 위치에 기즈모 표시
		glm::mat4 gizmoMat = volModel
			* glm::translate(glm::mat4(1.0f), kf.position)
			* glm::mat4_cast(kf.rotation);

		ImGuizmo::Manipulate(
			glm::value_ptr(viewMat),
			glm::value_ptr(projMat),
			op,
			ImGuizmo::WORLD,
			glm::value_ptr(gizmoMat)
		);
		if (ImGuizmo::IsUsing())
		{
			// 기즈모 결과(월드) → 볼륨 로컬로 역변환
			glm::mat4 localMat = volModelInv * gizmoMat;

			glm::vec3 translation, rotEuler, scale;
			ImGuizmo::DecomposeMatrixToComponents(
				glm::value_ptr(localMat),
				glm::value_ptr(translation),
				glm::value_ptr(rotEuler),
				glm::value_ptr(scale)
			);
			if (op == ImGuizmo::TRANSLATE)
				kf.position = translation;
			else if (op == ImGuizmo::ROTATE)
				kf.rotation = glm::quat(glm::radians(rotEuler));
			mSelectedSweptVolume->mSourceTrajectory->rebuild();
			resweepVolume(mSelectedSweptVolume, true);  // preview
			mKeyframeGizmoWasUsing = true;
		}
		else if (mKeyframeGizmoWasUsing)
		{
			resweepVolume(mSelectedSweptVolume, false);  // full quality
			mKeyframeGizmoWasUsing = false;
		}
	}

	// ImGuizmo 렌더링
	if (!keyframeGizmoActive && hasSelectedVolumes() && mEditMode != EditMode::Select && mEditMode != EditMode::Trajectory)
	{
		glm::mat4 projMat = glm::perspective(glm::radians(30.0f), mSceneSize.x / mSceneSize.y, 1.0f, 1000.0f);
		glm::mat4 viewMat(1.0f);
		viewMat = glm::translate(viewMat, glm::vec3(0.0f, 0.0f, mZoom));
		viewMat = viewMat * mRotMat;
		viewMat = glm::translate(viewMat, glm::vec3(mPan[0], mPan[1], mPan[2]));

		ImGuizmo::SetOrthographic(false); // 원근 투영 사용
		ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());	// Scene 창의 DrawList 사용

		// ImGuizmo의 조작 영역을 Scene 창의 이미지 영역으로 설정 (콘텐츠 영역 기준)
		ImGuizmo::SetRect(sceneImagePos.x, sceneImagePos.y, mSceneSize.x, mSceneSize.y);

		// 조작 모드 결정(버튼이랑 연결)
		ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
		if (mEditMode == EditMode::Rotate) op = ImGuizmo::ROTATE;
		if (mEditMode == EditMode::Scale)  op = ImGuizmo::SCALE;

		// 선택된 첫 번째 볼륨에 기즈모 적용
		for (DgVolume* vol : mSDFList)
		{
			if (!vol || !vol->mSelected) continue;

			// 볼륨 데이터 공간에서의 중심 (로드된 VTI 바운딩박스 중심)
			glm::vec3 localCenter = (vol->getLocalMin() + vol->getLocalMax()) * 0.5f;
			
			// 이동이 반영된 월드 공간에서의 실제 중심
			glm::vec3 worldCenter = vol->getCenter();

			// 기즈모 행렬을 worldCenter 기준으로 구성
			glm::mat4 gizmoMat = glm::translate(glm::mat4(1.0f), worldCenter);
			gizmoMat *= glm::mat4_cast(vol->mRotation);
			gizmoMat = glm::scale(gizmoMat, vol->mScale);

			ImGuizmo::Manipulate(		// 기즈모 렌더링 및 입력 처리
				glm::value_ptr(viewMat),
				glm::value_ptr(projMat),
				op,
				ImGuizmo::WORLD,
				glm::value_ptr(gizmoMat)
			);

			if (ImGuizmo::IsUsing())	// 기즈모를 실제로 드래그 중일 때만 볼륨에 반영
			{
				glm::vec3 translation, rotation, scale;
				ImGuizmo::DecomposeMatrixToComponents(
					glm::value_ptr(gizmoMat),
					glm::value_ptr(translation),
					glm::value_ptr(rotation),
					glm::value_ptr(scale)
				);
				vol->mPosition = translation - localCenter;
				vol->mRotation = glm::quat(glm::radians(rotation));
				vol->mScale = scale;
			}
			break; // 첫 번째 선택 볼륨만
		}
	}
}

// FPS 및 마우스 좌표 출력 함수
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

		ImGui::Text("Edit Mode: %s", mEditMode == EditMode::Move ? "Move" : "Select");

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

// 키보드 이벤트 처리
void DgScene::processKeyboardEvent()
{
	// 키보드 이벤트를 처리한다.
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None))
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			// 궤적 모드면 종료, 아니면 프로그램 종료
			if (mEditMode == EditMode::Trajectory)
			{
				exitTrajectoryMode();
			}
			else
			{
				glfwSetWindowShouldClose(ImGuiManager::instance().mWindow, true);
			}
		}		
	}
}

// 장면 우클릭 팝업 메뉴 렌더링
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

// SDF 볼륨 추가 함수
void DgScene::addSDFVolume(DgVolume* volume)
{
	mSDFList.push_back(volume);
}

// 장면 초기화 함수
void DgScene::resetScene()
{
	// 1. 모든 볼륨 삭제
	for (DgVolume* v : mSDFList)
	{
		delete v;  // 소멸자에서 mMesh와 mTextureID도 정리됨
	}
	mSDFList.clear();
	mSavedTrajectories.clear();

	// 2. 카메라 초기화
	mZoom = -45.0f;
	mRotMat = glm::mat4(1.0f);
	mRotMat = glm::rotate(mRotMat, glm::radians(30.0f), glm::vec3(1, 0, 0));  // pitch
	mRotMat = glm::rotate(mRotMat, glm::radians(60.0f), glm::vec3(0, 1, 0));  // yaw
	mPan = glm::vec3(0.0f);
	mEditMode = EditMode::Select;
	std::cout << "장면 초기화 완료" << std::endl;
}

void DgScene::exitTrajectoryMode()
{
	mDrawingVolume = nullptr;
	mTrajectory.clear();
	mEditMode = EditMode::Select;

	std::cout << "궤적 모드 종료" << std::endl;
}

// 마우스 좌표를 월드 좌표로 변환하는 함수
glm::vec3 DgScene::mouseToWorld(ImVec2 mouse, float planeY)
{
	glm::mat4 proj = glm::perspective(glm::radians(30.0f), mSceneSize.x / mSceneSize.y, 1.0f, 1000.0f);
	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, mZoom));
	view = view * mRotMat;
	view = glm::translate(view, mPan);

	glm::mat4 invView = glm::inverse(view);
	glm::mat4 invProj = glm::inverse(proj);

	// 스크린 → NDC
	ImVec2 content = ImGui::GetCursorScreenPos();
	float ndcX = ((mouse.x - content.x) / mSceneSize.x) * 2.0f - 1.0f;
	float ndcY = 1.0f - ((mouse.y - content.y) / mSceneSize.y) * 2.0f;

	// NDC → 월드 레이
	glm::vec4 near = invProj * glm::vec4(ndcX, ndcY, -1, 1);
	glm::vec4 far = invProj * glm::vec4(ndcX, ndcY, 1, 1);
	near /= near.w;
	far /= far.w;

	glm::vec3 rayO = glm::vec3(invView * near);
	glm::vec3 rayD = glm::normalize(glm::vec3(invView * far) - rayO);

	// Y = planeY 평면과 교차
	if (std::abs(rayD.y) < 0.0001f) return glm::vec3(rayO.x, planeY, rayO.z);
	float t = (planeY - rayO.y) / rayD.y;
	return rayO + rayD * t;
}

// 궤적 렌더링 함수
void DgScene::renderTrajectory(const glm::mat4& viewMat, const glm::mat4& projMat)
{
	if (mEditMode != EditMode::Trajectory || mTrajectory.size() < 2) return;

	glDisable(GL_DEPTH_TEST);

	// 정점 데이터 생성
	std::vector<float> verts;
	for (auto& frame : mTrajectory.frames) {
		verts.push_back(frame.position.x);
		verts.push_back(frame.position.y);
		verts.push_back(frame.position.z);
	}

	// VAO/VBO 초기화
	if (mTrajectoryVAO == 0) {
		glGenVertexArrays(1, &mTrajectoryVAO);
		glGenBuffers(1, &mTrajectoryVBO);
	}

	// VBO 업데이트
	glBindVertexArray(mTrajectoryVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mTrajectoryVBO);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// 바운딩 박스 셰이더 사용
	loadBBoxShader();
	glUseProgram(mBBoxShader);

	glUniformMatrix4fv(glGetUniformLocation(mBBoxShader, "uModel"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	glUniformMatrix4fv(glGetUniformLocation(mBBoxShader, "uView"), 1, GL_FALSE, glm::value_ptr(viewMat));
	glUniformMatrix4fv(glGetUniformLocation(mBBoxShader, "uProjection"), 1, GL_FALSE, glm::value_ptr(projMat));

	// 녹색 라인으로 궤적 그리기
	glUniform3f(glGetUniformLocation(mBBoxShader, "uColor"), 0.2f, 0.9f, 0.3f);
	glLineWidth(3.0f);
	glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)mTrajectory.size());

	if (!mTrajectory.keyframes.empty())
	{
		GLint colorLoc = glGetUniformLocation(mBBoxShader, "uColor");

		for (int i = 0; i < (int)mTrajectory.keyframes.size(); ++i)
		{
			glm::vec3 kfPos = mTrajectory.keyframes[i].position;
			float pt[3] = { kfPos.x, kfPos.y, kfPos.z };
			glBufferData(GL_ARRAY_BUFFER, sizeof(pt), pt, GL_DYNAMIC_DRAW);

			if (i == 0 || i == (int)mTrajectory.keyframes.size() - 1) {
				glUniform3f(colorLoc, 1.0f, 0.78f, 0.1f);   // 시작/끝: 노랑
				glPointSize(14.0f);
			}
			else {
				glUniform3f(colorLoc, 0.4f, 0.8f, 1.0f);    // 중간 키프레임: 하늘색
				glPointSize(12.0f);
			}
			glDrawArrays(GL_POINTS, 0, 1);
		}

		// frames 버퍼로 복원
		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
	}
	else
	{
		// keyframes 기본 색상 적용
		glUniform3f(glGetUniformLocation(mBBoxShader, "uColor"), 1.0f, 1.0f, 0.0f);
		glPointSize(8.0f);
		glDrawArrays(GL_POINTS, 0, (GLsizei)mTrajectory.size());
	}
	
	// 정리
	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);
	glUseProgram(0);
	glLineWidth(1.0f);
	glPointSize(1.0f);
}

void DgScene::renderSweptVolumeTrajectory(const glm::mat4& viewMat,
	const glm::mat4& projMat)
{
	if (!mSelectedSweptVolume || !mSelectedSweptVolume->mSourceTrajectory) return;
	const DgTrajectory& traj = *mSelectedSweptVolume->mSourceTrajectory;
	if (traj.size() < 2) return;

	glDisable(GL_DEPTH_TEST);

	std::vector<float> verts;
	for (const auto& f : traj.frames) {
		verts.push_back(f.position.x);
		verts.push_back(f.position.y);
		verts.push_back(f.position.z);
	}

	if (mTrajectoryVAO == 0) {
		glGenVertexArrays(1, &mTrajectoryVAO);
		glGenBuffers(1, &mTrajectoryVBO);
	}
	glBindVertexArray(mTrajectoryVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mTrajectoryVBO);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(0);

	loadBBoxShader();
	glUseProgram(mBBoxShader);
	glUniformMatrix4fv(glGetUniformLocation(mBBoxShader, "uModel"), 1, GL_FALSE, glm::value_ptr(mSelectedSweptVolume->getModelMatrix()));
	glUniformMatrix4fv(glGetUniformLocation(mBBoxShader, "uView"), 1, GL_FALSE, glm::value_ptr(viewMat));
	glUniformMatrix4fv(glGetUniformLocation(mBBoxShader, "uProjection"), 1, GL_FALSE, glm::value_ptr(projMat));
	GLint colorLoc = glGetUniformLocation(mBBoxShader, "uColor");

	glUniform3f(colorLoc, 0.0f, 0.95f, 0.95f);
	glLineWidth(3.0f);
	glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)traj.size());

	if (!traj.keyframes.empty())
	{
		GLint colorLoc = glGetUniformLocation(mBBoxShader, "uColor");

		for (int i = 0; i < (int)traj.keyframes.size(); ++i)
		{
			glm::vec3 kfPos = traj.keyframes[i].position;
			float pt[3] = { kfPos.x, kfPos.y, kfPos.z };
			glBufferData(GL_ARRAY_BUFFER, sizeof(pt), pt, GL_DYNAMIC_DRAW);

			if (i == mSelectedKeyframeIdx) {
				glUniform3f(colorLoc, 1.0f, 0.25f, 0.25f);
				glPointSize(18.0f);
			}
			else if (i == 0 || i == (int)traj.keyframes.size() - 1) {
				glUniform3f(colorLoc, 0.0f, 1.0f, 1.0f);
				glPointSize(14.0f);
			}
			else {
				glUniform3f(colorLoc, 0.3f, 0.85f, 0.85f);
				glPointSize(12.0f);
			}
			glDrawArrays(GL_POINTS, 0, 1);
		}
	}

	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);
	glUseProgram(0);
	glLineWidth(1.0f);
	glPointSize(1.0f);
}

void DgScene::resweepVolume(DgVolume* vol, bool preview)
{
	if (!vol || !vol->mIsSweptVolume || !vol->mSourceTrajectory || !vol->mBrushVolume) return;

	int res = preview ? 128 : vol->mSweepResolution;
	int steps = preview ? 50 : vol->mSweepTimeSteps;
	int method = preview ? 3 : vol->mSweepMethod;

	const DgTrajectory& traj = *vol->mSourceTrajectory;
	DgVolume* newVol = nullptr;
	switch (method) {
	case 0: newVol = DgSweep::generateSweptVolume(vol->mBrushVolume, traj, res, steps, false); break;
	case 1: newVol = DgSweep::generateSweptVolume(vol->mBrushVolume, traj, res, steps, true); break;
	case 2: newVol = DgSweep::generateBrentCPU(vol->mBrushVolume, traj, res, steps); break;
	default:newVol = DgSweep::generateBrentGPU(vol->mBrushVolume, traj, res, steps, preview); break;
	}
	if (!newVol) return;

	for (int i = 0; i < 3; ++i) {
		vol->mDim[i] = newVol->mDim[i];
		vol->mSpacing[i] = newVol->mSpacing[i];
	}
	vol->mMin = newVol->mMin;
	vol->mMax = newVol->mMax;

	if (vol->mTextureID != 0) glDeleteTextures(1, &vol->mTextureID);
	vol->mTextureID = newVol->mTextureID;
	newVol->mTextureID = 0;

	if (!preview) {
		vol->mData = std::move(newVol->mData);
	}

	delete vol->mMesh;
	vol->mMesh = newVol->mMesh;
	newVol->mMesh = nullptr;
	delete newVol;
}

void DgScene::startCollisionDemo(DgVolume* sv)
{
	if (!sv || !sv->mIsSweptVolume) return;
	clearCollisionDemo();

	struct ObstacleInfo {
		const char* path;
		glm::vec3   position;
	};

	ObstacleInfo obstacles[] = {
		{ ".\\res\\object\\Obstacle_sphere.obj", glm::vec3(4.0f, 0.0f,  10.0f) },
		{ ".\\res\\object\\Obstacle_box.obj",    glm::vec3(17.0f, 0.0f,  8.0f) },
		{ ".\\res\\object\\Obstacle_sphere.obj", glm::vec3(30.0f, 0.0f,  0.0f) },
		{ ".\\res\\object\\Obstacle_box.obj",    glm::vec3(43.0f, 0.0f, -4.0f) },
		{ ".\\res\\object\\Obstacle_sphere.obj", glm::vec3(43.0f, 0.0f, -1.0f) },
		{ ".\\res\\object\\Obstacle_box.obj",    glm::vec3(50.0f, 0.0f,  1.0f) },
	};

	for (auto& info : obstacles)
	{
		DgMesh* mesh = import_mesh_obj(info.path);
		if (!mesh) {
			std::cout << "장애물 로드 실패: " << info.path << std::endl;
			continue;
		}

		// 정점을 목표 위치로 이동
		for (auto& v : mesh->mVerts) {
			v.mPos[0] += info.position.x;
			v.mPos[1] += info.position.y;
			v.mPos[2] += info.position.z;
		}

		mesh->mShaderId = mShaders[2];  // phong
		mesh->setupBuffers();
		mMeshList.push_back(mesh);		 // 전체 메쉬 목록에 추가(렌더링용)
		mObstacleMeshes.push_back(mesh); // 충돌 데모용 장애물 목록에도 추가
	}

	mCollisionDemoActive = true;
	std::cout << "충돌 데모: 장애물 " << mObstacleMeshes.size() << "개 배치" << std::endl;
}

void DgScene::runCollisionOptimization(DgVolume* sv, float safetyFactor, int maxIter)
{
	if (!sv || !sv->mIsSweptVolume || !sv->mSourceTrajectory) return;
	if (mObstacleMeshes.empty()) return;

	auto& kfs = sv->mSourceTrajectory->keyframes;
	int numKFs = (int)kfs.size();

	std::cout << "=== 충돌 최적화 시작 ===" << std::endl;

	for (int iter = 0; iter < maxIter; ++iter)
	{
		// 모든 장애물 중 가장 깊은 충돌 찾기
		CollisionResult worst;
		worst.deepestSDF = 0.0f;

		for (DgMesh* obs : mObstacleMeshes)
		{
			CollisionResult cr = DgCollision::detectCollision(sv, obs);
			if (cr.hasCollision && cr.deepestSDF < worst.deepestSDF)
				worst = cr;
		}

		if (!worst.hasCollision) {
			std::cout << "충돌 해소 완료 (iteration " << iter << ")" << std::endl;
			break;
		}

		// 유클리드 거리로 가장 가까운 키프레임 (시작/끝 제외)
		int nearestKF = 1;
		float minDist = FLT_MAX;
		for (int i = 1; i < numKFs - 1; ++i)
		{
			float d = glm::length(kfs[i].position - worst.deepestPoint);
			if (d < minDist) {
				minDist = d;
				nearestKF = i;
			}
		}

		// 법선 방향으로 키프레임 이동
		glm::vec3 displacement = worst.normal * (-worst.deepestSDF) * safetyFactor;
		kfs[nearestKF].position += displacement;

		std::cout << "Iter " << iter
			<< " | SDF=" << worst.deepestSDF
			<< " | KF=" << nearestKF
			<< " | disp=" << glm::length(displacement) << std::endl;

		// 궤적 재구성 + SV 재생성 (preview)
		sv->mSourceTrajectory->rebuild();
		resweepVolume(sv, true);
	}

	std::cout << "최종 full quality resweep..." << std::endl;
	resweepVolume(sv, false);
	std::cout << "=== 충돌 최적화 완료 ===" << std::endl;
}

void DgScene::clearCollisionDemo()
{
	for (DgMesh* obs : mObstacleMeshes)
	{
		auto it = std::find(mMeshList.begin(), mMeshList.end(), obs);
		if (it != mMeshList.end())
			mMeshList.erase(it);
		delete obs;
	}
	mObstacleMeshes.clear();
	mCollisionDemoActive = false;
}