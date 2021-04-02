#include "model.h"
#include "mesh_loader.h"
#include "GL_CALL.h"


Model::Model(std::vector<vertex> verts,
	         std::vector<unsigned int> inds,
	         std::vector<Texture> texts)
{
	this->vertices = verts;
	this->indices = inds;
	this->textures = texts;
			
	
	setupMesh();
}


Model::~Model()
{
}

void Model::setupMesh()
{

	GLCall(glGenVertexArrays(1, &vao));
	GLCall(glGenBuffers(1 , &vbo));
	GLCall(glGenBuffers(1 , &ibo));

	GLCall(glBindVertexArray(vao));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
	GLCall(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), &vertices[0] , GL_STATIC_DRAW  ));
	
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW ));

	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0));

	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, uv)));

	GLCall(glEnableVertexAttribArray(2));
	GLCall(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex,normal)));
	
	GLCall(glBindVertexArray(0));
	
}

void Model::Draw(Shader shader)
{
	unsigned int diffuseNr = 1;
	unsigned int specularNr = 1;
	unsigned int normalNr = 1;
	unsigned int heightNr = 1;

	for (int i = 0; i < textures.size(); i++)
	{
		GLCall(glActiveTexture(GL_TEXTURE0 + i));
		std::string number;
		std::string name = textures[i].type;
		if (name == "texture_diffuse")
			number = std::to_string(diffuseNr++);
		else if (name == "texture_specular")
			number = std::to_string(specularNr++);
		else if (name == "texture_normal")
			number = std::to_string(normalNr++); 
		else if (name == "texture_height")
			number = std::to_string(heightNr++); 

		//std::cout << name + number << std::endl;
		//std::cout << "path : " << textures[i].path << std::endl;

		glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
	}
	glActiveTexture(GL_TEXTURE0);	

	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

}