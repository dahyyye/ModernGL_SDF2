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

bool show_window_model_property = true;
void OpenProperty();

void ShowWindowModelProperty(bool* p_open)
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

	// 속성 윈도우를 생성하고, collapsed된 상태라면 바로 리턴한다.
	if (!ImGui::Begin("Property", p_open, window_flags))
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
		".\\res\\icons\\Union-A-B.png",
		".\\res\\icons\\sculpt_add.png",
		".\\res\\icons\\sculpt_remove.png"
	};

	static GLuint icon_tex_id[NumIcons] = { 0 };
	if (icon_tex_id[0] == 0)
	{
		for (int i = 0; i < NumIcons; ++i)
			icon_tex_id[i] = LoadTexture2D(icon_files[i]);
	}

	// 기본 모델 정보를 출력한다.
	if (ImGui::CollapsingHeader("Boolean"))
	{
		if (ImGui::ImageButton("Union", ToImTex(icon_tex_id[0]), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
		{

		}
		ImGui::SameLine();
		if (ImGui::ImageButton("Intersection", ToImTex(icon_tex_id[1]), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
		{

		}
		ImGui::SameLine();
		if (ImGui::ImageButton("Difference", ToImTex(icon_tex_id[2]), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
		{

		}
	}

	if (ImGui::CollapsingHeader("Offset"))
	{

	}

	if (ImGui::CollapsingHeader("Sculpt"))
	{
		if (ImGui::ImageButton("sculpt_add", ToImTex(icon_tex_id[3]), ImVec2(84, 84), ImVec2(0, 1), ImVec2(1, 0)))
		{

		}
		ImGui::SameLine();
		if (ImGui::ImageButton("sculpt_remove", ToImTex(icon_tex_id[4]), ImVec2(84, 84), ImVec2(0, 1), ImVec2(1, 0)))
		{

		}
	}

	if (ImGui::CollapsingHeader("Sweeping"))
	{

	}
}