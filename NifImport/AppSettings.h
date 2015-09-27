/**********************************************************************
*<
FILE: AppSettings.h

DESCRIPTION:	AppSetting helper class for managing game specific
               file settings.

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#ifndef _APPSETTINGS_H_
#define _APPSETTINGS_H_

#include "niutils.h"

class AppSettings
{
public:
   AppSettings(const tstring& name) 
      : Name(name)
      , parsedImages(false) 
      , useSkeleton(false)
      , goToSkeletonBindPosition(true)
      , disableCreateNubsForBones(false)
   {}

   tstring Name;
   tstring rootPath;
   bool parsedImages;
   tstringlist searchPaths;
   tstringlist textureRootPaths;
   tstringlist rootPaths;
   tstringlist extensions;
   tstring Skeleton;
   bool useSkeleton;
   bool goToSkeletonBindPosition;
   bool disableCreateNubsForBones;
   NameValueCollection Environment;
   NameValueCollection imgTable;
   tstringlist dummyNodeMatches;
   int applyOverallTransformToSkinAndBones;

   static void Initialize(Interface *gi);
   void ReadSettings(tstring iniFile);
   tstring FindImage(const tstring& fname);

   // Check whether the given file is a child of the root paths
   bool IsFileInRootPaths(const tstring& fname);

   template<typename T>
   inline T GetSetting(tstring setting){
      T v;
      NameValueCollection::iterator itr = Environment.find(setting);
      if (itr != Environment.end()){
         tstringstream sstr((*itr).second);
         sstr >> v;
      }
      return v;
   }
   template<>
   inline tstring GetSetting(tstring setting){
      NameValueCollection::iterator itr = Environment.find(setting);
      if (itr != Environment.end())
         return (*itr).second;
      return tstring();
   }

   template<typename T>
   inline T GetSetting(tstring setting, T Default){
      NameValueCollection::iterator itr = Environment.find(setting);
      if (itr != Environment.end()){
         T v;
         tstringstream sstr((*itr).second);
         sstr >> v;
         return v;
      }
      return Default;
   }
   template<>
   inline tstring GetSetting(tstring setting, tstring Default){
      NameValueCollection::iterator itr = Environment.find(setting);
      if (itr != Environment.end())
         return (*itr).second;
      return Default;
   }
};

typedef std::list<AppSettings> AppSettingsMap;

struct AppSettingsNameEquivalence : public ltstr
{
   bool operator()(const AppSettings& n1, const AppSettings& n2) const { 
      return ltstr::operator()(n1.Name, n2.Name);
   }
   bool operator()(const tstring& n1, const AppSettings& n2) const { 
      return ltstr::operator()(n1, n2.Name);
   }
   bool operator()(const AppSettings& n1, const tstring& n2) const { 
      return ltstr::operator()(n1.Name, n2);
   }
};

// The Global Map
//  Global so that I dont have to parse the texture directories on every import
extern AppSettingsMap TheAppSettings;

inline AppSettings* FindAppSetting(const tstring& name){
   AppSettingsNameEquivalence equiv;
   for (AppSettingsMap::iterator itr=TheAppSettings.begin(), end = TheAppSettings.end(); itr != end; ++itr){
      if (!equiv(*itr, name) && !equiv(name, *itr))
         return &(*itr);
   }
   return nullptr;
}

#endif //_APPSETTINGS_H_