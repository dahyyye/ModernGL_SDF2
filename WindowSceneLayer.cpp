#include "DgViewer.h"
#define GLM_ENABLE_EXPERIMENTAL
#include ".\\include\\STB\\stb_image.h"

bool show_window_scene_layer = true;

void ShowWindowSceneLayer(bool* p_open)
{
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
	if (no_close)           p_open = NULL; // Don't pass our bool* to Begin

	// SceneLayer 윈도우를 생성하고, collapsed된 상태라면 바로 리턴한다.
	if (!ImGui::Begin("SceneLayer", p_open, window_flags))
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