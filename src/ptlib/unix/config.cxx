/*
 * CONFIG.CXX
 *
 * Portable Windows Library
 *
 * Operating System Classes Implementation
 *
 * Copyright 1993 by Robert Jongbloed and Craig Southeren
 */

#define _CONFIG_CXX

#pragma implementation "config.h"

#include "ptlib.h"


#define SYS_CONFIG_NAME		"pwlib"

#define	APP_CONFIG_DIR		".pwlib_config/"
#define	SYS_CONFIG_DIR		"/usr/local/pwlib/"

#define	EXTENSION		".ini"
#define	ENVIRONMENT_CONFIG_STR	"/\~~environment~~\/"

//
//  a single key/value pair
//
PDECLARE_CLASS (PXConfigValue, PCaselessString)
  public:
    PXConfigValue(const PString & theKey, const PString & theValue = "") 
      : PCaselessString(theKey), value(theValue) { }
    PString GetValue() const { return value; }
    void    SetValue(const PString & theValue) { value = theValue; }

  protected:
    PString  value;
};

//
//  a list of key/value pairs
//
PLIST (PXConfigSectionList, PXConfigValue);

//
//  a list of key value pairs, with a section name
//
PDECLARE_CLASS(PXConfigSection, PCaselessString)
  public:
    PXConfigSection(const PCaselessString & theName) 
      : PCaselessString(theName) { list.AllowDeleteObjects(); }

    PXConfigSectionList & GetList() { return list; }

  protected:
    PXConfigSectionList list;
};

//
// a list of sections
//

PDECLARE_LIST(PXConfig, PXConfigSection)
  public:
    PXConfig(int i = 0);

    void Wait()   { mutex.Wait(); }
    void Signal() { mutex.Signal(); }

    BOOL ReadFromFile (const PFilePath & filename);
    void ReadFromEnvironment (char **envp);

    void SetDirty()   { dirty = TRUE; }

    void      AddInstance();
    BOOL      RemoveInstance(const PFilePath & filename);

  protected:
    int       instanceCount;
    PMutex    mutex;
    BOOL      dirty;
    BOOL      saveOnExit;
};

//
// a dictionary of configurations, keyed by filename
//
PDECLARE_DICTIONARY(ConfigDictionary, PFilePath, PXConfig)
  public:
    ConfigDictionary(int i = 0);
    PXConfig * GetFileConfigInstance(const PFilePath & key, const PFilePath & readKey);
    PXConfig * GetEnvironmentInstance();
    void RemoveInstance(PXConfig * instance);

  protected:
    PMutex     mutex;
    PXConfig * environmentInstance;
};

static ConfigDictionary configDict(0);

#define	new PNEW

//////////////////////////////////////////////////////

PXConfig::PXConfig(int)
{
  // make sure content gets removed
  AllowDeleteObjects();

  // no instances, initially
  instanceCount = 0;

  // we start off clean
  dirty = FALSE;

  // normally save on exit (except for environment configs)
  saveOnExit = TRUE;
}

void PXConfig::AddInstance()
{
  mutex.Wait();
  instanceCount++;
  mutex.Signal();
}

BOOL PXConfig::RemoveInstance(const PFilePath & filename)
{
  mutex.Wait();
  PAssert(instanceCount != 0, "PConfig instance count dec past zero");
  BOOL stat = --instanceCount == 0;

  // save the data if this is the last instance
  if (!stat)
    PError << "Closed instance of " << filename << endl;
  else if (!saveOnExit || !dirty) 
    PError << "Closed unchanged final instance of " << filename << endl;
  else {
    PError << "Closing changed final instance of " << filename << endl;

    // make sure the directory that the file is to be written into exists
    PDirectory dir = filename.GetDirectory();
    if (!dir.Exists() && !dir.Create( 
                                     PFileInfo::UserExecute |
                                     PFileInfo::UserWrite |
                                     PFileInfo::UserRead))
      PProcess::PXShowSystemWarning(2000, "Cannot create PWLIB config dir");
    else {
      PTextFile file;
      if (!file.Open(filename, PFile::WriteOnly))
        PProcess::PXShowSystemWarning(2001, "Cannot create PWLIB config file");
      else {
        for (PINDEX i = 0; i < GetSize(); i++) {
          PXConfigSectionList & section = (*this)[i].GetList();
          file << "[" << (*this)[i] << "]" << endl;
          for (PINDEX j = 0; j < section.GetSize(); j++) {
            PXConfigValue & value = section[j];
            file << value << "=" << value.GetValue() << endl;
          }
          file << endl;
        }
        file.Close();
      }
    }
  }
  mutex.Signal();

  return stat;
}

BOOL PXConfig::ReadFromFile (const PFilePath & filename)
{
PError << "Reading config file " << filename << endl;

  PINDEX len;
  PString line;

  // clear out all information
  RemoveAll();

  // attempt to open file
  PTextFile file;
  if (!file.Open(filename, PFile::ReadOnly))
    return FALSE;

  PXConfigSection * currentSection = NULL;

  // read lines in the file
  while (file.ReadLine(line)) {
    line = line.Trim();
    if ((len = line.GetLength()) > 0) {

      // ignore comments and blank lines 
      char ch = line[0];
      if ((len > 0) && (ch != ';') && (ch != '#')) {
        if (ch == '[') {
          PCaselessString sectionName = (line.Mid(1,len-(line[len-1]==']'?2:1))).Trim();
          PINDEX  index;
          if ((index = GetValuesIndex(sectionName)) != P_MAX_INDEX)
            currentSection = &(*this )[index];
          else {
            currentSection = new PXConfigSection(sectionName);
            Append(currentSection);
          }
        } else if (currentSection != NULL) {
          PINDEX equals = line.Find('=');
          if (equals > 0) {
            PString keyStr = line.Left(equals).Trim();
            PString valStr = line.Right(len - equals - 1).Trim();
            PXConfigValue * value = new PXConfigValue(keyStr, valStr);
            currentSection->GetList().Append(value);
          }
        }
      }
    }
  }
  
  // close the file and return
  file.Close();
  return TRUE;
}

void PXConfig::ReadFromEnvironment (char **envp)
{
  // clear out all information
  RemoveAll();

  PXConfigSection * currentSection = new PXConfigSection("Options");
  Append(currentSection);

  while (*envp != NULL && **envp != '\0') {
    PString line(*envp);
    PINDEX equals = line.Find('=');
    if (equals > 0) {
      PXConfigValue * value = new PXConfigValue(line.Left(equals), line.Right(line.GetLength() - equals - 1));
      currentSection->GetList().Append(value);
    }
    envp++;
  }

  // can't save environment configs
  saveOnExit = FALSE;
}


static BOOL LocateFile(const PString & baseName,
                       PFilePath & readFilename,
                       PFilePath & filename)
{
  PFilePath userFile;

  // check the user's home directory first
  filename = readFilename = PProcess::Current().PXGetHomeDir() +
             APP_CONFIG_DIR + baseName + EXTENSION;
  if (PFile::Exists(filename))
    return TRUE;

  // otherwise check the system directory for a file to read,
  // and then create 
  readFilename = SYS_CONFIG_DIR + baseName + EXTENSION;
  return PFile::Exists(readFilename);
}

////////////////////////////////////////////////////////////
//
// ConfigDictionary
//

ConfigDictionary::ConfigDictionary(int)
{
  environmentInstance = NULL;
}

PXConfig * ConfigDictionary::GetEnvironmentInstance()
{
  mutex.Wait();
  if (environmentInstance == NULL) {
    environmentInstance = new PXConfig(0);
    environmentInstance->ReadFromEnvironment(PProcess::Current().PXGetEnvp());
  }
  mutex.Signal();
  return environmentInstance;
}


PXConfig * ConfigDictionary::GetFileConfigInstance(const PFilePath & key, const PFilePath & readKey)
{
  mutex.Wait();

  PXConfig * config = GetAt(key);
  if (config != NULL) {
    PError << "Opening another instance of config file " << key << endl;
    config->AddInstance();
  } else {
    PError << "Opening first instance of config file " << key << endl;
    config = new PXConfig(0);
    config->ReadFromFile(readKey);
    config->AddInstance();
    SetAt(key, config);
  }

  mutex.Signal();
  return config;
}

void ConfigDictionary::RemoveInstance(PXConfig * instance)
{
  mutex.Wait();

  if (instance != environmentInstance) {
    PINDEX index = GetObjectsIndex(instance);
    PAssert(index != P_MAX_INDEX, "Cannot find PXConfig instance to remove");
    if (instance->RemoveInstance(GetKeyAt(index))) 
      SetDataAt(index, NULL);
  }

  mutex.Signal();
}

////////////////////////////////////////////////////////////
//
// PConfig::
//
// Create a new configuration object
//
////////////////////////////////////////////////////////////

void PConfig::Construct(Source src,
                        const PString & appname,
                        const PString & /*manuf*/)
{
  PString name;
  PFilePath filename, readFilename;

  // handle cnvironment configs differently
  if (src == PConfig::Environment) 
    config = configDict.GetEnvironmentInstance();

  // look up file name to read, and write
  if (src == PConfig::System)
    LocateFile(SYS_CONFIG_NAME, readFilename, filename);
  else {
    if (appname.IsEmpty())
      name = PProcess::Current().GetName();
    else
      name = appname;
    if (!LocateFile(name, readFilename, filename)) {
      name = PProcess::Current().GetFile().GetTitle();
      LocateFile(name, readFilename, filename);
    }
  }

  // get, or create, the configuration
  config = configDict.GetFileConfigInstance(filename, readFilename);
}

PConfig::PConfig(int, const PString & name)
  : defaultSection("Options")
{
  PFilePath readFilename, filename;
  LocateFile(name, readFilename, filename);
  config = configDict.GetFileConfigInstance(filename, readFilename);
}

void PConfig::Construct(const PFilePath & theFilename)

{
  PFilePath filename;
  config = configDict.GetFileConfigInstance(filename, theFilename);
}

PConfig::~PConfig()

{
  configDict.RemoveInstance(config);
}

////////////////////////////////////////////////////////////
//
// PConfig::
//
// Return a list of all the section names in the file.
//
////////////////////////////////////////////////////////////

PStringList PConfig::GetSections() const
{
  PAssert(config != NULL, "config instance not set");
  config->Wait();

  PStringList list;

  for (PINDEX i = 0; i < (*config).GetSize(); i++)
    list.AppendString((*config)[i]);

  config->Signal();

  return list;
}


////////////////////////////////////////////////////////////
//
// PConfig::
//
// Return a list of all the keys in the section. If the section name is
// not specified then use the default section.
//
////////////////////////////////////////////////////////////

PStringList PConfig::GetKeys(const PString & theSection) const
{
  PAssert(config != NULL, "config instance not set");
  config->Wait();

  PINDEX index;
  PStringList list;

  if ((index = config->GetValuesIndex(theSection)) != P_MAX_INDEX) {
    PXConfigSectionList & section = (*config)[index].GetList();
    for (PINDEX i = 0; i < section.GetSize(); i++)
      list.AppendString(section[i]);
  }

  config->Signal();
  return list;
}



////////////////////////////////////////////////////////////
//
// PConfig::
//
// Delete all variables in the specified section. If the section name is
// not specified then use the default section.
//
////////////////////////////////////////////////////////////

void PConfig::DeleteSection(const PString & theSection)

{
  PAssert(config != NULL, "config instance not set");
  config->Wait();

  PStringList list;

  PINDEX index;
  if ((index = config->GetValuesIndex(theSection)) != P_MAX_INDEX) {
    config->RemoveAt(index);
    config->SetDirty();
  }

  config->Signal();
}


////////////////////////////////////////////////////////////
//
// PConfig::
//
// Delete the particular variable in the specified section.
//
////////////////////////////////////////////////////////////

void PConfig::DeleteKey(const PString & theSection, const PString & theKey)
{
  PAssert(config != NULL, "config instance not set");
  config->Wait();

  PINDEX index;
  if ((index = config->GetValuesIndex(theSection)) != P_MAX_INDEX) {
    PXConfigSectionList & section = (*config)[index].GetList();
    PINDEX index_2;
    if ((index_2 = section.GetValuesIndex(theKey)) != P_MAX_INDEX) {
      section.RemoveAt(index_2);
      config->SetDirty();
    }
  }

  config->Signal();
}



////////////////////////////////////////////////////////////
//
// PConfig::
//
// Get a string variable determined by the key in the section.
//
////////////////////////////////////////////////////////////

PString PConfig::GetString(const PString & theSection,
                                    const PString & theKey, const PString & dflt) const
{
  PAssert(config != NULL, "config instance not set");
  config->Wait();

  PString value = dflt;
  PINDEX index;
  if ((index = config->GetValuesIndex(theSection)) != P_MAX_INDEX) {

    PXConfigSectionList & section = (*config)[index].GetList();
    if ((index = section.GetValuesIndex(theKey)) != P_MAX_INDEX) 
      value = section[index].GetValue();
  }

  config->Signal();
  return value;
}


////////////////////////////////////////////////////////////
//
// PConfig::
//
// Set a string variable determined by the key in the section.
//
////////////////////////////////////////////////////////////

void PConfig::SetString(const PString & theSection,
                        const PString & theKey,
                        const PString & theValue)
{
  PAssert(config != NULL, "config instance not set");
  config->Wait();

  PINDEX index;
  PXConfigSection * section;
  PXConfigValue   * value;

  if ((index = config->GetValuesIndex(theSection)) != P_MAX_INDEX) 
    section = &(*config)[index];
  else {
    section = new PXConfigSection(theSection);
    config->Append(section);
    config->SetDirty();
  } 

  if ((index = section->GetList().GetValuesIndex(theKey)) != P_MAX_INDEX) 
    value = &(section->GetList()[index]);
  else {
    value = new PXConfigValue(theKey);
    section->GetList().Append(value);
    config->SetDirty();
  }

  if (theValue != value->GetValue()) {
    value->SetValue(theValue);
    config->SetDirty();
  }

  config->Signal();
}

#undef NEW

