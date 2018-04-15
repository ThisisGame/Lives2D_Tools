#pragma once
#include "inode.h"
#include "3dsmaxsdk_preinclude.h"
#include <sceneapi.h>
#include <triobj.h>
#include <impapi.h>
#include <conio.h>  

class MeshEnumProc :
	public ITreeEnumProc
{
public:
	MeshEnumProc();
	MeshEnumProc(std::wstring varFileName,IScene* varScene,Interface* varIntercace,DWORD varOptions);
	~MeshEnumProc(void);

	int callback( INode *varNode );

	void export(INode *varNode,TriObject *varTriObject);



	std::wstring mFileName;
	FILE *mFile;
	IScene *mScene;
	Interface *mInterface;

	int mSelected;

	int mNum_Surfaces;
};

