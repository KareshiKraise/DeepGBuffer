#pragma once


#include <iostream>
#include <gl/glew.h>
#include <GLM/glm.hpp>
#include "GL_CALL.h"
#include "shader.h"

//framebuffer for deferred rendering only """NO OTHER USES""""
class ssao_gbuffer
{
public:
	ssao_gbuffer(const int w, const int h)
	{
		GLCall(glGenFramebuffers(1, &fbo));
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

		// - position color buffer
		GLCall(glGenTextures(1, &pos));
		GLCall(glBindTexture(GL_TEXTURE_2D, pos));
		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pos, 0));


		// - normal color buffer
		GLCall(glGenTextures(1, &normal));
		GLCall(glBindTexture(GL_TEXTURE_2D, normal));
		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normal, 0));


		// - color + albedo color buffer
		GLCall(glGenTextures(1, &albedo));
		GLCall(glBindTexture(GL_TEXTURE_2D, albedo));
		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, albedo, 0));


		GLCall(attachments[0] = GL_COLOR_ATTACHMENT0);
		GLCall(attachments[1] = GL_COLOR_ATTACHMENT1);
		GLCall(attachments[2] = GL_COLOR_ATTACHMENT2);
		GLCall(glDrawBuffers(3, attachments));

		GLCall(glGenRenderbuffers(1, &rboDepth));
		GLCall(glBindRenderbuffer(GL_RENDERBUFFER, rboDepth));
		GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h));
		GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth));

		if (validate())
		{
			std::cout << "framebuffer is valid" << std::endl;
		}

		unbind();

	};
	~ssao_gbuffer()
	{
		glDeleteFramebuffers(1, &fbo);
	}

	//returns true if its valid
	bool validate() const
	{
		bind();
		return (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	};
	void init() {};

	void bind() const
	{
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
	};

	void unbind() const
	{
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}

	GLuint pos, normal, albedo; //albedo is actually albedo
	GLuint fbo;
	GLuint rboDepth;

private:
	bool initialized;


	GLuint attachments[3];



};