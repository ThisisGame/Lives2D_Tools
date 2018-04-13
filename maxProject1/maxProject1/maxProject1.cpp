//**************************************************************************/
// Copyright (c) 1998-2007 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Appwizard generated plugin
// AUTHOR: 
//***************************************************************************/

#include "maxProject1.h"
#include <conio.h>  
#include"MeshEnumProc.h"


#define maxProject1_CLASS_ID	Class_ID(0x8d231fbc, 0xae14656a)

class maxProject1 : public SceneExport {
public:
	//Constructor/Destructor
	maxProject1();
	~maxProject1();

	int				ExtCount();					// Number of extensions supported
	const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
	const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	const TCHAR *	AuthorName();				// ASCII Author name
	const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	const TCHAR *	OtherMessage1();			// Other message #1
	const TCHAR *	OtherMessage2();			// Other message #2
	unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box

	BOOL SupportsOptions(int ext, DWORD options);
	int  DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0);
};



class maxProject1ClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 		{ return new maxProject1(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID SuperClassID() 				{ return SCENE_EXPORT_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return maxProject1_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("maxProject1"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* GetmaxProject1Desc() { 
	static maxProject1ClassDesc maxProject1Desc;
	return &maxProject1Desc; 
}





INT_PTR CALLBACK maxProject1OptionsDlgProc(HWND hWnd,UINT message,WPARAM,LPARAM lParam) {
	static maxProject1* imp = nullptr;

	switch(message) {
		case WM_INITDIALOG:
			imp = (maxProject1 *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));
			return TRUE;

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return 1;
	}
	return 0;
}


//--- maxProject1 -------------------------------------------------------
maxProject1::maxProject1()
{

}

maxProject1::~maxProject1() 
{

}

int maxProject1::ExtCount()
{
	//#pragma message(TODO("Returns the number of file name extensions supported by the plug-in."))
	return 1;
}

const TCHAR *maxProject1::Ext(int /*i*/)
{		
	//#pragma message(TODO("Return the 'i-th' file name extension (i.e. \"3DS\")."))
	return _T("model");
}

const TCHAR *maxProject1::LongDesc()
{
	//#pragma message(TODO("Return long ASCII description (i.e. \"Targa 2.0 Image File\")"))
	return _T("lives2d anim model file");
}
	
const TCHAR *maxProject1::ShortDesc() 
{			
	//#pragma message(TODO("Return short ASCII description (i.e. \"Targa\")"))
	return _T("lives2d anim model file");
}

const TCHAR *maxProject1::AuthorName()
{			
	//#pragma message(TODO("Return ASCII Author name"))
	return _T("");
}

const TCHAR *maxProject1::CopyrightMessage() 
{	
	//#pragma message(TODO("Return ASCII Copyright message"))
	return _T("");
}

const TCHAR *maxProject1::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *maxProject1::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int maxProject1::Version()
{				
	//#pragma message(TODO("Return Version number * 100 (i.e. v3.01 = 301)"))
	return 100;
}

void maxProject1::ShowAbout(HWND /*hWnd*/)
{			
	// Optional
}

BOOL maxProject1::SupportsOptions(int /*ext*/, DWORD /*options*/)
{
	//#pragma message(TODO("Decide which options to support.  Simply return true for each option supported by each Extension the exporter supports."))
	return TRUE;
}


//bool CreateExportPluginDialog(IGameScene* varGameScene,const string& varFileName);

int	maxProject1::DoExport(const TCHAR* name, ExpInterface* ei, Interface* ip, BOOL suppressPrompts, DWORD options)
{
	//#pragma message(TODO("Implement the actual file Export here and"))

	if(!suppressPrompts)
		DialogBoxParam(hInstance, 
				MAKEINTRESOURCE(IDD_PANEL), 
				GetActiveWindow(), 
				maxProject1OptionsDlgProc, (LPARAM)this);

	//#pragma message(TODO("return TRUE If the file is exported properly"))

	//AllocConsole();

	//_cprintf("Export Begin %s ",name);

	//MeshEnumProc tmpMeshEnumProc(name,ei->theScene,options);
	//const char* tmpName=(const char*)name;
	//std::wstring tmpName=std::wstring(name);
	//MeshEnumProc* tmpProc=new MeshEnumProc(tmpName,ei->theScene,ip,options);
	//ei->theScene->EnumTree(tmpProc);

	bool tmpSelected=(options & SCENE_EXPORT_SELECTED)?true:false;
	IGameScene* tmpGameScene=GetIGameInterface();

	IGameConversionManager* tmpGameConversionManager=GetConversionManager();
	tmpGameConversionManager->SetCoordSystem(IGameConversionManager::IGAME_OGL);
	
	tmpGameScene->InitialiseIGame(tmpSelected);

	tmpGameScene->SetStaticFrame(0);

	//窗口
	//CreateExportPluginDialog(tmpGameScene,name);


	vector<Vertex> tmpVectorVertex;
	vector<unsigned short> tmpVectorIndices;
	vector<unsigned short> tmpVectorIndicesSrc;//顶点索引，复用的顶点被拆分，所以在新的顶点数据里面，顶点索引是1-n自动生成的，这里要存储对应的真实的顶点索引，后面动画数据中会用到
	vector<glm::vec2> tmpVectorTexCoords;
	vector<Texture> tmpVectorTexture;
	int tmpTextureSize=0;

	vector<IGameNode*> tmpVectorGameNodeBones;
	vector<map<unsigned short,float>> tmpVectorWeight;

	//得到第一级
	int tmpSize=tmpGameScene->GetTopLevelNodeCount();
	for (int i=0;i<tmpSize;i++)
	{
		IGameNode* tmpGameNode=tmpGameScene->GetTopLevelNode(i);

		IGameObject* tmpGameObject=tmpGameNode->GetIGameObject();

		IGameObject::ObjectTypes tmpObjectType=tmpGameObject->GetIGameType();

		switch(tmpObjectType)
		{
		case IGameObject::IGAME_MESH:
			{
				IGameMesh* tmpGameMesh=(IGameMesh*)tmpGameObject;
				tmpGameMesh->GetMaxMesh()->buildNormals();

				if(!tmpGameMesh->InitializeData())
				{
					return FALSE;
				}

				int tmpFaceCount=tmpGameMesh->GetNumberOfFaces();

				for (int tmpFaceIndex=0;tmpFaceIndex<tmpFaceCount;tmpFaceIndex++)
				{
					FaceEx* tmpFaceEx=tmpGameMesh->GetFace(tmpFaceIndex);

					for (int tmpFaceVertexIndex=0;tmpFaceVertexIndex<3;tmpFaceVertexIndex++)
					{
						//顶点
						Vertex tmpVertex;

						//坐标
						int tmpVertexIndex=tmpFaceEx->vert[tmpFaceVertexIndex];
						Point3 tmpPoint3Position=tmpGameMesh->GetVertex(tmpVertexIndex);
						glm::vec3 tmpPosition(tmpPoint3Position.x,tmpPoint3Position.y,tmpPoint3Position.z);
						tmpVertex.Position=tmpPosition;

						//Normal
						int tmpNormalIndex=tmpFaceEx->norm[tmpFaceVertexIndex];
						Point3 tmpPoint3Normal=tmpGameMesh->GetNormal(tmpNormalIndex);
						glm::vec3 tmpNormal=glm::vec3(tmpPoint3Normal.x,tmpPoint3Normal.y,tmpPoint3Normal.z);
						tmpVertex.Normal=tmpNormal;

						//纹理坐标
						int tmpTexCoordIndex=tmpFaceEx->texCoord[tmpFaceVertexIndex];
						Point2 tmpPoint2TexVertex= tmpGameMesh->GetTexVertex(tmpTexCoordIndex);
						glm::vec2 tmpTexCoord=glm::vec2(tmpPoint2TexVertex.x,tmpPoint2TexVertex.y);
						tmpVertex.TexCoords=tmpTexCoord;

						tmpVectorVertex.push_back(tmpVertex);

						//存储新旧顶点索引关系
						tmpVectorIndicesSrc.push_back((unsigned short)tmpVertexIndex);
					}

					//获取当前面的材质
					IGameMaterial* tmpGameMaterial=tmpGameMesh->GetMaterialFromFace(tmpFaceEx);
					if(tmpGameMaterial==NULL)
					{
						continue;
					}
					else
					{

					}
				}

				//写入索引
				for (int tmpVetexIndex=0;tmpVetexIndex<tmpVectorVertex.size();tmpVetexIndex++)
				{
					tmpVectorIndices.push_back((short)tmpVetexIndex);
				}
			}
			break;
		default:
			break;
		}

		//判断有没有修改器，有修改器的就是骨骼动画
		int tmpModifiersNum=tmpGameObject->GetNumModifiers();
		for (int tmpModifierIndex=0;tmpModifierIndex<tmpModifiersNum;tmpModifierIndex++)
		{
			IGameModifier* tmpGameModifier=tmpGameObject->GetIGameModifier(tmpModifierIndex);

			//只处理骨骼动画修改器
			if(tmpGameModifier->IsSkin())
			{
				TimeValue tmpTimeValueBegin=tmpGameScene->GetSceneStartTime();
				TimeValue tmpTimeValueEnd=tmpGameScene->GetSceneEndTime();
				TimeValue tmpTimeValueTicks=tmpGameScene->GetSceneTicks();

				int tmpFrameCount=(tmpTimeValueEnd-tmpTimeValueBegin)/tmpTimeValueTicks;

				IGameSkin* tmpGameSkin=(IGameSkin*)tmpGameModifier;
				int tmpNumOfSkinnedVerts=tmpGameSkin->GetNumOfSkinnedVerts();

				//获取顶点受骨骼影响数
				for (int tmpVertexIndex=0;tmpVertexIndex<tmpVectorVertex.size();tmpVertexIndex++)
				{
					int tmpVertexIndexSrc=tmpVectorIndicesSrc[tmpVertexIndex];

					int tmpNumberOfBoneOnVertex=tmpGameSkin->GetNumberOfBones(tmpVertexIndexSrc);


					map<unsigned short,float> tmpMapWeightOneVertex;
					for (int tmpBoneIndexOnVertex=0;tmpBoneIndexOnVertex<tmpNumberOfBoneOnVertex;tmpBoneIndexOnVertex++)
					{
						//获取当前顶点的骨骼
						IGameNode* tmpGameNodeBone=tmpGameSkin->GetIGameBone(tmpVertexIndexSrc,tmpBoneIndexOnVertex);
						if(tmpGameNodeBone==nullptr)
						{
							continue;
						}
						bool tmpContais=false;
						int tmpGameNodeBoneIndex=0;
						for (tmpGameNodeBoneIndex=0;tmpGameNodeBoneIndex<tmpVectorGameNodeBones.size();tmpGameNodeBoneIndex++)
						{
							if (tmpVectorGameNodeBones[tmpGameNodeBoneIndex]==tmpGameNodeBone)
							{
								tmpContais=true;
								break;
							}
						}
						if(tmpContais==false)
						{
							tmpVectorGameNodeBones.push_back(tmpGameNodeBone);
							tmpGameNodeBoneIndex++;
						}
						

						float tmpWeight=tmpGameSkin->GetWeight(tmpVertexIndexSrc,tmpBoneIndexOnVertex);

						tmpMapWeightOneVertex.insert(pair<unsigned short,float>((unsigned short)tmpGameNodeBoneIndex,tmpWeight));
					}

					tmpVectorWeight.push_back(tmpMapWeightOneVertex);
				}
			}
		}

		//获取子节点
		int tmpChildNodeCount=tmpGameNode->GetChildCount();
		for (int tmpChildNodeIndex=0;tmpChildNodeIndex<tmpChildNodeCount;tmpChildNodeIndex++)
		{
			IGameNode* tmpGameNodeChild=tmpGameNode->GetNodeChild(tmpChildNodeIndex);
			if(tmpGameNodeChild->IsTarget())
			{
				continue;
			}
			
		}
	}



	//写文件
	ofstream fout(name, ios::binary);

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
			foutLog << "(" << tmpVectorVertex[vertexindex].Position.x << "," << tmpVectorVertex[vertexindex].Position.y << "," << tmpVectorVertex[vertexindex].Position.z << ")";
			
			//顶点权重信息
			map<unsigned short,float> tmpMapWeightOneVertex=tmpVectorWeight[vertexindex];
			for (map<unsigned short,float>::iterator tmpIterBegin=tmpMapWeightOneVertex.begin();tmpIterBegin!=tmpMapWeightOneVertex.end();tmpIterBegin++)
			{
				foutLog<<" "<<tmpIterBegin->first<<":"<<tmpIterBegin->second;
			}
			foutLog<<endl;
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
		for (size_t tmpIndicesIndex = 0; tmpIndicesIndex < tmpVectorIndices.size();)
		{
			int tmpIndex0=tmpIndicesIndex++;
			int tmpIndex1=tmpIndicesIndex++;
			int tmpIndex2=tmpIndicesIndex++;
			foutLog <<tmpVectorIndices[tmpIndex0] << "," << tmpVectorIndices[tmpIndex1] << "," << tmpVectorIndices[tmpIndex2] << endl;
		}


		//写入texturesize;
		fout.write((char*)(&texturesize), sizeof(texturesize));

		//写入texture数据;
		for (size_t textureindex = 0; textureindex < texturesize; textureindex++)
		{
			//fout.write((char*)(&mesh.textures[textureindex]), sizeof(mesh.textures[textureindex]));
		}


		//写入骨骼数据
		for (size_t tmpGameNodeBoneIndex=0;tmpGameNodeBoneIndex<tmpVectorGameNodeBones.size();tmpGameNodeBoneIndex++)
		{
			wstring tmpGameNodeBoneName=tmpVectorGameNodeBones[tmpGameNodeBoneIndex]->GetName();
			foutLog<<tmpGameNodeBoneName<<std::endl;
		}

	}

	fout.close();
	foutLog.close();

	tmpGameScene->ReleaseIGame();

	return TRUE;
}


