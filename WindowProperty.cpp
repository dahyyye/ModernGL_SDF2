#include "DgViewer.h"

bool show_window_model_property = true;
void OpenProperty();

static void SaveGridToText(DgVolume* vol, const std::string& filename)
{
	std::ofstream file(filename);
	if (!file.is_open()) return;

	int dimX = vol->mDim[0];
	int dimY = vol->mDim[1];
	int dimZ = vol->mDim[2];
	int midZ = dimZ / 2;

	file << "Grid Size: " << dimX << " x " << dimY << " x " << dimZ << "\n";
	file << "Slice Z = " << midZ << " (center)\n\n";

	for (int y = 0; y < dimY; ++y) {
		for (int x = 0; x < dimX; ++x) {
			size_t idx = (size_t)midZ * (dimY * dimX) + (size_t)y * dimX + x;
			file << std::fixed << std::setprecision(2) << vol->mData[idx] << "\t";
		}
		file << "\n";
	}
	file.close();
	std::cout << "[SaveGrid] " << filename << " 저장 완료 (z=" << midZ << ")" << std::endl;

}

void ShowWindowModelProperty(bool* p_open)
{

	ImGuiWindowFlags window_flags = 0;

	// 속성 윈도우를 생성하고, collapsed된 상태라면 바로 리턴한다.
	if (!ImGui::Begin("Property", nullptr, window_flags))
	{
		ImGui::End();
		return;
	}
	OpenProperty();	
	ImGui::End();
}

void OpenProperty() {
	const int NumIcons = 5;

	const char* icon_files[NumIcons] = {
		".\\res\\icons\\Union-A-B.png",
		".\\res\\icons\\Intersection-A-B.png",
		".\\res\\icons\\Difference-A-B.png",
		".\\res\\icons\\create_linear.png",
		".\\res\\icons\\create_crv.png"
	};

	static GLuint icon_tex_id[NumIcons] = { 0 };
	if (icon_tex_id[0] == 0)
	{
		for (int i = 0; i < NumIcons; ++i)
			icon_tex_id[i] = DgUtil::loadTexture2D(icon_files[i]);
	}

	if (ImGui::CollapsingHeader("Boolean"))
	{
		// 선택된 볼륨 수집
		std::vector<DgVolume*> selected;
		for (DgVolume* vol : DgScene::instance().getSDFList())
		{
			if (vol && vol->mSelected)
				selected.push_back(vol);
		}

		if (ImGui::ImageButton("Union", DgUtil::toImTextureID(icon_tex_id[0]), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
		{
			if (selected.size() >= 2)
			{
				DgVolume* result = DgBoolean::Boolean(selected, BooleanMode::Union, 256);
				if (result)
				{
					DgScene::instance().addSDFVolume(result);
					for (DgVolume* vol : selected)
						vol->mSelected = false;
					result->mSelected = true;
				}
			}
		}
		ImGui::SameLine();
		if (ImGui::ImageButton("Intersection", DgUtil::toImTextureID(icon_tex_id[1]), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
		{
			if (selected.size() >= 2)
			{
				DgVolume* result = DgBoolean::Boolean(selected, BooleanMode::Intersection, 256);
				if (result)
				{
					DgScene::instance().addSDFVolume(result);
					for (DgVolume* vol : selected) vol->mSelected = false;
					result->mSelected = true;
				}
			}
		}
		ImGui::SameLine();
		if (ImGui::ImageButton("Difference", DgUtil::toImTextureID(icon_tex_id[2]), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
		{
			if (selected.size() >= 2)
			{
				DgVolume* result = DgBoolean::Boolean(selected, BooleanMode::Difference, 256);
				if (result)
				{
					DgScene::instance().addSDFVolume(result);
					for (DgVolume* vol : selected) vol->mSelected = false;
					result->mSelected = true;
				}
			}
		}
	}

	if (ImGui::CollapsingHeader("Offset"))
	{
		// 선택된 볼륨 찾기
		DgVolume* selectedVol = nullptr;
		for (DgVolume* vol : DgScene::instance().getSDFList())
		{
			if (vol && vol->mSelected) {
				selectedVol = vol;
				break;
			}
		}

		if (selectedVol)
		{
			float offset = selectedVol->mOffset;

			ImGui::Text("Offset = %.2f", offset);

			if (ImGui::SliderFloat("##OffsetSlider", &offset, -2.0f, 1.0f, ""))
			{
				// 선택된 모든 볼륨에 적용
				for (DgVolume* vol : DgScene::instance().getSDFList())
				{
					if (vol && vol->mSelected)
					{
						vol->mOffset = offset;
					}
				}
			}

			// 리셋 버튼
			ImGui::SameLine();
			if (ImGui::Button("Reset"))
			{
				for (DgVolume* vol : DgScene::instance().getSDFList())
				{
					if (vol && vol->mSelected)
						vol->mOffset = 0.0f;
				}
			}
		}
		else
		{
			ImGui::TextDisabled("No volume selected");
		}
	}

	if (ImGui::CollapsingHeader("Sweeping"))
	{
		// 선택된 볼륨 확인
		DgVolume* selectedVol = nullptr;
		for (DgVolume* vol : DgScene::instance().getSDFList())
		{
			if (vol && vol->mSelected) {
				selectedVol = vol;
				break;
			}
		}

		ImGui::Separator();

		// Linear 
		if (ImGui::ImageButton("create_linear", DgUtil::toImTextureID(icon_tex_id[3]), ImVec2(84, 84), ImVec2(0, 1), ImVec2(1, 0)))
		{
			if (selectedVol) {
				glm::vec3 center = selectedVol->getCenter();
				DgScene::instance().mTrajectory.generateLinear(center);
				DgScene::instance().mDrawingVolume = selectedVol;
				DgScene::instance().setEditMode(EditMode::Trajectory);
			}
		}
		ImGui::SameLine();

		// Catmull-Rom
		if (ImGui::ImageButton("create_Curve", DgUtil::toImTextureID(icon_tex_id[4]), ImVec2(84, 84), ImVec2(0, 1), ImVec2(1, 0)))
		{
			if (selectedVol) {
				glm::vec3 center = selectedVol->getCenter();
				DgScene::instance().mTrajectory.generateCurve(center);
				DgScene::instance().mDrawingVolume = selectedVol;
				DgScene::instance().setEditMode(EditMode::Trajectory);
			}
		}

		DgTrajectory& traj = DgScene::instance().mTrajectory;

		if (!traj.keyframes.empty())
		{
			ImGui::Separator();
			ImGui::Text("Curve Rotation");

			static glm::vec3 startEuler(0.f);  // degrees
			static glm::vec3 endEuler(0.f);

			bool changed = false;
			changed |= ImGui::SliderFloat3("Start (deg)", glm::value_ptr(startEuler), -180.f, 180.f);
			changed |= ImGui::SliderFloat3("End (deg)", glm::value_ptr(endEuler), -180.f, 180.f);

			if (changed)
			{
				traj.keyframes.front().rotation = glm::quat(glm::radians(startEuler));
				traj.keyframes.back().rotation = glm::quat(glm::radians(endEuler));
				traj.rebuild();  // 슬라이더 움직일 때마다 frames 재생성
			}

			static int numKF = 4;
			if (ImGui::SliderInt("Keyframes", &numKF, 2, 10))
			{
				// 현재 곡선(frames)을 따라 numKF개 위치로 재분배
				std::vector<DgTrajectoryFrame> newKF(numKF);
				for (int i = 0; i < numKF; ++i)
				{
					float t = (float)i / (numKF - 1);
					glm::mat4 T = traj.getTransformAt(t);
					newKF[i].position = glm::vec3(T[3]);
					newKF[i].rotation = glm::quat_cast(T);
				}
				traj.keyframes = newKF;
				traj.rebuild();
			}
		}

		if (traj.size() >= 2)
		{
			ImGui::Separator();
			ImGui::Text("Trajectory: %d frames", (int)traj.size());

			// 해상도 설정
			static int sweepResolution = 256;
			ImGui::SliderInt("Resolution", &sweepResolution, 64, 512);

			// 타임 스텝 설정
			static int timeSteps = 10;
			ImGui::SliderInt("sampling", &timeSteps, 20, 500);

			// Sweep 버튼 (기존 Stamping)
			if (ImGui::Button("Stamping CPU", ImVec2(-1,0)))
			{
				DgVolume* brush = DgScene::instance().mDrawingVolume;
				if (brush)
				{
					DgVolume* swept = DgSweep::generateSweptVolume(
						brush, traj, sweepResolution, timeSteps, false
					);

					if (swept)
					{
						DgScene::instance().addSDFVolume(swept);
						swept->mSelected = true;
						brush->mSelected = false;
						DgScene::instance().exitTrajectoryMode();
						std::cout << "Swept Volume 생성 완료 (CPU)" << std::endl;
					}
				}
			}

			// GPU 버전 버튼
			if (ImGui::Button("Stamping GPU", ImVec2(-1, 0)))
			{
				DgVolume* brush = DgScene::instance().mDrawingVolume;
				if (brush)
				{
					DgVolume* swept = DgSweep::generateSweptVolume(
						brush, traj, sweepResolution, timeSteps, true
					);

					if (swept)
					{
						DgScene::instance().addSDFVolume(swept);
						swept->mIsSweptVolume = true;                           // ← 추가
						swept->mSourceTrajectory = new DgTrajectory(traj);      // ← 추가
						swept->mSweepResolution = sweepResolution;              // ← 추가
						swept->mSweepTimeSteps = timeSteps;                     // ← 추가
						swept->mSweepMethod = 1;                                // ← 추가
						swept->mSelected = true;
						brush->mSelected = false;
						DgScene::instance().exitTrajectoryMode();
						std::cout << "Swept Volume 생성 완료 (GPU)" << std::endl;
					}
				}
			}

			// Brent CPU 버튼
			if (ImGui::Button("Brent CPU", ImVec2(-1, 0)))
			{
				DgVolume* brush = DgScene::instance().mDrawingVolume;
				if (brush)
				{
					DgVolume* swept = DgSweep::generateBrentCPU(
						brush, traj, sweepResolution, timeSteps
					);

					if (swept)
					{
						DgScene::instance().addSDFVolume(swept);
						swept->mSelected = true;
						brush->mSelected = false;
						DgScene::instance().exitTrajectoryMode();
						std::cout << "Swept Volume 생성 완료 (Brent)" << std::endl;
					}
				}
			}

			// Brent GPU 버튼
			if (ImGui::Button("Brent GPU", ImVec2(-1, 0)))
			{
				DgVolume* brush = DgScene::instance().mDrawingVolume;
				if (brush)
				{
					DgVolume* swept = DgSweep::generateBrentGPU(
						brush, traj, sweepResolution, timeSteps, false
					);

					if (swept)
					{
						// brush deep copy
						DgVolume* brushCopy = new DgVolume();
						brushCopy->mData = brush->mData;
						brushCopy->mDim[0] = brush->mDim[0];
						brushCopy->mDim[1] = brush->mDim[1];
						brushCopy->mDim[2] = brush->mDim[2];
						brushCopy->mSpacing[0] = brush->mSpacing[0];
						brushCopy->mSpacing[1] = brush->mSpacing[1];
						brushCopy->mSpacing[2] = brush->mSpacing[2];
						brushCopy->mMin = brush->mMin;
						brushCopy->mMax = brush->mMax;
						brushCopy->createTexture();
						brushCopy->mMesh = createBoundingBoxMesh(brushCopy->mMin, brushCopy->mMax);

						swept->mIsSweptVolume = true;
						swept->mSourceTrajectory = new DgTrajectory(traj);
						swept->mBrushVolume = brushCopy;
						swept->mSweepResolution = sweepResolution;
						swept->mSweepTimeSteps = timeSteps;
						swept->mSweepMethod = 3;  // 버튼마다 0/1/2/3
						DgScene::instance().addSDFVolume(swept);
						swept->mSelected = true;
						brush->mSelected = false;
						DgScene::instance().exitTrajectoryMode();
						std::cout << "Swept Volume 생성 완료 (Brent GPU)" << std::endl;
					}
				}
			}

			// 궤적 초기화 버튼
			if (ImGui::Button("Clear Trajectory", ImVec2(-1, 0)))
			{
				DgScene::instance().exitTrajectoryMode();
			}
		}

		ImGui::Separator();

		// 전후 비교없는 Fast Sweeping 
		/*if (ImGui::Button("Fast Sweeping", ImVec2(-1, 0)))
		{
			DgVolume* brush = DgScene::instance().mDrawingVolume;
			if (brush)
			{
				DgSweep::fastSweeping(brush);
				std::cout << "Fast Sweeping 적용 (Drawing Volume)" << std::endl;
			}
		}*/

		if (ImGui::Button("Fast Sweeping", ImVec2(-1, 0)))
		{
			DgVolume* target = nullptr;
			for (DgVolume* v : DgScene::instance().getSDFList()) {
				if (v->mSelected) { target = v; break; }
			}

			if (target)
			{
				SaveGridToText(target, "C:\\Users\\user\\바탕 화면\\학교\\sdf_before.txt");
				DgSweep::fastSweeping(target);
				SaveGridToText(target, "C:\\Users\\user\\바탕 화면\\학교\\sdf_after.txt");
				std::cout << "Fast Sweeping 완료 + txt 추출 완료" << std::endl;
			}
			else
			{
				std::cout << "선택된 볼륨이 없음!" << std::endl;
			}
		}
	}

	if (ImGui::CollapsingHeader("Collision Demo"))
	{
		DgVolume* selectedSV = nullptr;

		for (DgVolume* vol : DgScene::instance().getSDFList())
		{
			if (vol && vol->mSelected && vol->mIsSweptVolume) {
				selectedSV = vol;
				break;
			}
		}

		if (selectedSV)
		{
			DgScene& scene = DgScene::instance();

			if (!scene.mCollisionDemoActive)
			{
				if (ImGui::Button("Start Collision Demo", ImVec2(-1, 0)))
					scene.startCollisionDemo(selectedSV);
			}
			else
			{
				static float safetyFactor = 1.5f;
				static int   maxIter = 50;
				ImGui::SliderFloat("Safety Factor", &safetyFactor, 1.0f, 3.0f);
				ImGui::SliderInt("Max Iterations", &maxIter, 5, 200);

				if (ImGui::Button("Start", ImVec2(-1, 0)))
					scene.runCollisionOptimization(selectedSV, safetyFactor, maxIter);

				ImGui::Separator();
				if (ImGui::Button("Clear Demo", ImVec2(-1, 0)))
					scene.clearCollisionDemo();
			}
		}
		else
		{
			ImGui::TextDisabled("Select a Swept Volume");
		}
	}
	
	if (ImGui::CollapsingHeader("Rendering"))
	{
		bool showGround = DgScene::instance().mShowGround;
		bool showBBox = DgScene::instance().mShowBBox;

		if (ImGui::Checkbox("Show Ground Grid", &showGround))
		{
			DgScene::instance().mShowGround = showGround;
		}
		
		if (ImGui::Checkbox("Show Bounding Box", &showBBox))
		{
			DgScene::instance().mShowBBox = showBBox;
		}
	}
}

