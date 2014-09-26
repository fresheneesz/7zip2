// UpdateGUI.cpp

#include "StdAfx.h"

#include "UpdateGUI.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"
#include "Common/StringToInt.h"

#include "Windows/Error.h"
#include "Windows/FileDir.h"
#include "Windows/Thread.h"

#include "../Common/WorkDir.h"

#include "../Explorer/MyMessages.h"

#include "../FileManager/LangUtils.h"
#include "../FileManager/ProgramLocation.h"
#include "../FileManager/StringUtils.h"
#include "../FileManager/resourceGui.h"

#include "CompressDialog.h"
#include "UpdateGUI.h"

#include "resource2.h"

using namespace NWindows;
using namespace NFile;

static const wchar_t *kDefaultSfxModule = L"7z.sfx";
static const wchar_t *kSFXExtension = L"exe";

extern void AddMessageToString(UString &dest, const UString &src);

UString HResultToMessage(HRESULT errorCode);

class CThreadUpdating: public CProgressThreadVirt
{
  HRESULT ProcessVirt();
public:
  CCodecs *codecs;
  CUpdateCallbackGUI *UpdateCallbackGUI;
  const NWildcard::CCensor *WildcardCensor;
  CUpdateOptions *Options;
};
 
HRESULT CThreadUpdating::ProcessVirt()
{
  CUpdateErrorInfo ei;
  HRESULT res = UpdateArchive(codecs, *WildcardCensor, *Options,
     ei, UpdateCallbackGUI, UpdateCallbackGUI);
  ErrorMessage = ei.Message;
  ErrorPath1 = ei.FileName;
  ErrorPath2 = ei.FileName2;
  if (ei.SystemError != S_OK && ei.SystemError != E_FAIL && ei.SystemError != E_ABORT)
    return ei.SystemError;
  return res;
}

static void AddProp(CObjectVector<CProperty> &properties, const UString &name, const UString &value)
{
  CProperty prop;
  prop.Name = name;
  prop.Value = value;
  properties.Add(prop);
}

static void AddProp(CObjectVector<CProperty> &properties, const UString &name, UInt32 value)
{
  wchar_t tmp[32];
  ConvertUInt64ToString(value, tmp);
  AddProp(properties, name, tmp);
}

static void AddProp(CObjectVector<CProperty> &properties, const UString &name, bool value)
{
  AddProp(properties, name, value ? UString(L"on"): UString(L"off"));
}

static bool IsThereMethodOverride(bool is7z, const UString &propertiesString)
{
  UStringVector strings;
  SplitString(propertiesString, strings);
  for (int i = 0; i < strings.Size(); i++)
  {
    const UString &s = strings[i];
    if (is7z)
    {
      const wchar_t *end;
      UInt64 n = ConvertStringToUInt64(s, &end);
      if (n == 0 && *end == L'=')
        return true;
    }
    else
    {
      if (s.Length() > 0)
        if (s[0] == L'm' && s[1] == L'=')
          return true;
    }
  }
  return false;
}

static void ParseAndAddPropertires(CObjectVector<CProperty> &properties,
    const UString &propertiesString)
{
  UStringVector strings;
  SplitString(propertiesString, strings);
  for (int i = 0; i < strings.Size(); i++)
  {
    const UString &s = strings[i];
    CProperty property;
    int index = s.Find(L'=');
    if (index < 0)
      property.Name = s;
    else
    {
      property.Name = s.Left(index);
      property.Value = s.Mid(index + 1);
    }
    properties.Add(property);
  }
}

static UString GetNumInBytesString(UInt64 v)
{
  wchar_t s[32];
  ConvertUInt64ToString(v, s);
  size_t len = wcslen(s);
  s[len++] = L'B';
  s[len] = L'\0';
  return s;
}

static void SetOutProperties(
    CObjectVector<CProperty> &properties,
    bool is7z,
    UInt32 level,
    bool setMethod,
    const UString &method,
    UInt32 dictionary,
    bool orderMode,
    UInt32 order,
    bool solidIsSpecified, UInt64 solidBlockSize,
    bool multiThreadIsAllowed, UInt32 numThreads,
    const UString &encryptionMethod,
    bool encryptHeadersIsAllowed, bool encryptHeaders,
    bool /* sfxMode */)
{
  if (level != (UInt32)(Int32)-1)
    AddProp(properties, L"x", (UInt32)level);
  if (setMethod)
  {
    if (!method.IsEmpty())
      AddProp(properties, is7z ? L"0": L"m", method);
    if (dictionary != (UInt32)(Int32)-1)
    {
      UString name;
      if (is7z)
        name = L"0";
      if (orderMode)
        name += L"mem";
      else
        name += L"d";
      AddProp(properties, name, GetNumInBytesString(dictionary));
    }
    if (order != (UInt32)(Int32)-1)
    {
      UString name;
      if (is7z)
        name = L"0";
      if (orderMode)
        name += L"o";
      else
        name += L"fb";
      AddProp(properties, name, (UInt32)order);
    }
  }
    
  if (!encryptionMethod.IsEmpty())
    AddProp(properties, L"em", encryptionMethod);

  if (encryptHeadersIsAllowed)
    AddProp(properties, L"he", encryptHeaders);
  if (solidIsSpecified)
    AddProp(properties, L"s", GetNumInBytesString(solidBlockSize));
  if (multiThreadIsAllowed)
    AddProp(properties, L"mt", numThreads);
}

static HRESULT ShowDialog(
    CCodecs *codecs,
    const NWildcard::CCensor &censor,
    CUpdateOptions &options, CUpdateCallbackGUI *callback, HWND hwndParent)
{

  if (options.Commands.Size() != 1)
    throw "It must be one command";
  UString currentDirPrefix;
  #ifndef UNDER_CE
  {
    if (!NDirectory::MyGetCurrentDirectory(currentDirPrefix))
      return E_FAIL;
    NName::NormalizeDirPathPrefix(currentDirPrefix);
  }
  #endif


  //::MessageBoxW(NULL, (LPCWSTR)L"ShowDialog1", (LPCWSTR)L"mooo", MB_OK);

  bool oneFile = false;
  NFind::CFileInfoW fileInfo;
  UString name;
  if (censor.Pairs.Size() > 0)
  {
    const NWildcard::CPair &pair = censor.Pairs[0];
    if (pair.Head.IncludeItems.Size() > 0)
    {
      const NWildcard::CItem &item = pair.Head.IncludeItems[0];
      if (item.ForFile)
      {
        name = pair.Prefix;
        for (int i = 0; i < item.PathParts.Size(); i++)
        {
          if (i > 0)
            name += WCHAR_PATH_SEPARATOR;
          name += item.PathParts[i];
        }
        if (fileInfo.Find(name))
        {
          if (censor.Pairs.Size() == 1 && pair.Head.IncludeItems.Size() == 1)
            oneFile = !fileInfo.IsDir();
        }
      }
    }
  }

    //::MessageBoxW(NULL, (LPCWSTR)L"ShowDialog1.2", (LPCWSTR)L"mooo", MB_OK);
    
  CCompressDialog dialog;
  NCompressDialog::CInfo &di = dialog.Info;
  dialog.ArcFormats = &codecs->Formats;
  for (int i = 0; i < codecs->Formats.Size(); i++)
  {
    const CArcInfoEx &ai = codecs->Formats[i];
    if (ai.Name.CompareNoCase(L"swfc") == 0)
      if (!oneFile || name.Right(4).CompareNoCase(L".swf") != 0)
        continue;
    if (ai.UpdateEnabled && (oneFile || !ai.KeepName))
      dialog.ArcIndices.Add(i);
  }
  if (dialog.ArcIndices.Size() == 0)
  {
    ShowErrorMessage(L"No Update Engines");
    return E_FAIL;
  }

//::MessageBoxW(NULL, (LPCWSTR)L"ShowDialog1.5", (LPCWSTR)L"mooo", MB_OK);

  // di.ArchiveName = options.ArchivePath.GetFinalPath();
  di.ArchiveName = options.ArchivePath.GetPathWithoutExt();
  dialog.OriginalFileName = options.ArchivePath.Prefix + fileInfo.Name;
    
  di.CurrentDirPrefix = currentDirPrefix;
  di.SFXMode = options.SfxMode;
  di.OpenShareForWrite = options.OpenShareForWrite;
  
  if (callback->PasswordIsDefined)
    di.Password = callback->Password;
    
  di.KeepName = !oneFile;

    //::MessageBoxW(NULL, (LPCWSTR)L"ShowDialog1.6", (LPCWSTR)L"mooo", MB_OK);
  if (dialog.Create(hwndParent) != IDOK)
    return E_ABORT;
    
  options.VolumesSizes = di.VolumeSizes;
  /*
  if (di.VolumeSizeIsDefined)
  {
    MyMessageBox(L"Splitting to volumes is not supported");
    return E_FAIL;
  }
  */

  //::MessageBoxW(NULL, (LPCWSTR)L"ShowDialog1.7", (LPCWSTR)L"mooo", MB_OK);
  NUpdateArchive::CActionSet &actionSet = options.Commands.Front().ActionSet;


  //::MessageBoxW(NULL, (LPCWSTR)L"ShowDialog2", (LPCWSTR)L"mooo", MB_OK);

  switch(di.UpdateMode)
  {
    case NCompressDialog::NUpdateMode::kAdd:
      actionSet = NUpdateArchive::kAddActionSet;
      break;
    case NCompressDialog::NUpdateMode::kUpdate:
      actionSet = NUpdateArchive::kUpdateActionSet;
      break;
    case NCompressDialog::NUpdateMode::kFresh:
      actionSet = NUpdateArchive::kFreshActionSet;
      break;
    case NCompressDialog::NUpdateMode::kSynchronize:
      actionSet = NUpdateArchive::kSynchronizeActionSet;
      break;
    default:
      throw 1091756;
  }
  const CArcInfoEx &archiverInfo = codecs->Formats[di.FormatIndex];
  callback->PasswordIsDefined = (!di.Password.IsEmpty());
  if (callback->PasswordIsDefined)
    callback->Password = di.Password;

  options.MethodMode.Properties.Clear();

  bool is7z = archiverInfo.Name.CompareNoCase(L"7z") == 0;
  bool methodOverride = IsThereMethodOverride(is7z, di.Options);

  SetOutProperties(
      options.MethodMode.Properties,
      is7z,
      di.Level,
      !methodOverride,
      di.Method,
      di.Dictionary,
      di.OrderMode, di.Order,
      di.SolidIsSpecified, di.SolidBlockSize,
      di.MultiThreadIsAllowed, di.NumThreads,
      di.EncryptionMethod,
      di.EncryptHeadersIsAllowed, di.EncryptHeaders,
      di.SFXMode);
  
  options.OpenShareForWrite = di.OpenShareForWrite;
  ParseAndAddPropertires(options.MethodMode.Properties, di.Options);


  //::MessageBoxW(NULL, (LPCWSTR)L"ShowDialog3", (LPCWSTR)L"mooo", MB_OK);

  if (di.SFXMode)
    options.SfxMode = true;
  options.MethodMode.FormatIndex = di.FormatIndex;

  options.ArchivePath.VolExtension = archiverInfo.GetMainExt();
  if (di.SFXMode)
    options.ArchivePath.BaseExtension = kSFXExtension;
  else
    options.ArchivePath.BaseExtension = options.ArchivePath.VolExtension;
  options.ArchivePath.ParseFromPath(di.ArchiveName);

  NWorkDir::CInfo workDirInfo;
  workDirInfo.Load();
  options.WorkingDir.Empty();
  if (workDirInfo.Mode != NWorkDir::NMode::kCurrent)
  {
    UString fullPath;
    NDirectory::MyGetFullPathName(di.ArchiveName, fullPath);
    options.WorkingDir = GetWorkDir(workDirInfo, fullPath);
    NDirectory::CreateComplexDirectory(options.WorkingDir);
  }
  return S_OK;
}

HRESULT UpdateGUI(
    CCodecs *codecs,
    const NWildcard::CCensor &censor,
    CUpdateOptions &options,
    bool showDialog,
    bool &messageWasDisplayed,
    CUpdateCallbackGUI *callback,
    HWND hwndParent)
{

  //::MessageBoxW(NULL, (LPCWSTR)L"UpdateGUI", (LPCWSTR)L"mooo", MB_OK);
  //*
  messageWasDisplayed = false;
  if (showDialog)
  {
    RINOK(ShowDialog(codecs, censor, options, callback, hwndParent));
  }
  if (options.SfxMode && options.SfxModule.IsEmpty())
  {
    UString folder;
    if (!GetProgramFolderPath(folder))
      folder.Empty();
    options.SfxModule = folder + kDefaultSfxModule;
  }

  CThreadUpdating tu;

  tu.codecs = codecs;

//::MessageBoxW(NULL, (LPCWSTR)L"UpdateGUI2", (LPCWSTR)L"mooo", MB_OK);

  tu.UpdateCallbackGUI = callback;
  tu.UpdateCallbackGUI->ProgressDialog = &tu.ProgressDialog;
  tu.UpdateCallbackGUI->Init();

  UString title = LangString(IDS_PROGRESS_COMPRESSING, 0x02000DC0);

  /*   NOTE: this was commented out before
  if (hwndParent != 0)
  {
    tu.ProgressDialog.MainWindow = hwndParent;
    // tu.ProgressDialog.MainTitle = fileName;
    tu.ProgressDialog.MainAddTitle = title + L" ";
  }
  */
  //*
  tu.WildcardCensor = &censor;
  tu.Options = &options;
  tu.ProgressDialog.IconID = IDI_ICON;

    //::MessageBoxW(NULL, L"UpdateGUI3 ", (LPCWSTR)L"mooo", MB_OK);

    //::MessageBoxW(NULL, tu.Options->MethodMode.Properties.is7z, (LPCWSTR)L"mooo", MB_OK);

  //*/

  //CThreadUpdating tu = createCTUObject(codecs);
  RINOK(tu.Create(title, hwndParent));

    //::MessageBoxW(NULL, (LPCWSTR)L"UpdateGUI4", (LPCWSTR)L"mooo", MB_OK);

  messageWasDisplayed = tu.ThreadFinishedOK &
      tu.ProgressDialog.MessagesDisplayed;
  return tu.Result;
}
    /*
CThreadUpdating createCTUObject(CCodecs *codecs, CUpdateCallbackGUI *callback, const NWildcard::CCensor &censor, CUpdateOptions &options) {
    tu.codecs = codecs;
    tu.UpdateCallbackGUI = callback;

    tu.UpdateCallbackGUI->ProgressDialog = &tu.ProgressDialog;
    tu.UpdateCallbackGUI->Init();

    tu.WildcardCensor = &censor;


    CObjectVector<CProperty> &properties,
    bool is7z,
    UInt32 level,
    bool setMethod,
    const UString &method,
    UInt32 dictionary,
    bool orderMode,
    UInt32 order,
    bool solidIsSpecified, UInt64 solidBlockSize,
    bool multiThreadIsAllowed, UInt32 numThreads,
    const UString &encryptionMethod,
    bool encryptHeadersIsAllowed, bool encryptHeaders,
    bool )

    SetOutProperties(
      options.MethodMode.Properties,
      true,
      kFast,   // from CompressDialog.cpp
      true,
      (UString)L"",
      di.Dictionary,
      di.OrderMode, di.Order,
      di.SolidIsSpecified, di.SolidBlockSize,
      di.MultiThreadIsAllowed, di.NumThreads,
      di.EncryptionMethod,
      di.EncryptHeadersIsAllowed, di.EncryptHeaders,
      di.SFXMode);

    tu.Options = &options;
    tu.ProgressDialog.IconID = IDI_ICON;
}
        */