//////////////////////////////////////////////////////////////////////////////
//
//    Strauss Shader plug-in, implementation
//
//    Created: 10/26/98 Kells Elmquist
//
#include "gport.h"
#include "shaders.h"
#include "shaderUtil.h"
#include "macrorec.h"
#include "..\NifProps\NifProps.h"
#include "3dsmaxport.h"
#include "../iNifProps.h"
#include "../materialfile.h"

extern TCHAR *GetString(int id);
extern TSTR shortDescription;

// Class Ids
const Class_ID FO4SHADER_CLASS_ID(0x7a6bc2e7, 0x71106f41);

enum { ref_base, ref_mtl, ref_bgsm, ref_bgem, ref_activemtl, ref_oldmtl, MAX_REFERENCES };

// Paramblock2 name
enum { shader_params, };
// Paramblock2 parameter list

const EnumLookupType TransparencyModes[] = {
   { 0, TEXT("ONE")},
   { 1, TEXT("ZERO")},
   { 2, TEXT("SRCCOLOR")},
   { 3, TEXT("INVSRCCOLOR")},
   { 4, TEXT("DESTCOLOR")},
   { 5, TEXT("INVDESTCOLOR")},
   { 6, TEXT("SRCALPHA")},
   { 7, TEXT("INVSRCALPHA")},
   { 8, TEXT("DESTALPHA")},
   { 9, TEXT("INVDESTALPHA")},
   {10, TEXT("SRCALPHASAT")},
   {-1, nullptr},
};
const EnumLookupType TestModes[] = {
   { 0, TEXT("ALWAYS")},
   { 1, TEXT("LESS")},
   { 2, TEXT("EQUAL")},
   { 3, TEXT("LESSEQUAL")},
   { 4, TEXT("GREATER")},
   { 5, TEXT("NOTEQUAL")},
   { 6, TEXT("GREATEREQUAL")},
   { 7, TEXT("NEVER")},
   {-1, nullptr},
};
const EnumLookupType ApplyModes[] = {
   { 0, TEXT("REPLACE")},
   { 1, TEXT("DECAL")},
   { 2, TEXT("MODULATE")},
   {-1, nullptr},
};

const EnumLookupType VertexModes[] = {
   { 0, TEXT("IGNORE")},
   { 1, TEXT("EMISSIVE")},
   { 2, TEXT("AMB_DIFF")},
   {-1, nullptr},
};
const EnumLookupType LightModes[] = {
   { 0, TEXT("E")},
   { 1, TEXT("E_A_D")},
   {-1, nullptr},
};

const EnumLookupType MaterialFileTypes[] = {
   { MFT_BGSM, TEXT("BGSM - Lighting Shader")},
   { MFT_BGEM, TEXT("BGEM - Effect Shader")},
   {-1, nullptr},
};


struct TexChannel
{
	DWORD channelName;
	DWORD maxName;
	DWORD channelType;
};

static const TexChannel texChannelNames[STD2_NMAX_TEXMAPS] = {
   {IDS_CHAN_BASE,      IDS_MAXCHAN_DIFFUSE,       CLR_CHANNEL    },             //C_DIFFUSE,
   {IDS_CHAN_NORMAL,    IDS_MAXCHAN_NORMAL,        CLR_CHANNEL    },			 //C_NORMAL,
   {IDS_CHAN_SPECULAR,  IDS_MAXCHAN_DETAIL,        CLR_CHANNEL    },			 //C_SMOOTHSPEC,
   {IDS_CHAN_GREYSCALE, IDS_CHAN_GREYSCALE,        CLR_CHANNEL    },			 //C_GREYSCALE,
   {IDS_CHAN_ENV,       IDS_CHAN_ENV,              CLR_CHANNEL    },			 //C_ENVMAP,
   {IDS_CHAN_GLOW,      IDS_MAXCHAN_GLOW,          CLR_CHANNEL    },			 //C_GLOW,
   {IDS_CHAN_INNER,     IDS_CHAN_INNER,            CLR_CHANNEL    },			 //C_INNERLAYER,
   {IDS_CHAN_WRINKLES,  IDS_CHAN_WRINKLES,         CLR_CHANNEL    },			 //C_WRINKLES,
   {IDS_CHAN_DISPLACE,  IDS_CHAN_DISPLACE,         CLR_CHANNEL    },			 //C_DISPLACE,
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
};

enum
{
	C_DIFFUSE,
	C_NORMAL,
	C_SMOOTHSPEC,
	C_GREYSCALE,
	C_ENVMAP,
	C_GLOW,
	C_INNERLAYER,
	C_WRINKLES,
	C_DISPLACE,
	C_MAX_SUPPORTED,
};



// map from custom channel to standard map
static const int FO4ShaderStdIDToChannel[] = {
   -1,           // 0 - ambient
   C_DIFFUSE,    // 1 - diffuse           
   C_SMOOTHSPEC, // 2 - specular
   -1,           // 3 - Glossiness (Shininess in 3ds Max release 2.0 and earlier)
   -1,           // 4 - Specular Level (Shininess strength in 3ds Max release 2.0 and earlier)
   -1,       // 5 - self-illumination 
   -1,           // 6 - opacity
   -1,           // 7 - filter color
	-1,    // 8 - bump              
   -1,           // 9 - reflection        
   -1,           // 10 - refraction 
	-1,          // 11 - displacement
	-1,           // 12 - 
	-1,           // 13 -  
	-1,           // 14 -  
	-1,           // 15 -  
	-1,           // 16 -  
	-1,           // 17 -  
	-1,           // 18 -  
	-1,           // 19 -  
	-1,           // 21 -  
	-1,           // 22 -  
	-1,           // 23 -  
};

const ULONG SHADER_PARAMS = (STD_PARAM_SELFILLUM | STD_PARAM_SELFILLUM_CLR
	| STD_PARAM_SPECULAR_CLR | STD_PARAM_GLOSSINESS
	| STD_PARAM_SELFILLUM_CLR_ON
	);

class FO4ShaderDlg;
class FO4ShaderDlg;

class FO4Shader : public Shader, IBSShaderMaterialData {
	friend class FO4ShaderCB;
	friend class FO4ShaderDlg;
	friend class FO4ShaderDlg;
	friend class FO4ShaderBaseRollup;
	friend class FO4ShaderMtlRollup;
	friend class FO4ShaderBGSMRollup;
	friend class FO4ShaderBGEMRollup;
	BOOL rolloutOpen;
public:
	IParamBlock2      *pb_base;     // ref 0
	IParamBlock2      *pb_mtl;      // ref 1
	IParamBlock2      *pb_bgsm;     // ref 2
	IParamBlock2      *pb_bgem;     // ref 3
	MaterialReference *pMtlFileRef; // ref 4
	MaterialReference *pMtlFileRefOld; // ref 5
	Interval    ivalid;
	FO4ShaderDlg* pDlg;

public:
	FO4Shader();
	~FO4Shader();
	ULONG SupportStdParams() override { return SHADER_PARAMS; }

	// copy std params, for switching shaders
	void CopyStdParams(Shader* pFrom) override;

	// texture maps
	long nTexChannelsSupported() override { return STD2_NMAX_TEXMAPS - 4; }
	TSTR GetTexChannelName(long nChan) override { return GetString(texChannelNames[nChan].channelName); }
	TSTR GetTexChannelInternalName(long nChan) override { return GetString(texChannelNames[nChan].maxName); }
	long ChannelType(long nChan) override { return texChannelNames[nChan].channelType; }
	long StdIDToChannel(long stdID) override { return FO4ShaderStdIDToChannel[stdID]; }

	//BOOL KeyAtTime(int id,TimeValue t) { return pb->KeyFrameAtTime(id,t); }
	ULONG GetRequirements(int subMtlNum) override  { return MTLREQ_TRANSP|MTLREQ_PHONG; }

	int NParamDlgs() override;
	ShaderParamDlg* GetParamDlg(int n) override;
	void SetParamDlg(ShaderParamDlg* newDlg, int n) override;
	ShaderParamDlg* CreateParamDialog(HWND hOldRollup, HWND hwMtlEdit, IMtlParams *imp, StdMat2* theMtl, int rollupOpen, int) override;

	Class_ID ClassID() override { return FO4SHADER_CLASS_ID; }
	SClass_ID SuperClassID() override { return SHADER_CLASS_ID; }
	TSTR GetName() override { return GetString(IDS_FO4_SHADER); }
	void GetClassName(TSTR& s) override { s = GetName(); }
	void DeleteThis() override { delete this; }

	int NumSubs() override { return 1; }

	Animatable* SubAnim(int i) override;
	TSTR SubAnimName(int i) override;

	int SubNumToRefNum(int subNum) override { return subNum; }

	// add direct ParamBlock2 access
	int   NumParamBlocks() override { return 4; }
	IParamBlock2* GetParamBlock(int i) override;
	IParamBlock2* GetParamBlockByID(BlockID id) override;
	int NumRefs() override;

	RefTargetHandle GetReference(int i) override;
	void SetReference(int i, RefTargetHandle rtarg) override;
	void NotifyChanged() { NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE); }

	void* GetInterface(ULONG id) override {
		return (id == I_BSSHADERDATA) ? static_cast<IBSShaderMaterialData*>(this) : Shader::GetInterface(id);
	}

	IOResult Load(ILoad* iload) override;
	IOResult Save(ISave* isave) override;

	void Update(TimeValue t, Interval& valid) override;
	void Reset() override;
	RefTargetHandle Clone(RemapDir &remap /*=DefaultRemapDir()*/) override;
#if VERSION_3DSMAX < (17000<<16) // Version 17 (2015)
	RefResult	NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message) override;
#else
	RefResult	NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate) override;
#endif

	void GetIllumParams(ShadeContext &sc, IllumParams& ip) override;

	// Strauss Shader specific section
	void  Illum(ShadeContext &sc, IllumParams &ip) override;

	// std params not supported
	void SetLockDS(BOOL lock)  override { }
	BOOL GetLockDS()  override { return FALSE; }
	void SetLockAD(BOOL lock)  override { }
	BOOL GetLockAD()  override { return FALSE; }
	void SetLockADTex(BOOL lock)  override { }
	BOOL GetLockADTex()  override { return FALSE; }

	virtual void SetSelfIllum(float v, TimeValue t)  override { }
	virtual void SetSelfIllumClrOn(BOOL on)  override { }
	virtual void SetSelfIllumClr(Color c, TimeValue t) override { }
	virtual void SetAmbientClr(Color c, TimeValue t)  override { }
	virtual void SetDiffuseClr(Color c, TimeValue t)  override { }
	virtual void SetSpecularClr(Color c, TimeValue t)  override { }
	virtual void SetGlossiness(float v, TimeValue t)  override { }
	virtual void SetSpecularLevel(float v, TimeValue t)  override { }
	virtual void SetSoftenLevel(float v, TimeValue t)  override { }

	virtual BOOL IsSelfIllumClrOn(int mtlNum, BOOL backFace)  override { return FALSE; }
	virtual Color GetAmbientClr(int mtlNum = 0, BOOL backFace = 0)  override { return Color(0.5f, 0.5f, 0.5f);; }
	virtual Color GetDiffuseClr(int mtlNum = 0, BOOL backFace = 0)  override { return Color(0.5f, 0.5f, 0.5f);; }
	virtual Color GetSpecularClr(int mtlNum = 0, BOOL backFace = 0)  override { return Color(0.5f, 0.5f, 0.5f);; }
	virtual Color GetSelfIllumClr(int mtlNum = 0, BOOL backFace = 0)  override { return Color(0.5f, 0.5f, 0.5f);; }
	virtual float GetSelfIllum(int mtlNum = 0, BOOL backFace = 0)  override { return 0.0f; }
	virtual float GetGlossiness(int mtlNum = 0, BOOL backFace = 0)  override { return 0.0f; }
	virtual float GetSpecularLevel(int mtlNum = 0, BOOL backFace = 0)  override { return 0.0f; }
	virtual float GetSoftenLevel(int mtlNum = 0, BOOL backFace = 0)  override { return 0.0f; }

	virtual BOOL IsSelfIllumClrOn()  override { return FALSE; }
	virtual Color GetAmbientClr(TimeValue t)  override { return Color(0.5f, 0.5f, 0.5f); }
	virtual Color GetDiffuseClr(TimeValue t)  override { return Color(0.5f, 0.5f, 0.5f); }
	virtual Color GetSpecularClr(TimeValue t)  override { return Color(0.5f, 0.5f, 0.5f); }
	virtual float GetGlossiness(TimeValue t)  override { return 0.0f; }
	virtual float GetSpecularLevel(TimeValue t)  override { return 0.0f; }
	virtual float GetSoftenLevel(TimeValue t)  override { return 0.0f; }
	virtual float GetSelfIllum(TimeValue t)  override { return 0.0f; }
	virtual Color GetSelfIllumClr(TimeValue t)  override { return Color(0.5f, 0.5f, 0.5f); }
	virtual float EvalHiliteCurve2(float x, float y, int level = 0)  override { return 0.0f; }

	void SetPanelOpen(BOOL open) { rolloutOpen = open; }

	void AffectReflection(ShadeContext &sc, IllumParams &ip, Color &rClr) override;

	float EvalHiliteCurve(float x)  override {
		double phExp = pow(2.0, 1.0f * 10.0); // expensive.!! TBD
		return 1.0f*static_cast<float>(pow(static_cast<double>(cos(x*PI)), phExp));
	}


	// IBSShaderMaterialData
	BOOL HasBGSM() const override { return pMtlFileRef && pMtlFileRef->ClassID() == BGSMFILE_CLASS_ID; }
	BGSMFile* GetBGSMData() const override { return HasBGSM() ? &static_cast<BGSMFileReference*>(pMtlFileRef)->file : nullptr; }
	BOOL LoadBGSM(BGSMFile&) override;

	BOOL HasBGEM() const override { return pMtlFileRef && pMtlFileRef->ClassID() == BGEMFILE_CLASS_ID; }
	BGEMFile* GetBGEMData() const override { return HasBGEM() ? &static_cast<BGEMFileReference*>(pMtlFileRef)->file : nullptr; }
	BOOL LoadBGEM(BGEMFile&) override;

	BaseMaterial* GetMtlData() const {
		return HasBGSM() ? static_cast<BaseMaterial*>(GetBGSMData()) : HasBGEM() ? static_cast<BaseMaterial*>(GetBGEMData()) : nullptr;
	}

	void FixRollups();
	BOOL ChangeShader(const Class_ID& clsid);

	//getName
	LPCTSTR GetName() const override { return pMtlFileRef ? pMtlFileRef->materialName : TEXT(""); }
	LPCTSTR GetFileName() const override { return pMtlFileRef ? pMtlFileRef->materialFileName : TEXT(""); }
	void SetName(const TSTR& name) const { pMtlFileRef->SetName(name); }
	void SetFileName(const TSTR& name, const TSTR& path) override { pMtlFileRef->SetFileName(name, path); }
	BOOL UpdateMaterial(StdMat2* mtl) override;

	typedef Texmap* (*pfCreateWrapper)(LPCTSTR name, Texmap* nmap);
	Texmap * GetOrCreateTexture(StdMat2* mtl, BaseMaterial* base_mtl, int map, tstring texture, IFileResolver* resolver, pfCreateWrapper wrapper = nullptr);
	BOOL LoadMaterial(StdMat2* mtl, IFileResolver* resolver) override;

	BOOL LoadMaterial(BaseMaterial&);
	Texmap* CreateTexture(const tstring& name, BaseMaterial* base_material, IFileResolver* resolver);

};

///////////// Class Descriptor ////////////////////////
class FO4ShaderClassDesc :public ClassDesc2 {
public:
	int            IsPublic()  override { return TRUE; }
	void *         Create(BOOL loading = FALSE)  override { return new FO4Shader(); }
	const TCHAR *  ClassName()  override { return GetString(IDS_FO4_SHADER); }
	SClass_ID      SuperClassID()  override { return SHADER_CLASS_ID; }
	Class_ID       ClassID()  override { return FO4SHADER_CLASS_ID; }
	const TCHAR*   Category()  override { return GetString(IDS_CATEGORY); }
	const TCHAR*   InternalName()  override { return _T("FO4Shader"); }   // returns fixed parsable name (scripter-visible name)
	HINSTANCE      HInstance()  override { return hInstance; }          // returns owning module handle
};

FO4ShaderClassDesc FO4ShaderDesc;
extern ClassDesc2 * GetFO4ShaderDesc() { return &FO4ShaderDesc; }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////
// Extended Rollout
#if 0
class BasePBAccessor : public PBAccessor
{
public:
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)    // set from v
	{
		auto* m = static_cast<FO4Shader*>(owner);
		IParamMap2* map = m->pb_base ? m->pb_base->GetMap() : NULL;

		switch (id)
		{
		}
	}
};

static BasePBAccessor base_pb_accessor;

// extra rollout dialog proc
class ExtraDlgProc : public ParamMap2UserDlgProc
{
public:
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_INITDIALOG: {
			auto* m = static_cast<FO4Shader*>(map->GetParamBlock()->GetOwner());
			//m->UpdateExtraParams(m->GetShader()->SupportStdParams());
			return TRUE;
		}
		}
		return FALSE;
	}
	void DeleteThis() { }
};

static ExtraDlgProc extraDlgProc;

// extended parameters
static ParamBlockDesc2 std2_extended_blk(fos_shader, _T("extendedParameters"), 0, &stdmtl2CD, P_AUTO_CONSTRUCT + P_AUTO_UI, EXTENDED_PB_REF,
	//rollout
	IDD_DMTL_EXTRA6, IDS_DS_EXTRA, 0, APPENDROLL_CLOSED, &extraDlgProc,
	// params

	std2_opacity, _T("opacity"), TYPE_PCNT_FRAC, P_ANIMATABLE, IDS_DS_OPACITY,
	p_default, 0.0,
	p_range, 0.0, 100.0,   // UI us in the shader rollout
	p_accessor, &base_pb_accessor,
	p_end,

	p_end
	);

#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////


FO4Shader::FO4Shader()
{
	pb_base = nullptr;
	pb_mtl = nullptr;
	pb_bgsm = nullptr;
	pb_bgem = nullptr;
	pMtlFileRef = nullptr;
	pMtlFileRefOld = nullptr;
	FO4ShaderDesc.MakeAutoParamBlocks(this);   // make and intialize paramblock2
	pDlg = nullptr;

	ivalid.SetEmpty();
	rolloutOpen = TRUE;

	FO4Shader::Reset();
}

FO4Shader::~FO4Shader()
{

}

void FO4Shader::CopyStdParams(Shader* pFrom)
{
	// We don't want to see this parameter copying in macrorecorder
	macroRecorder->Disable();

	TimeValue t = 0;

	SetSelfIllum(pFrom->GetSelfIllum(0, 0), t);
	SetSelfIllumClrOn(pFrom->IsSelfIllumClrOn(0, 0));
	SetSelfIllumClr(pFrom->GetSelfIllumClr(0, 0), t);
	SetAmbientClr(pFrom->GetAmbientClr(0, 0), t);
	SetDiffuseClr(pFrom->GetDiffuseClr(0, 0), t);
	SetSpecularClr(pFrom->GetSpecularClr(0, 0), t);
	SetGlossiness(pFrom->GetGlossiness(0, 0), t);
	SetSpecularLevel(pFrom->GetSpecularLevel(0, 0), t);
	SetSoftenLevel(pFrom->GetSoftenLevel(0, 0), t);

	macroRecorder->Enable();
	ivalid.SetEmpty();
}


RefTargetHandle FO4Shader::Clone(RemapDir &remap)
{
	FO4Shader* pShader = new FO4Shader();
	for (int i = 0; i < MAX_REFERENCES; ++i)
		pShader->ReplaceReference(i, remap.CloneRef(GetReference(i)));
	return pShader;
}

Animatable* FO4Shader::SubAnim(int i)
{
	switch (i) {
	case 0: return pb_base;
	case 1: return pb_mtl;
	case 2: return pb_bgsm;
	case 3: return pb_bgem;
	}
	return nullptr;
}

WStr FO4Shader::SubAnimName(int i)
{
	switch (i) {
	case 0: return TSTR(GetString(IDS_FOS_BASENAME));
	case 1: return TSTR(GetString(IDS_FOS_MTLNAME));
	case 2: return TSTR(GetString(IDS_FOS_BGSMNAME));
	case 3: return TSTR(GetString(IDS_FOS_BGEMNAME));
	}
	return TSTR(GetString(IDS_FOS_BASENAME));
}

IParamBlock2* FO4Shader::GetParamBlock(int i)
{
	switch (i) {
	case 0: return pb_base;
	case 1: return pb_mtl;
	case 2: return pb_bgsm;
	case 3: return pb_bgem;
	}
	return nullptr;
}

IParamBlock2* FO4Shader::GetParamBlockByID(BlockID id)
{
	if (pb_base->ID() == id) return pb_base;
	if (pb_mtl->ID() == id) return pb_mtl;
	if (pb_bgsm->ID() == id) return pb_bgsm;
	if (pb_bgem->ID() == id) return pb_bgem;
	return nullptr;
}

int FO4Shader::NumRefs() {
	return MAX_REFERENCES-1;
}

RefTargetHandle FO4Shader::GetReference(int i)
{
	switch (i) {
	case ref_base: return pb_base;
	case ref_mtl: return pb_mtl;
	case ref_bgsm: return pb_bgsm;
	case ref_bgem: return pb_bgem;
	case ref_activemtl: return pMtlFileRef;
	case ref_oldmtl: return pMtlFileRefOld;
	}
	return nullptr;
}

void FO4Shader::SetReference(int i, RefTargetHandle rtarg)
{
	switch (i) {
	case ref_base: pb_base = static_cast<IParamBlock2*>(rtarg); return;
	case ref_mtl: pb_mtl = static_cast<IParamBlock2*>(rtarg); return;
	case ref_bgsm: pb_bgsm = static_cast<IParamBlock2*>(rtarg); return;
	case ref_bgem: pb_bgem = static_cast<IParamBlock2*>(rtarg); return;
	case ref_activemtl: pMtlFileRef = static_cast<MaterialReference*>(rtarg); return;
	case ref_oldmtl: pMtlFileRefOld = static_cast<MaterialReference*>(rtarg); return;
	}
	assert(0);
}

IOResult FO4Shader::Load(ILoad* iload)
{
	return Shader::Load(iload);
}

IOResult FO4Shader::Save(ISave* isave)
{
	return Shader::Save(isave);
}

void FO4Shader::Update(TimeValue t, Interval &valid) {
	if (!ivalid.InInterval(t)) {
		ivalid.SetInfinite();
	}
	valid &= ivalid;
}

void FO4Shader::Reset()
{
	FO4ShaderDesc.MakeAutoParamBlocks(this);

	ivalid.SetEmpty();
	macroRecorder->Disable();  // don't want to see this parameter reset in macrorecorder

	SetSoftenLevel(0.1f, 0);
	SetAmbientClr(Color(0.588f, 0.588f, 0.588f), 0);
	SetDiffuseClr(Color(0.588f, 0.588f, 0.588f), 0);
	SetSpecularClr(Color(0.9f, 0.9f, 0.9f), 0);
	SetGlossiness(.10f, 0);   // change from .25, 11/6/00
	SetSpecularLevel(.0f, 0);

	SetSelfIllum(.0f, 0);
	SetSelfIllumClr(Color(.0f, .0f, .0f), 0);
	SetSelfIllumClrOn(FALSE);
	SetLockADTex(TRUE);
	SetLockAD(TRUE); // DS 10/26/00: changed to TRUE
	SetLockDS(FALSE);

	// create reference for BGSM file
	ReplaceReference(1, new BGSMFileReference(), 1);

	macroRecorder->Enable();
}

///////////////////////////////////////////////////////////////////////////////////////////
// The Shader
//
//#define USE_BLINN_SHADER
#define USE_STRAUSS_SHADER
//#define USE_CUSTOM_SHADER
#ifdef USE_BLINN_SHADER

void FO4Shader::GetIllumParams(ShadeContext &sc, IllumParams& ip)
{
	ip.stdParams = SupportStdParams();
	// ip.shFlags = selfIllumClrOn? SELFILLUM_CLR_ON : 0;
	TimeValue t = 0;
	ip.channels[C_BASE] = GetDiffuseClr();
	//ip.channels[(ID_DI)] = GetDiffuseClr();
	//ip.channels[StdIDToChannel(ID_SP)] = GetSpecularClr();
	//ip.channels[StdIDToChannel(ID_SH)].r = GetGlossiness();
	//ip.channels[StdIDToChannel(ID_SS)].r = GetSpecularLevel();
	//if( IsSelfIllumClrOn() )
	//ip.channels[C_GLOW] = pb->GetColor(ns_mat_emittance, 0, 0);
	//else
	//   ip.channels[C_GLOW].r = ip.channels[C_GLOW].g = ip.channels[C_GLOW].b = GetSelfIllum();
}


void FO4Shader::Illum(ShadeContext &sc, IllumParams &ip)
{
	LightDesc* l = nullptr;
	Color lightCol;

	// Blinn style phong
	BOOL is_shiny = (ip.channels[StdIDToChannel(ID_SS)].r > 0.0f) ? 1 : 0;
	double phExp = pow(2.0, ip.channels[StdIDToChannel(ID_SH)].r * 10.0) * 4.0; // expensive.!!  TBD

	for (int i = 0; i < sc.nLights; i++) {
		l = sc.Light(i);
		register float NL, diffCoef;
		Point3 L;
		if (l->Illuminate(sc, sc.Normal(), lightCol, L, NL, diffCoef)) {
			if (l->ambientOnly) {
				ip.ambIllumOut += lightCol;
				continue;
			}
			if (NL <= 0.0f)
				continue;

			// diffuse
			if (l->affectDiffuse)
				ip.diffIllumOut += diffCoef * lightCol;

			// specular (Phong2) 
			if (is_shiny&&l->affectSpecular) {
				Point3 H = Normalize(L - sc.V());
				float c = DotProd(sc.Normal(), H);
				if (c > 0.0f) {
					c = (float)pow((double)c, phExp); // could use table lookup for speed
					ip.specIllumOut += c * ip.channels[StdIDToChannel(ID_SS)].r * lightCol;
				}
			}
		}
	}


	// Apply mono self illumination
	if (!IsSelfIllumClrOn()) {
		// lerp between diffuse & white
		// changed back, fixed in getIllumParams, KE 4/27
		float si = 0.3333333f * (ip.channels[ID_SI].r + ip.channels[ID_SI].g + ip.channels[ID_SI].b);
		if (si > 0.0f) {
			si = UBound(si);
			ip.selfIllumOut = si * ip.channels[ID_DI];
			ip.diffIllumOut *= (1.0f - si);
			// fade the ambient down on si: 5/27/99 ke
			ip.ambIllumOut *= 1.0f - si;
		}
	} else {
		// colored self illum, 
		ip.selfIllumOut += ip.channels[ID_SI];
	}
	// now we can multiply by the clrs, 
	ip.ambIllumOut *= ip.channels[ID_AM];
	ip.diffIllumIntens = Intens(ip.diffIllumOut);
	ip.diffIllumOut *= ip.channels[ID_DI];
	ip.specIllumOut *= ip.channels[ID_SP];

	ShadeTransmission(sc, ip, ip.channels[ID_RR], ip.refractAmt);
	ShadeReflection(sc, ip, ip.channels[ID_RL]);
	CombineComponents(sc, ip);
}

void FO4Shader::AffectReflection(ShadeContext &sc, IllumParams &ip, Color &rcol)
{ rcol *= ip.channels[ID_SP]; };

#endif
#ifdef USE_STRAUSS_SHADER

// my magic constants
static float SpecBoost = 1.3f;

// Strauss's Magic Constants
static float kf = 1.12f;
static float kf2 = 1.0f / (kf * kf);
static float kf3 = 1.0f / ((1.0f - kf) * (1.0f - kf));
static float kg = 1.01f;
static float kg2 = 1.0f / (kg * kg);
static float kg3 = 1.0f / ((1.0f - kg) * (1.0f - kg));
static float kj = 0.1f; //.1 strauss

static float OneOverHalfPi = 1.0f / (0.5f * Pi);

inline float F(float x) {
	float xb = Bound(x);
	float xkf = 1.0f / ((xb - kf)*(xb - kf));
	return (xkf - kf2) / (kf3 - kf2);
}

inline float G(float x) {
	float xb = Bound(x);
	float xkg = 1.0f / ((xb - kg)*(xb - kg));
	return (kg3 - xkg) / (kg3 - kg2);
}

#define REFL_BRIGHTNESS_ADJUST   3.0f;

void FO4Shader::AffectReflection(ShadeContext &sc, IllumParams &ip, Color &rClr)
{
	float opac = 0.0f;
	float g = ip.channels[C_GLOW].r;
	float m = 0.0f;
	Color Cd = ip.channels[C_DIFFUSE];

	float rn = opac - (1.0f - g * g * g) * opac;

	// the reflection of the reflection vector is just the view vector
	// so dot(v, r) is 1, to any power is still 1
	float a, b;
	// NB: this has been transformed for existing in-pointing v
	float NV = Dot(sc.V(), sc.Normal());
	Point3 R = sc.V() - 2.0f * NV * sc.Normal();
	float NR = Dot(sc.Normal(), R);
	a = (float)acos(NR) * OneOverHalfPi;
	b = (float)acos(NV) * OneOverHalfPi;

	float fa = F(a);
	float j = fa * G(a) * G(b);
	float rj = Bound(rn + (rn + kj)*j);
	Color white(1.0f, 1.0f, 1.0f);

	Color Cs = white + m * (1.0f - fa) * (Cd - white);
	rClr *= Cs * rj * REFL_BRIGHTNESS_ADJUST;
}

static int stopX = -1;
static int stopY = -1;

static float   greyVal = 0.3f;
static float   clrVal = 0.3f;

static float   softThresh = 0.15f;

void FO4ShaderCombineComponents(ShadeContext &sc, IllumParams& ip)
{
	float o = (ip.hasComponents & HAS_REFRACT) ? ip.finalAttenuation : 1.0f;

	ip.finalC = o * (ip.ambIllumOut + ip.diffIllumOut) + ip.specIllumOut
		+ ip.reflIllumOut + ip.transIllumOut;
}


void FO4Shader::GetIllumParams(ShadeContext &sc, IllumParams& ip)
{
	ip.stdParams = SupportStdParams();
	// ip.shFlags = selfIllumClrOn? SELFILLUM_CLR_ON : 0;
	TimeValue t = 0;
	ip.channels[C_DIFFUSE] = GetDiffuseClr();
	//ip.channels[C_BASE] = GetDiffuseClr();
	//ip.channels[C_OPACITY] = Color(1.0f, 1.0f, 1.0f);
	//ip.channels[(ID_DI)] = GetDiffuseClr();
	//ip.channels[StdIDToChannel(ID_SP)] = GetSpecularClr();
	//ip.channels[StdIDToChannel(ID_SH)].r = GetGlossiness();
	//ip.channels[StdIDToChannel(ID_SS)].r = GetSpecularLevel();
	//if( IsSelfIllumClrOn() )
		//ip.channels[C_GLOW] = pb->GetColor(ns_mat_emittance, 0, 0);
	//else
	//   ip.channels[C_GLOW].r = ip.channels[C_GLOW].g = ip.channels[C_GLOW].b = GetSelfIllum();
}


void FO4Shader::Illum(ShadeContext &sc, IllumParams &ip)
{
	LightDesc *l;
	Color lightClr;

#ifdef _DEBUG
	IPoint2 sp = sc.ScreenCoord();
	if (sp.x == stopX && sp.y == stopY)
		sp.x = stopX;
#endif

	//float opac = ip.channels[C_OPACITY].r;
	//float opac = ip.channels[C_].r;
	float opac = 1.0f;
	float g = ip.channels[C_GLOW].r;
	float m = 0.0f;
	Color Cd = ip.channels[C_DIFFUSE];
	// BOOL dimDiffuse = ip.hasComponents & HAS_REFLECT;
	BOOL dimDiffuse = ip.hasComponents & HAS_REFLECT_MAP;

	float rd;
	float g3 = Cube(g);
	if (dimDiffuse)
		rd = (1.0f - g3) * opac;
	else
		rd = (1.0f - m * g3) * opac;  //ke 10/28/98

	float rn = opac - (1.0f - g3) * opac;

	float h = (g == 1.0f) ? 600.0f : 3.0f / (1.0f - g);
	float d = 1.0f - m * g;

	for (int i = 0; i < sc.nLights; i++) {
		l = sc.Light(i);
		float NL, Kl;
		Point3 L;
		if (l->Illuminate(sc, sc.Normal(), lightClr, L, NL, Kl)) {
			if (l->ambientOnly) {
				ip.ambIllumOut += lightClr;
				continue;
			}
			if (NL <= 0.0f)
				continue;

			// diffuse
			if (l->affectDiffuse) {
				ip.diffIllumOut += Kl * d * rd * lightClr;
			}

			// specular  
			if (l->affectSpecular) {
				// strauss uses the reflected LIGHT vector
				Point3 R = L - 2.0f * NL * sc.Normal();
				R = Normalize(R);

				float RV = -Dot(R, sc.V());

				float s;
				if (RV < 0.0f) {
					// soften
					if (NL < softThresh)
						RV *= SoftSpline2(NL / softThresh);
					// specular function
					s = SpecBoost * (float)pow(-RV, h);
				} else
					continue;

				float a, b;
				a = (float)acos(NL) * OneOverHalfPi;
				b = (float)acos(-Dot(sc.Normal(), sc.V())) * OneOverHalfPi;

				float fa = F(a);
				float j = fa * G(a) * G(b);
				float rj = rn > 0.0f ? Bound(rn + (rn + kj)*j) : rn;
				Color Cl = lightClr;
				// normalize the light color in case it's really bright
				float I = NormClr(Cl);
				Color Cs = Cl + m * (1.0f - fa) * (Cd - Cl);

				ip.specIllumOut += s * rj * I * Cs;

			} // p_end, if specular
		}  // p_end, illuminate

	} // for each light

	// now we can multiply by the clrs, except specular, which is already done
	ip.ambIllumOut *= 0.5f * rd * Cd;
	ip.diffIllumIntens = Intens(ip.diffIllumOut);
	ip.diffIllumOut *= Cd;

	// next due reflection
	if (ip.hasComponents & HAS_REFLECT) {
		Color rc = ip.channels[ip.stdIDToChannel[ID_RL]];
		AffectReflection(sc, ip, rc);
		ip.reflIllumOut = rc * ip.reflectAmt;
	}

	// last do refraction/ opacity
	if ((ip.hasComponents & HAS_REFRACT)) {
		// Set up attenuation opacity for Refraction map. dim diffuse & spec by this
		ip.finalAttenuation = ip.finalOpac * (1.0f - ip.refractAmt);

		// Make more opaque where specular hilite occurs:
		float max = Max(ip.specIllumOut);
		if (max > 1.0f) max = 1.0f;
		float newOpac = ip.finalAttenuation + max - ip.finalAttenuation * max;

		// Evaluate refraction map, filtered by filter color.
		//    Color tClr = ((StdMat2*)(ip.pMtl))->TranspColor( newOpac, ip.channels[filtChan], ip.channels[diffChan]);
		//Color tClr = transpColor( TRANSP_FILTER, newOpac, Cd, Cd );
		//ip.transIllumOut = ip.channels[ ip.stdIDToChannel[ ID_RR ] ] * tClr;
		ip.transIllumOut = Color(0.0f, 0.0f, 0.0f);

		// no transparency when doing refraction
		ip.finalT.Black();

	} else {
		// no refraction, transparent?
		ip.finalAttenuation = opac;
		if (ip.hasComponents & HAS_OPACITY) {
			// ip.finalT = Cd * (1.0f-opac);
			Cd = greyVal * Color(1.0f, 1.0f, 1.0f) + clrVal * Cd;
			ip.finalT = transpColor(TRANSP_FILTER, opac, Cd, Cd);
		}
	}

	FO4ShaderCombineComponents(sc, ip);
}
#endif
#ifdef USE_CUSTOM_SHADER

//---------------------------------------------------------------------------
// Called to combine the various color and shading components
void FO4ShaderCombineComponents(ShadeContext &sc, IllumParams& ip)
{
	ip.finalC = (ip.ambIllumOut + ip.diffIllumOut + ip.selfIllumOut) + ip.specIllumOut;
}

//---------------------------------------------------------------------------
void FO4Shader::GetIllumParams(ShadeContext &sc, IllumParams &ip)
{
	ip.stdParams = SupportStdParams();
	ip.channels[C_BASE] = pb->GetColor(ns_mat_diffuse, 0, 0);
	ip.channels[C_GLOW] = pb->GetColor(ns_mat_selfillumclr, 0, 0);
	ip.channels[C_SPECULAR] = pb->GetColor(ns_mat_specular, 0, 0);

}

//---------------------------------------------------------------------------
void FO4Shader::Illum(ShadeContext &sc, IllumParams &ip)
{
	LightDesc *pLight;
	Color lightCol;

	// Get our parameters our of the channels
	Color base = ip.channels[C_BASE];
	Color dark = ip.channels[C_DARK];
	Color detail = ip.channels[C_DETAIL];
	Color decal1 = ip.channels[C_DECAL1];
	Color bump = ip.channels[C_BUMP];
	Color gloss = ip.channels[C_GLOSS];
	Color glow = ip.channels[C_GLOW];
	Color reflection = ip.channels[C_REFLECTION];
	Color specular = ip.channels[C_SPECULAR];
	Color emittance = pb->GetColor(ns_mat_emittance);
	int iApplyMode = pb->GetInt(ns_apply_mode);
	bool bSpecularOn = pb->GetInt(ns_mat_specenable) != 0;
	float fShininess = pb->GetFloat(ns_mat_shininess);
	Color ambient = Color(pb->GetColor(ns_mat_ambient));

	ip.specIllumOut.Black();

	if (iApplyMode)
	{
		for (int i = 0; i < sc.nLights; i++)
		{
			register float fNdotL, fDiffCoef;
			Point3 L;

			pLight = sc.Light(i);
#if MAX_RELEASE < 4000
			if (pLight->Illuminate(sc, ip.N, lightCol, L, fNdotL, fDiffCoef))
#else
			if (pLight->Illuminate(sc, sc.Normal(), lightCol, L, fNdotL, fDiffCoef))
#endif
			{
				if (pLight->ambientOnly)
				{
					ip.ambIllumOut += lightCol;
					continue;
				}

				if (fNdotL <= 0.0f)
					continue;

				if (pLight->affectDiffuse)
					ip.diffIllumOut += fDiffCoef * lightCol;

				if (bSpecularOn && pLight->affectSpecular)
				{
#if MAX_RELEASE < 4000
					Point3 H = Normalize(L - ip.V);
					float c = DotProd(ip.N, H);
#else
					Point3 H = Normalize(L - sc.V());
					float c = DotProd(sc.Normal(), H);
#endif
					if (c > 0.0f)
					{
						c = (float)pow(c, fShininess);
						// c * bright * lightCol;
						ip.specIllumOut += c * lightCol;
					}
				}
			}
		}
	}
	else
	{
		ip.ambIllumOut.Black();
		ip.diffIllumOut.White();
	}

	ip.ambIllumOut *= ambient;
	ip.diffIllumOut *= dark * (base * detail);  // + decal;
	ip.selfIllumOut = emittance + glow + decal1;
	ip.specIllumOut *= specular;

	FO4ShaderCombineComponents(sc, ip);
}


//---------------------------------------------------------------------------
void FO4Shader::AffectReflection(ShadeContext &sc, IllumParams &ip, Color &rcol)
{
	rcol *= ip.channels[C_SPECULAR];
};
#endif



BOOL FO4Shader::LoadMaterial(BaseMaterial& bgsm)
{
	return TRUE;
}



static Texmap* CreateNormalBump(LPCTSTR name, Texmap* nmap)
{
	Interface *gi = GetCOREInterface();
	Texmap *texmap = (Texmap*)gi->CreateInstance(TEXMAP_CLASS_ID, GNORMAL_CLASS_ID);
	if (texmap != nullptr)
	{
		TSTR tname = (name == nullptr) ? FormatText(TEXT("Norm %s"), nmap->GetName().data()) : TSTR(name);
		texmap->SetName(tname);
		texmap->SetSubTexmap(0, nmap);
		return texmap;
	}
	return nmap;
}

static Texmap* CreateMask(LPCTSTR name, Texmap* map, Texmap* mask)
{
	Interface *gi = GetCOREInterface();
	Texmap *texmap = (Texmap*)gi->CreateInstance(TEXMAP_CLASS_ID, Class_ID(MASK_CLASS_ID, 0));
	if (texmap != nullptr)
	{
		TSTR tname = (name == nullptr) ? FormatText(TEXT("Mask %s"), map->GetName().data()) : TSTR(name);
		texmap->SetName(tname);
		texmap->SetSubTexmap(0, map);
		texmap->SetSubTexmap(1, mask);
		return texmap;
	}
	return map;
}

static Texmap* MakeAlpha(Texmap* tex)
{
	if (BitmapTex *bmp_tex = GetIBitmapTextInterface(tex)) {
		bmp_tex->SetName(TEXT("Alpha"));
		bmp_tex->SetAlphaAsMono(TRUE);
		bmp_tex->SetAlphaSource(ALPHA_FILE);
		bmp_tex->SetPremultAlpha(FALSE);
		bmp_tex->SetOutputLevel(INFINITE, 0.0f);
	}
	return tex;
}

Texmap* FO4Shader::CreateTexture(const tstring& name, BaseMaterial* base_material, IFileResolver* resolver)
{
	if (name.empty())
		return nullptr;

	BitmapManager *bmpMgr = TheManager;
	if (bmpMgr->CanImport(name.c_str())) {
		BitmapTex *bmpTex = NewDefaultBitmapTex();

		tstring filename;
		resolver->FindFileByType(name, IFileResolver::FT_Texture, filename);
		bmpTex->SetName(name.c_str());
		bmpTex->SetMapName(filename.c_str());
		bmpTex->SetAlphaAsMono(TRUE);
		bmpTex->SetAlphaSource(ALPHA_NONE);

		bmpTex->SetFilterType(FILTER_PYR);

		if (true) {
			bmpTex->SetMtlFlag(MTL_TEX_DISPLAY_ENABLED, TRUE);
			bmpTex->ActivateTexDisplay(TRUE);
			bmpTex->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		}

		if (UVGen *uvGen = bmpTex->GetTheUVGen()) {
			int textureTiling = ((base_material->TileU ? 0x1 : 0) | (base_material->TileV ? 0x2 : 0));
			uvGen->SetTextureTiling(textureTiling);
			if (RefTargetHandle ref = uvGen->GetReference(0)) {
				setMAXScriptValue(ref, TEXT("U_Offset"), 0, base_material->UOffset);
				setMAXScriptValue(ref, TEXT("V_Offset"), 0, base_material->VOffset);
				setMAXScriptValue(ref, TEXT("U_Tiling"), 0, base_material->UScale);
				setMAXScriptValue(ref, TEXT("V_Tiling"), 0, base_material->VScale);
			}
		}

		return bmpTex;
	}
	return nullptr;
}
BOOL FO4Shader::UpdateMaterial(StdMat2* mtl)
{
	return TRUE;
}

Texmap * FO4Shader::GetOrCreateTexture(StdMat2* mtl, BaseMaterial* base_mtl, int map, tstring texture, IFileResolver* resolver, pfCreateWrapper wrapper )
{
	Texmap * tex = mtl->GetSubTexmap(map);
	if (tex != nullptr && tex->GetName() == texture.c_str())
		return tex;

	if (tex && tex->NumSubTexmaps() > 0) {
		tex = tex->GetSubTexmap(0);
		if (tex != nullptr && tex->GetName() == texture.c_str())
			return tex;
	}
	if (texture.empty())
		return nullptr;

	tex = CreateTexture(texture, base_mtl, resolver);
	tex = wrapper ? wrapper(nullptr, tex) : tex;
	if (tex) {
		mtl->SetSubTexmap(map, tex);
	}
	return tex;
}

BOOL FO4Shader::LoadMaterial(StdMat2* mtl, IFileResolver* resolver)
{
	BaseMaterial* base_mtl = GetMtlData();
	if (base_mtl == nullptr)
		return FALSE;

	// handle base material stuff
	mtl->SetTwoSided(base_mtl->TwoSided ? TRUE : FALSE);
	mtl->SetOpacity(base_mtl->Alpha, INFINITE);

	if (this->HasBGSM())
	{
		auto* bgsm = GetBGSMData();
		mtl->SetSpecular(TOCOLOR(bgsm->SpecularColor), 0);
		mtl->GetSelfIllumColorOn(bgsm->Glowmap);

		GetOrCreateTexture(mtl, base_mtl, C_DIFFUSE, bgsm->DiffuseTexture, resolver);
		GetOrCreateTexture(mtl, base_mtl, C_NORMAL, bgsm->NormalTexture, resolver, CreateNormalBump);
		GetOrCreateTexture(mtl, base_mtl, C_SMOOTHSPEC, bgsm->SmoothSpecTexture, resolver);
		GetOrCreateTexture(mtl, base_mtl, C_GREYSCALE, bgsm->GreyscaleTexture, resolver);
		GetOrCreateTexture(mtl, base_mtl, C_GLOW, bgsm->GlowTexture, resolver);
		GetOrCreateTexture(mtl, base_mtl, C_INNERLAYER, bgsm->InnerLayerTexture, resolver);
		GetOrCreateTexture(mtl, base_mtl, C_WRINKLES, bgsm->WrinklesTexture, resolver);
		GetOrCreateTexture(mtl, base_mtl, C_DISPLACE, bgsm->DisplacementTexture, resolver);
	}
	if (this->HasBGEM())
	{
		auto* bgem = GetBGEMData();

		if (Texmap* tex = CreateTexture(bgem->BaseTexture, base_mtl, resolver))
			mtl->SetSubTexmap(C_DIFFUSE, tex);
		if (Texmap* tex = CreateTexture(bgem->NormalTexture, base_mtl, resolver))
			mtl->SetSubTexmap(C_NORMAL, CreateNormalBump(nullptr, tex));
		if (Texmap* tex = CreateTexture(bgem->EnvmapTexture, base_mtl, resolver)) {
			if (Texmap* mask = CreateTexture(bgem->EnvmapMaskTexture, base_mtl, resolver)) {
				tex = CreateMask(nullptr, tex, mask);
				mtl->SetSubTexmap(C_ENVMAP, tex);
				mtl->SetTexmapAmt(C_ENVMAP, 0.0f, INFINITE);
				tex->SetOutputLevel(INFINITE, 0.0f);
			}
		}
		if (Texmap* tex = CreateTexture(bgem->GrayscaleTexture, base_mtl, resolver))
			mtl->SetSubTexmap(C_GREYSCALE, tex);
	}

	return TRUE;
}

//
//BOOL FO4Shader::UpdateMaterial(StdMat2* mtl)
//{
//	BaseMaterial* base_material = GetMtlData();
//	mtl->SetTwoSided(base_material->TwoSided ? TRUE : FALSE);
//	//mtl->SetSpecular(TOCOLOR(pMaterialData->SpecularColor), 0);
//	//mtl->GetSelfIllumColorOn(pMaterialData->Glowmap);
//
//
//	return TRUE;
//}

BOOL FO4Shader::ChangeShader(const Class_ID& clsid)
{
	// careful with references in this routine
	if ( clsid == BGSMFILE_CLASS_ID ) {
		if (!HasBGSM()) {
			MaterialReference* oldmtl = static_cast<MaterialReference*>(GetReference(ref_oldmtl));
			MaterialReference* curmtl = static_cast<MaterialReference*>(GetReference(ref_activemtl));
			if (!oldmtl || oldmtl->ClassID() == clsid) { // swap
				pMtlFileRef = oldmtl;
				pMtlFileRefOld = curmtl;
			} 
		}
		if (GetReference(ref_activemtl) == nullptr) {
			ReplaceReference(ref_activemtl, new BGSMFileReference(), TRUE);
		}
	} else if (clsid == BGEMFILE_CLASS_ID) {
		if (!HasBGEM()) {
			MaterialReference* oldmtl = static_cast<MaterialReference*>(GetReference(ref_oldmtl));
			MaterialReference* curmtl = static_cast<MaterialReference*>(GetReference(ref_activemtl));
			if (!oldmtl || oldmtl->ClassID() == clsid) { // swap
				pMtlFileRef = oldmtl;
				pMtlFileRefOld = curmtl;
			}
		}
		if (GetReference(ref_activemtl) == nullptr) {
			ReplaceReference(ref_activemtl, new BGEMFileReference(), TRUE);
		}
	}
	FixRollups();
	return TRUE;
}


BOOL FO4Shader::LoadBGSM(BGSMFile& bgsm)
{
	// use the bgsm file as primary
	//LoadMaterial(bgsm);
	ChangeShader(BGSMFILE_CLASS_ID);
	if (HasBGSM()) {
		*GetBGSMData() = bgsm;
	}
	return TRUE;
}

BOOL FO4Shader::LoadBGEM(BGEMFile& bgem)
{
	ChangeShader(BGEMFILE_CLASS_ID);
	if (HasBGEM()) {
		*GetBGEMData() = bgem;
	}
	return TRUE;
}

class FO4ShaderRollupBase
{
public:
	FO4ShaderDlg *pDlg;
	HPALETTE hOldPal;
	HWND     hwHilite;   // the hilite window
	HWND     hRollup; // Rollup panel for Base Materials
	BOOL     isActive;
	BOOL     inUpdate;
	BOOL     valid;

	virtual ~FO4ShaderRollupBase();

	virtual void UpdateHilite();
	virtual void UpdateColSwatches() {}
	virtual void UpdateMapButtons();
	virtual void UpdateOpacity() { UpdateHilite(); }

	virtual void InitializeControls(HWND hwnd) = 0;
	virtual void ReleaseControls() = 0;
	virtual void UpdateControls() = 0;
	virtual void CommitValues() = 0;
	virtual void UpdateVisible() = 0;
	virtual void NotifyChanged();

	virtual void UpdateMtlDisplay();

	virtual void LoadPanel(BOOL );
	virtual void ReloadPanel();
	void FreeRollup();

	virtual INT_PTR PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
protected:
	FO4ShaderRollupBase(FO4ShaderDlg *pDlg);
};

class FO4ShaderBaseRollup;
class FO4ShaderMtlRollup;
class FO4ShaderBGSMRollup;
class FO4ShaderBGEMRollup;

#pragma region ("FO4 Shader Dialog")

///////////////////////////////////////////////////////////////////////////////////
//
// Fallout4 Base Material dlg panel
//
class FO4ShaderDlg : public ShaderParamDlg {
public:
	FO4Shader* pShader;
	StdMat2* pMtl;
	IMtlParams* pMtlPar;
	HWND     hwmEdit; // window handle of the materials editor dialog
	FO4ShaderRollupBase *pBaseRollup;
	FO4ShaderRollupBase *pMtlRollup;
	FO4ShaderRollupBase *pBGSMRollup;
	FO4ShaderRollupBase *pBGEMRollup;

	TimeValue   curTime;
	BOOL     isActive;
	BOOL     inUpdate;


	FO4ShaderDlg(HWND hwMtlEdit, IMtlParams *pParams);
	~FO4ShaderDlg();
	void DeleteThis() override {
		DeleteRollups();
		if (pShader) pShader->pDlg = nullptr;
		pShader = nullptr;
		delete this; 
	}

	// Methods
	Class_ID ClassID()  override { return FO4SHADER_CLASS_ID; }

	void SetThing(ReferenceTarget *m)  override { pMtl = static_cast<StdMat2*>(m); }
	void SetThings(StdMat2* theMtl, Shader* theShader) override
	{
		if (pShader) pShader->SetParamDlg(nullptr, 0);
		pShader = static_cast<FO4Shader*>(theShader);
		if (pShader)pShader->SetParamDlg(this, 0);
		pMtl = theMtl;
	}
	ReferenceTarget* GetThing()  override { return pMtl; } // mtl is the thing! (for DAD!)
	Shader* GetShader()  override { return pShader; }

	void SetTime(TimeValue t)  override {
		//DS 2/26/99: added interval test to prevent redrawing when not necessary
		curTime = t;
		if (!pShader->ivalid.InInterval(t)) {
			Interval v;
			pShader->Update(t, v);
			LoadDialog(TRUE);
		}
		else
			UpdateOpacity();  // always update opacity since it's not in validity computations
	}
	//BOOL KeyAtCurTime(int id) { return pShader->KeyAtTime(id,curTime); } 
	void ActivateDlg(BOOL dlgOn)  override { isActive = dlgOn; }
	HWND GetHWnd()  override {
		return nullptr;// pMtlRollup ? pMtlRollup->hRollup : nullptr; 
	}
	void NotifyChanged()  const { pShader->NotifyChanged(); }
	void LoadDialog(BOOL draw) override;
	void ReloadDialog()  override { Interval v; pShader->Update(pMtlPar->GetTime(), v); LoadDialog(FALSE); }
	void UpdateDialog(ParamID paramId)  override { if (!inUpdate) ReloadDialog(); }

	void UpdateMtlDisplay()  const { pMtlPar->MtlChanged(); } // redraw viewports

															  // required for correctly operating map buttons
	int FindSubTexFromHWND(HWND hw)  override {
		return -1;
	}
	void DeleteRollups();

	void UpdateHilite();
	void UpdateColSwatches();
	void UpdateMapButtons() override;
	void UpdateOpacity() override;

	INT_PTR PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) override;

};

FO4ShaderDlg::FO4ShaderDlg(HWND hwMtlEdit, IMtlParams *pParams) 
{
	hwmEdit = hwMtlEdit;
	pBaseRollup = nullptr;
	pMtlRollup = nullptr;
	pBGSMRollup = nullptr;
	pBGEMRollup = nullptr;
	pMtl = nullptr;
	pShader = nullptr;
	pMtlPar = pParams;
	curTime = pMtlPar->GetTime();
	isActive = FALSE;
	inUpdate = FALSE;
}

FO4ShaderDlg::~FO4ShaderDlg()
{
	DeleteRollups();

	if (pShader) pShader->SetParamDlg(nullptr, 0);
}


void  FO4ShaderDlg::LoadDialog(BOOL draw)
{
	if (pShader) {
		if (pBaseRollup) pBaseRollup->UpdateControls();
		if (pMtlRollup) pMtlRollup->UpdateControls();
		if (pBGSMRollup) pBGSMRollup->UpdateControls();
		if (pBGEMRollup) pBGEMRollup->UpdateControls();
	}
}


static TCHAR* mapStates[] = { _T(" "), _T("m"),  _T("M") };

void FO4ShaderDlg::UpdateMapButtons()
{
	if (pBaseRollup) pBaseRollup->UpdateMapButtons();
	if (pMtlRollup) pMtlRollup->UpdateMapButtons();
	if (pBGSMRollup) pBGSMRollup->UpdateMapButtons();
	if (pBGEMRollup) pBGEMRollup->UpdateMapButtons();
}


void FO4ShaderDlg::UpdateOpacity()
{
	if (pBaseRollup) pBaseRollup->UpdateOpacity();
	if (pMtlRollup) pMtlRollup->UpdateOpacity();
	if (pBGSMRollup) pBGSMRollup->UpdateOpacity();
	if (pBGEMRollup) pBGEMRollup->UpdateOpacity();

	//trSpin->SetValue(FracToPc(pMtl->GetOpacity(curTime)),FALSE);
	//trSpin->SetKeyBrackets(pMtl->KeyAtTime(OPACITY_PARAM, curTime));
}

INT_PTR FO4ShaderDlg::PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//if (pBaseRollup) pBaseRollup->PanelProc();
	//if (pMtlRollup) pMtlRollup->PanelProc();
	//if (pBGSMRollup) pBGSMRollup->PanelProc();
	//if (pBGEMRollup) pBGEMRollup->PanelProc();
	return FALSE;
}

void FO4ShaderDlg::UpdateColSwatches()
{
	//cs[0]->SetKeyBrackets( pShader->KeyAtTime(ns_mat_diffuse,curTime) );
	//cs[0]->SetColor( pShader->GetDiffuseClr() );
	if (pBaseRollup) pBaseRollup->UpdateColSwatches();
	if (pMtlRollup) pMtlRollup->UpdateColSwatches();
	if (pBGSMRollup) pBGSMRollup->UpdateColSwatches();
	if (pBGEMRollup) pBGEMRollup->UpdateColSwatches();
}


void FO4ShaderDlg::UpdateHilite()
{
	if (pBaseRollup) pBaseRollup->UpdateHilite();
	if (pMtlRollup) pMtlRollup->UpdateHilite();
	if (pBGSMRollup) pBGSMRollup->UpdateHilite();
	if (pBGEMRollup) pBGEMRollup->UpdateHilite();
}

#pragma endregion 

#pragma region ("Rollups")

class FO4ShaderBaseRollup : public FO4ShaderRollupBase
{
	typedef FO4ShaderRollupBase base;
public:
	ICustEdit *p_name_edit;
	ICustButton *iLoadBtn;
	ICustButton *iSaveBtn;

	FO4ShaderBaseRollup(FO4ShaderDlg *pDlg) : FO4ShaderRollupBase(pDlg), p_name_edit(nullptr), iLoadBtn(nullptr), iSaveBtn(nullptr)
	{}
	virtual ~FO4ShaderBaseRollup() {}

	void InitializeControls(HWND hwnd) override {
		p_name_edit = GetICustEdit(GetDlgItem(hwnd, IDC_ED_MTL_FILE));
		if (p_name_edit) p_name_edit->Enable(TRUE);

		iLoadBtn = GetICustButton(GetDlgItem(hwnd, IDC_BTN_MTL_LOAD));
		iSaveBtn = GetICustButton(GetDlgItem(hwnd, IDC_BTN_MTL_SAVE));
		if (iSaveBtn) iSaveBtn->Disable();

		for (const EnumLookupType* flag = MaterialFileTypes; flag->name != NULL; ++flag)
			SendDlgItemMessage(hwnd, IDC_CUSTOM_SHADER, CB_ADDSTRING, 0, LPARAM(flag->name));
		SendDlgItemMessage(hwnd, IDC_CUSTOM_SHADER, CB_SETCURSEL, 0, 0);
		//SendDlgItemMessage(hwnd, IDC_CUSTOM_SHADER, WM_SET)
		UpdateControls();
	}
	void ReleaseControls()  override {
		if (p_name_edit) { ReleaseICustEdit(p_name_edit), p_name_edit = nullptr; }
		if (iLoadBtn) { ReleaseICustButton(iLoadBtn); iLoadBtn = nullptr; }
		if (iSaveBtn) { ReleaseICustButton(iSaveBtn); iSaveBtn = nullptr; }
	}

	void UpdateControls() override;

	void CommitValues() override;

	void UpdateVisible()  override {}

	INT_PTR PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) override;
	static INT_PTR CALLBACK DlgRollupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

class FO4ShaderMtlRollup : public FO4ShaderRollupBase
{
	typedef FO4ShaderRollupBase base;
public:
	TexDADMgr dadMgr;

	IColorSwatch* clrSpecular;
	IColorSwatch* clrEmittance;

	ICustButton* texMButDiffuse;

	ISpinnerControl *pShininessSpinner;
	ISpinnerControl *pAlphaSpinner;
	ISpinnerControl *pTestRefSpinner;

	FO4ShaderMtlRollup(FO4ShaderDlg *pDlg);
	virtual ~FO4ShaderMtlRollup();

	void InitializeControls(HWND hwnd) override;
	void ReleaseControls()  override;
	void UpdateControls()  override;
	void CommitValues()  override;
	void UpdateVisible()  override;

	INT_PTR PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) override;
	static INT_PTR CALLBACK DlgRollupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

class FO4ShaderBGSMRollup : public FO4ShaderRollupBase
{
	typedef FO4ShaderRollupBase base;
public:
	FO4ShaderBGSMRollup(FO4ShaderDlg *pDlg);
	virtual ~FO4ShaderBGSMRollup();

	void InitializeControls(HWND hwnd) override {}
	void ReleaseControls()  override {}
	void UpdateControls()  override {}
	void CommitValues()  override {}
	void UpdateVisible()  override {}

	INT_PTR PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) override;
	static INT_PTR CALLBACK DlgRollupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};




class FO4ShaderBGEMRollup : public FO4ShaderRollupBase
{
	typedef FO4ShaderRollupBase base;
public:
	FO4ShaderBGEMRollup(FO4ShaderDlg *pDlg);
	virtual ~FO4ShaderBGEMRollup();

	void InitializeControls(HWND hwnd) override {}
	void ReleaseControls()  override {}
	void UpdateControls()  override {}
	void CommitValues()  override {}
	void UpdateVisible()  override {}

	INT_PTR PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) override;
	static INT_PTR CALLBACK DlgRollupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};


////////////////////////////////////////////////////////////////////////////
// 
//  Concrete Creation of Rollups in Shader Dialogs
//
FO4ShaderRollupBase::FO4ShaderRollupBase(FO4ShaderDlg *dlg)
{
	pDlg = dlg;
	hRollup = hwHilite = nullptr;
	hOldPal = nullptr;
	isActive = FALSE;
	inUpdate = FALSE;
	valid = FALSE;
}

void FO4ShaderRollupBase::FreeRollup()
{
	HDC hdc = GetDC(hRollup);
	GetGPort()->RestorePalette(hdc, hOldPal);
	ReleaseDC(hRollup, hdc);

	DLSetWindowLongPtr(hRollup, NULL);
	DLSetWindowLongPtr(hwHilite, NULL);

	FO4ShaderRollupBase::ReleaseControls();

	HWND hOldRollup = hRollup;
	hwHilite = hRollup = nullptr;

	if (hOldRollup && pDlg && pDlg->pMtlPar)
	{

		pDlg->pMtlPar->DeleteRollupPage(hOldRollup);
	}

}

FO4ShaderRollupBase::~FO4ShaderRollupBase()
{
	FreeRollup();
}

void FO4ShaderRollupBase::UpdateHilite()
{
	HDC hdc = GetDC(hwHilite);
	Rect r;
	GetClientRect(hwHilite, &r);
	DrawHilite(hdc, r, pDlg->pShader);
	ReleaseDC(hwHilite, hdc);
}

void FO4ShaderRollupBase::UpdateMapButtons()
{
	//int state = pDlg->pMtl->GetMapState(0);
	//texMButDiffuse->SetText(mapStates[state]);

#if VERSION_3DSMAX < ((10000<<16)+(24<<8)+0) // Version 7
	//TSTR nm = pMtl->GetMapName(0);
	//texMButDiffuse->SetTooltip(TRUE, nm);
#endif
}

void FO4ShaderRollupBase::ReleaseControls()
{
	
}


void FO4ShaderRollupBase::NotifyChanged()
{
	pDlg->NotifyChanged();
}

void FO4ShaderRollupBase::UpdateMtlDisplay()
{
	pDlg->UpdateMtlDisplay();
}

void FO4ShaderRollupBase::ReloadPanel()
{
	Interval v;
	pDlg->pShader->Update(pDlg->pMtlPar->GetTime(), v);
	LoadPanel(FALSE);
}

INT_PTR FO4ShaderRollupBase::PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
	{
		HDC theHDC = GetDC(hwndDlg);
		hOldPal = GetGPort()->PlugPalette(theHDC);
		ReleaseDC(hwndDlg, theHDC);

		InitializeControls(hwndDlg);
		LoadPanel(TRUE);
	}
	break;

	case WM_PAINT:
		if (!valid)
		{
			valid = TRUE;
			ReloadPanel();
		}
		return FALSE;

	case WM_CLOSE:
	case WM_DESTROY:
	case WM_NCDESTROY:
		ReleaseControls();
		break;

	case WM_COMMAND:
	{
		switch (HIWORD(wParam))
		{
		case CBN_SELCHANGE:
			CommitValues();
			break;
		}
	} break;
	}

	return FALSE;
}

void FO4ShaderRollupBase::LoadPanel(BOOL)
{
	if (hRollup) {
		UpdateControls();
		UpdateColSwatches();
		UpdateHilite();
	}
}


FO4ShaderMtlRollup::FO4ShaderMtlRollup(FO4ShaderDlg* pDlg) 
	: base(pDlg)
{
	clrSpecular = clrEmittance = nullptr;
	texMButDiffuse = nullptr;
	pShininessSpinner = pAlphaSpinner = pTestRefSpinner = nullptr;
}

FO4ShaderMtlRollup::~FO4ShaderMtlRollup()
{
	FO4ShaderMtlRollup::ReleaseControls();
}

INT_PTR FO4ShaderMtlRollup::PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	base::PanelProc(hwndDlg, msg, wParam, lParam);
	switch (msg) {
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_MAP_DIFFUSE:
			//PostMessage(hwmEdit, WM_TEXMAP_BUTTON, 0, LPARAM(pMtl));
			UpdateMapButtons();
			UpdateMtlDisplay();
			break;

		case IDC_CHK_DITHER:
			CommitValues();
			//UpdateControls();
			UpdateMtlDisplay();
			break;

		case IDC_CHK_SPECENABLE:
			CommitValues();
			//UpdateControls();
			UpdateMtlDisplay();
			break;

		default:
			CommitValues();
			break;
		}
	}
	break;

	case CC_COLOR_SEL:
	case CC_COLOR_DROP:
	{
		switch (LOWORD(wParam))
		{
		case IDC_CLR_SPECULAR:  clrSpecular->EditThis(FALSE); break;
		case IDC_CLR_EMITTANCE: clrEmittance->EditThis(FALSE); break;
		}
	}
	break;
	case CC_COLOR_BUTTONDOWN:
		theHold.Begin();
		break;
	case CC_COLOR_BUTTONUP:
		if (HIWORD(wParam)) theHold.Accept(GetString(IDS_DS_PARAMCHG));
		else theHold.Cancel();
		UpdateMtlDisplay();
		break;
	case CC_COLOR_CHANGE:
	{
		int buttonUp = HIWORD(wParam);
		if (buttonUp) theHold.Begin();
		CommitValues();
		//UpdateControls();
		if (buttonUp) {
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			// DS: 5/3/99-  this was commented out. I put it back in, because
			// it is necessary for the Reset button in the color picker to 
			// update the viewport.          
			UpdateMtlDisplay();
		}
	}
	break;

	case CC_SPINNER_CHANGE:
		if (!theHold.Holding()) theHold.Begin();
		CommitValues();
		// UpdateControls();
		// UpdateHilite();
		// UpdateMtlDisplay();
		break;

	case CC_SPINNER_BUTTONDOWN:
		theHold.Begin();
		break;

	case WM_CUSTEDIT_ENTER:
	case CC_SPINNER_BUTTONUP:
		if (HIWORD(wParam) || msg == WM_CUSTEDIT_ENTER)
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
		else
			theHold.Cancel();
		UpdateMtlDisplay();
		break;
	}
	//exit:
	return FALSE;
}

void FO4ShaderBaseRollup::UpdateControls()
{
	FO4Shader* pShader = pDlg->pShader;
	if (inUpdate)
		return;

	BOOL update = inUpdate;
	inUpdate = TRUE;

	HWND hWnd = this->hRollup;
	if (p_name_edit && pShader->pMtlFileRef)
		p_name_edit->SetText(T2W(pShader->pMtlFileRef->materialName));

	if ( pShader->HasBGSM() )
		SendDlgItemMessage(hWnd, IDC_CUSTOM_SHADER, CB_SETCURSEL, WPARAM(0), LPARAM(0));
	else if (pShader->HasBGEM())
		SendDlgItemMessage(hWnd, IDC_CUSTOM_SHADER, CB_SETCURSEL, WPARAM(1), LPARAM(0));
	else
		SendDlgItemMessage(hWnd, IDC_CUSTOM_SHADER, CB_SETCURSEL, WPARAM(-1), LPARAM(0));

	UpdateHilite();
	UpdateVisible();
	NotifyChanged();
	inUpdate = update;
}

void FO4ShaderBaseRollup::CommitValues()
{
	TSTR matName;
	FO4Shader* pShader = pDlg->pShader;

	if (p_name_edit && pShader->pMtlFileRef) {
		p_name_edit->GetText(matName);
		pShader->pMtlFileRef->materialName = matName;
	}
}

INT_PTR FO4ShaderBaseRollup::PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TCHAR tmp[MAX_PATH];
	Interface *gi = GetCOREInterface();
	FO4Shader* pShader = pDlg ? pDlg->pShader : nullptr;

	base::PanelProc(hwndDlg, msg, wParam, lParam);

	if (!pShader) return FALSE;

	switch (msg)
	{
	case WM_CLOSE:
	case WM_DESTROY:
	case WM_NCDESTROY:
		ReleaseControls();
		pDlg->DeleteRollups();
		break;

	case WM_COMMAND:
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			switch (LOWORD(wParam))
			{
			case IDC_CUSTOM_SHADER:
				int idx = int(SendDlgItemMessage(hwndDlg, IDC_CUSTOM_SHADER, CB_GETCURSEL, WPARAM(0), LPARAM(0)));
				if (idx == 0) { pShader->ChangeShader(BGSMFILE_CLASS_ID); }
				if (idx == 1) { pShader->ChangeShader(BGEMFILE_CLASS_ID); }
				break;
			}
		}
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case IDC_BTN_MTL_LOAD:
			{
				TCHAR filter[120], *pfilter = filter;
				pfilter = _tcscpy(pfilter, TEXT("Material Files (*.BGSM,*.BGEM, *.JSON)"));
				pfilter += _tcslen(pfilter), *pfilter++ = '\0';
				if (pShader->HasBGSM()) {
					_tcscpy(pfilter, TEXT("*.BGSM"));
					pfilter += _tcslen(pfilter), *pfilter++ = '\0';
				} else if (pShader->HasBGEM()) {
					_tcscpy(pfilter, TEXT("*.BGEM"));
					pfilter += _tcslen(pfilter), *pfilter++ = '\0';
				}
				_tcscpy(pfilter, TEXT("*.JSON"));
				pfilter += _tcslen(pfilter), *pfilter++ = '\0';
				*pfilter++ = '\0';
				*pfilter = '\0';

				if (_taccess(pShader->pMtlFileRef->materialFileName, 0) != -1) {
					_tcscpy(tmp, pShader->pMtlFileRef->materialFileName);
				} else {
					_tcscpy(tmp, pShader->pMtlFileRef->materialName);
				}

				OPENFILENAME ofn;
				memset(&ofn, 0, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = gi->GetMAXHWnd();
				ofn.lpstrFilter = filter;
				ofn.lpstrFile = tmp;
				ofn.nMaxFile = _countof(tmp);
				ofn.lpstrTitle = TEXT("Browse for Material File...");
				ofn.lpstrDefExt = pShader->HasBGSM() ? TEXT("BGSM") : TEXT("BGEM");
				ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
				if (GetOpenFileName(&ofn)) {
					// trim
					pShader->pMtlFileRef->materialName = tmp;
					pShader->pMtlFileRef->materialFileName = tmp;

					if (pShader->HasBGSM()) {
						BGSMFile materialData;
						if (ReadBGSMFile(tmp, materialData)) {
							pShader->LoadBGSM(materialData);
							pShader->LoadMaterial(pDlg->pMtl, nullptr);
						}
					} else if (pShader->HasBGEM()) {
						BGEMFile materialData;
						if (ReadBGEMFile(tmp, materialData)) {
							pShader->LoadBGEM(materialData);
							pShader->LoadMaterial(pDlg->pMtl, nullptr);
						}
					}
					// find the material prefix part
					PathRemoveExtension(tmp);
					PathAddExtension(tmp, pShader->HasBGSM() ? TEXT(".BGSM") : TEXT(".BGEM"));
					for (LPCTSTR filepart = tmp; filepart != nullptr; filepart = PathFindNextComponent(filepart)) {
						if (wildmatch(TEXT("materials\\*"), filepart)) {
							pShader->pMtlFileRef->materialName = filepart;							
							break;
						}						
					}
					UpdateControls();
				}
			} break;

			case IDC_BTN_MTL_SAVE:
			{
			} break;
			}
		}
	case WM_CUSTEDIT_ENTER:
		switch (LOWORD(wParam)) {

		case IDC_ED_MTL_FILE:
			if (p_name_edit) {
				TSTR text;
				p_name_edit->GetText(text);
				pShader->SetName(text);
			} break;
		}
		break;

	}
	return FALSE;
}

INT_PTR CALLBACK  FO4ShaderBaseRollup::DlgRollupProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	FO4ShaderBaseRollup *theDlg;
	if (msg == WM_INITDIALOG) {
		theDlg = reinterpret_cast<FO4ShaderBaseRollup*>(lParam);
		DLSetWindowLongPtr(hwndDlg, lParam);
	}
	else {
		if ((theDlg = DLGetWindowLongPtr<FO4ShaderBaseRollup *>(hwndDlg)) == nullptr)
			return FALSE;
	}
	++theDlg->isActive;
	INT_PTR res = theDlg->PanelProc(hwndDlg, msg, wParam, lParam);
	--theDlg->isActive;
	return res;
}


INT_PTR CALLBACK  FO4ShaderMtlRollup::DlgRollupProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	FO4ShaderMtlRollup *theDlg;
	if (msg == WM_INITDIALOG) {
		theDlg = reinterpret_cast<FO4ShaderMtlRollup*>(lParam);
		DLSetWindowLongPtr(hwndDlg, lParam);
	}
	else {
		if ((theDlg = DLGetWindowLongPtr<FO4ShaderMtlRollup *>(hwndDlg)) == nullptr)
			return FALSE;
	}
	++theDlg->isActive;
	INT_PTR res = theDlg->PanelProc(hwndDlg, msg, wParam, lParam);
	--theDlg->isActive;
	return res;
}

FO4ShaderBGSMRollup::FO4ShaderBGSMRollup(FO4ShaderDlg* pDlg) : FO4ShaderRollupBase(pDlg)
{
}

FO4ShaderBGSMRollup::~FO4ShaderBGSMRollup()
{
}

INT_PTR FO4ShaderBGSMRollup::PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return base::PanelProc(hwndDlg, msg, wParam, lParam);
}

INT_PTR CALLBACK  FO4ShaderBGSMRollup::DlgRollupProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	FO4ShaderBGSMRollup *theDlg;
	if (msg == WM_INITDIALOG) {
		theDlg = reinterpret_cast<FO4ShaderBGSMRollup*>(lParam);
		DLSetWindowLongPtr(hwndDlg, lParam);
	}
	else {
		if ((theDlg = DLGetWindowLongPtr<FO4ShaderBGSMRollup *>(hwndDlg)) == nullptr)
			return FALSE;
	}
	++theDlg->isActive;
	INT_PTR res = theDlg->PanelProc(hwndDlg, msg, wParam, lParam);
	--theDlg->isActive;
	return res;
}

FO4ShaderBGEMRollup::FO4ShaderBGEMRollup(FO4ShaderDlg* pDlg) : FO4ShaderRollupBase(pDlg)
{
}

FO4ShaderBGEMRollup::~FO4ShaderBGEMRollup()
{
}

INT_PTR FO4ShaderBGEMRollup::PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return base::PanelProc(hwndDlg, msg, wParam, lParam);
}

INT_PTR CALLBACK  FO4ShaderBGEMRollup::DlgRollupProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	FO4ShaderBGEMRollup *theDlg;
	if (msg == WM_INITDIALOG) {
		theDlg = reinterpret_cast<FO4ShaderBGEMRollup*>(lParam);
		DLSetWindowLongPtr(hwndDlg, lParam);
	}
	else {
		if ((theDlg = DLGetWindowLongPtr<FO4ShaderBGEMRollup *>(hwndDlg)) == nullptr)
			return FALSE;
	}
	++theDlg->isActive;
	INT_PTR res = theDlg->PanelProc(hwndDlg, msg, wParam, lParam);
	--theDlg->isActive;
	return res;
}

#pragma endregion

#pragma region (" Dialog Creation and Destruction ")

////////////////////////////////////////////////////////////////////////////
// 
//  Concrete Creation of Rollups in Shader Dialogs
//
int FO4Shader::NParamDlgs()
{
	return 1;
}

ShaderParamDlg* FO4Shader::GetParamDlg(int n)
{
	return reinterpret_cast<ShaderParamDlg*>(pDlg);
}

void FO4Shader::SetParamDlg(ShaderParamDlg* newDlg, int n)
{
	pDlg = reinterpret_cast<FO4ShaderDlg*>(newDlg);
}

ShaderParamDlg* FO4Shader::CreateParamDialog(HWND hOldRollup, HWND hwMtlEdit, IMtlParams *imp, StdMat2* theMtl, int rollupOpen, int n)
{
	Interval v;
	Update(imp->GetTime(), v);

	// if (pDlg) //?? check for existing dialog?
	pDlg = new FO4ShaderDlg(hwMtlEdit, imp);
	pDlg->SetThings(theMtl, this);
	LoadStdShaderResources();
	int rollupflags = rolloutOpen ? 0 : APPENDROLL_CLOSED;
	{
		FO4ShaderBaseRollup *pRollup = new FO4ShaderBaseRollup(pDlg);
		pDlg->pBaseRollup = pRollup;

		if (hOldRollup) {
			HWND hRollup = imp->ReplaceRollupPage(hOldRollup, hInstance, MAKEINTRESOURCE(IDD_FO4SHADER_BASE),
				FO4ShaderBaseRollup::DlgRollupProc, GetString(IDS_FO4_SHADER_BASIC),
				reinterpret_cast<LPARAM>(pRollup), rollupflags, ROLLUP_CAT_STANDARD);
			pRollup->hRollup = hRollup;
		} else {
			HWND hRollup = imp->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_FO4SHADER_BASE),
				FO4ShaderBaseRollup::DlgRollupProc, GetString(IDS_FO4_SHADER_BASIC),
				reinterpret_cast<LPARAM>(pRollup), rollupflags, ROLLUP_CAT_STANDARD);
			pRollup->hRollup = hRollup;
		}
	}
	{
		FO4ShaderMtlRollup *pRollup = new FO4ShaderMtlRollup(pDlg);
		pDlg->pMtlRollup = pRollup;
		HWND hRollup = imp->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_FO4SHADER_MTL),
			 FO4ShaderMtlRollup::DlgRollupProc, GetString(IDS_FO4_SHADER_MTL),
			reinterpret_cast<LPARAM>(pRollup), APPENDROLL_CLOSED, ROLLUP_CAT_STANDARD+1);
		pRollup->hRollup = hRollup;
	}
	FixRollups();
	return static_cast<ShaderParamDlg*>(pDlg);
}

// fix references too rollups
void FO4Shader::FixRollups()
{
	if (pDlg == nullptr)
		return;

	if (HasBGSM())
	{
		if (!pDlg->pBGSMRollup)
		{
			FO4ShaderBGSMRollup *pRollup = new FO4ShaderBGSMRollup(pDlg);
			pDlg->pBGSMRollup = pRollup;
			HWND hRollup = pDlg->pMtlPar->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_FO4SHADER_2),
				FO4ShaderBGSMRollup::DlgRollupProc, GetString(IDS_FO4_SHADER_BGSM),
				reinterpret_cast<LPARAM>(pRollup), APPENDROLL_CLOSED, ROLLUP_CAT_STANDARD+2);
			pRollup->hRollup = hRollup;
		}
	} else if (pDlg->pBGSMRollup) {
		pDlg->pMtlPar->DeleteRollupPage(pDlg->pBGSMRollup->hRollup);
		pDlg->pBGSMRollup = nullptr;
	}

	if (HasBGEM())
	{
		if (!pDlg->pBGEMRollup)
		{
			FO4ShaderBGEMRollup *pRollup = new FO4ShaderBGEMRollup(pDlg);
			pDlg->pBGEMRollup = pRollup;
			HWND hRollup = pDlg->pMtlPar->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_FO4SHADER_2),
				FO4ShaderBGEMRollup::DlgRollupProc, GetString(IDS_FO4_SHADER_BGEM),
				reinterpret_cast<LPARAM>(pRollup), APPENDROLL_CLOSED, ROLLUP_CAT_STANDARD+2);
			pRollup->hRollup = hRollup;
		}
	}
	else if (pDlg->pBGEMRollup) {
		pDlg->pMtlPar->DeleteRollupPage(pDlg->pBGEMRollup->hRollup);
		pDlg->pBGEMRollup = nullptr;
	}

}



#if VERSION_3DSMAX < (17000<<16) // Version 17 (2015)
RefResult FO4Shader::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message)
#else
RefResult FO4Shader::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate)
#endif
{
	switch (message) {
	case REFMSG_CHANGE:
		ivalid.SetEmpty();
		if (hTarget == pb_base) {
			// update UI if paramblock changed, possibly from scripter
			ParamID changingParam = pb_base->LastNotifyParamID();
			// reload the dialog if present
			if (pDlg) {
				pDlg->UpdateDialog(changingParam);
			}
		}
		break;
	}
	return(REF_SUCCEED);
}


void FO4ShaderDlg::DeleteRollups()
{
	delete pBaseRollup; pBaseRollup = nullptr;
	delete pMtlRollup; pMtlRollup = nullptr;
	delete pBGSMRollup; pBGSMRollup = nullptr;
	delete pBGEMRollup; pBGEMRollup = nullptr;
}
#pragma endregion

#pragma region ("Material Rollup")
/////////////////////////////////////////////////////////////////////////
//
//  Material Rollup
//
void FO4ShaderMtlRollup::InitializeControls(HWND hWnd)
{

	HDC theHDC = GetDC(hWnd);
	hOldPal = GetGPort()->PlugPalette(theHDC);
	ReleaseDC(hWnd, theHDC);

	//////////////////////////////////////////////////////////////////////////
	clrSpecular = GetIColorSwatch(GetDlgItem(hWnd, IDC_CLR_SPECULAR));
	clrEmittance = GetIColorSwatch(GetDlgItem(hWnd, IDC_CLR_EMITTANCE));

	//texMButDiffuse = GetICustButton(GetDlgItem(hWnd, IDC_MAP_DIFFUSE));
	//texMButDiffuse->SetRightClickNotify(TRUE);
	//texMButDiffuse->SetDADMgr(&dadMgr);

	pShininessSpinner = GetISpinner(GetDlgItem(hWnd, IDC_SPN_SHININESS));
	pShininessSpinner->SetLimits(0.0f, 200.0f, TRUE);
	pShininessSpinner->SetScale(10.0f);
	pShininessSpinner->SetResetValue(10.0f);
	pShininessSpinner->LinkToEdit(GetDlgItem(hWnd, IDC_EDT_SHININESS), EDITTYPE_POS_FLOAT);

	pAlphaSpinner = GetISpinner(GetDlgItem(hWnd, IDC_SPN_ALPHA));
	pAlphaSpinner->SetLimits(0.0f, 1.0f, TRUE);
	pAlphaSpinner->SetScale(0.1f);
	pAlphaSpinner->SetResetValue(0.0f);
	pAlphaSpinner->LinkToEdit(GetDlgItem(hWnd, IDC_EDT_ALPHA), EDITTYPE_POS_FLOAT);

	//////////////////////////////////////////////////////////////////////////
	for (const EnumLookupType* flag = TransparencyModes; flag->name != nullptr; ++flag) {
		SendDlgItemMessage(hWnd, IDC_CBO_TRANS_SRC, CB_ADDSTRING, 0, LPARAM(flag->name));
		SendDlgItemMessage(hWnd, IDC_CBO_TRANS_DEST, CB_ADDSTRING, 0, LPARAM(flag->name));
	}
	//////////////////////////////////////////////////////////////////////////
	for (const EnumLookupType* flag = VertexModes; flag->name != nullptr; ++flag)
		SendDlgItemMessage(hWnd, IDC_CBO_VERTEX_SRC, CB_ADDSTRING, 0, LPARAM(flag->name));

	for (const EnumLookupType* flag = LightModes; flag->name != nullptr; ++flag)
		SendDlgItemMessage(hWnd, IDC_CBO_VERTEX_LIGHT, CB_ADDSTRING, 0, LPARAM(flag->name));

	//////////////////////////////////////////////////////////////////////////
	for (const EnumLookupType* flag = ApplyModes; flag->name != nullptr; ++flag)
		SendDlgItemMessage(hWnd, IDC_CBO_APPLY_MODE, CB_ADDSTRING, 0, LPARAM(flag->name));

	//////////////////////////////////////////////////////////////////////////
	for (const EnumLookupType* flag = TestModes; flag->name != nullptr; ++flag)
		SendDlgItemMessage(hWnd, IDC_CBO_TESTMODE, CB_ADDSTRING, 0, LPARAM(flag->name));

	pTestRefSpinner = GetISpinner(GetDlgItem(hWnd, IDC_SPN_TESTREF));
	pTestRefSpinner->SetLimits(0, 256, TRUE);
	pTestRefSpinner->SetScale(1.0f);
	pTestRefSpinner->SetResetValue(0.0f);
	pTestRefSpinner->LinkToEdit(GetDlgItem(hWnd, IDC_EDT_TESTREF), EDITTYPE_POS_INT);

	UpdateControls();
}


void FO4ShaderMtlRollup::ReleaseControls()
{
	if (hOldPal)
	{
		HDC hdc = GetDC(hRollup);
		GetGPort()->RestorePalette(hdc, hOldPal);
		ReleaseDC(hRollup, hdc);
		hOldPal = nullptr;
	}

	if (clrSpecular) { ReleaseIColorSwatch(clrSpecular); clrSpecular = nullptr; };
	if (clrEmittance) { ReleaseIColorSwatch(clrEmittance); clrEmittance = nullptr; };

	if (texMButDiffuse) { ReleaseICustButton(texMButDiffuse); texMButDiffuse = nullptr; };

	if (pShininessSpinner) { ReleaseISpinner(pShininessSpinner); pShininessSpinner = nullptr; };
	if (pAlphaSpinner) { ReleaseISpinner(pAlphaSpinner); pAlphaSpinner = nullptr; };

	if (pTestRefSpinner) { ReleaseISpinner(pTestRefSpinner); pTestRefSpinner = nullptr; };
}

void FO4ShaderMtlRollup::UpdateControls()
{
	FO4Shader* pShader = pDlg->pShader;
	if (inUpdate)
		return;

	BOOL update = inUpdate;
	inUpdate = TRUE;

	HWND hWnd = this->hRollup;
	IParamBlock2 *pb = pShader->pb_base;

	UpdateColSwatches();
	UpdateMapButtons();

	//////////////////////////////////////////////////////////////////////////

	UpdateHilite();
	UpdateVisible();
	NotifyChanged();
	inUpdate = update;
}

void FO4ShaderMtlRollup::UpdateVisible()
{

}

void FO4ShaderMtlRollup::CommitValues()
{
	FO4Shader* pShader = pDlg->pShader;

	BOOL update = inUpdate;
	inUpdate = TRUE;


	HWND hWnd = this->hRollup;

	//////////////////////////////////////////////////////////////////////////

	UpdateVisible();
	inUpdate = update;
}

#pragma endregion
