#include<iostream>
#include"Model.h"
#include<fstream>
#include<ctime>
#include"Profiler.h"

#define _CRT_SECURE_NO_WARNINGS

void Convert(const char* srcfile, const char* dstfile);
void Read(const char* modelfile);

int main(int argc,char* argv[])
{
	if (argc == 3)
	{
		char* tmpFBXPath = argv[1];
		char* tmpModelPath = argv[2];

		Convert(tmpFBXPath, tmpModelPath);
	}
	else
	{
		std::cout << "command like:Convert 1000000/1000000.FBX 1000000/1000000.model";
	}

	//读取我们自己的格式;
	//Profiler::BeginSample("Read");
	//Read("nanosuit/nanosuit.model");
	//Read("9000022/9000022.model");
	//Profiler::EndSample();

	//把fbx转换为我们自己的格式;
	//Profiler::BeginSample("Convert");
	////Convert("nanosuit/nanosuit.obj", "nanosuit/nanosuit.model");
	//Convert("1000000/1000000.FBX", "1000000/1000000.model");
	//Profiler::EndSample();


	getchar();

	return 0;
}

//转换model格式;
void Convert(const char* srcfile,const char* dstfile)
{
	ofstream fout(dstfile, ios::binary);

	cout << "Assimp Read " << srcfile << endl;

	Model model(srcfile);

	//写入mesh count;
	int meshcount = model.meshes.size();
	fout.write((char*)(&meshcount), sizeof(meshcount));

	cout << "MeshCount: " << meshcount << endl;

	for (size_t meshindex = 0; meshindex < model.meshes.size(); meshindex++)
	{
		cout << "Mesh" << meshindex << endl;

		Mesh mesh = model.meshes[meshindex];


		cout << "VertexCount:" << mesh.vertices.size() << endl;
		cout << "IndicesCount:" << mesh.indices.size() << endl;
		cout << "TextureCount:" << mesh.textures.size() << endl;

		int vertexsize =sizeof(Vertex) * mesh.vertices.size();

		int indicessize = sizeof(unsigned short)* mesh.indices.size();

		int texturesize = (sizeof(Texture))* mesh.textures.size();


		//写入vertexsize;
		fout.write((char*)(&vertexsize), sizeof(vertexsize));


		//写入vertex数据;
		for (size_t vertexindex = 0; vertexindex < mesh.vertices.size(); vertexindex++)
		{
			fout.write((char*)(&mesh.vertices[vertexindex]), sizeof(mesh.vertices[vertexindex]));
		}

		//写入indicessize;
		fout.write((char*)(&indicessize), sizeof(indicessize));

		

		//写入indicess数据;
		for (size_t indexindex = 0; indexindex < mesh.indices.size(); indexindex++)
		{
			fout.write((char*)(&mesh.indices[indexindex]), sizeof(mesh.indices[indexindex]));
		}

		//写入texturesize;
		fout.write((char*)(&texturesize), sizeof(texturesize));

		//写入texture数据;
		for (size_t textureindex = 0; textureindex < mesh.textures.size(); textureindex++)
		{
			fout.write((char*)(&mesh.textures[textureindex]), sizeof(mesh.textures[textureindex]));
		}
	}

	fout.close();

	cout << "Sucess" << endl;

}

//读取model格式;
void Read(const char* modelfile)
{
	Model model;

	
	ifstream fin(modelfile, ios::binary);
	

	Profiler::BeginSample("Read");

	//读取meshcount;
	int meshcount = 0;
	fin.read((char*)(&meshcount), sizeof(meshcount));

	for (int meshindex = 0; meshindex < meshcount; meshindex++)
	{
		Mesh mesh;

		//读取vertexsize;
		int vertexsize = 0;
		fin.read((char*)(&vertexsize), sizeof(vertexsize));

		//计算vertexcount;
		int vertexcount = vertexsize / sizeof(Vertex);

		//读取vertex数据;
		for (int vertexindex = 0; vertexindex < vertexcount; vertexindex++)
		{
			Vertex vertex;
			fin.read((char*)(&vertex), sizeof(vertex));

			mesh.vertices.push_back(vertex);
		}

		//读取indicessize;
		int indicessize = 0;
		fin.read((char*)(&indicessize), sizeof(indicessize));

		//计算indexcount;
		int indexcount = indicessize / sizeof(int);

		//读取index数据;
		for (int indexindex = 0; indexindex < indexcount; indexindex++)
		{
			int index = 0;
			fin.read((char*)(&index), sizeof(index));
			mesh.indices.push_back(index);
		}

		//读取texturesize;
		int texturesize = 0;
		fin.read((char*)(&texturesize), sizeof(texturesize));

		//计算 texturecount;
		int texturecount = texturesize / sizeof(Texture);

		//读取texture数据;
		for (int textureindex = 0; textureindex < texturecount; textureindex++)
		{
			Texture tex;
			fin.read((char*)(&tex), sizeof(tex));

			mesh.textures.push_back(tex);
		}

		model.meshes.push_back(mesh);
	}


	fin.close();

	Profiler::EndSample();
}