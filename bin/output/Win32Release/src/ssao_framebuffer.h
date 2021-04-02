#pragma once


#include <iostream>
#include <gl/glew.h>
#include <GLM/glm.hpp>
#include "GL_CALL.h"
#include "shader.h"

enum class FBO {
	SSAO = 0x01,
	SSAO_BLUR
};

//screen space ambient occlusion specialized fbo
class ssao_framebuffer
{
public:
	ssao_framebuffer(const int w, const int h)
	{
		// - SSAO Color
		GLCall(glGenFramebuffers(1, &ssaoFBO));		
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO));		
		GLCall(glGenTextures(1, &ssaoColorBuffer));
		GLCall(glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer));
		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, NULL));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0));
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "SSAO Framebuffer not complete!" << std::endl;

		// - SSAO blur
		GLCall(glGenFramebuffers(1, &ssaoBlurFBO));
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO));
		GLCall(glGenTextures(1, &ssaoColorBufferBlur));
		GLCall(glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur));
		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, NULL));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0));

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
				

	};
	~ssao_framebuffer()
	{
		glDeleteFramebuffers(1, &ssaoFBO);
		glDeleteFramebuffers(1, &ssaoBlurFBO);
	}
		

	void bind(FBO fbo) const
	{
		switch (fbo)
		{
		case FBO::SSAO:
			GLCall(glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO));
			break;

		case FBO::SSAO_BLUR:
			GLCall(glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO));
			break;
		}
		
		
	};

	void unbind() const
	{
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));			
	}

	
	GLuint ssaoFBO, ssaoBlurFBO;
	GLuint ssaoColorBuffer, ssaoColorBufferBlur;


};