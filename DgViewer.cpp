// OpenGL 3.3 Core Profile을 사용한 3DViewer
#include "DgViewer.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glfw3_mt.lib")
#pragma comment(lib, "glew32.lib")

int main(int argc, char **argv) 
{
    // GLFW 윈도우를 생성하고, ImGui를 초기화 한다.
    if (!ImGuiManager::instance().init(1280, 1024))
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }
    
    // GLEW 초기화
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) 
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // OpenGL 초기화 및 쉐이더 로딩
    DgScene& scene = DgScene::instance();
    scene.initOpenGL();
    scene.mShaders.push_back(load_shaders(".\\shaders\\ground.vert", ".\\shaders\\ground.frag"));      // 0
    scene.mShaders.push_back(load_shaders(".\\shaders\\toon.vert", ".\\shaders\\toon.frag"));          // 1
    scene.mShaders.push_back(load_shaders(".\\shaders\\phong.vert", ".\\shaders\\phong.frag"));        // 2
    scene.mShaders.push_back(load_shaders(".\\shaders\\texture.vert", ".\\shaders\\texture.frag"));    // 3
    scene.mShaders.push_back(load_shaders(".\\shaders\\phong.vert", ".\\shaders\\effect-1.frag"));     // 4
    scene.mShaders.push_back(load_shaders(".\\shaders\\phong.vert", ".\\shaders\\effect-2.frag"));     // 5
    scene.mShaders.push_back(load_shaders(".\\shaders\\phong.vert", ".\\shaders\\effect-3.frag"));     // 6 법선 칼라
    scene.mShaders.push_back(load_shaders(".\\shaders\\phong.vert", ".\\shaders\\effect-4.frag"));     // 7 전반사 RGB
    scene.mShaders.push_back(load_shaders(".\\shaders\\phong.vert", ".\\shaders\\effect-5.frag"));     // 8 
    scene.mShaders.push_back(load_shaders(".\\shaders\\phong.vert", ".\\shaders\\effect-6.frag"));     // 9 
    
    // 바닥 평면 메쉬를 생성
    scene.createGroundMesh();

    /*
    // 메쉬를 임포트하여 장면에 추가
    DgMesh *pMesh1 = ::import_mesh_obj(".\\model\\bunny_8327v.obj");
    scene.mMeshList.push_back(pMesh1);
    pMesh1->mName = "Bunny";
    pMesh1->computeNormal(0);
    pMesh1->mShaderId = scene.mShaders[2];
    
    DgMesh *pMesh2 = ::import_mesh_obj(".\\model\\camel.obj");
    scene.mMeshList.push_back(pMesh2);
    pMesh2->mName = "Camel";
    pMesh2->computeNormal(0);
    pMesh2->mShaderId = scene.mShaders[2];*/
    
    // 메인 루프 진입
    while (!glfwWindowShouldClose(ImGuiManager::instance().mWindow))
    {
        glfwPollEvents();        
        ImGuiManager::instance().begin();
        {
            // 메인 메뉴 렌더링
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Exit"))
                    {
                        glfwSetWindowShouldClose(ImGuiManager::instance().mWindow, true);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }
            ShowWindowSceneLayer(&show_window_scene_layer);         // 씬 레이어 윈도우 출력
            ShowWindowToolBar(&show_window_tool_bar);               // 툴바 윈도우 출력
            DgScene::instance().showWindow();                       // SceneGL 윈도우 출력
			ShowWindowModelProperty(&show_window_model_property);   // 모델 속성 윈도우 출력
        }
        ImGuiManager::instance().end();        
    }
    ImGuiManager::instance().cleanUp();
    return 0;
}






