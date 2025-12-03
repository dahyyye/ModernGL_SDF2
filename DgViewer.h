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
#include "DgDeform.h"
#include "DgSweep.h"
#include "DgBoolean.h"
#include "DgBvh.h"

// Window관련 cpp에서 구현된 함수
void ShowWindowToolBar(bool* p_open);
void ShowWindowSceneLayer(bool* p_open);
void ShowWindowModelProperty(bool* p_open);

// 전역 변수 선언
extern bool show_window_tool_bar;
extern bool show_window_scene_layer;
extern bool show_window_model_property;

// 매크로 정의
#define MTYPE_EPS	1.0e-6
#define MAX_BVH_DEPTH 10
#define RAD2DEG(X)	((X) * 57.29577951308232)
#define M_PI       3.14159265358979323846

#define MIN(x, y)	((x) > (y) ? (y) : (x))
#define MAX(x, y)	((x) > (y) ? (x) : (y))
#define ABS(X)		(((X) > 0.0) ? (X) : (-(X)))
#define SQRT(X)		sqrt((X))
#define EQ_ZERO(X, EPS) (ABS(X) < EPS)
#define SQR(X)		((X) * (X))
#define EQ(X, Y, EPS)	(ABS((X) - (Y)) < EPS)
#define EQ_ZERO(X, EPS) (ABS(X) < EPS)

