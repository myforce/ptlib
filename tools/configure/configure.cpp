/*
 * configure.cxx
 *
 * Build options generated by the configure script.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#pragma warning(disable:4786)
#pragma warning(disable:4996)

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <list>
#include <algorithm>
#include <stdlib.h>
#include <windows.h>
#include <vector>
#include <map>
#include <io.h>


#define VERSION "1.14"

static char * VersionTags[] = { "MAJOR_VERSION", "MINOR_VERSION", "BUILD_NUMBER", "BUILD_TYPE" };

using namespace std;

bool DirExcluded(const string & dir);

string ToLower(const string & _str)
{
  string str = _str;
  string::iterator r;
  for(r = str.begin(); r != str.end(); ++r)
    *r = (char)tolower(*r);
  return str;
}

string GetFullPathNameString(const string & path)
{
  string fullPath;
  DWORD len = ::GetFullPathName(path.c_str(), 0, NULL, NULL);
  if (len > 1) {
    fullPath.resize(len-1);
    ::GetFullPathName(path.c_str(), len, &fullPath[0], NULL);
  }
  return ToLower(fullPath);
}


class Feature
{
  public:
    enum States
    {
      Enabled,
      NotFound,
      Disabled,
      Blocked,
      Dependency
    };

    Feature() : state(Enabled) { }
    Feature(const string & featureName, const string & optionName, const string & optionValue);

    void Parse(const string & optionName, const string & optionValue);
    void Adjust(string & line);
    bool Locate(const char * testDir);

    string featureName;
    string displayName;
    string directorySymbol;
    string simpleDefineName;
    string simpleDefineValue;
    map<string, string> defines;
    map<string, string> defineValues;

    struct CheckFileInfo {
      CheckFileInfo() : found(false), defineName("1") { }

      bool Locate(string & testDir);

      bool   found;
      string fileName;
      string fileText;
      string defineName;
      string defineValue;
    };
    list<CheckFileInfo> checkFiles;

    list<string> checkDirectories;

    struct FindFileInfo {
      string symbol;
      string basename;
      string subdir;
      string fullname;
    };
    list<FindFileInfo> findFiles;

    list<string> ifFeature;
    list<string> ifNotFeature;

    string breaker;
    string directory;
    States state;
};


vector<Feature> features;
list<string> excludeDirList;

///////////////////////////////////////////////////////////////////////

Feature::Feature(const string & featureNameParam,
                 const string & optionName,
                 const string & optionValue)
  : featureName(featureNameParam),
    state(Enabled)
{
  Parse(optionName, optionValue);
}


void Feature::Parse(const string & optionName, const string & optionValue)
{
  if (optionName == "DISPLAY")
    displayName = optionValue;

  else if (optionName == "DEFINE") {
    string::size_type equal = optionValue.find('=');
    if (equal == string::npos)
      simpleDefineName = optionValue;
    else {
      simpleDefineName.assign(optionValue, 0, equal);
      simpleDefineValue.assign(optionValue, equal+1, INT_MAX);
    }
  }

  else if (optionName == "VERSION") {
    string::size_type equal = optionValue.find('=');
    if (equal != string::npos)
      defines.insert(pair<string,string>(optionValue.substr(0, equal), optionValue.substr(equal+1)));
  }

  else if (optionName == "CHECK_FILE") {
    string::size_type comma = optionValue.find(',');
    if (comma == string::npos)
      return;

    CheckFileInfo check;
    string::size_type pipe = optionValue.find('|');
    if (pipe < 0 || pipe > comma)
      check.fileName.assign(optionValue, 0, comma);
    else {
      check.fileName.assign(optionValue, 0, pipe);
      check.fileText.assign(optionValue, pipe+1, comma-pipe-1);
    }

    string::size_type equal = optionValue.find('=', comma);
    if (equal == string::npos)
      check.defineName.assign(optionValue, comma+1, INT_MAX);
    else {
      check.defineName.assign(optionValue, comma+1, equal-comma-1);
      check.defineValue.assign(optionValue, equal+1, INT_MAX);
    }

    checkFiles.push_back(check);
  }

  else if (optionName == "DIR_SYMBOL")
    directorySymbol = '@' + optionValue + '@';

  else if (optionName == "CHECK_DIR")
    checkDirectories.push_back(GetFullPathNameString(optionValue));

  else if (optionName == "FIND_FILE") {
    string::size_type comma1 = optionValue.find(',');
    if (comma1 == string::npos)
      return;

    string::size_type comma2 = optionValue.find(',', comma1+1);
    if (comma2 == string::npos)
      comma2 = INT_MAX-1;

    FindFileInfo find;
    find.symbol.assign(optionValue, 0, comma1);
    find.basename.assign(optionValue, comma1+1, comma2-comma1-1);
    find.subdir.assign(optionValue, comma2+1, INT_MAX);
    if (find.subdir.empty())
      find.subdir = "...";
    findFiles.push_back(find);
  }

  else if (optionName == "IF_FEATURE") {
    const char * delimiters = "&";
    string::size_type lastPos = optionValue.find_first_not_of(delimiters, 0);
    string::size_type pos = optionValue.find_first_of(delimiters, lastPos);
    while (string::npos != pos || string::npos != lastPos) {
      string str = optionValue.substr(lastPos, pos - lastPos);
      if (str[0] == '!')
        ifNotFeature.push_back(str.substr(1));
      else
        ifFeature.push_back(str);
      lastPos = optionValue.find_first_not_of(delimiters, pos);
      pos = optionValue.find_first_of(delimiters, lastPos);
    }
  }
}


static bool CompareName(const string & line, const string & name)
{
  string::size_type pos = line.find(name);
  if (pos == string::npos)
    return false;

  pos += name.length();
  return !isalnum(line[pos]) && line[pos] != '_';
}

void Feature::Adjust(string & line)
{
  string::size_type undefPos = line.find("#undef");
  if (state == Enabled && undefPos != string::npos) {
    if (!simpleDefineName.empty() && CompareName(line, simpleDefineName)) {
      line.replace(undefPos, INT_MAX, "#define " + simpleDefineName + ' ');
      if (simpleDefineValue.empty())
        line += '1';
      else
        line += simpleDefineValue;
    }

    map<string,string>::iterator r, s;
    for (r = defines.begin(); r != defines.end(); ++r) {
      if (CompareName(line, r->first)) {
        s = defineValues.find(r->second);
        if (s != defineValues.end())
          line.replace(undefPos, INT_MAX, "#define " + r->first + ' ' + s->second);
      }
    }

    for (list<CheckFileInfo>::iterator file = checkFiles.begin(); file != checkFiles.end(); file++) {
      if (file->found && CompareName(line, file->defineName)) {
        line.replace(undefPos, INT_MAX, "#define " + file->defineName + ' ' + file->defineValue);
        break;
      }
    }

    for (list<FindFileInfo>::iterator file = findFiles.begin(); file != findFiles.end(); file++) {
      if (!file->fullname.empty() && CompareName(line, file->symbol)) {
        line.replace(undefPos, INT_MAX, "#define " + file->symbol + " \"" + file->fullname + '"');
        break;
      }
    }
  }

  if (directorySymbol[0] != '\0') {
    string::size_type pos = line.find(directorySymbol);
    if (pos != string::npos)
      line.replace(pos, directorySymbol.length(), directory);
  }
}


bool IsUsableDirectory(const WIN32_FIND_DATA & fileinfo)
{
  return (fileinfo.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) != 0 &&
         (fileinfo.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) == 0 &&
         (fileinfo.dwFileAttributes&FILE_ATTRIBUTE_SYSTEM) == 0 &&
          fileinfo.cFileName[0] != '.' &&
          stricmp(fileinfo.cFileName, "RECYCLER") != 0 &&
          stricmp(fileinfo.cFileName, "$recycle.bin") != 0;
}


bool FindFileInTree(const string & directory, string & filename)
{
  bool found = false;
  string wildcard = directory;
  wildcard += "*.*";

  WIN32_FIND_DATA fileinfo;
  HANDLE hFindFile = FindFirstFile(wildcard.c_str(), &fileinfo);
  if (hFindFile != INVALID_HANDLE_VALUE) {
    do {
      if (stricmp(fileinfo.cFileName, filename.c_str()) == 0) {
        filename.insert(0, directory);
        found = true;
        break;
      }

      string subdir = GetFullPathNameString(directory + fileinfo.cFileName);
      if (IsUsableDirectory(fileinfo) && !DirExcluded(subdir)) {
        subdir += '\\';

        found = FindFileInTree(subdir, filename);
        if (found)
          break;
      }
    } while (FindNextFile(hFindFile, &fileinfo));

    FindClose(hFindFile);
  }

  return found;
}


bool Feature::Locate(const char * testDir)
{
  if (state != Enabled)
    return true;

  if (!directory.empty())
    return true;

  if (checkFiles.empty())
    return true;

  bool noneFound = true;
  for (list<CheckFileInfo>::iterator file = checkFiles.begin(); file != checkFiles.end(); ++file) {
    string testDirectory = testDir;
    if (file->Locate(testDirectory) && noneFound) {
      char buf[_MAX_PATH];
      _fullpath(buf, testDirectory.c_str(), _MAX_PATH);
      if (!DirExcluded(buf)) {
        directory = buf;
        noneFound = false;
        // No break as want to continue with other CHECK_FILE entries, setting defines
      }
    }
  }

  if (noneFound)
    return false;
  
  cout << "Located " << displayName << " at " << directory << endl;

  string::size_type pos;
  while ((pos = directory.find('\\')) != string::npos)
    directory[pos] = '/';
  pos = directory.length()-1;
  if (directory[pos] == '/')
    directory.erase(pos);

  for (list<FindFileInfo>::iterator file = findFiles.begin(); file != findFiles.end(); file++) {
    string::size_type elipses = file->subdir.find("...");
    if (elipses == string::npos) {
      string filename = directory + '/' + file->subdir + '/' + file->basename;
      if (_access(filename.c_str(), 0) == 0)
        file->fullname = filename;
    }
    else {
      string subdir = directory + '/';
      subdir.append(file->subdir, 0, elipses);
      string filename = file->basename;
      if (FindFileInTree(subdir, filename)) {
        while ((pos = filename.find('\\')) != string::npos)
          filename[pos] = '/';
        file->fullname = filename;
      }
    }
  }

  return true;
}


bool Feature::CheckFileInfo::Locate(string & testDirectory)
{
  if (testDirectory.find_first_of("*?") != string::npos) {
    WIN32_FIND_DATA fileinfo;
    HANDLE hFindFile = FindFirstFile(testDirectory.c_str(), &fileinfo);
    if (hFindFile == INVALID_HANDLE_VALUE)
      return false;

    testDirectory.erase(testDirectory.rfind('\\')+1);

    do {
      string subdir = GetFullPathNameString(testDirectory + fileinfo.cFileName);
      if (IsUsableDirectory(fileinfo) && !DirExcluded(subdir) && Locate(subdir)) {
        testDirectory = subdir;
        return true;
      }
    } while (FindNextFile(hFindFile, &fileinfo));

    FindClose(hFindFile);
    return false;
  }

  if (testDirectory[testDirectory.length()-1] != '\\')
    testDirectory += '\\';

  string filename = testDirectory + fileName;
  ifstream file(filename.c_str(), ios::in);
  if (!file.is_open())
    return false;

  if (fileText.empty())
    found = true;
  else {
    while (file.good()) {
      string line;
      getline(file, line);
      if (line.find(fileText) != string::npos) {
        found = true;
        break;
      }
    }
  }

  return found;
}

string ExcludeDir(const string & _dir)
{
  if (_dir.length() == 0)
    return _dir;

  string dir(GetFullPathNameString(_dir));

  if (dir[dir.length()-1] != '\\')
    dir += '\\';

  excludeDirList.push_back(dir);
  
  return dir;
}


bool DirExcluded(const string & _dir)
{
  if (_dir.length() == 0)
    return false;

  string dir(_dir);
  if (dir[dir.length()-1] != '\\')
    dir += "\\";

  list<string>::const_iterator r;

  for (r = excludeDirList.begin(); r != excludeDirList.end(); ++r) {
    string excludeDir = *r;
    string dirPrefix(dir.substr(0, excludeDir.length()));
    if (dirPrefix == excludeDir)
      return true;
  }

  return false;
}


bool TreeWalk(const string & directory)
{
  bool foundAll = false;

  if (DirExcluded(directory))
    return false;

  string wildcard = directory;
  wildcard += "*.*";

  WIN32_FIND_DATA fileinfo;
  HANDLE hFindFile = FindFirstFile(wildcard.c_str(), &fileinfo);
  if (hFindFile != INVALID_HANDLE_VALUE) {
    do {
      string subdir = GetFullPathNameString(directory + fileinfo.cFileName);
      if (IsUsableDirectory(fileinfo) && !DirExcluded(subdir)) {
        subdir += '\\';

        foundAll = true;
        vector<Feature>::iterator feature;
        for (feature = features.begin(); feature != features.end(); feature++) {
          if (!feature->Locate(subdir.c_str()))
            foundAll = false;
        }

        if (foundAll)
          break;
        TreeWalk(subdir);
      }
    } while (FindNextFile(hFindFile, &fileinfo));

    FindClose(hFindFile);
  }

  return foundAll;
}


bool ProcessHeader(const string & headerFilename)
{
  string inFilename = headerFilename;
  inFilename += ".in";

  ifstream in(inFilename.c_str(), ios::in);
  if (!in.is_open()) {
    cerr << "Could not open " << inFilename << endl;
    return false;
  }

  ofstream out(headerFilename.c_str(), ios::out);
  if (!out.is_open()) {
    cerr << "Could not open " << headerFilename << endl;
    return false;
  }

  while (in.good()) {
    string line;
    getline(in, line);

    vector<Feature>::iterator feature;
    for (feature = features.begin(); feature != features.end(); feature++)
      feature->Adjust(line);

    out << line << '\n';
  }

  return !in.bad() && !out.bad();
}


bool FeatureEnabled(const string & name)
{
  vector<Feature>::iterator feature;
  for (feature = features.begin(); feature != features.end(); feature++) {
    if (feature->featureName == name && feature->state == Feature::Enabled)
      return true;
  }
  return false;
}


bool AllFeaturesAre(bool state, const list<string> & features, string & breaker)
{
  for (list<string>::const_iterator iter = features.begin(); iter != features.end(); ++iter) {
    size_t pos = iter->find('|');
    if (pos == string::npos) {
      if (FeatureEnabled(*iter) != state) {
        breaker = *iter;
        return false;
      }
    }
    else
    {
      bool anyOfState = false;
      string str = *iter;
      while (str.length() != 0) {
        string key = str.substr(0, pos);
        if (FeatureEnabled(key) == state) {
          anyOfState = true;
          break;
        }
        if (pos == string::npos)
          break;
        str = str.substr(pos+1);
        pos = str.find('|');
      }
      if (!anyOfState) {
        breaker = *iter;
        return false;
      }
    }
  }
  return true;
}


int main(int argc, char* argv[])
{
  // open and scan configure.ac
  cout << "PWLib Configure " VERSION " - ";
  ifstream conf("configure.ac", ios::in);
  if (conf.is_open()) {
    cout << "opened configure.ac" << endl;
  } else {
    conf.clear();
    conf.open("configure.in", ios::in);
    if (conf.is_open()) {
      cout << "opened " << "configure.in" << endl;
    } else {
      cerr << "could not open configure.ac/configure.in" << endl;
      return 1;
    }
  }

  // scan for features
  list<string> headers;
  vector<Feature>::iterator feature;

  while (conf.good()) {
    string line;
    getline(conf, line);
    string::size_type pos;
    if ((pos = line.find("AC_CONFIG_HEADERS")) != string::npos) {
      if ((pos = line.find('(', pos)) != string::npos) {
        string::size_type end = line.find(')', pos);
        if (end != string::npos)
          headers.push_back(line.substr(pos+1, end-pos-1));
      }
    }
    else if ((pos = line.find("dnl MSWIN_")) != string::npos) {
      string::size_type space = line.find(' ', pos+10);
      if (space != string::npos) {
        string optionName(line, pos+10, space-pos-10);
        while (line[space] == ' ')
          space++;
        string::size_type comma = line.find(',', space);
        if (comma != string::npos) {
          string optionValue(line, comma+1, INT_MAX);
          if (!optionValue.empty()) {
            string featureName(line, space, comma-space);
            bool found = false;
            for (feature = features.begin(); feature != features.end(); feature++) {
              if (feature->featureName == featureName) {
                found = true;
                break;
              }
            }
            if (found)
              feature->Parse(optionName, optionValue);
            else
              features.push_back(Feature(featureName, optionName, optionValue));
          }
        }
      }
    }
  }

  // scan version.h if there is a feature called version and if version.h exists
  for (feature = features.begin(); feature != features.end(); ++feature)
    if (feature->featureName == "version")
      break;
  if (feature != features.end()) {
    ifstream version("version.h", ios::in);
    if (version.is_open()) {
      while (version.good()) {
        string line;
        getline(version, line);
        string::size_type pos;
        int i;
        for (i = 0; i < (sizeof(VersionTags)/sizeof(VersionTags[0])); ++i) {
          size_t tagLen = strlen(VersionTags[i]);
          if ((pos = line.find(VersionTags[i])) != string::npos) {
            string::size_type space = line.find(' ', pos+tagLen);
            if (space != string::npos) {
              while (line[space] == ' ')
                space++;
              string version = line.substr(space);
              while (::iswspace(version[0]))
                version.erase(0);
              while (version.length() > 0 && ::iswspace(version[version.length()-1]))
                version.erase(version.length()-1);
              feature->defineValues.insert(pair<string,string>(VersionTags[i], version));
            }
          }
        }
      }
      string version("\"");
      map<string,string>::iterator ver;
      if ((ver = feature->defineValues.find(VersionTags[0])) != feature->defineValues.end())
        version += ver->second;
      else
        version += "0";
      version += ".";
      if ((ver = feature->defineValues.find(VersionTags[1])) != feature->defineValues.end())
        version += ver->second;
      else
        version += "0";
      version += ".";
      if ((ver = feature->defineValues.find(VersionTags[2])) != feature->defineValues.end())
        version += ver->second;
      else
        version += "0";
      version += "\"";
      feature->defineValues.insert(pair<string,string>("VERSION", version));
    }
  }

  const char EXTERN_DIR[]  = "--extern-dir=";
  const char EXCLUDE_DIR[] = "--exclude-dir=";
  const char EXCLUDE_ENV[] = "--exclude-env=";

  bool searchDisk = true;
  char *externDir = NULL;
  char *externEnv = NULL;
  int i;
  for (i = 1; i < argc; i++) {
    if (stricmp(argv[i], "--no-search") == 0 || stricmp(argv[i], "--disable-search") == 0)
      searchDisk = false;
    else if (strnicmp(argv[i], EXCLUDE_ENV, sizeof(EXCLUDE_ENV) - 1) == 0){
        externEnv = argv[i] + sizeof(EXCLUDE_ENV) - 1;
    }
    else if (strnicmp(argv[i], EXTERN_DIR, sizeof(EXTERN_DIR) - 1) == 0){
        externDir = argv[i] + sizeof(EXTERN_DIR) - 1;
    }
    else if (strnicmp(argv[i], EXCLUDE_DIR, sizeof(EXCLUDE_DIR) - 1) == 0) {
      string dir(argv[i] + sizeof(EXCLUDE_DIR) - 1);
      cout << "Excluding \"" << ExcludeDir(dir) << "\" from feature search" << endl;
    }
    else if (stricmp(argv[i], "-v") == 0 || stricmp(argv[i], "--version") == 0) {
      cout << "configure version " VERSION "\n";
      return 1;
    }
    else if (stricmp(argv[i], "-h") == 0 || stricmp(argv[i], "--help") == 0) {
      cout << "configure version " VERSION "\n"
              "usage: configure args\n"
              "  --no-search           Do not search disk for libraries.\n"
              "  --extern-dir=dir      Specify where to search disk for libraries.\n"
              "  --exclude-dir=dir     Exclude dir from search path.\n";
              "  --exclude-env=var     Exclude dirs decribed by specified env var from search path.\n";
      for (feature = features.begin(); feature != features.end(); feature++) {
        if (feature->featureName[0] != '\0') {
            cout << "  --disable-" << feature->featureName
                 << setw(20-feature->featureName.length()) << "Disable " << feature->displayName << '\n';
          if (!feature->checkFiles.empty())
            cout << "  --" << feature->featureName << "-dir=dir"
                 << setw(30-feature->featureName.length()) << "Set directory for " << feature->displayName << '\n';
        }
      }
      return 1;
    }
    else {
      // parse environment variable if it exists
      std::vector<std::string> envConfigureList;
      char * envStr = getenv("PWLIB_CONFIGURE_FEATURES");
      if (envStr != NULL) {
        string str(envStr);
        string::size_type offs = 0;
        while (offs < str.length()) {
          string::size_type n = str.find(',', offs);
          string xable;
          if (n != string::npos) {
            xable = str.substr(offs, n-offs);
            offs = n+1;
          } else {
            xable = str.substr(offs);
            offs += str.length();
          }
          envConfigureList.push_back(ToLower(xable));
        }
      }

      // go through features and disable ones that need disabling either from command line or environment
      for (feature = features.begin(); feature != features.end(); feature++) {
        if (stricmp(argv[i], ("--no-"     + feature->featureName).c_str()) == 0 ||
            stricmp(argv[i], ("--disable-"+ feature->featureName).c_str()) == 0) {
          feature->state = Feature::Disabled;
          break;
        }
        else if (stricmp(argv[i], ("--enable-"+ feature->featureName).c_str()) == 0) {
          feature->state = Feature::Enabled;
          break;
        }
        else if (find(envConfigureList.begin(), envConfigureList.end(), "enable-"+ feature->featureName) != envConfigureList.end()) {
          feature->state = Feature::Enabled;
          break;
        }
        else if (find(envConfigureList.begin(), envConfigureList.end(), "disable-"+ feature->featureName) != envConfigureList.end() ||
                 find(envConfigureList.begin(), envConfigureList.end(), "no-"+ feature->featureName) != envConfigureList.end()) {
          feature->state = Feature::Disabled;
          break;
        }
        else if (strstr(argv[i], ("--" + feature->featureName + "-dir=").c_str()) == argv[i] &&
               !feature->Locate(argv[i] + strlen(("--" + feature->featureName + "-dir=").c_str())))
          cerr << feature->displayName << " not found in "
               << argv[i] + strlen(("--" + feature->featureName+"-dir=").c_str()) << endl;

      }
    }
  }

  for (i = 0; i < 2; i++) {
    char * env = NULL;
    switch (i) {
      case 0: 
        env = getenv("PWLIB_CONFIGURE_EXCLUDE_DIRS");
        break;
      case 1: 
        if (externEnv != NULL)
          env = getenv(externEnv);
        break;
    }
    if (env != NULL) {
      string str(env);
      string::size_type offs = 0;
      while (offs < str.length()) {
        string::size_type n = str.find(';', offs);
        string dir;
        if (n != string::npos) {
          dir = str.substr(offs, n-offs);
          offs = n+1;
        } else {
          dir = str.substr(offs);
          offs += str.length();
        }
        cout << "Excluding \"" << ExcludeDir(dir) << "\" from feature search" << endl;
      }
    }
  }

  bool foundAll = true;
  for (feature = features.begin(); feature != features.end(); feature++) {
#if 0 // Does not work yet
    if (feature->state == Feature::Enabled && !AllFeaturesAre(true, feature->ifFeature, feature->breaker))
      feature->state = Feature::Dependency;
    if (feature->state == Feature::Enabled && !AllFeaturesAre(false, feature->ifNotFeature, feature->breaker))
      feature->state = Feature::Blocked;
#endif
    if (feature->state == Feature::Enabled && !feature->checkFiles.empty()) {
      bool foundOne = false;
      list<string>::iterator dir;
      for (dir = feature->checkDirectories.begin(); dir != feature->checkDirectories.end(); dir++) {
        if (!DirExcluded(*dir) && feature->Locate(dir->c_str())) {
          foundOne = true;
          break;
        }
      }
      if (!foundOne)
        foundAll = false;
    }
  }

  if (searchDisk && !foundAll) {
    // Do search of entire system
    char drives[1024];
    if (!externDir){
      if (!GetLogicalDriveStrings(sizeof(drives), drives))
        strcpy(drives, "C:\\");
    }
    else {
      strcpy(drives, externDir);
      drives[strlen(externDir)+1] = '\0';
    }

    const char * drive = drives;
    while (*drive != '\0') {
      if (externDir || GetDriveType(drive) == DRIVE_FIXED) {
        cout << "Searching " << drive << endl;
        if (TreeWalk(drive))
          break;
      }
      drive += strlen(drive)+1;
    }
  }

  for (feature = features.begin(); feature != features.end(); feature++) {
    if (feature->state == Feature::Enabled && !AllFeaturesAre(true, feature->ifFeature, feature->breaker))
      feature->state = Feature::Dependency;
    else if (feature->state == Feature::Enabled && !AllFeaturesAre(false, feature->ifNotFeature, feature->breaker))
      feature->state = Feature::Blocked;
    else if (feature->state == Feature::Enabled && !feature->checkFiles.empty()) {
      feature->state = Feature::NotFound;
      for (list<Feature::CheckFileInfo>::iterator file = feature->checkFiles.begin(); file != feature->checkFiles.end(); file++) {
        if (feature->checkFiles.begin()->found) {
          feature->state = Feature::Enabled;
          break;
        }
      }
    }
  }

  int longestNameWidth = 0;
  for (feature = features.begin(); feature != features.end(); feature++) {
    int length = feature->displayName.length();
    if (length > longestNameWidth)
      longestNameWidth = length;
  }

  cout << "\n\nFeatures:\n";
  for (feature = features.begin(); feature != features.end(); feature++) {
    cout << setw(longestNameWidth+3) << feature->displayName << ' ';
    switch (feature->state)
    {
      case Feature::Enabled:
        cout << "enabled";
        break;

      case Feature::Disabled :
        cout << "DISABLED by user";
        break;

      case Feature::Dependency :
        cout << "DISABLED due to absence of feature " << feature->breaker;
        break;

      case Feature::Blocked :
        cout << "DISABLED due to presence of feature " << feature->breaker;
        break;

      default :
        cout << "DISABLED";
    }
    cout << '\n';
  }
  cout << endl;

  for (list<string>::iterator hdr = headers.begin(); hdr != headers.end(); hdr++)
    ProcessHeader(*hdr);

  cout << "Configuration completed.\n";

  return 0;
}
