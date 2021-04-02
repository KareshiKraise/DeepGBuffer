#include <iostream>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <SOIL/SOIL.h>
#include "model.h"
#include "mesh_loader.h"
#include <algorithm>
#include "framebuffer.h"
#include "ssao_framebuffer.h"
#include "ssao_gbuffer.h"
#include <random>
#include "camera.h"


/*------------glm includes----------------*/
#include <GLM/glm.hpp>
#include <GLM/mat4x4.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
/*----------------------------------------*/

float  Wid = 1280.0f;
float  Hei = 720.0f;

mesh_loader *mesh;

const int num_samples = 32; //SSAO samples

Shader *shader_lighting_pass;
Shader *gbuffer;
Shader *SSAO;
Shader *SSAO_blur;

framebuffer *fb;
ssao_framebuffer *ssao_fb;
ssao_gbuffer *ssao_gb;
std::vector<glm::vec3> ssaoKernel;
std::vector<glm::vec3> ssaoNoise;
GLuint noiseTexture;

//used to keep track of center of mass , view matrix' forward position
glm::vec3 obj;

/* Uniforms to control SSAO */
float radius = 2.0f;
float bias = 0.025f;
float beta = 0.05f;
float epsilon = 0.000001f;

//fov near and far planes
float gfov = 60.0f;
float n_v = 0.1f;
float f_v = 1000.0f;

Camera *camera;
bool cam_mouse_down = false;
float lastX = 0;
float curX = 0;
float lastY = 0;
float curY = 0;

float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

void generateSampleKernel()
{
	std::uniform_real_distribution<GLfloat> randomFloats(0.0f, 1.0f);
	std::default_random_engine generator;

	for (int i = 0; i < num_samples; ++i)
	{
		glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0,
					     randomFloats(generator) * 2.0 - 1.0,
						 randomFloats(generator));
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		
		float scale = float(i)/ num_samples;
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}
}

void generateNoise()
{
	std::uniform_real_distribution<GLfloat> randomFloats(0.0f, 1.0f);
	std::default_random_engine generator;
	for (int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, 
						randomFloats(generator) * 2.0 - 1.0,
						0.0f);
		noise = glm::normalize(noise);
		ssaoNoise.push_back(noise);
	}
	GLCall(glGenTextures(1, &noiseTexture));
	GLCall(glBindTexture(GL_TEXTURE_2D, noiseTexture));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

struct arcball
{
	int curx=0, cury=0, lastx=0, lasty=0;
	bool active = false;
	bool locked = true;

	glm::vec3 get_vector(int x, int y)
	{
		float midw = Wid / 2.0f;
		float midh = Hei / 2.0f;
		glm::vec3 P = glm::vec3( (((float)x / midw) - 1.0f),
								 (((float)y / midh) - 1.0f),
								  0.0f);
		P.y = -P.y;
		float OP_pow2 = (P.x * P.x) + (P.y * P.y);
		if (OP_pow2 <= 1.0f)
		{
			P.z = sqrt(1.0f - OP_pow2);
			P = glm::normalize(P);
		}
		else
		{
			P = glm::normalize(P);
		}
		return P;
	}

	void update_model_matrix(glm::mat4& Model, glm::mat4& View)
	{
		if (curx != lastx || cury != lasty)
		{
			glm::vec3 va = get_vector(lastx, lasty);
			glm::vec3 vb = get_vector(curx, cury);
			float angle = acos(std::min(1.0f, glm::dot(va, vb)));
			angle *= 0.05f;

			glm::vec3 axis_in_camera = glm::cross(va, vb);
			glm::mat3 inverse = glm::inverse(glm::mat3(View) * glm::mat3(Model));

			glm::vec3 axis_in_model = inverse * axis_in_camera;
									
			//Lembrar disso para sempre
			//column major, must translate positive first, then negative
			Model = glm::translate(Model, mesh->bb_mid);
			Model = glm::rotate(Model, glm::degrees(angle), axis_in_model);
			Model = glm::translate(Model, -mesh->bb_mid);
			
			lastx = curx;
			lasty = cury;
		}
	}

}g_arcball;

glm::vec3 eyePos;
glm::mat4 gModel = glm::mat4(1);

glm::mat4 gView = glm::mat4(1);//glm::lookAt(eyePos, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
glm::mat4 gProj = glm::perspective(glm::radians(gfov), Wid/Hei, n_v, f_v);


struct quad
{
	GLuint quadVAO;
	GLuint quadVBO;

	void renderQuad()
	{
		if (quadVAO == 0)
		{
			float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};


			GLCall(glGenVertexArrays(1, &quadVAO));
			GLCall(glGenBuffers(1, &quadVBO));
			GLCall(glBindVertexArray(quadVAO));
			GLCall(glBindBuffer(GL_ARRAY_BUFFER, quadVBO));
			GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW));
			GLCall(glEnableVertexAttribArray(0));
			GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0));
			GLCall(glEnableVertexAttribArray(1));
			GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))));
		}
	
		GLCall(glBindVertexArray(quadVAO));
		GLCall(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
		GLCall(glBindVertexArray(0));
	}
} screen_quad;

struct cube {
	GLuint cubeVAO;
	GLuint cubeVBO;

	void renderCube()
	{
		if(cubeVAO == 0)
		{
			float vertices[] = {
				// back face
				-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
				1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
				1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
				1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
				-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
				-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
				// front face
				-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
				1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
				1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
				1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
				-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
				-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
				// left face
				-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
				-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
				-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
				-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
				-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
				-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
				// right face
				1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
				1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
				1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
				1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
				1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
				1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
				// bottom face
				-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
				1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
				1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
				1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
				-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
				-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
				// top face
				-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
				1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
				1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
				1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
				-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
				-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
			};

			glGenVertexArrays(1, &cubeVAO);
			glGenBuffers(1, &cubeVBO);
			glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			glBindVertexArray(cubeVAO);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
	}
} screen_cube;


Shader *deep_g_shader;
Shader *print_shader;
Shader *deep_g_ssao;
Shader *deep_g_blur;
Shader *deep_g_light;

/*---Eye Vectors---*/

glm::vec3 look;
//glm::vec3 forward;
const glm::vec3 up(0.0f, 1.0f, 0.0f);
//glm::vec3 right;
float move_speed = 1.0f;

int AO = 0; //see ambient occlusion map
int remove_AO = 0; // turn ao on or off

/*-----------------*/

void onIdle()
{
	glutPostRedisplay();
}

void onKeyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case '1':
		AO = !AO;		
		break;	
	case '2':
		remove_AO = !remove_AO;
		break;
	case '+':
		eyePos = eyePos + up * move_speed;
		look = look + up * move_speed;		
		break;

	case '-':
		eyePos = eyePos - up * move_speed;
		look = look - up * move_speed;
		break;
	case 'b':
	case 'B':
		radius -= 0.25;		
		break;
	case 'g':
	case 'G':
		radius += 0.25;
		break;

	case 'h':
	case 'H':
		bias += 0.1;
		break;
	case 'n':
	case 'N':
		bias -= 0.1;		
		break;

	case 'j':
	case 'J':
		epsilon += 0.00001;		
		break;
	case 'm':
	case 'M':
		epsilon -= 0.00001;
		break;
	
	case 'w':
		camera->ProcessKeyboard(FORWARD, 1);
		break;
	case 'a':
		camera->ProcessKeyboard(LEFT, 1);
		break;
	case 's':
		camera->ProcessKeyboard(BACKWARD, 1);
		break;
	case 'd':
		camera->ProcessKeyboard(RIGHT, 1);
		break;
	case 'e':
		std::cout << camera->Position.x << std::endl;
		std::cout << camera->Position.y << std::endl;
		std::cout << camera->Position.z << std::endl;
		//std::cout << "mesh mid " << std::endl;
		//std::cout << mesh->bb_mid.x << std::endl;
		//std::cout << mesh->bb_mid.y << std::endl;
		//std::cout << mesh->bb_mid.z << std::endl;
		break;
	}	

	glutPostRedisplay();
}

void onSpecialInput(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		//eyePos = eyePos + forward * move_speed;
		//look = (look + forward * move_speed);
		camera->ProcessKeyboard(FORWARD, 1);
		
		break;
	case GLUT_KEY_DOWN:
		//eyePos = eyePos - forward * move_speed;
		//look = (look - forward * move_speed);
		camera->ProcessKeyboard(BACKWARD, 1);
		
		break;
	case GLUT_KEY_LEFT:
		//eyePos = eyePos + right * move_speed;
		//look = (look + right * move_speed);
		camera->ProcessKeyboard(LEFT, 1);
		
		break;
	case GLUT_KEY_RIGHT:
		//eyePos = eyePos - right * move_speed;
		//look = (look - right * move_speed);
		camera->ProcessKeyboard(RIGHT, 1);
		
		break;
	}

	glutPostRedisplay();

}

void onMouse(int button, int state, int x, int y)
{
	
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		cam_mouse_down = true;
		lastX = curX = x;
		lastY = curY = y;

	}
	else
	{
		cam_mouse_down = false;
	}	
	
	glutPostRedisplay();
}

void onMotion(int x, int y)
{
	if (cam_mouse_down)
	{
		
		curX = x;
		curY = y;

		float xoff = curX - lastX;
		float yoff = lastY - curY;

		lastX = x;
		lastY = y;

		camera->ProcessMouseMovement(xoff, yoff);
	}

	glutPostRedisplay();
}

void onResize(int w, int h)
{
	glViewport(0, 0, w, h);
	Wid = w;
	Hei = h;
	gProj = glm::perspective(glm::radians(gfov), Wid/Hei, n_v, f_v);
	glutPostRedisplay();
}

//deep g buffer texture array experiment 
void renderTest(void)
{			
	glm::mat4 M = gModel;
	glm::mat4 V = camera->GetViewMatrix();
	glm::mat4 MV = V * M;
	glm::mat4 P = gProj;		
		
	
	fb->bind();	
	fb->copyToComparisonDepth();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	deep_g_shader->use();	

	//glViewport(0, 0, Wid, Hei);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	
	deep_g_shader->setMat4("MV", MV);
	deep_g_shader->setMat4("Proj", P);
	deep_g_shader->setFloat("near",n_v);
	deep_g_shader->setFloat("far",f_v);

	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glUniform1i(glGetUniformLocation(deep_g_shader->ID, "prevDepth"), 1));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, fb->comparisonDepth));

	mesh->Draw(*deep_g_shader);
	
	fb->unbind();	
	
	/*-----------------------SSAO-----------------------------------*/
	
	ssao_fb->bind(FBO::SSAO);
	glClear(GL_COLOR_BUFFER_BIT);

	deep_g_ssao->use();

	deep_g_ssao->setFloat("radius", radius);
	deep_g_ssao->setFloat("bias", bias);
	//deep_g_ssao->setFloat("epsilon",epsilon);

	deep_g_ssao->setMat4("P", P);
	
	for (unsigned int i = 0; i < num_samples; ++i)
	{
		std::string location = "kernel_samples[" + std::to_string(i) + "]";
		deep_g_ssao->setVec3(location, ssaoKernel[i]);
	}

	GLCall(glActiveTexture(GL_TEXTURE0));
	deep_g_ssao->setInt("Position", 0);
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, fb->pos));

	GLCall(glActiveTexture(GL_TEXTURE1));
	deep_g_ssao->setInt("Normal", 1);
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, fb->normal));

	GLCall(glActiveTexture(GL_TEXTURE2));
	deep_g_ssao->setInt("texNoise", 2);
	GLCall(glBindTexture(GL_TEXTURE_2D, noiseTexture));
	
	screen_quad.renderQuad();

	ssao_fb->unbind();
	
	/*-----------------------------BLUR-------------------------------------------*/
	
	ssao_fb->bind(FBO::SSAO_BLUR);
	glClear(GL_COLOR_BUFFER_BIT);

	deep_g_blur->use();
	GLCall(glActiveTexture(GL_TEXTURE0));
	deep_g_blur->setInt("SSAOInput", 0);
	GLCall(glBindTexture(GL_TEXTURE_2D, ssao_fb->ssaoColorBuffer));
	screen_quad.renderQuad();
	ssao_fb->unbind();
		

	/*--------------------------LIGHTING PASS ---------------------------------------*/
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	deep_g_light->use();
	glm::vec3 lightpos = glm::vec3(camera->GetViewMatrix() * glm::vec4(mesh->bb_mid, 1.0f));
	deep_g_light->setVec3("light.pos", lightpos);
	deep_g_light->setVec3("light.col", glm::vec3(0.3, 0.3, 0.8));
	deep_g_light->setInt("AO", AO);

	deep_g_light->setInt("Position", 0);
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, fb->pos));

	deep_g_light->setInt("Normal", 1);
	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, fb->normal));

	deep_g_light->setInt("Albedo", 2);
	GLCall(glActiveTexture(GL_TEXTURE2));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, fb->spec));

	deep_g_light->setInt("ssao", 3);
	GLCall(glActiveTexture(GL_TEXTURE3));
	GLCall(glBindTexture(GL_TEXTURE_2D, ssao_fb->ssaoColorBufferBlur));

	screen_quad.renderQuad();
	


	/*--------------------------DEBUG DRAW ON SCREEN --------------------------------*/
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//
	//print_shader->use();
	//print_shader->setInt("fbcol", 0);
	//GLCall(glActiveTexture(GL_TEXTURE0));
	//GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, fb->spec));
	//
	//print_shader->setInt("normcol", 1);
	//GLCall(glActiveTexture(GL_TEXTURE1));
	//GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, fb->normal));
	//
	//print_shader->setInt("prevDepth", 2);
	//GLCall(glActiveTexture(GL_TEXTURE2));
	//GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, fb->comparisonDepth));
	//
	//print_shader->setInt("occlusionBlur", 3);
	//GLCall(glActiveTexture(GL_TEXTURE3));
	//GLCall(glBindTexture(GL_TEXTURE_2D, ssao_fb->ssaoColorBufferBlur));
	//
	//print_shader->setInt("occlusion", 4);
	//GLCall(glActiveTexture(GL_TEXTURE4));
	//GLCall(glBindTexture(GL_TEXTURE_2D, ssao_fb->ssaoColorBuffer));
	//
	//print_shader->setInt("fbpos", 5);
	//GLCall(glActiveTexture(GL_TEXTURE5));
	//GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, fb->pos));
	//
	//screen_quad.renderQuad();
	
	
	glutSwapBuffers();
}


//simple G-Buffer + SSAO working properly as in learnopengl.com
void renderScene(void) {
				
	//if (!g_arcball.locked)
	//{
	//	gView = glm::lookAt(eyePos, mesh->bb_mid, glm::vec3(0.0, 1.0, 0.0));
	//}
	//else
	//{
	//	gView = glm::lookAt(eyePos, look, up);
	//}			
	SSAO->use();
	SSAO->setInt("gPosition", 0);
	SSAO->setInt("gNormal", 1);
	SSAO->setInt("texNoise", 2);
	SSAO->setFloat("radius", radius);
	SSAO->setFloat("bias", bias);
	
	SSAO_blur->use();
	SSAO_blur->setInt("ssaoInput", 0);
	
	shader_lighting_pass->use();
	shader_lighting_pass->setInt("gPosition", 0);
	shader_lighting_pass->setInt("gNormal", 1);
	shader_lighting_pass->setInt("gAlbedo", 2);
	shader_lighting_pass->setInt("ssao", 3);
	

	glm::mat4  P  = gProj;
	//glm::mat4 V = glm::lookAt(position, position + front, up);
	glm::mat4  V = camera->GetViewMatrix();
	glm::mat4  M  = gModel;
	
	/*G BUFFER*/
	ssao_gb->bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	gbuffer->use();
	gbuffer->setMat4("P", P);
	gbuffer->setMat4("V", V);
	//glm::mat4 model = glm::mat4(1);
	//model = glm::translate(model, glm::vec3(0.0, 7.0f, 0.0f));
	//model = glm::scale(model, glm::vec3(5.0f));
	//gbuffer->setMat4("M", model);
	//gbuffer->setInt("invertedNormals", 1);
	//screen_cube.renderCube();
	//
	gbuffer->setInt("invertedNormals", 0);
	//
	//model = gModel;
	//model = glm::translate(model, glm::vec3(0.0f, 0.0f, 5.0));
	//model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	//model = glm::scale(model, glm::vec3(0.25f));
	gbuffer->setMat4("M", M);
	
	mesh->Draw(*gbuffer);
	
	ssao_gb->unbind();
	
	

	/*---------------------------------SSAO-------------------------------------*/
	
	ssao_fb->bind(FBO::SSAO);
	glClear(GL_COLOR_BUFFER_BIT);
	
	SSAO->use();
	SSAO->setFloat("radius", radius);
	SSAO->setFloat("bias", bias);
	for (unsigned int i = 0; i < num_samples; ++i)
	{
		std::string location = "samples[" + std::to_string(i) + "]";
		SSAO->setVec3(location, ssaoKernel[i]);
	}
	
	SSAO->setMat4("P", P);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ssao_gb->pos);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, ssao_gb->normal);
	
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	
	screen_quad.renderQuad();
	
	ssao_fb->unbind();
	/*--------------------------------------------------------------------------*/
	
	ssao_fb->bind(FBO::SSAO_BLUR);
	glClear(GL_COLOR_BUFFER_BIT);
	SSAO_blur->use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,ssao_fb->ssaoColorBuffer);
	screen_quad.renderQuad();
	
	ssao_fb->unbind();
	
	/*----------------------------------------------------------------------------*/
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	shader_lighting_pass->use();
	//shader_lighting_pass->setVec3("eyePos", eyePos);
	glm::vec3 lightPos = glm::vec3(camera->GetViewMatrix() * glm::vec4(mesh->bb_mid, 1.0)); //glm::vec4(2.0, 4.0, -2.0, 1.0)
	shader_lighting_pass->setVec3("light.Position", lightPos);
	shader_lighting_pass->setVec3("light.Color", glm::vec3(0.8, 0.8, 0.8)); //glm::vec3(0.2, 0.2, 0.7)
	shader_lighting_pass->setFloat("light.Linear", 0.09);
	shader_lighting_pass->setFloat("light.Quadratic", 0.032);
	shader_lighting_pass->setInt("AO", AO);
	shader_lighting_pass->setInt("remove_AO", remove_AO);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ssao_gb->pos);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, ssao_gb->normal);
	
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, ssao_gb->albedo);
	
	glActiveTexture(GL_TEXTURE3); // add extra SSAO texture to lighting pass
	glBindTexture(GL_TEXTURE_2D, ssao_fb->ssaoColorBufferBlur);	
	
	screen_quad.renderQuad();		
	
	glutSwapBuffers();
}

void GLInit(int *argc, char **argv)
{
	glutInit(argc, argv);
	glutInitContextVersion(4, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize((int)Wid, (int)Hei);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow("OpenGl");

	glClearColor(1.0, 1.0, 1.0, 1.0);

	glutDisplayFunc(renderTest);
	glutReshapeFunc(onResize);
	glutIdleFunc(onIdle);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMotion);
	glutKeyboardFunc(onKeyboard);
	glutSpecialFunc(onSpecialInput);

	GLenum err;
	if ((err = glewInit()) != GLEW_OK)
	{
		std::cout << "failed to init glew" << std::endl;
	}
	
	//glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

}

int main(int argc, char **argv)
{
	GLInit(&argc, argv);
	
	//Neste demo apenas sponza é necessario
	//link para download : https://casual-effects.com/g3d/data10/common/model/crytek_sponza/sponza.zip
	//const char* nanosuit_path = "src/models/nanosuit/nanosuit.obj";
	const char* sponza_path   = "src/models/sponza/sponza.obj"; 
	//const char* dragon_path   = "src/models/dragon/dragon.obj";
	
	mesh  = new mesh_loader(sponza_path);		
	
	
	//Nota para o professor: logo abaixo a tecnica convencional, apenas para comparacoes
	//for traditional ssao methods
	gbuffer = new Shader("src/shaders/gbuffer-ssao.vert", "src/shaders/gbuffer-ssao.frag", nullptr);
	shader_lighting_pass = new Shader("src/shaders/ssao.vert", "src/shaders/ssao_lighting_pass.frag", nullptr);
	SSAO = new Shader("src/shaders/ssao.vert", "src/shaders/ssao.frag");
	SSAO_blur = new Shader("src/shaders/ssao.vert", "src/shaders/ssao-blur.frag");
	ssao_gb = new ssao_gbuffer(Wid, Hei);


	//Nota para o professor: logo abaixo, tecnica de ssao com deep gbuffer, conforme artivo de mcguire
	/*--------------DEEP G BUFFER---------------------------------------*/
	fb = new framebuffer(Wid, Hei);
	deep_g_shader = new Shader("src/deep_g.vert", "src/deep_g.frag", "src/deep_g.geom");
	print_shader = new Shader("src/print.vert", "src/print.frag");
	deep_g_ssao = new Shader("src/SSAO.vert", "src/SSAO.frag");
	deep_g_blur = new Shader("src/SSAO.vert", "src/blur.frag");
	deep_g_light = new Shader("src/SSAO.vert", "src/deep_g_lightpass.frag");
	ssao_fb = new ssao_framebuffer(Wid, Hei);
	
	generateSampleKernel();
	generateNoise();

	//SSAO->use();
	//SSAO->setInt("gPosition", 0);
	//SSAO->setInt("gNormal", 1);
	//SSAO->setInt("texNoise", 2);
	//SSAO->setFloat("radius", radius);
	//SSAO->setFloat("bias", bias);	
	//
	//SSAO_blur->use();
	//SSAO_blur->setInt("ssaoInput", 0);
	//
	//shader_lighting_pass->use();
	//shader_lighting_pass->setInt("gPosition", 0);
	//shader_lighting_pass->setInt("gNormal", 1);
	//shader_lighting_pass->setInt("gAlbedo", 2);
	//shader_lighting_pass->setInt("ssao", 3);
	
	obj = mesh->bb_mid;
		
	eyePos = obj * glm::vec3(0.05f);
		
	gModel = glm::scale(gModel, glm::vec3(0.05f));
	//gModel = glm::translate(gModel, -mesh->bb_mid);

	camera = new Camera(eyePos, up);



	//debug purposes, check if kernel and noise are valid
	//for (int i = 0; i < ssaoKernel.size(); i++)
	//{
	//	std::cout << "kernel[" << i << "] :" << ssaoKernel[i].x << " " << ssaoKernel[i].y << " " << ssaoKernel[i].z << std::endl;
	//}
	//
	//for (int i = 0; i < ssaoNoise.size(); i++)
	//{
	//	std::cout << "Noise[" << i << "] :" << ssaoNoise[i].x << " " << ssaoNoise[i].y << " " << ssaoNoise[i].z << std::endl;
	//}
	
	glutMainLoop();	
	


	/*free memory*/
	delete mesh;	
	delete shader_lighting_pass;
	delete SSAO;
	delete SSAO_blur;
	delete gbuffer;
	delete fb;
	delete deep_g_shader;
	delete print_shader;
	delete ssao_fb;
	delete ssao_gb;
	delete camera;
	
	return 0;
}