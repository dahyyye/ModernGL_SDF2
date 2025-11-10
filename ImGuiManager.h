#pragma once

/*!
 *	\class	ImGuiManager
 *	\brief	GLFW + OpenGL3.3 기반의 ImGui 응용프로그램 개발을 위한 매니저 싱글톤(singleton) 클래스
 *
 *	\author 윤승현(shyun@dongguk.edu)
 *	\date	2024-03-04
 */
class ImGuiManager
{
public:
	/*! \biref	메인 윈도우 포인터 */
	GLFWwindow *mWindow;

private:
	/*! 
	 *	\biref 생성자 
	 */
	ImGuiManager();

	/*! 
	 *	\biref 소멸자 
	 */
	~ImGuiManager();

public:
	/*!
	 *	\biref	클래스 객체를 반환한다.
	 *
	 *	\return 클래스 객체를 반환한다.
	 */
	static ImGuiManager& instance()
	{
		static ImGuiManager _inst;
		return _inst;
	}

	/*! 
	 *	\biref	GLFW 윈도우를 생성하고 ImGui를 초기화 한다.	
	 */
	bool init(int winWidth, int winHeight);

	/*! 
	 *	\biref	새로운 프레임을 시작한다.	
	 */
	void begin();

	/*! 
	 *	\biref	프레임의 내용을 렌더링한다. 
	 */
	void end();

	/*! 
	 *	\biref	생성된 윈도우와 ImGui를 종료한다. 
	 */
	void cleanUp();
};

/*!
 *	\class	DgFrmBuffer
 *	\brief	OpenGL 프레임 버퍼를 표현하는 클래스
 *
 *	\author 윤승현(shyun@dongguk.edu)
 *	\date	2023-12-02
 */
class DgFrmBuffer
{
public:
	/*! \biref 프레임 버퍼 객체 핸들 */
	unsigned int mFrameBufObj;

	/*! \biref 생성된 텍스처 아이디 */
	unsigned int mTexture;

	/*! \biref 렌더링 버퍼 객체 핸들 */
	unsigned int mRenderBufObj;

public:
	/*! \biref 생성자 */
	DgFrmBuffer(int width = 100, int height = 100) {
		// 프레임 버퍼의 핸들을 생성하고 바인딩한다.
		glGenFramebuffers(1, &mFrameBufObj);
		glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufObj);

		// 텍스처 핸들을 생성하여 바인딩한다.
		glGenTextures(1, &mTexture);
		glBindTexture(GL_TEXTURE_2D, mTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture, 0);

		// 렌더링 버퍼의 핸들을 생성하고 바인딩한다.
		glGenRenderbuffers(1, &mRenderBufObj);
		glBindRenderbuffer(GL_RENDERBUFFER, mRenderBufObj);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRenderBufObj);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

		// 버퍼를 해제한다.
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	/*! \biref 소멸자 */
	~DgFrmBuffer() {
		glDeleteFramebuffers(1, &mFrameBufObj);
		glDeleteTextures(1, &mTexture);
		glDeleteRenderbuffers(1, &mRenderBufObj);
	}

	/*!
	 *	\biref	생성된 텍스처를 반환한다.
	 *
	 *	\return	생성된 텍스처를 반환한다.
	 */
	unsigned int getFrameTexture() {
		return mTexture;
	}

	/*!
	 *	\biref	프레임 버퍼의 크기를 재설정한다.
	 *
	 *	\param[in]	width	버퍼의 너비
	 *	\param[in]	height	버퍼의 높이
	 */
	void rescaleFrameBuffer(int width, int height) {
		glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufObj);
		glBindTexture(GL_TEXTURE_2D, mTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture, 0);

		glBindRenderbuffer(GL_RENDERBUFFER, mRenderBufObj);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRenderBufObj);
	}

	/*!
	 *	\biref	프레임 버퍼를 바인딩한다.
	 */
	void bind() const {
		glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufObj);
	}

	/*!
	 *	\biref	프레임 버퍼를 언바인딩한다.
	 */
	void unbind() const {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
};
