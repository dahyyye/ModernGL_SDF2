#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <filesystem>

// GLEW/GLFW 관련 헤더 파일
#include "./include/gl/glew.h"
#include "./include/glfw/glfw3.h"

// GLM 관려 헤더 파일
#include "./include/glm/glm.hpp"
#include "./include/glm/gtc/matrix_transform.hpp"
#include "./include/glm/gtc/type_ptr.hpp"

// IMGUI 관련 헤더 파일
#define IMGUI_DEFINE_MATH_OPERATORS
#include "./include/ImGui/imgui.h"
#include "./include/ImGui/imgui_internal.h"
#include "./include/imgui/imgui_impl_opengl3.h"
#include "./include/imgui/imgui_impl_glfw.h"
#include "./include/ImGui/imgui_file_dlg.h"
#include "./include/ImGui/imgui_console.h"

// 자체 헤더 파일
#include "ImGuiManager.h"
#include "DgMesh.h"
#include "DgScene.h"
#include "DgVolume.h"
#include "DgSweep.h"
#include "DgBoolean.h"

//Window관련 cpp에서 구현된 함수
void ShowWindowToolBar(bool* p_open);
void ShowWindowSceneLayer(bool* p_open);
void ShowWindowModelProperty(bool* p_open);

// 전역 변수 선언
extern bool show_window_tool_bar;
extern bool show_window_scene_layer;
extern bool show_window_model_property;
