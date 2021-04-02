#pragma once

#include <iostream>
#include <gl/glew.h>
#include <GLM/glm.hpp>
#include <vector>
#include "shader.h"



struct vertex {
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 normal;
};

struct Texture {
	GLuint id;
	std::string type;
	std::string path;
};

class Model
{
public:
	friend class mesh_loader;

	Model(std::vector<vertex> verts,
	      std::vector<unsigned int> inds,
		  std::vector<Texture> texts);
	~Model();

	void Draw(Shader shader);
		
private:
	
	void setupMesh();

	GLuint vao;
	GLuint vbo;
	GLuint ibo;
		
	std::vector<vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;
	
};
