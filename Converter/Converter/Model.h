#pragma once
#include<iostream>
#include<string>
#include<vector>

using namespace std;

#include"Mesh.h"


#include"assimp\Importer.hpp"
#include"assimp\scene.h"
#include"assimp\postprocess.h"

class Model
{
public:
	Model(){};


	/*  Functions   */
	Model(string path)
	{
		this->loadModel(path);
	}
	
	/*  Model Data  */
	vector<Mesh> meshes;
	string directory;

private:
	

	/*  Functions   */
	void loadModel(string path)
	{
		Assimp::Importer import;
		const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

		if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
			return;
		}

		this->directory = path.substr(0, path.find_last_of('/'));

		

		this->processNode(scene->mRootNode, scene);
	}

	void processNode(aiNode* node, const aiScene* scene)
	{
		// Process all the node's meshes (if any)
		for (int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			this->meshes.push_back(this->processMesh(mesh, scene));
		}
		// Then do the same for each of its children
		for (int i = 0; i < node->mNumChildren; i++)
		{
			this->processNode(node->mChildren[i], scene);
		}
	}

	Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		vector<Vertex> vertices;
		vector<unsigned short> indices;
		vector<Texture> textures;

		for (int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			// Process vertex positions, normals and texture coordinates
			glm::vec3 vector;
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;

			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.Normal = vector;


			if (mesh->mTextureCoords[0]) // Does the mesh contain texture coordinates? assimp允许最多8个纹理，我们只用到一个纹理;
			{
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
			}
			else
			{
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);
			}

			vertices.push_back(vertex);
		}
		// Process indices
		for (int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (int j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back((unsigned short)face.mIndices[j]);
			}
		}
		
		// Process material
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			vector<Texture> diffuseMaps = this->loadMaterialTextures(material,
				aiTextureType_DIFFUSE, TextureType::Texture_Diffuse);
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());


			vector<Texture> specularMaps = this->loadMaterialTextures(material,
				aiTextureType_SPECULAR, TextureType::Texture_Specular);
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		}

		return Mesh(vertices, indices, textures);
	}

	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, TextureType texturetype)
	{
		vector<Texture> textures;
		for (int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);

			Texture texture;
			texture.id = 0;// TextureFromFile(str.C_Str(), this->directory);
			texture.texturetype = texturetype;

			memcpy(texture.path, str.data,str.length+1);
			textures.push_back(texture);
		}
		return textures;
	}
};