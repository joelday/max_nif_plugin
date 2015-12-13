#pragma once

#include "../NifProps/NifProps.h"
#include "../MtlUtils/mtldefine.h"

const Class_ID MTLFILE_CLASS_ID(0x8efac6e7, 0x86cf438a);
const Class_ID BGSMFILE_CLASS_ID(0x4b9416f7, 0x771e4e55);
const Class_ID BGEMFILE_CLASS_ID(0xb79b2136, 0x07df5403);
const SClass_ID MATERIALFILE_CLASS_ID = 0xC001;

enum MaterialFileType {
	MFT_None,
	MFT_BGSM,
	MFT_BGEM,
};

class MaterialReference : public ReferenceTarget
{
public:
	MaterialFileType file_type;
	TSTR materialName;
	TSTR materialFileName;

	MaterialReference() {
		file_type = MFT_None;
	}

	Class_ID ClassID() override { return MTLFILE_CLASS_ID; }
	SClass_ID SuperClassID() override { return MATERIALFILE_CLASS_ID; }
	static TSTR GetName() { return GetString(IDS_SH_NAME); }
	void GetClassName(TSTR& s) override { s = GetName(); }

	static IOResult LoadMaterialChunk(BaseMaterial &mtl, ILoad* iload);
	static IOResult SaveMaterialChunk(BaseMaterial &mtl, ISave* isave);

	ReferenceTarget* Clone(RemapDir& remap) override
	{
		ReferenceTarget* result = new MaterialReference();
		BaseClone(this, result, remap);
		return result;
	}

	void SetFileName(const TSTR& name, const TSTR& path) {
		materialName = name;
		materialFileName = path;
	}

	void SetName(const TSTR& name) {
		materialName = name;
	}

	//bool IsBGSM() { return ClassID() == BGSMFILE_CLASS_ID; }

	//const int MY_REFERENCE = 1;
	//void BaseClone(ReferenceTarget* from, ReferenceTarget* to, RemapDir& remap) override
	//{
	//	if (!to || !from || from == to)
	//		return;
	//	MaterialReference::BaseClone(from, to, remap);
	//	to->ReplaceReference(MY_REFERENCE, remap.CloneRef(from->GetReference(MY_REFERENCE)));
	//}

#if VERSION_3DSMAX < (17000<<16) // Version 17 (2015)
	RefResult	NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message) override { return(REF_SUCCEED); }
#else
	RefResult	NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate) override { return(REF_SUCCEED); }
#endif
protected:
	~MaterialReference() {}
};

class BGSMFileReference : public MaterialReference
{
public:
	BGSMFile file;

	BGSMFileReference() {
		file_type = MFT_BGSM;
		InitialzeBGSM(file);
	}
	Class_ID ClassID() override { return BGSMFILE_CLASS_ID; }
	void DeleteThis() override { delete this; }

	IOResult Load(ILoad* iload) override;
	IOResult Save(ISave* isave) override;

	IOResult LoadMaterialChunk(ILoad* iload);
	IOResult SaveMaterialChunk(ISave* isave);

};


class BGEMFileReference : public MaterialReference
{
public:
	BGEMFile file;

	BGEMFileReference() {
		file_type = MFT_BGEM;
		InitialzeBGEM(file);
	}
	Class_ID ClassID() override { return BGEMFILE_CLASS_ID; }
	void DeleteThis() override { delete this; }

	IOResult Load(ILoad* iload) override;
	IOResult Save(ISave* isave) override;

	IOResult LoadMaterialChunk(ILoad* iload);
	IOResult SaveMaterialChunk(ISave* isave);

};