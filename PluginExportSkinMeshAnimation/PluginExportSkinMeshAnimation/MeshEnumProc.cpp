#include "MeshEnumProc.h"
#include "Vertex.h"
#include "Texture.h"
#include <vector>
#include<fstream>
#include<iostream>
#include<string>

MeshEnumProc::MeshEnumProc()
{
}

MeshEnumProc::MeshEnumProc(std::wstring varFileName,IScene* varScene,Interface* varInterface,DWORD varOptions)
{
	mFileName=varFileName;
	mScene=varScene;
	mInterface=varInterface;
}

int MeshEnumProc::callback(INode *varNode)
{
	if(mSelected && varNode->Selected()==FALSE)
	{
		return TREE_CONTINUE;
	}

	Object* tmpObject=varNode->EvalWorldState(mInterface->GetTime()).obj;
	if(tmpObject->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID,0)))
	{
		TriObject* tmpTriObject=(TriObject*)tmpObject->ConvertToType(mInterface->GetTime(),Class_ID(TRIOBJ_CLASS_ID,0));
		export(varNode,tmpTriObject);
	}

	return TREE_CONTINUE;
}

void MeshEnumProc::export(INode *varNode,TriObject *varTriObject)
{
	_cprintf("TRIOBJECT %s\n",varNode->GetName());

	Mtl* tmpMtl=varNode->GetMtl();
	if(tmpMtl)
	{
		_cprintf("Material %s\n",tmpMtl->GetName());
	}


	vector<unsigned short> tmpVectorIndices;
	vector<Vertex> tmpVectorVertex;
	int tmpTextureSize=0;

	if(varTriObject)
	{
		Mesh* tmpMesh=&varTriObject->GetMesh();

		if(tmpMesh)
		{
			tmpMesh->buildNormals();

			//获取索引
			int tmpFaceNum=tmpMesh->getNumFaces();
			for (int i=0;i<tmpFaceNum;i++)
			{
				Face tmpFace=tmpMesh->faces[i];

				unsigned short tmpVertexIndex0=tmpFace.v[0];
				unsigned short tmpVertexIndex1=tmpFace.v[1];
				unsigned short tmpVertexIndex2=tmpFace.v[2];

				tmpVectorIndices.push_back(tmpVertexIndex0);
				tmpVectorIndices.push_back(tmpVertexIndex1);
				tmpVectorIndices.push_back(tmpVertexIndex2);
			}

			//获取顶点
			int tmpVertexNum=tmpMesh->getNumVerts();
			for(int i=0;i<tmpVertexNum;i++)
			{
				Vertex tmpVertex;

				//顶点坐标
				Point3 tmpPoint3Position=tmpMesh->getVert(i);
				
				glm::vec3 tmpVec3Position(tmpPoint3Position.x,tmpPoint3Position.y,tmpPoint3Position.z);
				tmpVertex.Position=tmpVec3Position;

				//顶点法向量
				Point3 tmpPoint3Normal=tmpMesh->getNormal(i);
				glm::vec3 tmpVec3Normal(tmpPoint3Normal.x,tmpPoint3Normal.y,tmpPoint3Normal.z);
				tmpVertex.Normal=tmpVec3Normal;

				//纹理坐标
				Point3 tmpPoint3TexCoord=tmpMesh->tVerts[i];
				glm::vec2 tmpVec2TexCoord(tmpPoint3TexCoord.x,tmpPoint3TexCoord.y);
				tmpVertex.TexCoords=tmpVec2TexCoord;

				tmpVectorVertex.push_back(tmpVertex);
			}
		}

		//写文件
		ofstream fout(mFileName, ios::binary);

		ofstream foutLog("c://log.txt");

		//写入mesh count;
		int meshcount = 1;
		fout.write((char*)(&meshcount), sizeof(meshcount));

		std::cout << "MeshCount: " << meshcount << std::endl;
		foutLog<< "MeshCount: " << meshcount << endl;

		for (size_t meshindex = 0; meshindex < 1; meshindex++)
		{
			std::cout << "Mesh" << meshindex << std::endl;


			std::cout << "VertexCount:" << tmpVectorVertex.size() << std::endl;
			std::cout << "IndicesCount:" << tmpVectorIndices.size() << std::endl;
			std::cout << "TextureCount:" << tmpTextureSize << std::endl;

			foutLog << "VertexCount:" << tmpVectorVertex.size() << endl;
			foutLog << "IndicesCount:" << tmpVectorIndices.size() << endl;
			foutLog << "TextureCount:" <<tmpTextureSize << endl;

			int vertexsize =sizeof(Vertex) * tmpVectorVertex.size();

			int indicessize = sizeof(unsigned short)* tmpVectorIndices.size();

			int texturesize = (sizeof(Texture))* tmpTextureSize;


			//写入vertexsize;
			fout.write((char*)(&vertexsize), sizeof(vertexsize));


			//写入vertex数据;
			for (size_t vertexindex = 0; vertexindex < tmpVectorVertex.size(); vertexindex++)
			{
				fout.write((char*)(&tmpVectorVertex[vertexindex]), sizeof(tmpVectorVertex[vertexindex]));
			}

			foutLog << "Vertex:" << tmpVectorVertex.size()<< std::endl;
			for (size_t vertexindex = 0; vertexindex < tmpVectorVertex.size(); vertexindex++)
			{
				foutLog << "(" << tmpVectorVertex[vertexindex].Position.x << "," << tmpVectorVertex[vertexindex].Position.y << "," << tmpVectorVertex[vertexindex].Position.z << ")" << endl;
			}

			foutLog << "UV:" << tmpVectorVertex.size()<< std::endl;
			for (size_t vertexindex = 0; vertexindex < tmpVectorVertex.size(); vertexindex++)
			{
				foutLog << "(" << tmpVectorVertex[vertexindex].TexCoords.x << "," << tmpVectorVertex[vertexindex].TexCoords.y << ")" << endl;
			}

			foutLog << "Normal:" <<tmpVectorVertex.size()<< std::endl;
			for (size_t vertexindex = 0; vertexindex < tmpVectorVertex.size(); vertexindex++)
			{
				foutLog << "(" << tmpVectorVertex[vertexindex].Normal.x << "," << tmpVectorVertex[vertexindex].Normal.y << "," << tmpVectorVertex[vertexindex].Normal.z << ")" << endl;
			}


			//写入indicessize;
			fout.write((char*)(&indicessize), sizeof(indicessize));



			//写入indicess数据;
			for (size_t indexindex = 0; indexindex < tmpVectorIndices.size(); indexindex++)
			{
				fout.write((char*)(&tmpVectorIndices[indexindex]), sizeof(tmpVectorIndices[indexindex]));
			}


			foutLog << "Indices:" << tmpVectorIndices.size()<< std::endl;
			for (size_t indexindex = 0; indexindex < tmpVectorIndices.size();)
			{
				foutLog <<tmpVectorIndices[indexindex++] << "," << tmpVectorIndices[indexindex++] << "," << tmpVectorIndices[indexindex++] << endl;
			}


			//写入texturesize;
			fout.write((char*)(&texturesize), sizeof(texturesize));

			//写入texture数据;
			for (size_t textureindex = 0; textureindex < texturesize; textureindex++)
			{
				//fout.write((char*)(&mesh.textures[textureindex]), sizeof(mesh.textures[textureindex]));
			}
		}

		fout.close();
		foutLog.close();

		std::cout << "Sucess" << std::endl;
	}
	
}


MeshEnumProc::~MeshEnumProc(void)
{
}
