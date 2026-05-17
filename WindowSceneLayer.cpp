#include "DgViewer.h"
#define GLM_ENABLE_EXPERIMENTAL
#include ".\\include\\STB\\stb_image.h"

bool show_window_scene_layer = true;

void ShowWindowSceneLayer(bool* p_open)
{
	
	ImGuiWindowFlags window_flags = 0;

	// SceneLayer 윈도우를 생성하고, collapsed된 상태라면 바로 리턴한다.
	if (!ImGui::Begin("SceneLayer", nullptr, window_flags))
	{
		ImGui::End();
		return;
	}

	// 볼륨 목록 표시
	std::vector<DgVolume*>& sdfList = DgScene::instance().getSDFList();

	if (ImGui::TreeNode("Volumes"))
	{
		if (sdfList.empty())
		{
			ImGui::TextDisabled("No volumes");
		}
		else
		{
			static int selectedIndex = -1;

			// 각 이름별 카운터 (sphere1, sphere2, bunny1, bunny2...)
			std::map<std::string, int> nameCounter;

			for (size_t i = 0; i < sdfList.size(); ++i)
			{
				DgVolume* vol = sdfList[i];
				if (vol == nullptr) continue;

				// 이름별 번호 계산
				std::string baseName = vol->mName.empty() ? "volume" : vol->mName;
				nameCounter[baseName]++;
				int number = nameCounter[baseName];

				// 라벨 생성 (예: sphere1, bunny2)
				char label[64];
				snprintf(label, sizeof(label), "%s%d", baseName.c_str(), number);

				// 선택 가능한 항목으로 표시
				bool isSelected = (selectedIndex == (int)i);
				if (ImGui::Selectable(label, isSelected))
				{
					selectedIndex = (int)i;
					for (DgVolume* v : sdfList) v->mSelected = false;  // 기존 선택 해제
					vol->mSelected = true;
				}

				// 우클릭 컨텍스트 메뉴
				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Delete"))
					{
						delete sdfList[i];
						sdfList.erase(sdfList.begin() + i);
						if (selectedIndex == (int)i)
							selectedIndex = -1;
						ImGui::EndPopup();
						break;
					}
					if (ImGui::MenuItem("Export"))
					{
						char path[256];
						snprintf(path, sizeof(path), "volume%zu.vti", i + 1);
						sdfList[i]->saveToVTI(path);
					}
					ImGui::EndPopup();
				}
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Trajectories"))
	{
		{
			DgVolume* selectedSV = nullptr;
			for (DgVolume* vol : DgScene::instance().getSDFList())
			{
				if (vol && vol->mSelected && vol->mIsSweptVolume
					&& vol->mSourceTrajectory != nullptr)
				{
					selectedSV = vol;
					break;
				}
			}

			if (selectedSV)
			{
				if (ImGui::Button("Save Trajectory", ImVec2(-1, 0)))
				{
					DgScene::instance().mSavedTrajectories.push_back(*selectedSV->mSourceTrajectory);
					std::cout << "Trajectory saved to list." << std::endl;
				}
			}
			else
			{
				ImGui::BeginDisabled();
				ImGui::Button("Save Trajectory", ImVec2(-1, 0));  // 회색 비활성 버튼
				ImGui::EndDisabled();
				ImGui::TextDisabled("Select a Swept Volume to save");
			}
		}
		ImGui::Separator();

		auto& trajList = DgScene::instance().mSavedTrajectories;
		if (trajList.empty())
		{
			ImGui::TextDisabled("No trajectories");
		}
		else
		{
			for (int i = 0; i < (int)trajList.size(); ++i)
			{
				char label[64];
				snprintf(label, sizeof(label), "trajectory%d", i + 1);
				if (ImGui::Selectable(label, false))
				{
					DgVolume* selectedVol = nullptr;
					for (DgVolume* vol : DgScene::instance().getSDFList())
						if (vol && vol->mSelected) { selectedVol = vol; break; }

					if (selectedVol)
					{
						DgScene::instance().mTrajectory = trajList[i];
						DgScene::instance().mDrawingVolume = selectedVol;
						DgScene::instance().setEditMode(EditMode::Trajectory);
					}
				}
				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Delete"))
					{
						trajList.erase(trajList.begin() + i);
						ImGui::EndPopup();
						break;
					}
					if (ImGui::MenuItem("Export"))
					{
						char path[256];
						snprintf(path, sizeof(path), "trajectory%d.txt", i + 1);
						trajList[i].saveToFile(path);
					}
					ImGui::EndPopup();
				}
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("State"))
	{
		if (sdfList.empty()) {
			ImGui::TextDisabled("No volumes");
		}
		else {

		}
		ImGui::TreePop();
	}

	ImGui::End();
}