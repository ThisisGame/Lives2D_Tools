/**********************************************************************
 *<
   FILE: boxobj.cpp

   DESCRIPTION:  A Box object implementation

 *> Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "prim.h"
#include "iparamm2.h"
#include "Simpobj.h"
#include "surf_api.h"
#include "MNMath.h"
#include "PolyObj.h"
#include "macroRec.h"
#include "RealWorldMapUtils.h"
#include <ReferenceSaveManager.h>

class BoxObject : public GenBoxObject, public RealWorldMapSizeInterface {
private:
   bool mPolyBoxSmoothingGroupFix;

public:
   // Class vars
   static IObjParam *ip;
   static bool typeinCreate;

   BoxObject(BOOL loading);

   // From Object
   int CanConvertToType(Class_ID obtype) override;
   Object* ConvertToType(TimeValue t, Class_ID obtype) override;
   void GetCollapseTypes(Tab<Class_ID> &clist, Tab<TSTR*> &nlist) override;

   // From BaseObject
   CreateMouseCallBack* GetCreateMouseCallBack() override;
   void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) override;
   void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) override;
   const TCHAR *GetObjectName() override { return GetString(IDS_RB_BOX); }
   BOOL HasUVW() override;
   void SetGenUVW(BOOL sw) override;

   // Animatable methods
   void DeleteThis() override { delete this; }
   Class_ID ClassID() override { return Class_ID(BOXOBJ_CLASS_ID, 0); }

   
   

   // From ref
   RefTargetHandle Clone(RemapDir& remap);
   bool SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager);
   IOResult Save(ISave *isave);
   IOResult Load(ILoad *iload);

   // From SimpleObject
   void BuildMesh(TimeValue t);
   BOOL OKtoDisplay(TimeValue t);
   void InvalidateUI();

   // From GenBoxObject
   void SetParams(float width, float height, float length, int wsegs, int lsegs,
      int hsegs, BOOL genUV);

   // Get/Set the UsePhyicalScaleUVs flag.
   BOOL GetUsePhysicalScaleUVs();
   void SetUsePhysicalScaleUVs(BOOL flag);
   void UpdateUI();

   //From FPMixinInterface
   BaseInterface* GetInterface(Interface_ID id)
   {
      if (id == RWS_INTERFACE)
         return (RealWorldMapSizeInterface*)this;

      BaseInterface* intf = GenBoxObject::GetInterface(id);
      if (intf)
         return intf;

      return FPMixinInterface::GetInterface(id);
   }

   // local
   Object *BuildPolyBox(TimeValue t);
};

// class variables for box class.
IObjParam *BoxObject::ip = NULL;
bool BoxObject::typeinCreate = false;

#define PBLOCK_REF_NO  0

#define BMIN_LENGTH  float(0)
#define BMAX_LENGTH  float(1.0E30)
#define BMIN_WIDTH  float(0)
#define BMAX_WIDTH  float(1.0E30)
#define BMIN_HEIGHT  float(-1.0E30)
#define BMAX_HEIGHT  float(1.0E30)

#define BDEF_DIM  float(0)
#define BDEF_SEGS  1

#define MIN_SEGMENTS 1
#define MAX_SEGMENTS 200

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

//--- ClassDescriptor and class vars ---------------------------------

static BOOL sInterfaceAdded = FALSE;

class BoxObjClassDesc :public ClassDesc2 {
public:
   int    IsPublic() { return 1; }
   void *   Create(BOOL loading = FALSE)
   {
      if (!sInterfaceAdded) {
         AddInterface(&gRealWorldMapSizeDesc);
         sInterfaceAdded = TRUE;
      }
      return new BoxObject(loading);
   }
   const TCHAR * ClassName() { return GetString(IDS_RB_BOX_CLASS); }
   SClass_ID  SuperClassID() { return GEOMOBJECT_CLASS_ID; }
   Class_ID  ClassID() { return Class_ID(BOXOBJ_CLASS_ID, 0); }
   const TCHAR*  Category() { return GetString(IDS_RB_PRIMITIVES); }
   const TCHAR* InternalName() { return _T("Box"); } // returns fixed parsable name (scripter-visible name)
   HINSTANCE  HInstance() { return hInstance; }   // returns owning module handle
};

static BoxObjClassDesc boxObjDesc;

ClassDesc* GetBoxobjDesc() { return &boxObjDesc; }

// ParamBlockDesc2 IDs
enum paramblockdesc2_ids { box_creation_type, box_type_in, box_params, };
enum box_creation_type_param_ids { box_create_meth, };
enum box_type_in_param_ids { box_ti_pos, box_ti_length, box_ti_width, box_ti_height, };
enum box_param_param_ids { box_length = BOXOBJ_LENGTH, box_width = BOXOBJ_WIDTH, box_height = BOXOBJ_HEIGHT, 
	box_wsegs = BOXOBJ_WSEGS, box_lsegs = BOXOBJ_LSEGS, box_hsegs = BOXOBJ_HSEGS, box_mapping = BOXOBJ_GENUVS, };

namespace
{
   class CreationType_Accessor : public PBAccessor
   {
      void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t);
   };
	class MapCoords_Accessor : public PBAccessor
	{
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
		{
			if (owner) ((BoxObject*)owner)->UpdateUI();
		}
	};
   static CreationType_Accessor creationType_Accessor;
	static MapCoords_Accessor mapCoords_Accessor;
}

// class creation type block
static ParamBlockDesc2 box_crtype_blk(box_creation_type, _T("BoxCreationType"), 0, &boxObjDesc, P_CLASS_PARAMS + P_AUTO_UI,
   //rollout
   IDD_BOXPARAM1, IDS_RB_CREATIONMETHOD, BEGIN_EDIT_CREATE, 0, NULL,
   // params
   box_create_meth, _T("typeinCreationMethod"), TYPE_INT, 0, IDS_RB_CREATIONMETHOD,
   p_default, 0,
   p_range, 0, 1,
   p_ui, TYPE_RADIO, 2, IDC_CREATEBOX, IDC_CREATECUBE,
   p_accessor, &creationType_Accessor,
   p_end,
   p_end
   );

// class type-in block
static ParamBlockDesc2 box_typein_blk(box_type_in, _T("BoxTypeIn"), 0, &boxObjDesc, P_CLASS_PARAMS + P_AUTO_UI,
   //rollout
   IDD_BOXPARAM3, IDS_RB_KEYBOARDENTRY, BEGIN_EDIT_CREATE, APPENDROLL_CLOSED, NULL,
   // params
   box_ti_pos, _T("typeInPos"), TYPE_POINT3, 0, IDS_RB_TYPEIN_POS,
   p_default, Point3(0, 0, 0),
   p_range, float(-1.0E30), float(1.0E30),
   p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_TI_POSX, IDC_TI_POSXSPIN, IDC_TI_POSY, IDC_TI_POSYSPIN, IDC_TI_POSZ, IDC_TI_POSZSPIN, SPIN_AUTOSCALE,
   p_end,
   box_ti_length, _T("typeInLength"), TYPE_FLOAT, 0, IDS_RB_LENGTH,
   p_default, BDEF_DIM,
   p_range, BMIN_LENGTH, BMAX_LENGTH,
   p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LENGTHEDIT, IDC_LENSPINNER, SPIN_AUTOSCALE,
   p_end,
   box_ti_width, _T("typeInWidth"), TYPE_FLOAT, 0, IDS_RB_WIDTH,
   p_default, BDEF_DIM,
   p_range, BMIN_WIDTH, BMAX_WIDTH,
   p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_WIDTHEDIT, IDC_WIDTHSPINNER, SPIN_AUTOSCALE,
   p_end,
   box_ti_height, _T("typeInHeight"), TYPE_FLOAT, 0, IDS_RB_HEIGHT,
   p_default, BDEF_DIM,
   p_range, BMIN_HEIGHT, BMAX_HEIGHT,
   p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_HEIGHTEDIT, IDC_HEIGHTSPINNER, SPIN_AUTOSCALE,
   p_end,
   p_end
   );

// per instance box block
static ParamBlockDesc2 box_param_blk(box_params, _T("BoxParameters"), 0, &boxObjDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
   //rollout
   IDD_BOXPARAM2, IDS_RB_PARAMETERS, 0, 0, NULL,
   // params
   box_length, _T("length"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_LENGTH,
   p_default, BDEF_DIM,
   p_ms_default, 25.0,
   p_range, BMIN_LENGTH, BMAX_LENGTH,
   p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LENGTHEDIT, IDC_LENSPINNER, SPIN_AUTOSCALE,
   p_end,
   box_width, _T("width"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_WIDTH,
   p_default, BDEF_DIM,
   p_ms_default, 25.0,
   p_range, BMIN_WIDTH, BMAX_WIDTH,
   p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_WIDTHEDIT, IDC_WIDTHSPINNER, SPIN_AUTOSCALE,
   p_end,
   box_height, _T("height"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_HEIGHT,
   p_default, BDEF_DIM,
   p_ms_default, 25.0,
   p_range, BMIN_HEIGHT, BMAX_HEIGHT,
   p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_HEIGHTEDIT, IDC_HEIGHTSPINNER, SPIN_AUTOSCALE,
   p_end,
   box_wsegs, _T("widthsegs"), TYPE_INT, P_ANIMATABLE, IDS_RB_WSEGS,
   p_default, BDEF_SEGS,
   p_range, MIN_SEGMENTS, MAX_SEGMENTS,
   p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_WSEGS, IDC_WSEGSPIN, 0.1f,
   p_end,
	box_lsegs, _T("lengthsegs"), TYPE_INT, P_ANIMATABLE, IDS_RB_LSEGS,
	p_default, BDEF_SEGS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_LSEGS, IDC_LSEGSPIN, 0.1f,
	p_end,
   box_hsegs, _T("heightsegs"), TYPE_INT, P_ANIMATABLE, IDS_RB_HSEGS,
   p_default, BDEF_SEGS,
   p_range, MIN_SEGMENTS, MAX_SEGMENTS,
   p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_HSEGS, IDC_HSEGSPIN, 0.1f,
   p_end,
   box_mapping, _T("mapCoords"), TYPE_BOOL, 0, IDS_RB_GENTEXCOORDS,
   p_default, TRUE,
   p_ms_default, FALSE,
   p_ui, TYPE_SINGLECHECKBOX, IDC_GENTEXTURE,
	p_accessor, &mapCoords_Accessor,
   p_end,
   p_end
   );

void CreationType_Accessor::Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
{
   // disable Keyboard Entry Width/Height spinners if creating cube
   IParamMap2* pmap = boxObjDesc.GetParamMap(&box_typein_blk);
   if (pmap)
   {
      bool createCube = v.i == 1;
      pmap->Enable(box_ti_width, !createCube);
      pmap->Enable(box_ti_height, !createCube);
   }
}

//--- Parameter map/block descriptors -------------------------------

ParamBlockDescID descVer0[] = {
   { TYPE_FLOAT, NULL, TRUE, box_length },
   { TYPE_FLOAT, NULL, TRUE, box_width },
   { TYPE_FLOAT, NULL, TRUE, box_height },
   { TYPE_INT, NULL, TRUE, box_wsegs },
   { TYPE_INT, NULL, TRUE, box_lsegs },
   { TYPE_INT, NULL, TRUE, box_hsegs }
};

ParamBlockDescID descVer1[] = {
   { TYPE_FLOAT, NULL, TRUE, box_length },
   { TYPE_FLOAT, NULL, TRUE, box_width },
   { TYPE_FLOAT, NULL, TRUE, box_height },
   { TYPE_INT, NULL, TRUE, box_wsegs },
   { TYPE_INT, NULL, TRUE, box_lsegs },
   { TYPE_INT, NULL, TRUE, box_hsegs },
   { TYPE_INT, NULL, FALSE, box_mapping }
};

// Array of old versions
static ParamVersionDesc versions[] = {
   ParamVersionDesc(descVer0,6,0),
   ParamVersionDesc(descVer1,7,1),
};
#define NUM_OLDVERSIONS 2

// ParamBlock data for SaveToPrevious support
#define PBLOCK_LENGTH 7
#define CURRENT_VERSION 1

//--- TypeInDlgProc --------------------------------

class BoxTypeInDlgProc : public ParamMap2UserDlgProc {
public:
   BoxObject *ob;

   BoxTypeInDlgProc(BoxObject *o) { ob = o; }
   INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
   void DeleteThis() { delete this; }
};

INT_PTR BoxTypeInDlgProc::DlgProc(
   TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
   case WM_INITDIALOG:
   {
      // disable width and height spinners if in Cube creation mode.
      bool createCube = box_crtype_blk.GetInt(box_create_meth) == 1;
      map->Enable(box_ti_width, !createCube);
      map->Enable(box_ti_height, !createCube);
   }
   break;

   case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case IDC_TI_CREATE: {
         // We only want to set the value if the object is not in the scene.
         if (ob->TestAFlag(A_OBJ_CREATING)) {
            bool createCube = box_crtype_blk.GetInt(box_create_meth) == 1;
            if (createCube)
            {
               float val = box_typein_blk.GetFloat(box_ti_length);
               ob->pblock2->SetValue(box_length, 0, val);
               ob->pblock2->SetValue(box_width, 0, val);
               ob->pblock2->SetValue(box_height, 0, val);
            }
            else
            {
               ob->pblock2->SetValue(box_length, 0, box_typein_blk.GetFloat(box_ti_length));
               ob->pblock2->SetValue(box_width, 0, box_typein_blk.GetFloat(box_ti_width));
               ob->pblock2->SetValue(box_height, 0, box_typein_blk.GetFloat(box_ti_height));
            }
         }
         else
            BoxObject::typeinCreate = true;

         Matrix3 tm(1);
         tm.SetTrans(box_typein_blk.GetPoint3(box_ti_pos));
         ob->suspendSnap = FALSE;
         ob->ip->NonMouseCreate(tm);
         // NOTE that calling NonMouseCreate will cause this
         // object to be deleted. DO NOT DO ANYTHING BUT RETURN.
         return TRUE;
      }
      }
      break;

   }
   return FALSE;
}

class BoxParamDlgProc : public ParamMap2UserDlgProc {
public:
   BoxObject *mpBoxObj;
   HWND mhWnd;
   BoxParamDlgProc(BoxObject *o) { mpBoxObj = o; mhWnd = NULL; }
   INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
   void DeleteThis() { delete this; }
   void UpdateUI();
   BOOL GetRWSState();
};

void BoxParamDlgProc::UpdateUI()
{
   if (mhWnd == NULL)
      return;
   BOOL usePhysUVs = mpBoxObj->GetUsePhysicalScaleUVs();
   CheckDlgButton(mhWnd, IDC_REAL_WORLD_MAP_SIZE, usePhysUVs);
   EnableWindow(GetDlgItem(mhWnd, IDC_REAL_WORLD_MAP_SIZE), mpBoxObj->HasUVW());
}

BOOL BoxParamDlgProc::GetRWSState()
{
   BOOL check = IsDlgButtonChecked(mhWnd, IDC_REAL_WORLD_MAP_SIZE);
   return check;
}

INT_PTR BoxParamDlgProc::DlgProc(
   TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
   case WM_INITDIALOG: {
      mhWnd = hWnd;
      UpdateUI();
      break;
   }
   case WM_COMMAND:
      switch (LOWORD(wParam)) {
       case IDC_REAL_WORLD_MAP_SIZE: {
         BOOL check = IsDlgButtonChecked(hWnd, IDC_REAL_WORLD_MAP_SIZE);
         theHold.Begin();
         mpBoxObj->SetUsePhysicalScaleUVs(check);
         theHold.Accept(GetString(IDS_DS_PARAMCHG));
         mpBoxObj->ip->RedrawViews(mpBoxObj->ip->GetTime());
         break;
      }
      }
      break;

   }
   return FALSE;
}

 
 



//--- Box methods -------------------------------

BoxObject::BoxObject(BOOL loading) : mPolyBoxSmoothingGroupFix(true)
{
   boxObjDesc.MakeAutoParamBlocks(this);

   if (!loading && !GetPhysicalScaleUVsDisabled())
      SetUsePhysicalScaleUVs(true);
}

const int kChunkPolyFix = 0x0100;

bool BoxObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
   // if saving to previous version that used pb1 instead of pb2...
   DWORD saveVersion = GetSavingVersion();
   if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
   {
	   ProcessPB2ToPB1SaveToPrevious(this, pblock2, PBLOCK_REF_NO, descVer1, PBLOCK_LENGTH, CURRENT_VERSION);
   }
   return __super::SpecifySaveReferences(referenceSaveManager);
}

IOResult BoxObject::Save(ISave *isave)
{
   ULONG nb;
   isave->BeginChunk(kChunkPolyFix);
   isave->Write(&mPolyBoxSmoothingGroupFix, sizeof(bool), &nb);
   isave->EndChunk();
   return IO_OK;
}

IOResult BoxObject::Load(ILoad *iload)
{
   ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &box_param_blk, this, PBLOCK_REF_NO);
   iload->RegisterPostLoadCallback(plcb);

   // For old Boxes with no kChunkPolyFix, the fix defaults to "off".
   mPolyBoxSmoothingGroupFix = false;

   ULONG nb;
   IOResult res = IO_OK;
   while (IO_OK == (res = iload->OpenChunk())) {
      switch (iload->CurChunkID()) {
      case kChunkPolyFix:
         iload->Read(&mPolyBoxSmoothingGroupFix, sizeof(bool), &nb);
         break;
      }
      iload->CloseChunk();
      if (res != IO_OK)  return res;
   }
   return IO_OK;
}

void BoxObject::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
   __super::BeginEditParams(ip, flags, prev);

   this->ip = ip;

   // If this has been freshly created by type-in, set creation values:
   if (BoxObject::typeinCreate)
   {
      bool createCube = box_crtype_blk.GetInt(box_create_meth) == 1;
      if (createCube)
      {
         float val = box_typein_blk.GetFloat(box_ti_length);
         pblock2->SetValue(box_length, 0, val);
         pblock2->SetValue(box_width, 0, val);
         pblock2->SetValue(box_height, 0, val);
      }
      else
      {
         pblock2->SetValue(box_length, 0, box_typein_blk.GetFloat(box_ti_length));
         pblock2->SetValue(box_width, 0, box_typein_blk.GetFloat(box_ti_width));
         pblock2->SetValue(box_height, 0, box_typein_blk.GetFloat(box_ti_height));
      }
      typeinCreate = false;
   }

   // throw up all the appropriate auto-rollouts
   boxObjDesc.BeginEditParams(ip, this, flags, prev);
   // if in Create Panel, install a callback for the type in.
   if (flags & BEGIN_EDIT_CREATE)
   {
      box_typein_blk.SetUserDlgProc(new BoxTypeInDlgProc(this));
   }
   // install a callback for the params.
   box_param_blk.SetUserDlgProc(new BoxParamDlgProc(this));
}

void BoxObject::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
   __super::EndEditParams(ip, flags, next);
   this->ip = NULL;
   boxObjDesc.EndEditParams(ip, this, flags, next);
}

void BoxObject::SetParams(float width, float height, float length, int wsegs, int lsegs,
   int hsegs, BOOL genUV) {
   pblock2->SetValue(box_width, 0, width);
   pblock2->SetValue(box_height, 0, height);
   pblock2->SetValue(box_length, 0, length);
   pblock2->SetValue(box_lsegs, 0, lsegs);
   pblock2->SetValue(box_wsegs, 0, wsegs);
   pblock2->SetValue(box_hsegs, 0, hsegs);
   pblock2->SetValue(box_mapping, 0, genUV);
}

// vertices ( a b c d ) are in counter clockwise order when viewed from 
// outside the surface unless bias!=0 in which case they are clockwise
static void MakeQuad(int nverts, Face *f, int a, int b, int c, int d, int sg, int bias) {
   int sm = 1 << sg;
   assert(a < nverts);
   assert(b < nverts);
   assert(c < nverts);
   assert(d < nverts);
   if (bias) {
      f[0].setVerts(b, a, c);
      f[0].setSmGroup(sm);
      f[0].setEdgeVisFlags(1, 0, 1);
      f[1].setVerts(d, c, a);
      f[1].setSmGroup(sm);
      f[1].setEdgeVisFlags(1, 0, 1);
   }
   else {
      f[0].setVerts(a, b, c);
      f[0].setSmGroup(sm);
      f[0].setEdgeVisFlags(1, 1, 0);
      f[1].setVerts(c, d, a);
      f[1].setSmGroup(sm);
      f[1].setEdgeVisFlags(1, 1, 0);
   }
}

#define POSX 0 // right
#define POSY 1 // back
#define POSZ 2 // top
#define NEGX 3 // left
#define NEGY 4 // front
#define NEGZ 5 // bottom

int direction(Point3 *v) {
   Point3 a = v[0] - v[2];
   Point3 b = v[1] - v[0];
   Point3 n = CrossProd(a, b);
   switch (MaxComponent(n)) {
   case 0: return (n.x < 0) ? NEGX : POSX;
   case 1: return (n.y < 0) ? NEGY : POSY;
   case 2: return (n.z < 0) ? NEGZ : POSZ;
   }
   return 0;
}

// Remap the sub-object material numbers so that the top face is the first one
// The order now is:
// Top / Bottom /  Left/ Right / Front / Back
static int mapDir[6] = { 3, 5, 0, 2, 4, 1 };

#define MAKE_QUAD(na,nb,nc,nd,sm,b) {MakeQuad(nverts,&(mesh.faces[nf]),na, nb, nc, nd, sm, b);nf+=2;}

BOOL BoxObject::HasUVW() {
   BOOL genUVs;
   Interval v;
   pblock2->GetValue(box_mapping, 0, genUVs, v);
   return genUVs;
}

void BoxObject::SetGenUVW(BOOL sw) {
   if (sw == HasUVW()) return;
   pblock2->SetValue(box_mapping, 0, sw);
}



void BoxObject::BuildMesh(TimeValue t)
{
   int ix, iy, iz, nf, kv, mv, nlayer, topStart, midStart;
   int nverts, wsegs, lsegs, hsegs, nv, nextk, nextm, wsp1;
   int nfaces;
   Point3 va, vb, p;
   float l, w, h;
   int genUVs = 1;
   BOOL bias = 0;

   // Start the validity interval at forever and widdle it down.
   ivalid = FOREVER;
   pblock2->GetValue(box_length, t, l, ivalid);
   pblock2->GetValue(box_width, t, w, ivalid);
   pblock2->GetValue(box_height, t, h, ivalid);
   pblock2->GetValue(box_lsegs, t, lsegs, ivalid);
   pblock2->GetValue(box_wsegs, t, wsegs, ivalid);
   pblock2->GetValue(box_hsegs, t, hsegs, ivalid);
   pblock2->GetValue(box_mapping, t, genUVs, ivalid);
   if (h < 0.0f) bias = 1;

   LimitValue(lsegs, MIN_SEGMENTS, MAX_SEGMENTS);
   LimitValue(wsegs, MIN_SEGMENTS, MAX_SEGMENTS);
   LimitValue(hsegs, MIN_SEGMENTS, MAX_SEGMENTS);

   // Number of verts
      // bottom : (lsegs+1)*(wsegs+1)
     // top    : (lsegs+1)*(wsegs+1)
     // sides  : (2*lsegs+2*wsegs)*(hsegs-1)

   // Number of rectangular faces.
      // bottom : (lsegs)*(wsegs)
     // top    : (lsegs)*(wsegs)
     // sides  : 2*(hsegs*lsegs)+2*(wsegs*lsegs)

   wsp1 = wsegs + 1;
   nlayer = 2 * (lsegs + wsegs);
   topStart = (lsegs + 1)*(wsegs + 1);
   midStart = 2 * topStart;

   nverts = midStart + nlayer*(hsegs - 1);
   nfaces = 4 * (lsegs*wsegs + hsegs*lsegs + wsegs*hsegs);

   mesh.setNumVerts(nverts);
   mesh.setNumFaces(nfaces);
   mesh.InvalidateTopologyCache();

   nv = 0;

   vb = Point3(w, l, h) / float(2);
   va = -vb;

   va.z = float(0);
   vb.z = h;

   float dx = w / wsegs;
   float dy = l / lsegs;
   float dz = h / hsegs;

   // do bottom vertices.
   p.z = va.z;
   p.y = va.y;
   for (iy = 0; iy <= lsegs; iy++) {
      p.x = va.x;
      for (ix = 0; ix <= wsegs; ix++) {
         mesh.setVert(nv++, p);
         p.x += dx;
      }
      p.y += dy;
   }

   nf = 0;

   // do bottom faces.
   for (iy = 0; iy < lsegs; iy++) {
      kv = iy*(wsegs + 1);
      for (ix = 0; ix < wsegs; ix++) {
         MAKE_QUAD(kv, kv + wsegs + 1, kv + wsegs + 2, kv + 1, 1, bias);
         kv++;
      }
   }
   assert(nf == lsegs*wsegs * 2);

   // do top vertices.
   p.z = vb.z;
   p.y = va.y;
   for (iy = 0; iy <= lsegs; iy++) {
      p.x = va.x;
      for (ix = 0; ix <= wsegs; ix++) {
         mesh.setVert(nv++, p);
         p.x += dx;
      }
      p.y += dy;
   }

   // do top faces (lsegs*wsegs);
   for (iy = 0; iy < lsegs; iy++) {
      kv = iy*(wsegs + 1) + topStart;
      for (ix = 0; ix < wsegs; ix++) {
         MAKE_QUAD(kv, kv + 1, kv + wsegs + 2, kv + wsegs + 1, 2, bias);
         kv++;
      }
   }
   assert(nf == lsegs*wsegs * 4);

   // do middle vertices 
   for (iz = 1; iz < hsegs; iz++) {

      p.z = va.z + dz * iz;

      // front edge
      p.x = va.x;  p.y = va.y;
      for (ix = 0; ix < wsegs; ix++) { mesh.setVert(nv++, p);  p.x += dx; }

      // right edge
      p.x = vb.x;   p.y = va.y;
      for (iy = 0; iy < lsegs; iy++) { mesh.setVert(nv++, p);  p.y += dy; }

      // back edge
      p.x = vb.x;  p.y = vb.y;
      for (ix = 0; ix < wsegs; ix++) { mesh.setVert(nv++, p);  p.x -= dx; }

      // left edge
      p.x = va.x;  p.y = vb.y;
      for (iy = 0; iy < lsegs; iy++) { mesh.setVert(nv++, p);  p.y -= dy; }
   }

   if (hsegs == 1) {
      // do FRONT faces -----------------------
      kv = 0;
      mv = topStart;
      for (ix = 0; ix < wsegs; ix++) {
         MAKE_QUAD(kv, kv + 1, mv + 1, mv, 3, bias);
         kv++;
         mv++;
      }

      // do RIGHT faces.-----------------------
      kv = wsegs;
      mv = topStart + kv;
      for (iy = 0; iy < lsegs; iy++) {
         MAKE_QUAD(kv, kv + wsp1, mv + wsp1, mv, 4, bias);
         kv += wsp1;
         mv += wsp1;
      }

      // do BACK faces.-----------------------
      kv = topStart - 1;
      mv = midStart - 1;
      for (ix = 0; ix < wsegs; ix++) {
         MAKE_QUAD(kv, kv - 1, mv - 1, mv, 5, bias);
         kv--;
         mv--;
      }

      // do LEFT faces.----------------------
      kv = lsegs*(wsegs + 1);  // index into bottom
      mv = topStart + kv;
      for (iy = 0; iy < lsegs; iy++) {
         MAKE_QUAD(kv, kv - wsp1, mv - wsp1, mv, 6, bias);
         kv -= wsp1;
         mv -= wsp1;
      }
   }

   else {
      // do front faces.
      kv = 0;
      mv = midStart;
      for (iz = 0; iz < hsegs; iz++) {
         if (iz == hsegs - 1) mv = topStart;
         for (ix = 0; ix < wsegs; ix++)
            MAKE_QUAD(kv + ix, kv + ix + 1, mv + ix + 1, mv + ix, 3, bias);
         kv = mv;
         mv += nlayer;
      }

      assert(nf == lsegs*wsegs * 4 + wsegs*hsegs * 2);

      // do RIGHT faces.-------------------------
      // RIGHT bottom row:
      kv = wsegs; // into bottom layer. 
      mv = midStart + wsegs; // first layer of mid verts


      for (iy = 0; iy < lsegs; iy++) {
         MAKE_QUAD(kv, kv + wsp1, mv + 1, mv, 4, bias);
         kv += wsp1;
         mv++;
      }

      // RIGHT middle part:
      kv = midStart + wsegs;
      for (iz = 0; iz < hsegs - 2; iz++) {
         mv = kv + nlayer;
         for (iy = 0; iy < lsegs; iy++) {
            MAKE_QUAD(kv + iy, kv + iy + 1, mv + iy + 1, mv + iy, 4, bias);
         }
         kv += nlayer;
      }

      // RIGHT top row:
      kv = midStart + wsegs + (hsegs - 2)*nlayer;
      mv = topStart + wsegs;
      for (iy = 0; iy < lsegs; iy++) {
         MAKE_QUAD(kv, kv + 1, mv + wsp1, mv, 4, bias);
         mv += wsp1;
         kv++;
      }

      assert(nf == lsegs*wsegs * 4 + wsegs*hsegs * 2 + lsegs*hsegs * 2);

      // do BACK faces. ---------------------
      // BACK bottom row:
      kv = topStart - 1;
      mv = midStart + wsegs + lsegs;
      for (ix = 0; ix < wsegs; ix++) {
         MAKE_QUAD(kv, kv - 1, mv + 1, mv, 5, bias);
         kv--;
         mv++;
      }

      // BACK middle part:
      kv = midStart + wsegs + lsegs;
      for (iz = 0; iz < hsegs - 2; iz++) {
         mv = kv + nlayer;
         for (ix = 0; ix < wsegs; ix++) {
            MAKE_QUAD(kv + ix, kv + ix + 1, mv + ix + 1, mv + ix, 5, bias);
         }
         kv += nlayer;
      }

      // BACK top row:
      kv = midStart + wsegs + lsegs + (hsegs - 2)*nlayer;
      mv = topStart + lsegs*(wsegs + 1) + wsegs;
      for (ix = 0; ix < wsegs; ix++) {
         MAKE_QUAD(kv, kv + 1, mv - 1, mv, 5, bias);
         mv--;
         kv++;
      }

      assert(nf == lsegs*wsegs * 4 + wsegs*hsegs * 4 + lsegs*hsegs * 2);

      // do LEFT faces. -----------------
      // LEFT bottom row:
      kv = lsegs*(wsegs + 1);  // index into bottom
      mv = midStart + 2 * wsegs + lsegs;
      for (iy = 0; iy < lsegs; iy++) {
         nextm = mv + 1;
         if (iy == lsegs - 1)
            nextm -= nlayer;
         MAKE_QUAD(kv, kv - wsp1, nextm, mv, 6, bias);
         kv -= wsp1;
         mv++;
      }

      // LEFT middle part:
      kv = midStart + 2 * wsegs + lsegs;
      for (iz = 0; iz < hsegs - 2; iz++) {
         mv = kv + nlayer;
         for (iy = 0; iy < lsegs; iy++) {
            nextm = mv + 1;
            nextk = kv + iy + 1;
            if (iy == lsegs - 1) {
               nextm -= nlayer;
               nextk -= nlayer;
            }
            MAKE_QUAD(kv + iy, nextk, nextm, mv, 6, bias);
            mv++;
         }
         kv += nlayer;
      }

      // LEFT top row:
      kv = midStart + 2 * wsegs + lsegs + (hsegs - 2)*nlayer;
      mv = topStart + lsegs*(wsegs + 1);
      for (iy = 0; iy < lsegs; iy++) {
         nextk = kv + 1;
         if (iy == lsegs - 1)
            nextk -= nlayer;
         MAKE_QUAD(kv, nextk, mv - wsp1, mv, 6, bias);
         mv -= wsp1;
         kv++;
      }
   }

   if (genUVs) {
      int ls = lsegs + 1;
      int ws = wsegs + 1;
      int hs = hsegs + 1;
      int ntverts = ls*hs + hs*ws + ws*ls;
      mesh.setNumTVerts(ntverts);
      mesh.setNumTVFaces(nfaces);

      int xbase = 0;
      int ybase = ls*hs;
      int zbase = ls*hs + hs*ws;

      if (w == 0.0f) w = .0001f;
      if (l == 0.0f) l = .0001f;
      if (h == 0.0f) h = .0001f;

      BOOL usePhysUVs = GetUsePhysicalScaleUVs();
      float maxW = usePhysUVs ? w : 1.0f;
      float maxL = usePhysUVs ? l : 1.0f;
      float maxH = usePhysUVs ? h : 1.0f;

      float dw = maxW / float(wsegs);
      float dl = maxL / float(lsegs);
      float dh = maxH / float(hsegs);

      float u, v;

      nv = 0;
      v = 0.0f;
      // X axis face
      for (iz = 0; iz < hs; iz++) {
         u = 0.0f;
         for (iy = 0; iy < ls; iy++) {
            mesh.setTVert(nv, u, v, 0.0f);
            nv++; u += dl;
         }
         v += dh;
      }

      v = 0.0f;
      //Y Axis face
      for (iz = 0; iz < hs; iz++) {
         u = 0.0f;
         for (ix = 0; ix < ws; ix++) {
            mesh.setTVert(nv, u, v, 0.0f);
            nv++; u += dw;
         }
         v += dh;
      }

      v = 0.0f;
      for (iy = 0; iy < ls; iy++) {
         u = 0.0f;
         for (ix = 0; ix < ws; ix++) {
            mesh.setTVert(nv, u, v, 0.0f);
            nv++; u += dw;
         }
         v += dl;
      }

      assert(nv == ntverts);

      for (nf = 0; nf < nfaces; nf++) {
         Face& f = mesh.faces[nf];
         DWORD* nv = f.getAllVerts();
         Point3 v[3];
         for (ix = 0; ix < 3; ix++)
            v[ix] = mesh.getVert(nv[ix]);
         int dir = direction(v);
         int ntv[3];
         for (ix = 0; ix < 3; ix++) {
            int iu, iv;
            switch (dir) {
            case POSX: case NEGX:
               iu = int(((float)lsegs*(v[ix].y - va.y) / l) + .5f);
               iv = int(((float)hsegs*(v[ix].z - va.z) / h) + .5f);
               if (dir == NEGX) iu = lsegs - iu;
               ntv[ix] = (xbase + iv*ls + iu);
               break;
            case POSY: case NEGY:
               iu = int(((float)wsegs*(v[ix].x - va.x) / w) + .5f);
               iv = int(((float)hsegs*(v[ix].z - va.z) / h) + .5f);
               if (dir == POSY) iu = wsegs - iu;
               ntv[ix] = (ybase + iv*ws + iu);
               break;
            case POSZ: case NEGZ:
               iu = int(((float)wsegs*(v[ix].x - va.x) / w) + .5f);
               iv = int(((float)lsegs*(v[ix].y - va.y) / l) + .5f);
               if (dir == NEGZ) iu = wsegs - iu;
               ntv[ix] = (zbase + iv*ws + iu);
               break;
            }
         }
         assert(ntv[0] < ntverts);
         assert(ntv[1] < ntverts);
         assert(ntv[2] < ntverts);

         mesh.tvFace[nf].setTVerts(ntv[0], ntv[1], ntv[2]);
         mesh.setFaceMtlIndex(nf, mapDir[dir]);
      }
   }
   else {
      mesh.setNumTVerts(0);
      mesh.setNumTVFaces(0);
      for (nf = 0; nf < nfaces; nf++) {
         Face& f = mesh.faces[nf];
         DWORD* nv = f.getAllVerts();
         Point3 v[3];
         for (int ix = 0; ix < 3; ix++)
            v[ix] = mesh.getVert(nv[ix]);
         int dir = direction(v);
         mesh.setFaceMtlIndex(nf, mapDir[dir]);
      }
   }

   mesh.InvalidateTopologyCache();
}


#define Tang(vv,ii) ((vv)*3+(ii))
inline Point3 operator+(const PatchVert &pv, const Point3 &p)
{
   return p + pv.p;
}
inline Point3 operator-(const PatchVert &pv1, const PatchVert &pv2)
{
   return pv1.p - pv2.p;
}
inline Point3 operator+(const PatchVert &pv1, const PatchVert &pv2)
{
   return pv1.p + pv2.p;
}

void BuildBoxPatch(
   PatchMesh &patch,
   float width, float length, float height, int textured, BOOL usePhysUVs)
{
   int nverts = 8;
   int nvecs = 48;
   int npatches = 6;
   patch.setNumVerts(nverts);
   patch.setNumTVerts(textured ? 12 : 0);
   patch.setNumVecs(nvecs);
   patch.setNumPatches(npatches);
   patch.setNumTVPatches(textured ? npatches : 0);

   float w2 = width / 2.0f, w3 = width / 3.0f;
   float l2 = length / 2.0f, l3 = length / 3.0f;
   float h2 = height / 2.0f, h3 = height / 3.0f;
   int i;
   Point3 v;
   DWORD a, b, c, d;

   patch.setVert(0, -w2, -l2, 0.0f);
   patch.setVert(1, w2, -l2, 0.0f);
   patch.setVert(2, w2, l2, 0.0f);
   patch.setVert(3, -w2, l2, 0.0f);
   patch.setVert(4, -w2, -l2, height);
   patch.setVert(5, w2, -l2, height);
   patch.setVert(6, w2, l2, height);
   patch.setVert(7, -w2, l2, height);

   if (textured) {
      float maxW = usePhysUVs ? width : 1.0f;
      float maxL = usePhysUVs ? length : 1.0f;
      float maxH = usePhysUVs ? height : 1.0f;

      patch.setTVert(0, UVVert(maxW, 0.0f, 0.0f));
      patch.setTVert(1, UVVert(maxW, maxH, 0.0f));
      patch.setTVert(2, UVVert(0.0f, maxH, 0.0f));
      patch.setTVert(3, UVVert(0.0f, 0.0f, 0.0f));

      patch.setTVert(4, UVVert(maxL, 0.0f, 0.0f));
      patch.setTVert(5, UVVert(maxL, maxH, 0.0f));
      patch.setTVert(6, UVVert(0.0f, maxH, 0.0f));
      patch.setTVert(7, UVVert(0.0f, 0.0f, 0.0f));

      patch.setTVert(8, UVVert(maxW, 0.0f, 0.0f));
      patch.setTVert(9, UVVert(maxW, maxL, 0.0f));
      patch.setTVert(10, UVVert(0.0f, maxL, 0.0f));
      patch.setTVert(11, UVVert(0.0f, 0.0f, 0.0f));

   }

   int ix = 0;
   for (i = 0; i < 4; i++) {
      v = (patch.verts[(i + 1) % 4] - patch.verts[i]) / 3.0f;
      patch.setVec(ix++, patch.verts[i] + v);
      v = (patch.verts[i + 4] - patch.verts[i]) / 3.0f;
      patch.setVec(ix++, patch.verts[i] + v);
      v = (patch.verts[i == 0 ? 3 : i - 1] - patch.verts[i]) / 3.0f;
      patch.setVec(ix++, patch.verts[i] + v);
   }
   for (i = 0; i < 4; i++) {
      v = (patch.verts[(i + 1) % 4 + 4] - patch.verts[i + 4]) / 3.0f;
      patch.setVec(ix++, patch.verts[i + 4] + v);
      v = (patch.verts[i] - patch.verts[i + 4]) / 3.0f;
      patch.setVec(ix++, patch.verts[i + 4] + v);
      v = (patch.verts[i == 0 ? 7 : i + 3] - patch.verts[i + 4]) / 3.0f;
      patch.setVec(ix++, patch.verts[i + 4] + v);
   }

   int px = 0;
   int tvert[6][4] =
   { {2, 3, 0, 1},
    {6, 7, 4, 5},
    {2, 3, 0, 1},
    {6, 7, 4, 5},
    {8, 9, 10, 11},
    {10, 11, 8, 9} };

   for (i = 0; i < 4; i++) {
      Patch &p = patch.patches[px];
      a = i + 4;
      b = i;
      c = (i + 1) % 4;
      d = (i + 1) % 4 + 4;
      p.SetType(PATCH_QUAD);
      p.setVerts(a, b, c, d);
      p.setVecs(
         Tang(a, 1), Tang(b, 1), Tang(b, 0), Tang(c, 2),
         Tang(c, 1), Tang(d, 1), Tang(d, 2), Tang(a, 0));
      p.setInteriors(ix, ix + 1, ix + 2, ix + 3);
      p.smGroup = 1 << px;
      if (textured)
         patch.getTVPatch(px).setTVerts(tvert[i][0], tvert[i][1], tvert[i][2], tvert[i][3]);

      ix += 4;
      px++;
   }

   a = 0;
   b = 3;
   c = 2;
   d = 1;
   patch.patches[px].SetType(PATCH_QUAD);
   patch.patches[px].setVerts(a, b, c, d);
   patch.patches[px].setVecs(
      Tang(a, 2), Tang(b, 0), Tang(b, 2), Tang(c, 0),
      Tang(c, 2), Tang(d, 0), Tang(d, 2), Tang(a, 0));
   patch.patches[px].setInteriors(ix, ix + 1, ix + 2, ix + 3);
   patch.patches[px].smGroup = 1 << px;
   if (textured)
      patch.getTVPatch(px).setTVerts(tvert[4][0], tvert[4][1], tvert[4][2], tvert[4][3]);
   //watje 3-17-99 to support patch matids
   patch.patches[px].setMatID(1);

   ix += 4;
   px++;

   a = 7;
   b = 4;
   c = 5;
   d = 6;
   patch.patches[px].SetType(PATCH_QUAD);
   patch.patches[px].setVerts(a, b, c, d);
   patch.patches[px].setVecs(
      Tang(a, 0), Tang(b, 2), Tang(b, 0), Tang(c, 2),
      Tang(c, 0), Tang(d, 2), Tang(d, 0), Tang(a, 2));
   patch.patches[px].setInteriors(ix, ix + 1, ix + 2, ix + 3);
   patch.patches[px].smGroup = 1 << px;
   if (textured)
      patch.getTVPatch(px).setTVerts(tvert[5][0], tvert[5][1], tvert[5][2], tvert[5][3]);
   //watje 3-17-99 to support patch matids
   patch.patches[px].setMatID(0);

   patch.patches[0].setMatID(4);
   patch.patches[1].setMatID(3);
   patch.patches[2].setMatID(5);
   patch.patches[3].setMatID(2);

   ix += 4;
   px++;

   if (!patch.buildLinkages())
   {
      assert(0);
   }
   patch.computeInteriors();
   patch.InvalidateGeomCache();
}

Object*
BuildNURBSBox(float width, float length, float height, int genUVs)
{
   int cube_faces[6][4] = { {0, 1, 2, 3}, // bottom
                     {5, 4, 7, 6}, // top
                     {2, 3, 6, 7}, // back
                     {1, 0, 5, 4}, // front
                     {3, 1, 7, 5}, // left
                     {0, 2, 4, 6} };// right
   Point3 cube_verts[8] = { Point3(-0.5, -0.5, 0.0),
                     Point3(0.5, -0.5, 0.0),
                     Point3(-0.5,  0.5, 0.0),
                     Point3(0.5,  0.5, 0.0),
                     Point3(-0.5, -0.5, 1.0),
                     Point3(0.5, -0.5, 1.0),
                     Point3(-0.5,  0.5, 1.0),
                     Point3(0.5,  0.5, 1.0) };
   int faceIDs[6] = { 2, 1, 6, 5, 4, 3 };

   NURBSSet nset;

   for (int face = 0; face < 6; face++) {
      Point3 bl = cube_verts[cube_faces[face][0]];
      Point3 br = cube_verts[cube_faces[face][1]];
      Point3 tl = cube_verts[cube_faces[face][2]];
      Point3 tr = cube_verts[cube_faces[face][3]];

      Matrix3 size;
      size.IdentityMatrix();
      Point3 lwh(width, length, height);
      size.Scale(lwh);

      bl = bl * size;
      br = br * size;
      tl = tl * size;
      tr = tr * size;

      NURBSCVSurface *surf = new NURBSCVSurface();
      nset.AppendObject(surf);
      surf->SetUOrder(4);
      surf->SetVOrder(4);
      surf->SetNumCVs(4, 4);
      surf->SetNumUKnots(8);
      surf->SetNumVKnots(8);

      Point3 top, bot;
      for (int r = 0; r < 4; r++) {
         top = tl + (((float)r / 3.0f) * (tr - tl));
         bot = bl + (((float)r / 3.0f) * (br - bl));
         for (int c = 0; c < 4; c++) {
            NURBSControlVertex ncv;
            ncv.SetPosition(0, bot + (((float)c / 3.0f) * (top - bot)));
            ncv.SetWeight(0, 1.0f);
            TCHAR bname[40];
            TCHAR sname[40];
            _tcscpy(bname, GetString(IDS_RB_BOX));
            _stprintf(sname, _T("%s%s%02dCV%02d"), bname, GetString(IDS_CT_SURF), face, r * 4 + c);
            ncv.SetName(sname);
            surf->SetCV(r, c, ncv);
         }
      }

      for (int k = 0; k < 4; k++) {
         surf->SetUKnot(k, 0.0);
         surf->SetVKnot(k, 0.0);
         surf->SetUKnot(k + 4, 1.0);
         surf->SetVKnot(k + 4, 1.0);
      }

      surf->Renderable(TRUE);
      surf->SetGenerateUVs(genUVs);
      if (height > 0.0f)
         surf->FlipNormals(TRUE);
      else
         surf->FlipNormals(FALSE);

      surf->MatID(faceIDs[face]);

      surf->SetTextureUVs(0, 0, Point2(1.0f, 0.0f));
      surf->SetTextureUVs(0, 1, Point2(0.0f, 0.0f));
      surf->SetTextureUVs(0, 2, Point2(1.0f, 1.0f));
      surf->SetTextureUVs(0, 3, Point2(0.0f, 1.0f));

      TCHAR bname[40];
      TCHAR sname[40];
      _tcscpy(bname, GetString(IDS_RB_BOX));
      _stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_SURF), face);
      surf->SetName(sname);
   }

#define NF(s1, s2, s1r, s1c, s2r, s2c) \
   fuse.mSurf1 = (s1); \
   fuse.mSurf2 = (s2); \
   fuse.mRow1 = (s1r); \
   fuse.mCol1 = (s1c); \
   fuse.mRow2 = (s2r); \
   fuse.mCol2 = (s2c); \
   nset.mSurfFuse.Append(1, &fuse);

   NURBSFuseSurfaceCV fuse;

   // Bottom(0) to Back (2)
   NF(0, 2, 3, 3, 3, 0);
   NF(0, 2, 2, 3, 2, 0);
   NF(0, 2, 1, 3, 1, 0);
   NF(0, 2, 0, 3, 0, 0);

   // Top(1) to Back (2)
   NF(1, 2, 0, 3, 3, 3);
   NF(1, 2, 1, 3, 2, 3);
   NF(1, 2, 2, 3, 1, 3);
   NF(1, 2, 3, 3, 0, 3);

   // Bottom(0) to Front (3)
   NF(0, 3, 0, 0, 3, 0);
   NF(0, 3, 1, 0, 2, 0);
   NF(0, 3, 2, 0, 1, 0);
   NF(0, 3, 3, 0, 0, 0);

   // Top(1) to Front (3)
   NF(1, 3, 3, 0, 3, 3);
   NF(1, 3, 2, 0, 2, 3);
   NF(1, 3, 1, 0, 1, 3);
   NF(1, 3, 0, 0, 0, 3);

   // Bottom(0) to Left (4)
   NF(0, 4, 3, 0, 3, 0);
   NF(0, 4, 3, 1, 2, 0);
   NF(0, 4, 3, 2, 1, 0);
   NF(0, 4, 3, 3, 0, 0);

   // Top(1) to Left (4)
   NF(1, 4, 0, 0, 3, 3);
   NF(1, 4, 0, 1, 2, 3);
   NF(1, 4, 0, 2, 1, 3);
   NF(1, 4, 0, 3, 0, 3);

   // Bottom(0) to Right (5)
   NF(0, 5, 0, 0, 0, 0);
   NF(0, 5, 0, 1, 1, 0);
   NF(0, 5, 0, 2, 2, 0);
   NF(0, 5, 0, 3, 3, 0);

   // Top(1) to Right (5)
   NF(1, 5, 3, 0, 0, 3);
   NF(1, 5, 3, 1, 1, 3);
   NF(1, 5, 3, 2, 2, 3);
   NF(1, 5, 3, 3, 3, 3);

   // Front (3)  to Right (5)
   NF(3, 5, 3, 1, 0, 1);
   NF(3, 5, 3, 2, 0, 2);

   // Right (5) to Back (2)
   NF(5, 2, 3, 1, 0, 1);
   NF(5, 2, 3, 2, 0, 2);

   // Back (2) to Left (4)
   NF(2, 4, 3, 1, 0, 1);
   NF(2, 4, 3, 2, 0, 2);

   // Left (4) to Front (3)
   NF(4, 3, 3, 1, 0, 1);
   NF(4, 3, 3, 2, 0, 2);


   Matrix3 mat;
   mat.IdentityMatrix();
   Object *obj = CreateNURBSObject(NULL, &nset, mat);
   return obj;
}

Object* BoxObject::ConvertToType(TimeValue t, Class_ID obtype)
{
   if (obtype == patchObjectClassID) {
      Interval valid = FOREVER;
      float length, width, height;
      int genUVs;
      pblock2->GetValue(box_length, t, length, valid);
      pblock2->GetValue(box_width, t, width, valid);
      pblock2->GetValue(box_height, t, height, valid);
      pblock2->GetValue(box_mapping, t, genUVs, valid);
      PatchObject *ob = new PatchObject();
      BuildBoxPatch(ob->patch, width, length, height, genUVs, GetUsePhysicalScaleUVs());
      ob->SetChannelValidity(TOPO_CHAN_NUM, valid);
      ob->SetChannelValidity(GEOM_CHAN_NUM, valid);
      ob->UnlockObject();
      return ob;
   }
   if (obtype == EDITABLE_SURF_CLASS_ID) {
      Interval valid = FOREVER;
      float length, width, height;
      int genUVs;
      pblock2->GetValue(box_length, t, length, valid);
      pblock2->GetValue(box_width, t, width, valid);
      pblock2->GetValue(box_height, t, height, valid);
      pblock2->GetValue(box_mapping, t, genUVs, valid);
      Object *ob = BuildNURBSBox(width, length, height, genUVs);
      ob->SetChannelValidity(TOPO_CHAN_NUM, valid);
      ob->SetChannelValidity(GEOM_CHAN_NUM, valid);
      ob->UnlockObject();
      return ob;
   }

   if (obtype == polyObjectClassID) {
      Object *ob = BuildPolyBox(t);
      ob->UnlockObject();
      return ob;
   }

   return __super::ConvertToType(t, obtype);
}

int BoxObject::CanConvertToType(Class_ID obtype)
{
   if (obtype == patchObjectClassID) return 1;
   if (obtype == EDITABLE_SURF_CLASS_ID) return 1;
   if (obtype == polyObjectClassID) return 1;
   return __super::CanConvertToType(obtype);
}

void BoxObject::GetCollapseTypes(Tab<Class_ID> &clist, Tab<TSTR*> &nlist)
{
   __super::GetCollapseTypes(clist, nlist);

   Class_ID id = EDITABLE_SURF_CLASS_ID;
   TSTR *name = new TSTR(GetString(IDS_SM_NURBS_SURFACE));
   clist.Append(1, &id);
   nlist.Append(1, &name);
}

class BoxObjCreateCallBack : public CreateMouseCallBack {
   BoxObject *ob;
   Point3 p0, p1;
   IPoint2 sp0, sp1;
   BOOL square;
public:
   int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat);
   void SetObj(BoxObject *obj) { ob = obj; }
};

int BoxObjCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) {
   if (!vpt || !vpt->IsAlive())
   {
      // why are we here
      DbgAssert(!_T("Invalid viewport!"));
      return FALSE;
   }
   bool createCube = box_crtype_blk.GetInt(box_create_meth) == 1;

   Point3 d;
   if (msg == MOUSE_FREEMOVE)
   {
      vpt->SnapPreview(m, m, NULL, SNAP_IN_3D);
   }

   else if (msg == MOUSE_POINT || msg == MOUSE_MOVE) {
      switch (point) {
      case 0:
         sp0 = m;
         ob->suspendSnap = TRUE;
         p0 = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);
         p1 = p0 + Point3(.01, .01, .01);
         mat.SetTrans(float(.5)*(p0 + p1));
         {
            Point3 xyz = mat.GetTrans();
            xyz.z = p0.z;
            mat.SetTrans(xyz);
         }
         break;
      case 1:
         sp1 = m;
         p1 = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);
         p1.z = p0.z + (float).01;
         if (createCube || (flags&MOUSE_CTRL)) {
            mat.SetTrans(p0);
         }
         else {
            mat.SetTrans(float(.5)*(p0 + p1));
            Point3 xyz = mat.GetTrans();
            xyz.z = p0.z;
            mat.SetTrans(xyz);
         }
         d = p1 - p0;

         square = FALSE;
         if (createCube) {
            // Constrain to cube
            d.x = d.y = d.z = Length(d)*2.0f;
         }
         else
            if (flags&MOUSE_CTRL) {
               // Constrain to square base
               float len;
               if (fabs(d.x) > fabs(d.y)) len = d.x;
               else len = d.y;
               d.x = d.y = 2.0f * len;
               square = TRUE;
            }

         ob->pblock2->SetValue(box_width, 0, float(fabs(d.x)));
         ob->pblock2->SetValue(box_length, 0, float(fabs(d.y)));
         ob->pblock2->SetValue(box_height, 0, float(fabs(d.z)));


         if (msg == MOUSE_POINT && createCube) {
            ob->suspendSnap = FALSE;
            return (Length(sp1 - sp0) < 3) ? CREATE_ABORT : CREATE_STOP;
         }
         else if (msg == MOUSE_POINT &&
            (Length(sp1 - sp0) < 3 || Length(d) < 0.1f)) {
            return CREATE_ABORT;
         }
         break;
      case 2:
         p1.z = p0.z + vpt->SnapLength(vpt->GetCPDisp(p0, Point3(0, 0, 1), sp1, m, TRUE));
         if (!square) {
            mat.SetTrans(float(.5)*(p0 + p1));
            mat.SetTrans(2, p0.z); // set the Z component of translation
         }

         d = p1 - p0;
         if (square) {
            // Constrain to square base
            float len;
            if (fabs(d.x) > fabs(d.y)) len = d.x;
            else len = d.y;
            d.x = d.y = 2.0f * len;
         }

         ob->pblock2->SetValue(box_width, 0, float(fabs(d.x)));
         ob->pblock2->SetValue(box_length, 0, float(fabs(d.y)));
         ob->pblock2->SetValue(box_height, 0, float(d.z));
 

         if (msg == MOUSE_POINT) {
            ob->suspendSnap = FALSE;
            return CREATE_STOP;
         }
         break;
      }
   }
   else
      if (msg == MOUSE_ABORT) {
         return CREATE_ABORT;
      }

   return TRUE;
}

static BoxObjCreateCallBack boxCreateCB;

CreateMouseCallBack* BoxObject::GetCreateMouseCallBack() {
   boxCreateCB.SetObj(this);
   return(&boxCreateCB);
}

BOOL BoxObject::OKtoDisplay(TimeValue t)
{
   return TRUE;
}

void BoxObject::InvalidateUI()
{
   box_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
}

RefTargetHandle BoxObject::Clone(RemapDir& remap)
{
   BoxObject* newob = new BoxObject(FALSE);
   newob->ReplaceReference(0, remap.CloneRef(pblock2));
   newob->mPolyBoxSmoothingGroupFix = mPolyBoxSmoothingGroupFix;
   newob->ivalid.SetEmpty();
   BaseClone(this, newob, remap);
   newob->SetUsePhysicalScaleUVs(GetUsePhysicalScaleUVs());
   return(newob);
}

void MakePQuad(MNFace *mf, int v1, int v2, int v3, int v4, DWORD smG, MtlID mt, int bias) {
   int vv[4];
   vv[0] = v1;
   vv[1 + bias] = v2;
   vv[2] = v3;
   vv[3 - bias] = v4;
   mf->MakePoly(4, vv);
   mf->smGroup = smG;
   mf->material = mt;
}

void MakeMapQuad(MNMapFace *mf, int v1, int v2, int v3, int v4, int bias) {
   int vv[4];
   vv[0] = v1;
   vv[1 + bias] = v2;
   vv[2] = v3;
   vv[3 - bias] = v4;
   mf->MakePoly(4, vv);
}

// NOTE: these separate macros for different surfaces spell out the smoothing
// group and material ID for each surface.
#define MAKE_TOP_QUAD(v1,v2,v3,v4) MakePQuad(mm.F(nf),v1,v2,v3,v4,(1<<1),0,bias)
#define MAKE_BOTTOM_QUAD(v1,v2,v3,v4) MakePQuad(mm.F(nf),v1,v2,v3,v4,(1<<2),1,bias)
#define MAKE_LEFT_QUAD(v1,v2,v3,v4) MakePQuad(mm.F(nf),v1,v2,v3,v4,(1<<6),2,bias)
#define MAKE_RIGHT_QUAD(v1,v2,v3,v4) MakePQuad(mm.F(nf),v1,v2,v3,v4,(1<<4),3,bias)
#define MAKE_FRONT_QUAD(v1,v2,v3,v4) MakePQuad(mm.F(nf),v1,v2,v3,v4,(1<<3),4,bias)
#define MAKE_BACK_QUAD(v1,v2,v3,v4) MakePQuad(mm.F(nf),v1,v2,v3,v4,(1<<5),5,bias)
#define MAKE_MAP_QUAD(v1,v2,v3,v4) if(tf)MakeMapQuad(&(tf[nf]),v1,v2,v3,v4,bias);

// Fix for Max 5.1: PolyObjects had been coming in with the top & bottom smoothing groups reversed.
#define MAKE_TOP_QUAD_FIX(v1,v2,v3,v4) MakePQuad(mm.F(nf),v1,v2,v3,v4,(1<<2),0,bias)
#define MAKE_BOTTOM_QUAD_FIX(v1,v2,v3,v4) MakePQuad(mm.F(nf),v1,v2,v3,v4,(1<<1),1,bias)

Object *BoxObject::BuildPolyBox(TimeValue t) {
   PolyObject *pobj = new PolyObject();
   MNMesh & mm = pobj->mm;
   int wsegs, lsegs, hsegs;
   float l, w, h;
   int genUVs = 1;
   int bias = 0;

   // Start the validity interval at forever and widdle it down.
   Interval gValid, tValid;
   tValid.SetInfinite();
   pblock2->GetValue(box_lsegs, t, lsegs, tValid);
   pblock2->GetValue(box_wsegs, t, wsegs, tValid);
   pblock2->GetValue(box_hsegs, t, hsegs, tValid);
   pblock2->GetValue(box_mapping, t, genUVs, tValid);
   LimitValue(lsegs, MIN_SEGMENTS, MAX_SEGMENTS);
   LimitValue(wsegs, MIN_SEGMENTS, MAX_SEGMENTS);
   LimitValue(hsegs, MIN_SEGMENTS, MAX_SEGMENTS);

   gValid = tValid;
   pblock2->GetValue(box_length, t, l, gValid);
   pblock2->GetValue(box_width, t, w, gValid);
   pblock2->GetValue(box_height, t, h, gValid);
   if (h < 0.0f) bias = 2;

   ChannelMask tParts = genUVs ? TOPO_CHANNEL | TEXMAP_CHANNEL : TOPO_CHANNEL;
   ChannelMask otherStuff = OBJ_CHANNELS & ~(GEOM_CHANNEL | tParts);
   pobj->SetPartValidity(otherStuff, FOREVER);
   pobj->SetPartValidity(GEOM_CHANNEL, gValid);
   pobj->SetPartValidity(tParts, tValid);

   // Number of verts
      // bottom : (lsegs+1)*(wsegs+1)
     // top    : (lsegs+1)*(wsegs+1)
     // sides  : (2*lsegs+2*wsegs)*(hsegs-1)

   // Number of rectangular faces.
      // bottom : (lsegs)*(wsegs)
     // top    : (lsegs)*(wsegs)
     // sides  : 2*(hsegs*lsegs)+2*(wsegs*lsegs)

   mm.Clear();

   int wsp1 = wsegs + 1;
   int nlayer = 2 * (lsegs + wsegs);
   int topStart = (lsegs + 1)*(wsegs + 1);
   int midStart = 2 * topStart;

   int nverts = midStart + nlayer*(hsegs - 1);
   int nfaces = 2 * (lsegs*wsegs + hsegs*lsegs + wsegs*hsegs);

   mm.setNumVerts(nverts);
   mm.setNumFaces(nfaces);
   mm.InvalidateTopoCache();

   // Do mapping verts first, since they're easy.
   int uvStart[6];
   int ix, iy, iz;
   int nv;
   MNMapFace *tf = NULL;
   if (genUVs) {
      int ls = lsegs + 1;
      int ws = wsegs + 1;
      int hs = hsegs + 1;
      int ntverts = 2 * (ls*hs + hs*ws + ws*ls);
      mm.SetMapNum(2);
      mm.M(1)->ClearFlag(MN_DEAD);
      mm.M(1)->setNumFaces(nfaces);
      mm.M(1)->setNumVerts(ntverts);
      UVVert *tv = mm.M(1)->v;
      tf = mm.M(1)->f;

      int xbase = 0;
      int ybase = ls*hs;
      int zbase = ls*hs + hs*ws;

      BOOL usePhysUVs = GetUsePhysicalScaleUVs();
      float maxW = usePhysUVs ? w : 1.0f;
      float maxL = usePhysUVs ? l : 1.0f;
      float maxH = usePhysUVs ? h : 1.0f;

      float dw = maxW / float(wsegs);
      float dl = maxL / float(lsegs);
      float dh = maxH / float(hsegs);

      if (w == 0.0f) w = .0001f;
      if (l == 0.0f) l = .0001f;
      if (h == 0.0f) h = .0001f;
      float u, v;

      // Bottom of box.
      nv = 0;
      uvStart[0] = nv;
      v = 0.0f;
      for (iy = 0; iy < ls; iy++) {
         u = 1.0f;
         for (ix = 0; ix < ws; ix++) {
            tv[nv] = UVVert(u, v, 0.0f);
            nv++; u -= dw;
         }
         v += dl;
      }

      // Top of box.
      uvStart[1] = nv;
      v = 0.0f;
      for (iy = 0; iy < ls; iy++) {
         u = 0.0f;
         for (ix = 0; ix < ws; ix++) {
            tv[nv] = UVVert(u, v, 0.0f);
            nv++; u += dw;
         }
         v += dl;
      }

      // Front Face
      uvStart[2] = nv;
      v = 0.0f;
      for (iz = 0; iz < hs; iz++) {
         u = 0.0f;
         for (ix = 0; ix < ws; ix++) {
            tv[nv] = UVVert(u, v, 0.0f);
            nv++; u += dw;
         }
         v += dh;
      }

      // Right Face
      uvStart[3] = nv;
      v = 0.0f;
      for (iz = 0; iz < hs; iz++) {
         u = 0.0f;
         for (iy = 0; iy < ls; iy++) {
            tv[nv] = UVVert(u, v, 0.0f);
            nv++; u += dl;
         }
         v += dh;
      }

      // Back Face
      uvStart[4] = nv;
      v = 0.0f;
      for (iz = 0; iz < hs; iz++) {
         u = 0.0f;
         for (ix = 0; ix < ws; ix++) {
            tv[nv] = UVVert(u, v, 0.0f);
            nv++; u += dw;
         }
         v += dh;
      }

      // Left Face
      uvStart[5] = nv;
      v = 0.0f;
      for (iz = 0; iz < hs; iz++) {
         u = 0.0f;
         for (iy = 0; iy < ls; iy++) {
            tv[nv] = UVVert(u, v, 0.0f);
            nv++; u += dl;
         }
         v += dh;
      }

      assert(nv == ntverts);
   }

   nv = 0;

   Point3 vb(w / 2.0f, l / 2.0f, h);
   Point3 va(-vb.x, -vb.y, 0.0f);

   float dx = w / wsegs;
   float dy = l / lsegs;
   float dz = h / hsegs;

   // do bottom vertices.
   Point3 p;
   p.z = va.z;
   p.y = va.y;
   for (iy = 0; iy <= lsegs; iy++) {
      p.x = va.x;
      for (ix = 0; ix <= wsegs; ix++) {
         mm.P(nv) = p;
         nv++;
         p.x += dx;
      }
      p.y += dy;
   }

   int kv, nf = 0;

   // do bottom faces.
   // (Note that mapping verts are indexed the same as regular verts on the bottom.)
   for (iy = 0; iy < lsegs; iy++) {
      kv = iy*(wsegs + 1);
      for (ix = 0; ix < wsegs; ix++) {
         if (mPolyBoxSmoothingGroupFix) {
            MAKE_BOTTOM_QUAD_FIX(kv, kv + wsegs + 1, kv + wsegs + 2, kv + 1);
         }
         else {
            MAKE_BOTTOM_QUAD(kv, kv + wsegs + 1, kv + wsegs + 2, kv + 1);
         }
         MAKE_MAP_QUAD(kv, kv + wsegs + 1, kv + wsegs + 2, kv + 1);
         nf++;
         kv++;
      }
   }
   assert(nf == lsegs*wsegs);

   // do top vertices.
   // (Note that mapping verts are indexed the same as regular verts on the top.)
   p.z = vb.z;
   p.y = va.y;
   for (iy = 0; iy <= lsegs; iy++) {
      p.x = va.x;
      for (ix = 0; ix <= wsegs; ix++) {
         mm.P(nv) = p;
         p.x += dx;
         nv++;
      }
      p.y += dy;
   }

   // do top faces (lsegs*wsegs);
   for (iy = 0; iy < lsegs; iy++) {
      kv = iy*(wsegs + 1) + topStart;
      for (ix = 0; ix < wsegs; ix++) {
         if (mPolyBoxSmoothingGroupFix) {
            MAKE_TOP_QUAD_FIX(kv, kv + 1, kv + wsegs + 2, kv + wsegs + 1);
         }
         else {
            MAKE_TOP_QUAD(kv, kv + 1, kv + wsegs + 2, kv + wsegs + 1);
         }
         MAKE_MAP_QUAD(kv, kv + 1, kv + wsegs + 2, kv + wsegs + 1);
         nf++;
         kv++;
      }
   }
   assert(nf == lsegs*wsegs * 2);

   // do middle vertices
   for (iz = 1; iz < hsegs; iz++) {
      p.z = va.z + dz * iz;
      // front edge
      p.x = va.x;  p.y = va.y;
      for (ix = 0; ix < wsegs; ix++) { mm.P(nv) = p; p.x += dx; nv++; }
      // right edge
      p.x = vb.x;   p.y = va.y;
      for (iy = 0; iy < lsegs; iy++) { mm.P(nv) = p; p.y += dy; nv++; }
      // back edge
      p.x = vb.x;  p.y = vb.y;
      for (ix = 0; ix < wsegs; ix++) { mm.P(nv) = p; p.x -= dx; nv++; }
      // left edge
      p.x = va.x;  p.y = vb.y;
      for (iy = 0; iy < lsegs; iy++) { mm.P(nv) = p; p.y -= dy; nv++; }
   }

   int mv;
   if (hsegs == 1) {
      // do FRONT faces -----------------------
      kv = 0;
      mv = topStart;
      for (ix = 0; ix < wsegs; ix++) {
         MAKE_FRONT_QUAD(kv, kv + 1, mv + 1, mv);
         if (tf) {
            int mapv = uvStart[2] + ix;
            MAKE_MAP_QUAD(mapv, mapv + 1, mapv + wsegs + 2, mapv + wsegs + 1);
         }
         nf++;
         kv++;
         mv++;
      }

      // do RIGHT faces.-----------------------
      kv = wsegs;
      mv = topStart + kv;
      for (iy = 0; iy < lsegs; iy++) {
         MAKE_RIGHT_QUAD(kv, kv + wsp1, mv + wsp1, mv);
         if (tf) {
            int mapv = uvStart[3] + iy;
            MAKE_MAP_QUAD(mapv, mapv + 1, mapv + lsegs + 2, mapv + lsegs + 1);
         }
         nf++;
         kv += wsp1;
         mv += wsp1;
      }

      // do BACK faces.-----------------------
      kv = topStart - 1;
      mv = midStart - 1;
      for (ix = 0; ix < wsegs; ix++) {
         MAKE_BACK_QUAD(kv, kv - 1, mv - 1, mv);
         if (tf) {
            int mapv = uvStart[4] + ix;
            MAKE_MAP_QUAD(mapv, mapv + 1, mapv + wsegs + 2, mapv + wsegs + 1);
         }
         nf++;
         kv--;
         mv--;
      }

      // do LEFT faces.----------------------
      kv = lsegs*(wsegs + 1);  // index into bottom
      mv = topStart + kv;
      for (iy = 0; iy < lsegs; iy++) {
         MAKE_LEFT_QUAD(kv, kv - wsp1, mv - wsp1, mv);
         if (tf) {
            int mapv = uvStart[5] + iy;
            MAKE_MAP_QUAD(mapv, mapv + 1, mapv + lsegs + 2, mapv + lsegs + 1);
         }
         nf++;
         kv -= wsp1;
         mv -= wsp1;
      }
   }
   else {
      // do FRONT faces.
      kv = 0;
      mv = midStart;
      for (iz = 0; iz < hsegs; iz++) {
         if (iz == hsegs - 1) mv = topStart;
         for (ix = 0; ix < wsegs; ix++) {
            MAKE_FRONT_QUAD(kv + ix, kv + ix + 1, mv + ix + 1, mv + ix);
            if (tf) {
               int mapv = uvStart[2] + iz*(wsegs + 1) + ix;
               MAKE_MAP_QUAD(mapv, mapv + 1, mapv + wsegs + 2, mapv + wsegs + 1);
            }
            nf++;
         }
         kv = mv;
         mv += nlayer;
      }

      assert(nf == lsegs*wsegs * 2 + wsegs*hsegs);

      // do RIGHT faces.-------------------------
      // RIGHT bottom row:
      kv = wsegs; // into bottom layer. 
      mv = midStart + wsegs; // first layer of mid verts


      for (iy = 0; iy < lsegs; iy++) {
         MAKE_RIGHT_QUAD(kv, kv + wsp1, mv + 1, mv);
         if (tf) {
            int mapv = uvStart[3] + iy;
            MAKE_MAP_QUAD(mapv, mapv + 1, mapv + lsegs + 2, mapv + lsegs + 1);
         }
         nf++;
         kv += wsp1;
         mv++;
      }

      // RIGHT middle part:
      kv = midStart + wsegs;
      for (iz = 0; iz < hsegs - 2; iz++) {
         mv = kv + nlayer;
         for (iy = 0; iy < lsegs; iy++) {
            MAKE_RIGHT_QUAD(kv + iy, kv + iy + 1, mv + iy + 1, mv + iy);
            if (tf) {
               int mapv = uvStart[3] + (iz + 1)*(lsegs + 1) + iy;
               MAKE_MAP_QUAD(mapv, mapv + 1, mapv + lsegs + 2, mapv + lsegs + 1);
            }
            nf++;
         }
         kv += nlayer;
      }

      // RIGHT top row:
      kv = midStart + wsegs + (hsegs - 2)*nlayer;
      mv = topStart + wsegs;
      for (iy = 0; iy < lsegs; iy++) {
         MAKE_RIGHT_QUAD(kv, kv + 1, mv + wsp1, mv);
         if (tf) {
            int mapv = uvStart[3] + (hsegs - 1)*(lsegs + 1) + iy;
            MAKE_MAP_QUAD(mapv, mapv + 1, mapv + lsegs + 2, mapv + lsegs + 1);
         }
         nf++;
         mv += wsp1;
         kv++;
      }

      assert(nf == lsegs*wsegs * 2 + wsegs*hsegs + lsegs*hsegs);

      // do BACK faces. ---------------------
      // BACK bottom row:
      kv = topStart - 1;
      mv = midStart + wsegs + lsegs;
      for (ix = 0; ix < wsegs; ix++) {
         MAKE_BACK_QUAD(kv, kv - 1, mv + 1, mv);
         if (tf) {
            int mapv = uvStart[4] + ix;
            MAKE_MAP_QUAD(mapv, mapv + 1, mapv + wsegs + 2, mapv + wsegs + 1);
         }
         nf++;
         kv--;
         mv++;
      }

      // BACK middle part:
      kv = midStart + wsegs + lsegs;
      for (iz = 0; iz < hsegs - 2; iz++) {
         mv = kv + nlayer;
         for (ix = 0; ix < wsegs; ix++) {
            MAKE_BACK_QUAD(kv + ix, kv + ix + 1, mv + ix + 1, mv + ix);
            if (tf) {
               int mapv = uvStart[4] + (iz + 1)*(wsegs + 1) + ix;
               MAKE_MAP_QUAD(mapv, mapv + 1, mapv + wsegs + 2, mapv + wsegs + 1);
            }
            nf++;
         }
         kv += nlayer;
      }

      // BACK top row:
      kv = midStart + wsegs + lsegs + (hsegs - 2)*nlayer;
      mv = topStart + lsegs*(wsegs + 1) + wsegs;
      for (ix = 0; ix < wsegs; ix++) {
         MAKE_BACK_QUAD(kv, kv + 1, mv - 1, mv);
         if (tf) {
            int mapv = uvStart[4] + (wsegs + 1)*(hsegs - 1) + ix;
            MAKE_MAP_QUAD(mapv, mapv + 1, mapv + wsegs + 2, mapv + wsegs + 1);
         }
         nf++;
         mv--;
         kv++;
      }

      assert(nf == lsegs*wsegs * 2 + wsegs*hsegs * 2 + lsegs*hsegs);

      // do LEFT faces. -----------------
      // LEFT bottom row:
      kv = lsegs*(wsegs + 1);  // index into bottom
      mv = midStart + 2 * wsegs + lsegs;
      for (iy = 0; iy < lsegs; iy++) {
         int nextm = mv + 1;
         if (iy == lsegs - 1) nextm -= nlayer;
         MAKE_LEFT_QUAD(kv, kv - wsp1, nextm, mv);
         if (tf) {
            int mapv = uvStart[5] + iy;
            MAKE_MAP_QUAD(mapv, mapv + 1, mapv + lsegs + 2, mapv + lsegs + 1);
         }
         nf++;
         kv -= wsp1;
         mv++;
      }

      // LEFT middle part:
      kv = midStart + 2 * wsegs + lsegs;
      for (iz = 0; iz < hsegs - 2; iz++) {
         mv = kv + nlayer;
         for (iy = 0; iy < lsegs; iy++) {
            int nextm = mv + 1;
            int nextk = kv + iy + 1;
            if (iy == lsegs - 1) {
               nextm -= nlayer;
               nextk -= nlayer;
            }
            MAKE_LEFT_QUAD(kv + iy, nextk, nextm, mv);
            if (tf) {
               int mapv = uvStart[5] + (iz + 1)*(lsegs + 1) + iy;
               MAKE_MAP_QUAD(mapv, mapv + 1, mapv + lsegs + 2, mapv + lsegs + 1);
            }
            nf++;
            mv++;
         }
         kv += nlayer;
      }

      // LEFT top row:
      kv = midStart + 2 * wsegs + lsegs + (hsegs - 2)*nlayer;
      mv = topStart + lsegs*(wsegs + 1);
      for (iy = 0; iy < lsegs; iy++) {
         int nextk = kv + 1;
         if (iy == lsegs - 1) nextk -= nlayer;
         MAKE_LEFT_QUAD(kv, nextk, mv - wsp1, mv);
         if (tf) {
            int mapv = uvStart[5] + (hsegs - 1)*(lsegs + 1) + iy;
            MAKE_MAP_QUAD(mapv, mapv + 1, mapv + lsegs + 2, mapv + lsegs + 1);
         }
         nf++;
         mv -= wsp1;
         kv++;
      }
   }

   //Make sure the MNMesh caches (both geometry and topology) are clean before returning the object
   mm.InvalidateGeomCache();
   mm.InvalidateTopoCache();
   mm.FillInMesh();
   return (Object *)pobj;
}

void BoxObject::UpdateUI()
{
   if (ip == NULL)
      return;
   BoxParamDlgProc* dlg = static_cast<BoxParamDlgProc*>(box_param_blk.GetUserDlgProc());
   dlg->UpdateUI();
}

BOOL BoxObject::GetUsePhysicalScaleUVs()
{
   return ::GetUsePhysicalScaleUVs(this);
}

void BoxObject::SetUsePhysicalScaleUVs(BOOL flag)
{
   BOOL curState = GetUsePhysicalScaleUVs();
   if (curState == flag)
      return;
   if (theHold.Holding())
      theHold.Put(new RealWorldScaleRecord<BoxObject>(this, curState));
   ::SetUsePhysicalScaleUVs(this, flag);
   if (pblock2 != NULL)
      pblock2->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
   UpdateUI();
   macroRec->SetProperty(this, _T("realWorldMapSize"), mr_bool, flag);
}

