#include "DgViewer.h"

bool show_window_tool_bar = true;
void CreateMesh();

void ShowWindowToolBar(bool* p_open) {
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

	// 속성 윈도우를 생성하고, collapsed된 상태라면 바로 리턴한다.
	if (!ImGui::Begin("ToolBar", p_open, window_flags))
	{
		ImGui::End();
		return;
	}
	CreateMesh();
	ImGui::End();
}

void CreateMesh() {
	const int NumIcons = 11;

	const char* icon_files[NumIcons] = {
		".\\res\\icons\\new_scene.png",
		".\\res\\icons\\sphere.png",
		".\\res\\icons\\box.png",
		".\\res\\icons\\torus.png",
		".\\res\\icons\\roundBox.png",
		".\\res\\icons\\boxFrame.png",
		".\\res\\icons\\cappedTorus.png",
		".\\res\\icons\\link.png",
		".\\res\\icons\\cylinder.png",
		".\\res\\icons\\cone.png",
		".\\res\\icons\\bunny.png",
	};

	static GLuint icon_tex_id[NumIcons] = { 0 };
	if (icon_tex_id[0] == 0)
	{
		for (int i = 0; i < NumIcons; ++i)
			icon_tex_id[i] = DgUtil::loadTexture2D(icon_files[i]);
	}

	if (ImGui::ImageButton("NewScene", DgUtil::toImTextureID(icon_tex_id[0]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{
		DgScene::instance().resetScene();
	}
	ImGui::SameLine();

	if (ImGui::ImageButton("Sphere", DgUtil::toImTextureID(icon_tex_id[1]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{
		DgVolume* volume = new DgVolume();

		// VTI 파일에서 SDF 로드
		if (volume->loadFromVTI(".\\res\\volume\\Sphere_128.vti")) {
			volume->mName = "sphere";
			// 바운딩 박스 메쉬 생성
			volume->mMesh = createBoundingBoxMesh(volume->mMin, volume->mMax);
			volume->createTexture();
			DgScene::instance().addSDFVolume(volume);
		}
		else {
			delete volume;
		}
	}
	ImGui::SameLine();

	if (ImGui::ImageButton("Box", DgUtil::toImTextureID(icon_tex_id[2]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{
		DgVolume* volume = new DgVolume();
		volume->mMesh = import_mesh_obj(".\\res\\object\\box.obj");
		volume->setDimensions(16, 16, 16);
		volume->setGridSpace(*volume->mMesh, 0.5);
		volume->computeSDF();
	}
	ImGui::SameLine();

	if (ImGui::ImageButton("Torus", DgUtil::toImTextureID(icon_tex_id[3]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{
		DgVolume* volume = new DgVolume();

		// VTI 파일에서 SDF 로드
		if (volume->loadFromVTI(".\\res\\volume\\Torus_128.vti")) {
			volume->mName = "Torus";
			// 바운딩 박스 메쉬 생성
			volume->mMesh = createBoundingBoxMesh(volume->mMin, volume->mMax);
			volume->createTexture();
			DgScene::instance().addSDFVolume(volume);
		}
		else {
			delete volume;
		}
	}
	ImGui::SameLine();

	if (ImGui::ImageButton("roundBox", DgUtil::toImTextureID(icon_tex_id[4]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("boxFrame", DgUtil::toImTextureID(icon_tex_id[5]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("cappedTorus", DgUtil::toImTextureID(icon_tex_id[6]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("link", DgUtil::toImTextureID(icon_tex_id[7]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("cylinder", DgUtil::toImTextureID(icon_tex_id[8]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("cone", DgUtil::toImTextureID(icon_tex_id[9]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("bunny", DgUtil::toImTextureID(icon_tex_id[10]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{
		DgVolume* volume = new DgVolume();
		volume->mName = "Bunny";
		if (volume->loadFromVTI(".\\res\\volume\\Bunny_128.vti")) {
			volume->mMesh = createBoundingBoxMesh(volume->mMin, volume->mMax);
			volume->createTexture();
			DgScene::instance().addSDFVolume(volume);
		}
		else {
			delete volume;
		}
	}
	ImGui::SameLine();
}
