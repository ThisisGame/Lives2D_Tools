#pragma once
#include<iostream>
#include<vector>
#include<string>

using namespace std;

#include"Vertex.h"
#include"Texture.h"


class Mesh 
{
public:
	/*  Mesh Data  */
	vector<Vertex> vertices;
	vector<unsigned short> indices;
	vector<Texture> textures;

	Mesh(){};

	/*  Functions  */
	Mesh(vector<Vertex> vertices, vector<unsigned short> indices, vector<Texture> textures)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;
	}

private:

	/*  Functions    */
	void setupMesh();
};