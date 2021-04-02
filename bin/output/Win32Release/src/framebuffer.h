#pragma once


#include <iostream>
#include <gl/glew.h>
#include <GLM/glm.hpp>
#include "GL_CALL.h"
#include "shader.h"

const int LAYERS = 2;

//framebuffer for deferred rendering only """NO OTHER USES""""
class framebuffer
{
public:
	framebuffer(const int w, const int h)
	{
		wid = w;
		hei = h;

		GLCall(glGenFramebuffers(1, &fbo));
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

		// - position color buffer
		GLCall(glGenTextures(1, &pos));
		GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, pos));
		GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB32F, w, h, LAYERS , 0, GL_RGB, GL_FLOAT, NULL));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCall(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pos, 0));
		
		
		// - normal color buffer
		GLCall(glGenTextures(1, &normal));
		GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, normal));
		GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB32F, w, h, LAYERS,  0, GL_RGB, GL_FLOAT, NULL));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCall(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, normal, 0));		
		

		GLCall(glGenTextures(1, &spec));
		GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, spec));
		GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, w, h, LAYERS, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCall(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, spec, 0));
			

		GLCall(glGenTextures(1, &rboDepth));
		GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, rboDepth));
		GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, w, h, LAYERS , 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));	
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCall(glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, rboDepth, 0));
		
		
		attachments[0] = GL_COLOR_ATTACHMENT0;
		attachments[1] = GL_COLOR_ATTACHMENT1;
		attachments[2] = GL_COLOR_ATTACHMENT2;
		glDrawBuffers(3, attachments);		

		//GLCall(glGenRenderbuffers(1, &rboDepth));
		//GLCall(glBindRenderbuffer(GL_RENDERBUFFER, rboDepth));
		//GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h));
		//GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth));

		std::cout << "creating comparison buffer" << std::endl;
		GLCall(glGenTextures(1, &comparisonDepth));
		GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, comparisonDepth));
		GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, w, h, LAYERS-1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		std::cout << "created with success" << std::endl;
		
		GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (fboStatus == GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "framebuffer is valid" << std::endl;
		}
		else {
			//if (fboStatus == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS) std::cout << "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS" << std::endl;
			std::cout << "error" << std::endl;
		}

		unbind();

		


	};
	~framebuffer()
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

	void copyToComparisonDepth()
	{
		GLCall(glCopyImageSubData(rboDepth, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, comparisonDepth, GL_TEXTURE_2D_ARRAY, 0,0,0,0, wid, hei, 1));
	}
		
	GLuint pos, normal, spec;
	GLuint fbo;
	GLuint rboDepth, comparisonDepth;
	int wid, hei;

private:
	bool initialized;	
	
	GLuint attachments[3];	

};