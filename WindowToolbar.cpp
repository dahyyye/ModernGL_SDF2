#include "DgViewer.h"
#define GLM_ENABLE_EXPERIMENTAL
#include ".\\include\\STB\\stb_image.h"

static inline ImTextureID ToImTex(GLuint tex) {
	return (ImTextureID)(uintptr_t)tex;
}

// 텍스처 로더
static GLuint LoadTexture2D(const char* file) {
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
			icon_tex_id[i] = LoadTexture2D(icon_files[i]);
	}

	if (ImGui::ImageButton("NewScene", ToImTex(icon_tex_id[0]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("Sphere", ToImTex(icon_tex_id[1]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{
	
	}
	ImGui::SameLine();

	if (ImGui::ImageButton("Box", ToImTex(icon_tex_id[2]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("Torus", ToImTex(icon_tex_id[3]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("roundBox", ToImTex(icon_tex_id[4]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("boxFrame", ToImTex(icon_tex_id[5]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("cappedTorus", ToImTex(icon_tex_id[6]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("link", ToImTex(icon_tex_id[7]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("cylinder", ToImTex(icon_tex_id[8]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("cone", ToImTex(icon_tex_id[9]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{

	}
	ImGui::SameLine();

	if (ImGui::ImageButton("bunny", ToImTex(icon_tex_id[10]), ImVec2(30, 30), ImVec2(0, 1), ImVec2(1, 0)))
	{
		DgVolume* volume = new DgVolume();
		volume->mMesh = import_mesh_obj(".\\res\\object\\bunny.obj");
		volume->setDimensions(16, 16, 16);
		volume->setGridSpace(*volume->mMesh, 0.5);
		volume->mDim[0] = 16;
		volume->mDim[1] = 16;
		volume->mDim[2] = 16;
		volume->computeSDF();
	}
	ImGui::SameLine();
}