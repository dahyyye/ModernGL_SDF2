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
	const int NumIcons = 14;

	const char* icon_files[NumIcons] = {
		".\\res\\icons\\new_scene.png",
		".\\res\\icons\\sphere.png",
		".\\res\\icons\\box.png",
		".\\res\\icons\\torus.png",
		".\\res\\icons\\cylinder.png",
		".\\res\\icons\\capsule.png",
		".\\res\\icons\\quad_pramid.png",
		".\\res\\icons\\cone.png",
		".\\res\\icons\\bunny.png",
		".\\res\\icons\\knots.png",
		".\\res\\icons\\Paint_roller.png",
		".\\res\\icons\\floatplane.png",
		".\\res\\icons\\dog.png",
		".\\res\\icons\\kitten.png"
	};

	static GLuint icon_tex_id[NumIcons] = { 0 };
	if (icon_tex_id[0] == 0)
	{
		for (int i = 0; i < NumIcons; ++i)
			icon_tex_id[i] = DgUtil::loadTexture2D(icon_files[i]);
	}

	auto loadVolumeVTI = [](const char* path, const char* name) {
		DgVolume* volume = new DgVolume();
		if (volume->loadFromVTI(path)) {
			volume->mName = name;
			volume->mMesh = createBoundingBoxMesh(volume->mMin, volume->mMax);
			volume->createTexture();
			DgScene::instance().addSDFVolume(volume);
		}
		else {
			delete volume;
		}
		};

	if (ImGui::ImageButton("NewScene", DgUtil::toImTextureID(icon_tex_id[0]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		DgScene::instance().resetScene();
	ImGui::SameLine();

	if (ImGui::ImageButton("Sphere", DgUtil::toImTextureID(icon_tex_id[1]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		loadVolumeVTI(".\\res\\volume\\(mini)Sphere_128.vti", "Sphere");
	ImGui::SameLine();

	if (ImGui::ImageButton("Box", DgUtil::toImTextureID(icon_tex_id[2]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		loadVolumeVTI(".\\res\\volume\\(mini)Box_128.vti", "Box");
	ImGui::SameLine();

	if (ImGui::ImageButton("Torus", DgUtil::toImTextureID(icon_tex_id[3]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		loadVolumeVTI(".\\res\\volume\\(mini)Torus_256.vti", "Torus");
	ImGui::SameLine();

	if (ImGui::ImageButton("Cylinder", DgUtil::toImTextureID(icon_tex_id[4]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		loadVolumeVTI(".\\res\\volume\\(mini)Cylinder_256.vti", "Cylinder");
	ImGui::SameLine();

	if (ImGui::ImageButton("Capsule", DgUtil::toImTextureID(icon_tex_id[5]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		loadVolumeVTI(".\\res\\volume\\(mini)Capsule_256.vti", "Capsule");
	ImGui::SameLine();

	if (ImGui::ImageButton("QuadPramid", DgUtil::toImTextureID(icon_tex_id[6]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		loadVolumeVTI(".\\res\\volume\\(mini)QuadPyramid_256.vti", "QuadPramid");
	ImGui::SameLine();

	if (ImGui::ImageButton("Cone", DgUtil::toImTextureID(icon_tex_id[7]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		loadVolumeVTI(".\\res\\volume\\(mini)Cone_256.vti", "Cone");
	//loadVolumeVTI(".\\res\\volume\\Wheel_256.vti", "Cone");
	ImGui::SameLine();

	if (ImGui::ImageButton("Bunny", DgUtil::toImTextureID(icon_tex_id[8]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		loadVolumeVTI(".\\res\\volume\\(mini)Bunny_256.vti", "Bunny");
	ImGui::SameLine();

	if (ImGui::ImageButton("knots", DgUtil::toImTextureID(icon_tex_id[9]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		loadVolumeVTI(".\\res\\volume\\knots_256.vti", "knots");
	ImGui::SameLine();

	if (ImGui::ImageButton("Paint_Roller", DgUtil::toImTextureID(icon_tex_id[10]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		loadVolumeVTI(".\\res\\volume\\Paint_Roller_2562.vti", "Paint_Roller");
	ImGui::SameLine();

	if (ImGui::ImageButton("Floatplane", DgUtil::toImTextureID(icon_tex_id[11]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		loadVolumeVTI(".\\res\\volume\\floatplane_256.vti", "Floatplane");
	ImGui::SameLine();

	if (ImGui::ImageButton("dog", DgUtil::toImTextureID(icon_tex_id[12]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		loadVolumeVTI(".\\res\\volume\\dog_2562.vti", "dog");
	ImGui::SameLine();

	if (ImGui::ImageButton("kitten", DgUtil::toImTextureID(icon_tex_id[13]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
		loadVolumeVTI(".\\res\\volume\\kitten_256.vti", "kitten");
	ImGui::SameLine();
}
