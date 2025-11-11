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

	ImGui::End();
	//if (ImGui::TreeNode("Model"))
	//{
	//	int node_clicked = -1;

	//	// 메쉬를 선택한다.
	//	if (node_clicked != -1)	{
	//		
	//	}
	//	ImGui::TreePop();

	//	//Context 팝업 메뉴를 생성한다.
	//	if () {
	//		if (ImGui::BeginPopupContextWindow())
	//		{
	//			//MenuFile();
	//			if (ImGui::MenuItem("Delete"))
	//			{
	//				//삭제
	//			}
	//			ImGui::EndPopup();
	//		}
	//	}
	//}
}