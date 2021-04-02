#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "model.h"
//#define SHOW_MSG 1

unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma);

class mesh_loader 
{
public:

	mesh_loader()
	{

	}

	mesh_loader(const char * path)
	{
		std::cout << "loading model at : " << path << std::endl;
		loadModel(path);
		bb_center();
		std::cout << "model loaded" << std::endl;


	}

	void Draw(Shader shader);

	void bb_center();
	glm::vec3 bb_mid;
private:
	
	std::vector<Model> models;
	std::vector<Texture> textures_loaded;
	std::string  directory;
	bool gammaCorrection;

	void loadModel(std::string path);
	void processNode(aiNode *node, const aiScene *scene);
	Model processModel(aiMesh *mesh, const aiScene *scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
	
	

};