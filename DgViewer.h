#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <queue>
#include <numeric>

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
#include "DgVolume.h"
#include "DgScene.h"
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

// 벡터의 원소의 개수를 구하는 매크로
#define NUM(List) ((int)(List.size()))

// 에지 E의 시작 정점과 끝 정점을 구하는 매크로
#define SV(E)	(E)->mVert
#define EV(E)	(E)->mNext->mVert
#define SP(E)	(E)->mVert->mPos
#define EP(E)	(E)->mNext->mVert->mPos
#define NEXT(E)	(E)->mNext
#define PREV(E)	(E)->mNext->mNext
#define MATE(E)	(E)->mMate

// 에지 E1, E2가 메이트 에지인지를 확인하는 매크로
#define IS_MATE_EDGE(E1, E2) (((E1)->mVert) == ((E2)->mNext->mVert))

