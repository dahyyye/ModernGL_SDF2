#pragma once

/*!
 *  \file   DgUtil.h
 *  \brief  프로젝트 공통 유틸리티 함수
 *
 *  텍스처 로딩, ImGui 변환 등 여러 파일에서 공통으로 사용하는
 *  헬퍼 함수를 한 곳에 모아둔다.
 *
 *  주의: stb_image 구현부(STB_IMAGE_IMPLEMENTATION)는 프로젝트 내
 *        단 하나의 .cpp에서만 정의해야 한다. 이 헤더는 stb_image.h를
 *        include하지 않으므로, 사용하는 쪽의 .cpp에서 include한다.
 */
namespace DgUtil
{
    /*!
     *  \brief  이미지 파일을 OpenGL 2D 텍스처로 로드
     *
     *  PNG, JPG, BMP 등 stb_image가 지원하는 포맷을 읽어서
     *  GL_TEXTURE_2D로 업로드한다. 아이콘, UI 이미지 등에 사용.
     *
     *  \param[in]  filepath    이미지 파일 경로
     *  \return     생성된 텍스처 ID (실패 시 0)
     */
    inline GLuint loadTexture2D(const char* filepath)
    {
        int w, h, ch;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* pixels = stbi_load(filepath, &w, &h, &ch, 0);
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

    /*!
     *  \brief  OpenGL 텍스처 ID를 ImGui 텍스처 ID로 변환
     *
     *  ImGui::Image(), ImGui::ImageButton() 등에서 사용할 수 있는
     *  ImTextureID 타입으로 캐스팅한다.
     *
     *  \param[in]  texId   OpenGL 텍스처 ID
     *  \return     ImTextureID로 변환된 값
     */
    inline ImTextureID toImTextureID(GLuint texId)
    {
        return (ImTextureID)(uintptr_t)texId;
    }

}
