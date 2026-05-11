#include "DgViewer.h"

bool show_window_tool_bar = true;
void CreateMesh();

void ShowWindowToolBar(bool* p_open) {

	ImGuiWindowFlags window_flags = 0;

	// 속성 윈도우를 생성하고, collapsed된 상태라면 바로 리턴한다.
	if (!ImGui::Begin("ToolBar", nullptr, window_flags))
	{
		ImGui::End();
		return;
	}
	CreateMesh();
	ImGui::End();
}

void CreateMesh() {
	const int NumIcons = 9;

	const char* icon_files[NumIcons] = {
		".\\res\\icons\\new_scene.png",
		".\\res\\icons\\sphere.png",
		".\\res\\icons\\box.png",
		".\\res\\icons\\torus.png",
		".\\res\\icons\\cylinder.png",
		".\\res\\icons\\capsule.png",
		".\\res\\icons\\quad_pramid.png",
		".\\res\\icons\\cone.png",
		".\\res\\icons\\bunny.png"
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
		if (volume->loadFromVTI(".\\res\\volume\\(mini)Sphere_128.vti")) {
			volume->mName = "Sphere";
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

		// VTI 파일에서 SDF 로드
		if (volume->loadFromVTI(".\\res\\volume\\(mini)Box_128.vti")) {
			volume->mName = "Box";
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

	if (ImGui::ImageButton("Torus", DgUtil::toImTextureID(icon_tex_id[3]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{
		DgVolume* volume = new DgVolume();

		// VTI 파일에서 SDF 로드
		if (volume->loadFromVTI(".\\res\\volume\\(mini)Torus_128.vti")) {
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

	if (ImGui::ImageButton("Cylinder", DgUtil::toImTextureID(icon_tex_id[4]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{
		DgVolume* volume = new DgVolume();

		// VTI 파일에서 SDF 로드
		if (volume->loadFromVTI(".\\res\\volume\\(mini)Cylinder_128.vti")) {
			volume->mName = "Cylinder";
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

	if (ImGui::ImageButton("Capsule", DgUtil::toImTextureID(icon_tex_id[5]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{
		DgVolume* volume = new DgVolume();

		// VTI 파일에서 SDF 로드
		if (volume->loadFromVTI(".\\res\\volume\\(mini)Capsule_128.vti")) {
			volume->mName = "Capsule";
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

	if (ImGui::ImageButton("QuadPramid", DgUtil::toImTextureID(icon_tex_id[6]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{
		DgVolume* volume = new DgVolume();

		// VTI 파일에서 SDF 로드
		if (volume->loadFromVTI(".\\res\\volume\\(mini)QuadPyramid_128.vti")) {
			volume->mName = "QuadPramid";
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

	if (ImGui::ImageButton("Cone", DgUtil::toImTextureID(icon_tex_id[7]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{
		DgVolume* volume = new DgVolume();

		// VTI 파일에서 SDF 로드
		if (volume->loadFromVTI(".\\res\\volume\\(mini)Cone_128.vti")) {
			volume->mName = "Cone";
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

	if (ImGui::ImageButton("Bunny", DgUtil::toImTextureID(icon_tex_id[8]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{
		DgVolume* volume = new DgVolume();
		volume->mName = "Bunny";
		if (volume->loadFromVTI(".\\res\\volume\\(mini)Bunny_128.vti")) {
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
