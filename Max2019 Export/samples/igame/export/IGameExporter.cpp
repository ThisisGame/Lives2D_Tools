/**********************************************************************
 *<
	FILE: IGameExporter.cpp

	DESCRIPTION:	Sample to test the IGame Interfaces.  It follows a similar format
					to the Ascii Exporter.  However is does diverge a little in order
					to handle properties etc..

	TODO: Break the file down into smaller chunks for easier loading.

	CREATED BY:		Neil Hazzard

	HISTORY:		parttime coding Summer 2002

 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/

#include "msxml2.h"
#include "IGameExporter.h"
#include "XMLUtility.h"
#include "decomp.h"

#include "IGame.h"
#include "IGameObject.h"
#include "IGameProperty.h"
#include "IGameControl.h"
#include "IGameModifier.h"
#include "IConversionManager.h"
#include "IGameError.h"

#include "3dsmaxport.h"

#include <conio.h>
#include <algorithm>
#include <vector>
#include<fstream>
#include<iostream>
#include<string>
#include<xstring>
#include<vector>
#include <map>
#include "Vertex.h"
#include "Texture.h"

#define IGAMEEXPORTER_CLASS_ID	Class_ID(0x79d613a4, 0x4f21c3ad)

#define BEZIER	0
#define TCB		1
#define LINEAR	2
#define SAMPLE	3

 //corresponds to XML schema
TCHAR* mapSlotNames[] = {
		_T("Diffuse"),
		_T("Ambient"),
		_T("Specular"),
		_T("SpecularLevel"),
		_T("Opacity"),
		_T("Glossiness"),
		_T("SelfIllumination"),
		_T("Filter"),
		_T("Bump"),
		_T("Reflection"),
		_T("Refraction"),
		_T("Displacement"),
		_T("Unknown") };



// XML helper class		
class CCoInitialize {
public:
	CCoInitialize() : m_hr(CoInitialize(NULL)) { }
	~CCoInitialize() { if (SUCCEEDED(m_hr)) CoUninitialize(); }
	operator HRESULT() const { return m_hr; }
	HRESULT m_hr;
};

class IGameExporter : public SceneExport {
public:

	IGameScene * pIgame;

	CCoInitialize init;              //must be declared before any IXMLDOM objects
	CComPtr<IXMLDOMDocument>  pXMLDoc;
	CComPtr<IXMLDOMNode> pRoot;		//this is our root node 	
	CComPtr<IXMLDOMNode> iGameNode;	//the IGame child - which is the main node
	CComPtr<IXMLDOMNode> rootNode;
	static HWND hParams;


	int curNode;

	int staticFrame;
	int framePerSample;
	BOOL exportGeom;
	BOOL exportNormals;
	BOOL exportVertexColor;
	BOOL exportControllers;
	BOOL exportFaceSmgp;
	BOOL exportTexCoords;
	BOOL exportMappingChannel;
	BOOL exportConstraints;
	BOOL exportMaterials;
	BOOL exportSplines;
	BOOL exportModifiers;
	BOOL exportSkin;
	BOOL exportGenMod;
	BOOL forceSample;
	BOOL splitFile;
	BOOL exportQuaternions;
	BOOL exportObjectSpace;
	BOOL exportRelative;
	BOOL exportNormalsPerFace;
	int cS;
	int exportCoord;
	bool showPrompts;
	bool exportSelected;

	TSTR fileName;
	TSTR splitPath;

	float igameVersion, exporterVersion;



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
	int	DoExport(const TCHAR *name, ExpInterface *ei, Interface *i, BOOL suppressPrompts = FALSE, DWORD options = 0);

	void ExportMesh(IGameMesh* varGameMesh, const wchar_t* varNodeName);

	void ExportNodeTraverse(IGameNode* tmpGameNode);

	IGameExporter();
	~IGameExporter();


	IGameScene* mGameScene;

	const TCHAR* mName;
};


class IGameExporterClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new IGameExporter(); }
	const TCHAR *	ClassName() { return GetString(IDS_CLASS_NAME); }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID() { return IGAMEEXPORTER_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("IGameExporter"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle

};



static IGameExporterClassDesc IGameExporterDesc;
ClassDesc2* GetIGameExporterDesc() { return &IGameExporterDesc; }

static bool IsFloatController(IGameControlType Type)
{
	if (Type == IGAME_FLOAT || Type == IGAME_EULER_X || Type == IGAME_EULER_Y ||
		Type == IGAME_EULER_Z || Type == IGAME_POS_X || Type == IGAME_POS_Y ||
		Type == IGAME_POS_Z)
		return true;

	return false;
}

int numVertex;

INT_PTR CALLBACK IGameExporterOptionsDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	IGameExporter *exp = DLGetWindowLongPtr<IGameExporter*>(hWnd);
	ISpinnerControl * spin;
	int ID;

	switch (message) {
	case WM_INITDIALOG:
		exp = (IGameExporter *)lParam;
		DLSetWindowLongPtr(hWnd, lParam);
		CenterWindow(hWnd, GetParent(hWnd));
		spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN));
		spin->LinkToEdit(GetDlgItem(hWnd, IDC_STATIC_FRAME), EDITTYPE_INT);
		spin->SetLimits(0, 100, TRUE);
		spin->SetScale(1.0f);
		spin->SetValue(exp->staticFrame, FALSE);
		ReleaseISpinner(spin);

		spin = GetISpinner(GetDlgItem(hWnd, IDC_SAMPLE_FRAME_SPIN));
		spin->LinkToEdit(GetDlgItem(hWnd, IDC_SAMPLE_FRAME), EDITTYPE_INT);
		spin->SetLimits(1, 100, TRUE);
		spin->SetScale(1.0f);
		spin->SetValue(exp->framePerSample, FALSE);
		ReleaseISpinner(spin);
		CheckDlgButton(hWnd, IDC_EXP_GEOMETRY, exp->exportGeom);
		CheckDlgButton(hWnd, IDC_EXP_NORMALS, exp->exportNormals);
		CheckDlgButton(hWnd, IDC_EXP_CONTROLLERS, exp->exportControllers);
		CheckDlgButton(hWnd, IDC_EXP_FACESMGRP, exp->exportFaceSmgp);
		CheckDlgButton(hWnd, IDC_EXP_VCOLORS, exp->exportVertexColor);
		CheckDlgButton(hWnd, IDC_EXP_TEXCOORD, exp->exportTexCoords);
		CheckDlgButton(hWnd, IDC_EXP_MAPCHAN, exp->exportMappingChannel);
		CheckDlgButton(hWnd, IDC_EXP_MATERIAL, exp->exportMaterials);
		CheckDlgButton(hWnd, IDC_EXP_SPLINES, exp->exportSplines);
		CheckDlgButton(hWnd, IDC_EXP_MODIFIERS, exp->exportModifiers);
		CheckDlgButton(hWnd, IDC_EXP_SAMPLECONT, exp->forceSample);
		CheckDlgButton(hWnd, IDC_EXP_CONSTRAINTS, exp->exportConstraints);
		CheckDlgButton(hWnd, IDC_EXP_SKIN, exp->exportSkin);
		CheckDlgButton(hWnd, IDC_EXP_GENMOD, exp->exportGenMod);
		CheckDlgButton(hWnd, IDC_SPLITFILE, exp->splitFile);
		CheckDlgButton(hWnd, IDC_EXP_OBJECTSPACE, exp->exportObjectSpace);
		CheckDlgButton(hWnd, IDC_EXP_QUATERNIONS, exp->exportQuaternions);
		CheckDlgButton(hWnd, IDC_EXP_RELATIVE, exp->exportRelative);


		ID = IDC_COORD_MAX + exp->cS;
		CheckRadioButton(hWnd, IDC_COORD_MAX, IDC_COORD_OGL, ID);

		ID = IDC_NORMALS_LIST + exp->exportNormalsPerFace;
		CheckRadioButton(hWnd, IDC_NORMALS_LIST, IDC_NORMALS_FACE, ID);

		EnableWindow(GetDlgItem(hWnd, IDC_EXP_NORMALS), exp->exportGeom);
		EnableWindow(GetDlgItem(hWnd, IDC_EXP_FACESMGRP), exp->exportGeom);
		EnableWindow(GetDlgItem(hWnd, IDC_EXP_VCOLORS), exp->exportGeom);
		EnableWindow(GetDlgItem(hWnd, IDC_EXP_TEXCOORD), exp->exportGeom);
		EnableWindow(GetDlgItem(hWnd, IDC_EXP_MAPCHAN), exp->exportGeom);
		EnableWindow(GetDlgItem(hWnd, IDC_EXP_OBJECTSPACE), exp->exportGeom);

		EnableWindow(GetDlgItem(hWnd, IDC_EXP_CONSTRAINTS), exp->exportControllers);
		EnableWindow(GetDlgItem(hWnd, IDC_EXP_SAMPLECONT), exp->exportControllers);
		EnableWindow(GetDlgItem(hWnd, IDC_EXP_QUATERNIONS), exp->exportControllers);
		EnableWindow(GetDlgItem(hWnd, IDC_EXP_RELATIVE), exp->exportControllers);

		EnableWindow(GetDlgItem(hWnd, IDC_EXP_SKIN), exp->exportModifiers);
		EnableWindow(GetDlgItem(hWnd, IDC_EXP_GENMOD), exp->exportModifiers);

		EnableWindow(GetDlgItem(hWnd, IDC_NORMALS_LIST), exp->exportNormals);
		EnableWindow(GetDlgItem(hWnd, IDC_NORMALS_FACE), exp->exportNormals);

		//Versioning
		TCHAR Title[256];
		_stprintf(Title, _T("IGame Exporter version %.3f; IGame version %.3f"),
			exp->exporterVersion, exp->igameVersion);
		SetWindowText(hWnd, Title);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDC_EXP_GEOMETRY:
			EnableWindow(GetDlgItem(hWnd, IDC_EXP_NORMALS), IsDlgButtonChecked(hWnd, IDC_EXP_GEOMETRY));
			EnableWindow(GetDlgItem(hWnd, IDC_EXP_FACESMGRP), IsDlgButtonChecked(hWnd, IDC_EXP_GEOMETRY));
			EnableWindow(GetDlgItem(hWnd, IDC_EXP_VCOLORS), IsDlgButtonChecked(hWnd, IDC_EXP_GEOMETRY));
			EnableWindow(GetDlgItem(hWnd, IDC_EXP_TEXCOORD), IsDlgButtonChecked(hWnd, IDC_EXP_GEOMETRY));
			EnableWindow(GetDlgItem(hWnd, IDC_EXP_MAPCHAN), IsDlgButtonChecked(hWnd, IDC_EXP_GEOMETRY));
			EnableWindow(GetDlgItem(hWnd, IDC_EXP_OBJECTSPACE), IsDlgButtonChecked(hWnd, IDC_EXP_GEOMETRY));
			break;
		case IDC_EXP_CONTROLLERS:
			EnableWindow(GetDlgItem(hWnd, IDC_EXP_CONSTRAINTS), IsDlgButtonChecked(hWnd, IDC_EXP_CONTROLLERS));
			EnableWindow(GetDlgItem(hWnd, IDC_EXP_SAMPLECONT), IsDlgButtonChecked(hWnd, IDC_EXP_CONTROLLERS));
			EnableWindow(GetDlgItem(hWnd, IDC_EXP_QUATERNIONS), IsDlgButtonChecked(hWnd, IDC_EXP_CONTROLLERS));
			EnableWindow(GetDlgItem(hWnd, IDC_EXP_RELATIVE), IsDlgButtonChecked(hWnd, IDC_EXP_CONTROLLERS));
			break;
		case IDC_EXP_MODIFIERS:
			EnableWindow(GetDlgItem(hWnd, IDC_EXP_SKIN), IsDlgButtonChecked(hWnd, IDC_EXP_MODIFIERS));
			EnableWindow(GetDlgItem(hWnd, IDC_EXP_GENMOD), IsDlgButtonChecked(hWnd, IDC_EXP_MODIFIERS));

			break;

		case IDC_EXP_NORMALS:
			if (exp->igameVersion >= 1.12)
			{
				EnableWindow(GetDlgItem(hWnd, IDC_NORMALS_FACE), IsDlgButtonChecked(hWnd, IDC_EXP_NORMALS));
			}
			EnableWindow(GetDlgItem(hWnd, IDC_NORMALS_LIST), IsDlgButtonChecked(hWnd, IDC_EXP_NORMALS));
			break;

		case IDOK:
			exp->exportGeom = IsDlgButtonChecked(hWnd, IDC_EXP_GEOMETRY);
			exp->exportNormals = IsDlgButtonChecked(hWnd, IDC_EXP_NORMALS);
			exp->exportControllers = IsDlgButtonChecked(hWnd, IDC_EXP_CONTROLLERS);
			exp->exportFaceSmgp = IsDlgButtonChecked(hWnd, IDC_EXP_FACESMGRP);
			exp->exportVertexColor = IsDlgButtonChecked(hWnd, IDC_EXP_VCOLORS);
			exp->exportTexCoords = IsDlgButtonChecked(hWnd, IDC_EXP_TEXCOORD);
			exp->exportMappingChannel = IsDlgButtonChecked(hWnd, IDC_EXP_MAPCHAN);
			exp->exportMaterials = IsDlgButtonChecked(hWnd, IDC_EXP_MATERIAL);
			exp->exportSplines = IsDlgButtonChecked(hWnd, IDC_EXP_SPLINES);
			exp->exportModifiers = IsDlgButtonChecked(hWnd, IDC_EXP_MODIFIERS);
			exp->forceSample = IsDlgButtonChecked(hWnd, IDC_EXP_SAMPLECONT);
			exp->exportConstraints = IsDlgButtonChecked(hWnd, IDC_EXP_CONSTRAINTS);
			exp->exportSkin = IsDlgButtonChecked(hWnd, IDC_EXP_SKIN);
			exp->exportGenMod = IsDlgButtonChecked(hWnd, IDC_EXP_GENMOD);
			exp->splitFile = IsDlgButtonChecked(hWnd, IDC_SPLITFILE);
			exp->exportQuaternions = IsDlgButtonChecked(hWnd, IDC_EXP_QUATERNIONS);
			exp->exportObjectSpace = IsDlgButtonChecked(hWnd, IDC_EXP_OBJECTSPACE);
			exp->exportRelative = IsDlgButtonChecked(hWnd, IDC_EXP_RELATIVE);
			if (IsDlgButtonChecked(hWnd, IDC_COORD_MAX))
				exp->cS = IGameConversionManager::IGAME_MAX;
			else if (IsDlgButtonChecked(hWnd, IDC_COORD_OGL))
				exp->cS = IGameConversionManager::IGAME_OGL;
			else
				exp->cS = IGameConversionManager::IGAME_D3D;


			exp->exportNormalsPerFace = (IsDlgButtonChecked(hWnd, IDC_NORMALS_LIST)) ? FALSE : TRUE;

			spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN));
			exp->staticFrame = spin->GetIVal();
			ReleaseISpinner(spin);
			spin = GetISpinner(GetDlgItem(hWnd, IDC_SAMPLE_FRAME_SPIN));
			exp->framePerSample = spin->GetIVal();
			ReleaseISpinner(spin);
			EndDialog(hWnd, 1);
			break;
		case IDCANCEL:
			EndDialog(hWnd, 0);
			break;
		}

	default:
		return FALSE;

	}
	return TRUE;

}

// Replace some characters we don't care for.
TCHAR *FixupName(TCHAR *buf)
{
	static TCHAR buffer[256];
	TCHAR* cPtr;

	_tcscpy(buffer, buf);
	cPtr = buffer;

	while (*cPtr) {
		if (*cPtr == _T('"')) *cPtr = 39;	// Replace double-quote with single quote.
		else if (*cPtr <= 31) *cPtr = _T('_');	// Replace control characters with underscore
		cPtr++;
	}

	return buffer;
}

//void stripWhiteSpace(TSTR * buf, TCHAR &newBuf)
//{
//
//	TCHAR newb[256]={""};
//	strcpy(newb,buf->data());
//
//	int len = strlen(newb);
//
//	int index = 0;
//
//	for(int i=0;i<len;i++)
//	{
//		if((newb[i] != ' ') && (!ispunct(newb[i])))
//			(&newBuf)[index++] = newb[i];
//	}
//}

//--- IGameExporter -------------------------------------------------------
IGameExporter::IGameExporter()
{

	staticFrame = 0;
	framePerSample = 4;
	exportGeom = TRUE;
	exportNormals = TRUE;
	exportVertexColor = FALSE;
	exportControllers = FALSE;
	exportFaceSmgp = FALSE;
	exportTexCoords = TRUE;
	exportMappingChannel = FALSE;
	exportMaterials = TRUE;
	exportConstraints = FALSE;
	exportSplines = FALSE;
	exportModifiers = FALSE;
	forceSample = FALSE;
	exportSkin = TRUE;
	exportGenMod = FALSE;
	cS = 0;	//max default
	rootNode = NULL;
	iGameNode = NULL;
	pRoot = NULL;
	pXMLDoc = NULL;
	splitFile = TRUE;
	exportQuaternions = TRUE;
	exportObjectSpace = FALSE;
	exportRelative = FALSE;
	exportNormalsPerFace = FALSE;
	exporterVersion = 2.0f;

}

IGameExporter::~IGameExporter()
{
	rootNode.Release();
	iGameNode.Release();
	pRoot.Release();
	pXMLDoc.Release();
}

int IGameExporter::ExtCount()
{
	//TODO: Returns the number of file name extensions supported by the plug-in.
	return 1;
}

const TCHAR *IGameExporter::Ext(int n)
{
	//TODO: Return the 'i-th' file name extension (i.e. "3DS").
	return _T("model");
}

const TCHAR *IGameExporter::LongDesc()
{
	//TODO: Return long ASCII description (i.e. "Targa 2.0 Image File")
	return _T("lives2d anim model file");
}

const TCHAR *IGameExporter::ShortDesc()
{
	//TODO: Return short ASCII description (i.e. "Targa")
	return _T("lives2d anim model file");
}

const TCHAR *IGameExporter::AuthorName()
{
	//TODO: Return ASCII Author name
	return _T("Neil Hazzard");
}

const TCHAR *IGameExporter::CopyrightMessage()
{
	// Return ASCII Copyright message
	return _T("");
}

const TCHAR *IGameExporter::OtherMessage1()
{
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *IGameExporter::OtherMessage2()
{
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int IGameExporter::Version()
{
	//TODO: Return Version number * 100 (i.e. v3.01 = 301)
	return exporterVersion * 100;
}

void IGameExporter::ShowAbout(HWND hWnd)
{
	// Optional
}

BOOL IGameExporter::SupportsOptions(int ext, DWORD options)
{
	// TODO Decide which options to support.  Simply return
	// true for each option supported by each Extension 
	// the exporter supports.

	return TRUE;
}



// Dummy function for progress bar
DWORD WINAPI fn(LPVOID arg)
{
	return(0);
}



class MyErrorProc : public IGameErrorCallBack
{
public:
	void ErrorProc(IGameError error)
	{
		const TCHAR * buf = GetLastIGameErrorText();
		DebugPrint(_T("ErrorCode = %d ErrorText = %s\n"), error, buf);
	}
};




std::string WChar2Ansi(LPCWSTR pwszSrc)
{
	int nLen = WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, NULL, 0, NULL, NULL);

	if (nLen <= 0) return std::string("");

	char* pszDst = new char[nLen];
	if (NULL == pszDst) return std::string("");

	WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, pszDst, nLen, NULL, NULL);
	pszDst[nLen - 1] = 0;

	std::string strTemp(pszDst);
	delete[] pszDst;

	return strTemp;
}

string ws2s(wstring& inputws) { return WChar2Ansi(inputws.c_str()); }

//Converting a Ansi string to WChar string  

std::wstring Ansi2WChar(LPCSTR pszSrc, int nLen)

{
	int nSize = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pszSrc, nLen, 0, 0);
	if (nSize <= 0) return NULL;

	WCHAR *pwszDst = new WCHAR[nSize + 1];
	if (NULL == pwszDst) return NULL;

	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pszSrc, nLen, pwszDst, nSize);
	pwszDst[nSize] = 0;

	if (pwszDst[0] == 0xFEFF) // skip Oxfeff  
		for (int i = 0; i < nSize; i++)
			pwszDst[i] = pwszDst[i + 1];

	wstring wcharString(pwszDst);
	delete pwszDst;

	return wcharString;
}

std::wstring s2ws(const string& s) { return Ansi2WChar(s.c_str(), s.size()); }

void IGameExporter::ExportMesh(IGameMesh* varGameMesh, const wchar_t* varNodeName)
{
	vector<Vertex> tmpVectorVertex;
	vector<int> tmpVectorIndices;
	vector<glm::vec2> tmpVectorTexCoords;
	vector<Texture> tmpVectorTexture;
	int tmpTextureSize = 0;

	vector<IGameNode*> tmpVectorGameNodeBones;
	vector<map<unsigned short, float>> tmpVectorWeight;

	vector<GMatrix> tmpVectorBoneGMatrixZeroFrame;
	vector<GMatrix> tmpVectorBoneGMatrixInvert;//存储第0帧逆矩阵
	map<TimeValue, vector<GMatrix>> tmpMapBoneGMatrix;//存储每一帧的矩阵

	map<IGameMaterial*, vector<int>> tmpMapMaterial;//存储材质与顶点的关系

	IGameMesh* tmpGameMesh = varGameMesh;
	tmpGameMesh->GetMaxMesh()->buildNormals();

	if (!tmpGameMesh->InitializeData())
	{
		return;
	}

	int tmpVertexCount = tmpGameMesh->GetNumberOfVerts();
	for (int tmpVertexIndex = 0; tmpVertexIndex < tmpVertexCount; tmpVertexIndex++)
	{
		Vertex tmpVertex;
		tmpVectorVertex.push_back(tmpVertex);
	}

	int tmpFaceCount = tmpGameMesh->GetNumberOfFaces();

	for (int tmpFaceIndex = 0; tmpFaceIndex < tmpFaceCount; tmpFaceIndex++)
	{
		FaceEx* tmpFaceEx = tmpGameMesh->GetFace(tmpFaceIndex);

		for (int tmpFaceVertexIndex = 0; tmpFaceVertexIndex < 3; tmpFaceVertexIndex++)
		{
			//顶点
			Vertex tmpVertex;

			//坐标
			int tmpVertexIndex = tmpFaceEx->vert[tmpFaceVertexIndex];
			Point3 tmpPoint3Position = tmpGameMesh->GetVertex(tmpVertexIndex);
			glm::vec3 tmpPosition(tmpPoint3Position.x, tmpPoint3Position.y, tmpPoint3Position.z);
			tmpVertex.Position = tmpPosition;

			//Normal
			int tmpNormalIndex = tmpFaceEx->norm[tmpFaceVertexIndex];
			Point3 tmpPoint3Normal = tmpGameMesh->GetNormal(tmpNormalIndex);
			glm::vec3 tmpNormal = glm::vec3(tmpPoint3Normal.x, tmpPoint3Normal.y, tmpPoint3Normal.z);
			tmpVertex.Normal = tmpNormal;

			//纹理坐标
			int tmpTexCoordIndex = tmpFaceEx->texCoord[tmpFaceVertexIndex];
			Point2 tmpPoint2TexVertex = tmpGameMesh->GetTexVertex(tmpTexCoordIndex);
			glm::vec2 tmpTexCoord = glm::vec2(tmpPoint2TexVertex.x, tmpPoint2TexVertex.y);
			tmpVertex.TexCoords = tmpTexCoord;


			tmpVectorIndices.push_back(tmpVertexIndex);

			tmpVectorVertex[tmpVertexIndex] = tmpVertex;
		}

		//获取当前面的材质
		IGameMaterial* tmpGameMaterial = tmpGameMesh->GetMaterialFromFace(tmpFaceEx);
		if (tmpGameMaterial == NULL)
		{
			continue;
		}
		else
		{

			bool tmpFind = false;
			for (std::map<IGameMaterial*, vector<int>>::iterator tmpIterBegin = tmpMapMaterial.begin(); tmpIterBegin != tmpMapMaterial.end(); tmpIterBegin++)
			{
				if (tmpIterBegin->first == tmpGameMaterial)
				{
					tmpFind = true;
					tmpIterBegin->second.push_back(tmpFaceEx->vert[0]);
					tmpIterBegin->second.push_back(tmpFaceEx->vert[1]);
					tmpIterBegin->second.push_back(tmpFaceEx->vert[2]);
					break;
				}
			}

			if (tmpFind == false)
			{
				vector<int> tmpVectorFaceIndex;
				tmpVectorFaceIndex.push_back(tmpFaceEx->vert[0]);
				tmpVectorFaceIndex.push_back(tmpFaceEx->vert[1]);
				tmpVectorFaceIndex.push_back(tmpFaceEx->vert[2]);
				tmpMapMaterial.insert(std::pair<IGameMaterial*, std::vector<int>>(tmpGameMaterial, tmpVectorFaceIndex));
			}
		}
	}

	//判断有没有修改器，有修改器的就是骨骼动画
	int tmpModifiersNum = tmpGameMesh->GetNumModifiers();
	for (int tmpModifierIndex = 0; tmpModifierIndex < tmpModifiersNum; tmpModifierIndex++)
	{
		IGameModifier* tmpGameModifier = tmpGameMesh->GetIGameModifier(tmpModifierIndex);

		//只处理骨骼动画修改器
		if (tmpGameModifier->IsSkin())
		{
			TimeValue tmpTimeValueBegin = mGameScene->GetSceneStartTime();
			TimeValue tmpTimeValueEnd = mGameScene->GetSceneEndTime();
			TimeValue tmpTimeValueTicks = mGameScene->GetSceneTicks();

			IGameSkin* tmpGameSkin = (IGameSkin*)tmpGameModifier;
			int tmpNumOfSkinnedVerts = tmpGameSkin->GetNumOfSkinnedVerts();

			//获取顶点受骨骼影响数
			for (int tmpVertexIndex = 0; tmpVertexIndex < tmpVectorVertex.size(); tmpVertexIndex++)
			{
				int tmpNumberOfBoneOnVertex = tmpGameSkin->GetNumberOfBones(tmpVertexIndex);


				map<unsigned short, float> tmpMapWeightOneVertex;
				for (int tmpBoneIndexOnVertex = 0; tmpBoneIndexOnVertex < tmpNumberOfBoneOnVertex; tmpBoneIndexOnVertex++)
				{
					//获取当前顶点的骨骼
					IGameNode* tmpGameNodeBone = tmpGameSkin->GetIGameBone(tmpVertexIndex, tmpBoneIndexOnVertex);
					if (tmpGameNodeBone == nullptr)
					{
						continue;
					}
					bool tmpContais = false;
					int tmpGameNodeBoneIndex = 0;
					for (tmpGameNodeBoneIndex = 0; tmpGameNodeBoneIndex < tmpVectorGameNodeBones.size(); tmpGameNodeBoneIndex++)
					{
						if (tmpVectorGameNodeBones[tmpGameNodeBoneIndex] == tmpGameNodeBone)
						{
							tmpContais = true;
							break;
						}
					}
					if (tmpContais == false)
					{
						tmpVectorGameNodeBones.push_back(tmpGameNodeBone);
					}


					float tmpWeight = tmpGameSkin->GetWeight(tmpVertexIndex, tmpBoneIndexOnVertex);

					tmpMapWeightOneVertex.insert(pair<unsigned short, float>((unsigned short)tmpGameNodeBoneIndex, tmpWeight));
				}

				tmpVectorWeight.push_back(tmpMapWeightOneVertex);
			}

			//获取第0帧骨骼逆矩阵
			for (int tmpGameNodeBoneIndex = 0; tmpGameNodeBoneIndex < tmpVectorGameNodeBones.size(); tmpGameNodeBoneIndex++)
			{
				INode* tmpNodeBone = tmpVectorGameNodeBones[tmpGameNodeBoneIndex]->GetMaxNode();
				Matrix3 tmpMatrix3NodeBone = tmpNodeBone->GetObjTMAfterWSM(0);

				tmpVectorBoneGMatrixZeroFrame.push_back(tmpMatrix3NodeBone);

				tmpMatrix3NodeBone.Invert();
				//GMatrix tmpGMatrixNodeBone(tmpMatrix3NodeBone);
				GMatrix tmpGMatrixNodeBoneInvert(tmpMatrix3NodeBone);

				tmpVectorBoneGMatrixInvert.push_back(tmpGMatrixNodeBoneInvert);

				//test
				//Matrix3 tmpTestMatrix3=tmpNodeBone->GetObjTMAfterWSM(0) * tmpMatrix3NodeBone;

				GMatrix tmpGMatrixNodeBoneTest(tmpMatrix3NodeBone);
				GMatrix tmpTest = tmpGMatrixNodeBoneTest*tmpGMatrixNodeBoneInvert;

				int a = 0;
			}

			//获取骨骼矩阵
			for (; tmpTimeValueBegin <= tmpTimeValueEnd; tmpTimeValueBegin += tmpTimeValueTicks)
			{
				vector<GMatrix> tmpVectorBoneGMatrix;
				for (int tmpGameNodeBoneIndex = 0; tmpGameNodeBoneIndex < tmpVectorGameNodeBones.size(); tmpGameNodeBoneIndex++)
				{
					INode* tmpNodeBone = tmpVectorGameNodeBones[tmpGameNodeBoneIndex]->GetMaxNode();
					Matrix3 tmpMatrix3NodeBone = tmpNodeBone->GetObjTMAfterWSM(tmpTimeValueBegin);
					GMatrix tmpGMatrixNodeBone(tmpMatrix3NodeBone);

					tmpVectorBoneGMatrix.push_back(tmpGMatrixNodeBone);
				}

				tmpMapBoneGMatrix.insert(pair<TimeValue, vector<GMatrix>>(tmpTimeValueBegin, tmpVectorBoneGMatrix));
			}

		}
	}


	//Export
	wstring tmpExportFullPath(mName);
	std::size_t tmpFind = tmpExportFullPath.find_last_of(L".");

	const wchar_t* tmpNodeName_w = varNodeName;
	wstring tmpNodeName_wstr(tmpNodeName_w);
	std::string tmpNodeName = ws2s(tmpNodeName_wstr);

	/*Split Mesh 65535*/
	vector<vector<int>> tmpVectorIndicesAllPart;
	vector<vector<unsigned short>> tmpVectorIndicesAllPartNew;

	//原始Indices 可能>65535
	int tmpIndicesWrite = 0;
	while (tmpIndicesWrite < tmpVectorIndices.size())
	{
		vector<int> tmpVectorIndicesWrite;
		while (tmpVectorIndicesWrite.size() < 65535)
		{
			tmpVectorIndicesWrite.push_back(tmpVectorIndices[tmpIndicesWrite]);

			tmpIndicesWrite++;

			if (tmpIndicesWrite >= tmpVectorIndices.size())
			{
				break;
			}
		}

		tmpVectorIndicesAllPart.push_back(tmpVectorIndicesWrite);
	}


	for (size_t tmpVectorIndicesPartIndex = 0; tmpVectorIndicesPartIndex < tmpVectorIndicesAllPart.size(); tmpVectorIndicesPartIndex++)
	{
		wstring tmpExportFullPath(mName);
		std::size_t tmpFind = tmpExportFullPath.find_last_of(L".");

		string tmpExportMeshPath;

		char tmpMeshFilePath[100];
		tmpExportMeshPath = ws2s(tmpExportFullPath.substr(0, tmpFind) + L"_%s_%d.mesh");


		sprintf(tmpMeshFilePath, tmpExportMeshPath.c_str(), tmpNodeName.c_str(), tmpVectorIndicesPartIndex);
		tmpExportMeshPath = tmpMeshFilePath;

		//写文件
		ofstream tmpOfStreamMesh(tmpExportMeshPath, ios::binary);

		vector<int>& tmpVectorIndicesOnePart = tmpVectorIndicesAllPart[tmpVectorIndicesPartIndex];

		vector<Vertex> tmpVectorVertexOnePart;
		vector<int> tmpVectorIndicesOnePartOnlyOne;

		std::map<int, unsigned short> tmpMapIndicesOldToNew;

		for (size_t i = 0; i < tmpVectorIndicesOnePart.size(); i++)
		{
			int tmpVertexIndex = tmpVectorIndicesOnePart[i];

			auto tmpIter = std::find(tmpVectorIndicesOnePartOnlyOne.begin(), tmpVectorIndicesOnePartOnlyOne.end(), tmpVertexIndex);
			if (tmpIter == tmpVectorIndicesOnePartOnlyOne.end())
			{
				tmpVectorIndicesOnePartOnlyOne.push_back(tmpVertexIndex);

				Vertex& tmpVertex = tmpVectorVertex[tmpVertexIndex];
				tmpVectorVertexOnePart.push_back(tmpVertex);

				tmpMapIndicesOldToNew[tmpVertexIndex]= tmpVectorVertexOnePart.size() - 1;
			}
		}

		vector<unsigned short> tmpVectorIndicesOnePartNew;
		for (size_t i = 0; i < tmpVectorIndicesOnePart.size(); i++)
		{
			int tmpVertexIndex = tmpVectorIndicesOnePart[i];

			auto tmpIter = tmpMapIndicesOldToNew.find(tmpVertexIndex);
			if (tmpIter != tmpMapIndicesOldToNew.end())
			{
				int tmpVertexIndexNew = tmpMapIndicesOldToNew[tmpVertexIndex];
				tmpVectorIndicesOnePartNew.push_back(tmpVertexIndexNew);
			}
		}

		//写入vertexsize;
		int tmpVectorVertexOnePartMemorySize = sizeof(Vertex) * tmpVectorVertexOnePart.size();
		tmpOfStreamMesh.write((char*)(&tmpVectorVertexOnePartMemorySize), sizeof(tmpVectorVertexOnePartMemorySize));

		//写入vertex数据;
		for (size_t vertexindex = 0; vertexindex < tmpVectorVertexOnePart.size(); vertexindex++)
		{
			tmpOfStreamMesh.write((char*)(&tmpVectorVertexOnePart[vertexindex]), sizeof(tmpVectorVertexOnePart[vertexindex]));
		}

		//写入indicessize;
		int tmpVectorIndicesOnePartMemorySize = sizeof(unsigned short)*tmpVectorIndicesOnePartNew.size();
		tmpOfStreamMesh.write((char*)(&tmpVectorIndicesOnePartMemorySize), sizeof(tmpVectorIndicesOnePartMemorySize));

		//写入indicess数据;
		for (size_t indexindex = 0; indexindex < tmpVectorIndicesOnePartNew.size(); indexindex++)
		{
			tmpOfStreamMesh.write((char*)(&tmpVectorIndicesOnePartNew[indexindex]), sizeof(tmpVectorIndicesOnePartNew[indexindex]));
		}

		int tmpMaterialCount = tmpMapMaterial.size();
		tmpOfStreamMesh.write((char*)(&tmpMaterialCount), sizeof(tmpMaterialCount));

		for (std::map<IGameMaterial*, vector<int>>::iterator tmpIterBegin = tmpMapMaterial.begin(); tmpIterBegin != tmpMapMaterial.end(); tmpIterBegin++)
		{
			IGameMaterial* tmpGameMaterial = tmpIterBegin->first;
			string tmpMaterialName = WChar2Ansi(tmpGameMaterial->GetMaterialName());

			unsigned char tmpMaterialNameSize = tmpMaterialName.size() + 1;
			tmpOfStreamMesh.write((char*)(&tmpMaterialNameSize), sizeof(tmpMaterialNameSize));
			tmpOfStreamMesh.write((char*)(tmpMaterialName.c_str()), tmpMaterialNameSize);

			unsigned char tmpNumberOfTextureMaps = tmpGameMaterial->GetNumberOfTextureMaps();		//how many texture of the material
			tmpOfStreamMesh.write((char*)(&tmpNumberOfTextureMaps), sizeof(tmpNumberOfTextureMaps));

			for (int tmpTextureMapIndex = 0; tmpTextureMapIndex < tmpNumberOfTextureMaps; tmpTextureMapIndex++)
			{
				IGameTextureMap* tmpGameTextureMap = tmpGameMaterial->GetIGameTextureMap(tmpTextureMapIndex);
				if (tmpGameTextureMap != NULL)
				{
					//文件路径						
					string tmpBitmapPath = WChar2Ansi(tmpGameTextureMap->GetBitmapFileName());

					//文件名
					int tmpLastCharPosition = tmpBitmapPath.find_last_of('\\');
					string tmpBitmapName(tmpBitmapPath.substr(tmpLastCharPosition + 1));
					unsigned char tmpBitmapNameSize = tmpBitmapName.size() + 1;
					tmpOfStreamMesh.write((char*)(&tmpBitmapNameSize), sizeof(tmpBitmapNameSize));
					tmpOfStreamMesh.write((char*)(tmpBitmapName.c_str()), tmpBitmapNameSize);

					//获取UV的Tilling和Offset值
					IGameUVGen* tmpGameUVGen = tmpGameTextureMap->GetIGameUVGen();
					std::string tmpTextureClass = WChar2Ansi(tmpGameTextureMap->GetTextureClass());
					transform(tmpTextureClass.begin(), tmpTextureClass.end(), tmpTextureClass.begin(), toupper);

					if (strcmp(tmpTextureClass.c_str(), "BITMAP") != 0)
					{
						continue;
					}

					IGameProperty* tmpGamePropertyUTiling = tmpGameUVGen->GetUTilingData();
					float tmpUTilingValue = 0.0f;
					if (tmpGamePropertyUTiling->GetPropertyValue(tmpUTilingValue))
					{
						tmpOfStreamMesh.write((char*)(&tmpUTilingValue), sizeof(tmpUTilingValue));
					}

					IGameProperty* tmpGamePropertyVTiling = tmpGameUVGen->GetVTilingData();
					float tmpVTilingValue = 0.0f;
					if (tmpGamePropertyVTiling->GetPropertyValue(tmpVTilingValue))
					{
						tmpOfStreamMesh.write((char*)(&tmpVTilingValue), sizeof(tmpVTilingValue));
					}

					IGameProperty* tmpGamePropertyUOffset = tmpGameUVGen->GetUOffsetData();
					float tmpUOffsetValue = 0.0f;
					if (tmpGamePropertyUOffset->GetPropertyValue(tmpUOffsetValue))
					{
						tmpOfStreamMesh.write((char*)(&tmpUOffsetValue), sizeof(tmpUOffsetValue));
					}

					IGameProperty* tmpGamePropertyVOffset = tmpGameUVGen->GetVOffsetData();
					float tmpVOffsetValue = 0.0f;
					if (tmpGamePropertyVOffset->GetPropertyValue(tmpVOffsetValue))
					{
						tmpOfStreamMesh.write((char*)(&tmpVOffsetValue), sizeof(tmpVOffsetValue));
					}
				}
			}
		}


		/*Animation*/
		if (tmpVectorGameNodeBones.size() == 0)
		{
			return;
		}
		string tmpExportAnimPath;

		char tmpAnimFilePath[100];
		tmpExportAnimPath = ws2s(tmpExportFullPath.substr(0, tmpFind) + L"_%s_%d.anim");


		sprintf(tmpAnimFilePath, tmpExportAnimPath.c_str(), tmpNodeName.c_str(), tmpVectorIndicesPartIndex);
		tmpExportAnimPath = tmpAnimFilePath;

		//写文件
		ofstream tmpOfStreamAnim(tmpExportAnimPath, ios::binary);

		//写入骨骼数据
		TimeValue tmpTimeValueBegin = mGameScene->GetSceneStartTime();
		TimeValue tmpTimeValueEnd = mGameScene->GetSceneEndTime();
		TimeValue tmpTimeValueTicks = mGameScene->GetSceneTicks();
		int tmpFrameCount = (tmpTimeValueEnd - tmpTimeValueBegin) / tmpTimeValueTicks;
		tmpFrameCount = tmpFrameCount + 1;
		tmpOfStreamAnim.write((char*)(&tmpFrameCount), sizeof(tmpFrameCount));

		tmpOfStreamAnim.write((char*)(&tmpTimeValueTicks), sizeof(tmpTimeValueTicks));


		int tmpGameNodeBoneSize = tmpVectorGameNodeBones.size();
		tmpOfStreamAnim.write((char*)(&tmpGameNodeBoneSize), sizeof(tmpGameNodeBoneSize));

		for (size_t tmpGameNodeBoneIndex = 0; tmpGameNodeBoneIndex < tmpVectorGameNodeBones.size(); tmpGameNodeBoneIndex++)
		{
			const wchar_t* tmpGameNodeBoneName = tmpVectorGameNodeBones[tmpGameNodeBoneIndex]->GetName();
			std::wstring tmpGameNodeBoneNameWString(tmpGameNodeBoneName);
			std::string tmpGameNodeBoneNameString = ws2s(tmpGameNodeBoneNameWString);


			int tmpGameNodeBoneNameStringSize = tmpGameNodeBoneNameString.size() + 1;
			tmpOfStreamAnim.write((char*)(&tmpGameNodeBoneNameStringSize), sizeof(tmpGameNodeBoneNameStringSize));
			tmpOfStreamAnim.write(tmpGameNodeBoneNameString.c_str(), tmpGameNodeBoneNameStringSize);
		}


		//写入第0帧逆矩阵
		int tmpVectorBoneGMatrixInvertSize = tmpVectorBoneGMatrixInvert.size();
		tmpOfStreamAnim.write((char*)(&tmpVectorBoneGMatrixInvertSize), sizeof(tmpVectorBoneGMatrixInvertSize));


		for (size_t tmpBoneGMatrixInvertIndex = 0; tmpBoneGMatrixInvertIndex < tmpVectorBoneGMatrixInvert.size(); tmpBoneGMatrixInvertIndex++)
		{

			GMatrix tmpBoneGMatrixInvert = tmpVectorBoneGMatrixInvert[tmpBoneGMatrixInvertIndex];


			glm::mat4x4 tmpMat4x4BoneGMatrixInvert;
			tmpMat4x4BoneGMatrixInvert[0][0] = tmpBoneGMatrixInvert[0][0]; tmpMat4x4BoneGMatrixInvert[0][1] = tmpBoneGMatrixInvert[0][1]; tmpMat4x4BoneGMatrixInvert[0][2] = tmpBoneGMatrixInvert[0][2]; tmpMat4x4BoneGMatrixInvert[0][3] = tmpBoneGMatrixInvert[0][3];
			tmpMat4x4BoneGMatrixInvert[1][0] = tmpBoneGMatrixInvert[1][0]; tmpMat4x4BoneGMatrixInvert[1][1] = tmpBoneGMatrixInvert[1][1]; tmpMat4x4BoneGMatrixInvert[1][2] = tmpBoneGMatrixInvert[1][2]; tmpMat4x4BoneGMatrixInvert[1][3] = tmpBoneGMatrixInvert[1][3];
			tmpMat4x4BoneGMatrixInvert[2][0] = tmpBoneGMatrixInvert[2][0]; tmpMat4x4BoneGMatrixInvert[2][1] = tmpBoneGMatrixInvert[2][1]; tmpMat4x4BoneGMatrixInvert[2][2] = tmpBoneGMatrixInvert[2][2]; tmpMat4x4BoneGMatrixInvert[2][3] = tmpBoneGMatrixInvert[2][3];
			tmpMat4x4BoneGMatrixInvert[3][0] = tmpBoneGMatrixInvert[3][0]; tmpMat4x4BoneGMatrixInvert[3][1] = tmpBoneGMatrixInvert[3][1]; tmpMat4x4BoneGMatrixInvert[3][2] = tmpBoneGMatrixInvert[3][2]; tmpMat4x4BoneGMatrixInvert[3][3] = tmpBoneGMatrixInvert[3][3];

			tmpOfStreamAnim.write((char*)(&tmpMat4x4BoneGMatrixInvert), sizeof(tmpMat4x4BoneGMatrixInvert));
		}

		//写入骨骼时间轴矩阵
		int tmpMapBoneGMatrixSize = tmpMapBoneGMatrix.size();
		tmpOfStreamAnim.write((char*)(&tmpMapBoneGMatrixSize), sizeof(tmpMapBoneGMatrixSize));

		for (map<TimeValue, vector<GMatrix>>::iterator tmpIterBegin = tmpMapBoneGMatrix.begin(); tmpIterBegin != tmpMapBoneGMatrix.end(); tmpIterBegin++)
		{
			TimeValue tmpTimeValueCurrent = tmpIterBegin->first;

			tmpOfStreamAnim.write((char*)(&tmpTimeValueCurrent), sizeof(tmpTimeValueCurrent));

			vector<GMatrix> tmpVectorGMatrixCurrent = tmpIterBegin->second;

			int tmpVectorGMatrixCurrentSize = tmpVectorGMatrixCurrent.size();
			tmpOfStreamAnim.write((char*)(&tmpVectorGMatrixCurrentSize), sizeof(tmpVectorGMatrixCurrentSize));

			for (size_t tmpVectorGMatrixCurrentIndex = 0; tmpVectorGMatrixCurrentIndex < tmpVectorGMatrixCurrent.size(); tmpVectorGMatrixCurrentIndex++)
			{

				GMatrix tmpGMatrixNodeBone = tmpVectorGMatrixCurrent[tmpVectorGMatrixCurrentIndex];

				glm::mat4x4 tmpMat4x4BoneGMatrix;
				tmpMat4x4BoneGMatrix[0][0] = tmpGMatrixNodeBone[0][0]; tmpMat4x4BoneGMatrix[0][1] = tmpGMatrixNodeBone[0][1]; tmpMat4x4BoneGMatrix[0][2] = tmpGMatrixNodeBone[0][2]; tmpMat4x4BoneGMatrix[0][3] = tmpGMatrixNodeBone[0][3];
				tmpMat4x4BoneGMatrix[1][0] = tmpGMatrixNodeBone[1][0]; tmpMat4x4BoneGMatrix[1][1] = tmpGMatrixNodeBone[1][1]; tmpMat4x4BoneGMatrix[1][2] = tmpGMatrixNodeBone[1][2]; tmpMat4x4BoneGMatrix[1][3] = tmpGMatrixNodeBone[1][3];
				tmpMat4x4BoneGMatrix[2][0] = tmpGMatrixNodeBone[2][0]; tmpMat4x4BoneGMatrix[2][1] = tmpGMatrixNodeBone[2][1]; tmpMat4x4BoneGMatrix[2][2] = tmpGMatrixNodeBone[2][2]; tmpMat4x4BoneGMatrix[2][3] = tmpGMatrixNodeBone[2][3];
				tmpMat4x4BoneGMatrix[3][0] = tmpGMatrixNodeBone[3][0]; tmpMat4x4BoneGMatrix[3][1] = tmpGMatrixNodeBone[3][1]; tmpMat4x4BoneGMatrix[3][2] = tmpGMatrixNodeBone[3][2]; tmpMat4x4BoneGMatrix[3][3] = tmpGMatrixNodeBone[3][3];

				tmpOfStreamAnim.write((char*)(&tmpMat4x4BoneGMatrix), sizeof(tmpMat4x4BoneGMatrix));
			}
		}

		//顶点权重信息
		vector<map<unsigned short, float>> tmpVectorWeightNew;
		for (size_t tmpVertexIndex = 0; tmpVertexIndex < tmpVectorWeight.size(); tmpVertexIndex++)
		{
			auto tmpIter = tmpMapIndicesOldToNew.find(tmpVertexIndex);
			if (tmpIter != tmpMapIndicesOldToNew.end())
			{
				map<unsigned short, float> tmpMapVertexWeight;
				tmpVectorWeightNew.push_back(tmpMapVertexWeight);
			}
		}

		for (size_t tmpVertexIndex = 0; tmpVertexIndex < tmpVectorWeight.size(); tmpVertexIndex++)
		{
			auto tmpIter = tmpMapIndicesOldToNew.find(tmpVertexIndex);
			if (tmpIter != tmpMapIndicesOldToNew.end())
			{
				int tmpVertexIndexNew = tmpMapIndicesOldToNew[tmpVertexIndex];

				map<unsigned short, float> tmpMapWeightOneVertex = tmpVectorWeight[tmpVertexIndex];
				tmpVectorWeightNew[tmpVertexIndexNew]=tmpMapWeightOneVertex;
			}
		}

		int tmpVectorVertexSize = tmpVectorWeightNew.size();
		tmpOfStreamAnim.write((char*)(&tmpVectorVertexSize), sizeof(tmpVectorVertexSize));

		for (size_t tmpVertexIndex = 0; tmpVertexIndex < tmpVectorWeightNew.size(); tmpVertexIndex++)
		{
			
			map<unsigned short, float> tmpMapWeightOneVertex = tmpVectorWeightNew[tmpVertexIndex];

			int tmpMapWeightOneVertexSize = tmpMapWeightOneVertex.size();
			tmpOfStreamAnim.write((char*)(&tmpMapWeightOneVertexSize), sizeof(tmpMapWeightOneVertexSize));

			for (map<unsigned short, float>::iterator tmpIterBegin = tmpMapWeightOneVertex.begin(); tmpIterBegin != tmpMapWeightOneVertex.end(); tmpIterBegin++)
			{
				tmpOfStreamAnim.write((char*)(&tmpIterBegin->first), sizeof(tmpIterBegin->first));
				tmpOfStreamAnim.write((char*)(&tmpIterBegin->second), sizeof(tmpIterBegin->second));
			}
		}


		//计算顶点初始位置 并存储
		for (size_t tmpVertexIndex = 0; tmpVertexIndex < tmpVectorWeightNew.size(); tmpVertexIndex++)
		{
			map<unsigned short, float> tmpMapWeightOneVertex = tmpVectorWeightNew[tmpVertexIndex];

			//写入当前顶点受影响的骨骼数
			unsigned short tmpMapWeightOneVertexSize = tmpMapWeightOneVertex.size();
			tmpOfStreamAnim.write((char*)(&tmpMapWeightOneVertexSize), sizeof(tmpMapWeightOneVertexSize));

			std::vector<glm::vec3> tmpVectorOneVertexPositionNoBone;
			for (map<unsigned short, float>::iterator tmpIterBegin = tmpMapWeightOneVertex.begin(); tmpIterBegin != tmpMapWeightOneVertex.end(); tmpIterBegin++)
			{
				GMatrix tmpBoneGMatrixInvert = tmpVectorBoneGMatrixInvert[tmpIterBegin->first];

				glm::mat4x4 tmpMat4x4BoneGMatrixInvert;
				tmpMat4x4BoneGMatrixInvert[0][0] = tmpBoneGMatrixInvert[0][0]; tmpMat4x4BoneGMatrixInvert[0][1] = tmpBoneGMatrixInvert[0][1]; tmpMat4x4BoneGMatrixInvert[0][2] = tmpBoneGMatrixInvert[0][2]; tmpMat4x4BoneGMatrixInvert[0][3] = tmpBoneGMatrixInvert[0][3];
				tmpMat4x4BoneGMatrixInvert[1][0] = tmpBoneGMatrixInvert[1][0]; tmpMat4x4BoneGMatrixInvert[1][1] = tmpBoneGMatrixInvert[1][1]; tmpMat4x4BoneGMatrixInvert[1][2] = tmpBoneGMatrixInvert[1][2]; tmpMat4x4BoneGMatrixInvert[1][3] = tmpBoneGMatrixInvert[1][3];
				tmpMat4x4BoneGMatrixInvert[2][0] = tmpBoneGMatrixInvert[2][0]; tmpMat4x4BoneGMatrixInvert[2][1] = tmpBoneGMatrixInvert[2][1]; tmpMat4x4BoneGMatrixInvert[2][2] = tmpBoneGMatrixInvert[2][2]; tmpMat4x4BoneGMatrixInvert[2][3] = tmpBoneGMatrixInvert[2][3];
				tmpMat4x4BoneGMatrixInvert[3][0] = tmpBoneGMatrixInvert[3][0]; tmpMat4x4BoneGMatrixInvert[3][1] = tmpBoneGMatrixInvert[3][1]; tmpMat4x4BoneGMatrixInvert[3][2] = tmpBoneGMatrixInvert[3][2]; tmpMat4x4BoneGMatrixInvert[3][3] = tmpBoneGMatrixInvert[3][3];

				glm::vec3& tmpVec3PositionZeroFrame = tmpVectorVertexOnePart[tmpVertexIndex].Position;

				glm::vec4 tmpVec4PositionZeroFrame;
				tmpVec4PositionZeroFrame.x = tmpVec3PositionZeroFrame.x;
				tmpVec4PositionZeroFrame.y = -tmpVec3PositionZeroFrame.z;
				tmpVec4PositionZeroFrame.z = tmpVec3PositionZeroFrame.y;
				tmpVec4PositionZeroFrame.w = 1;

				glm::vec4 tmpPositionNoBone = tmpMat4x4BoneGMatrixInvert * tmpVec4PositionZeroFrame;

				tmpOfStreamAnim.write((char*)(&tmpPositionNoBone), sizeof(tmpPositionNoBone));
			}
		}
		tmpOfStreamAnim.close();
	}

	/*Material*/
	char tmpMaterialFilePath[100];
	sprintf(tmpMaterialFilePath, ws2s(tmpExportFullPath.substr(0, tmpFind) + L"_%s.material").c_str(), tmpNodeName.c_str());
	ofstream foutMaterial(tmpMaterialFilePath);

	foutMaterial << "Materials:" << endl;
	for (std::map<IGameMaterial*, vector<int>>::iterator tmpIterBegin = tmpMapMaterial.begin(); tmpIterBegin != tmpMapMaterial.end(); tmpIterBegin++)
	{
		IGameMaterial* tmpGameMaterial = tmpIterBegin->first;
		string tmpMaterialName = WChar2Ansi(tmpGameMaterial->GetMaterialName());

		foutMaterial << tmpMaterialName << endl;

		unsigned char tmpNumberOfTextureMaps = tmpGameMaterial->GetNumberOfTextureMaps();		//how many texture of the material
		foutMaterial << "Texture Count:" << (int)tmpNumberOfTextureMaps << endl;

		for (int tmpTextureMapIndex = 0; tmpTextureMapIndex < tmpNumberOfTextureMaps; tmpTextureMapIndex++)
		{
			IGameTextureMap* tmpGameTextureMap = tmpGameMaterial->GetIGameTextureMap(tmpTextureMapIndex);
			if (tmpGameTextureMap != NULL)
			{
				//文件路径						
				string tmpBitmapPath = WChar2Ansi(tmpGameTextureMap->GetBitmapFileName());
				foutMaterial << "Texture BitmapPath:" << tmpBitmapPath << endl;

				//拷贝图片到导出目录
				wstring tmpBitmapPathW = tmpGameTextureMap->GetBitmapFileName();
				wstring tmpBitmapNameW = tmpBitmapPathW.substr(tmpBitmapPathW.find_last_of('\\') + 1);
				wstring tmpBitmapExportPathW = tmpExportFullPath.substr(0, tmpExportFullPath.find_last_of(L"\\") + 1) + tmpBitmapNameW;
				CopyFile(tmpGameTextureMap->GetBitmapFileName(), tmpBitmapExportPathW.c_str(), FALSE);
			}
		}
	}
	foutMaterial.close();
}

void IGameExporter::ExportNodeTraverse(IGameNode* varGameNode)
{
	const wchar_t* tmpNodeName = varGameNode->GetName();

	IGameObject* tmpGameObject = varGameNode->GetIGameObject();

	IGameObject::ObjectTypes tmpObjectType = tmpGameObject->GetIGameType();

	switch (tmpObjectType)
	{
	case IGameObject::IGAME_UNKNOWN:
		break;
	case IGameObject::IGAME_LIGHT:
		break;
	case IGameObject::IGAME_MESH:
	{
		IGameMesh* tmpGameMesh = (IGameMesh*)tmpGameObject;

		ExportMesh(tmpGameMesh, tmpNodeName);
	}

	break;
	case IGameObject::IGAME_SPLINE:
		break;
	case IGameObject::IGAME_CAMERA:
		break;
	case IGameObject::IGAME_HELPER:
	{
		IGameSupportObject* tmpGameSupportObject = (IGameSupportObject*)tmpGameObject;
		IGameMesh* tmpGameMesh = tmpGameSupportObject->GetMeshObject();
		if (tmpGameMesh != nullptr && tmpGameMesh->InitializeData())
		{
			ExportMesh(tmpGameMesh, tmpNodeName);
		}
	}
	break;
	case IGameObject::IGAME_BONE:
		break;
	case IGameObject::IGAME_IKCHAIN:
		break;
	case IGameObject::IGAME_XREF:
		break;
	default:
		break;
	}

	//遍历子节点
	int tmpChildCount = varGameNode->GetChildCount();
	for (size_t i = 0; i < tmpChildCount; i++)
	{
		IGameNode* tmpGameNode = varGameNode->GetNodeChild(i);
		ExportNodeTraverse(tmpGameNode);
	}
}

#include "utilapi.h"
int	IGameExporter::DoExport(const TCHAR *name, ExpInterface *ei, Interface *i, BOOL suppressPrompts, DWORD options)
{
	//#pragma message(TODO("Implement the actual file Export here and"))

	if (!suppressPrompts)
		DialogBoxParam(hInstance,
			MAKEINTRESOURCE(IDD_PANEL),
			GetActiveWindow(),
			IGameExporterOptionsDlgProc, (LPARAM)this);


	bool tmpSelected = (options & SCENE_EXPORT_SELECTED) ? true : false;
	IGameScene* tmpGameScene = GetIGameInterface();

	IGameConversionManager* tmpGameConversionManager = GetConversionManager();
	tmpGameConversionManager->SetCoordSystem(IGameConversionManager::IGAME_OGL);

	tmpGameScene->InitialiseIGame(tmpSelected);

	tmpGameScene->SetStaticFrame(0);

	mGameScene = tmpGameScene;
	mName = name;

	//得到第一级
	int tmpTopLevelNodeCount = tmpGameScene->GetTopLevelNodeCount();
	if (tmpTopLevelNodeCount == 0)
	{
		tmpGameScene->ReleaseIGame();
		return TRUE;
	}


	for (int i = 0; i < tmpTopLevelNodeCount; i++)
	{
		IGameNode* tmpGameNode = tmpGameScene->GetTopLevelNode(i);
		ExportNodeTraverse(tmpGameNode);
	}

	tmpGameScene->ReleaseIGame();

	return TRUE;
}



