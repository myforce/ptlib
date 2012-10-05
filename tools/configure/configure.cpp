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
#include <direct.h>


#define VERSION "1.22"

static char * VersionTags[] = { "MAJOR_VERSION", "MINOR_VERSION", "BUILD_NUMBER", "BUILD_TYPE" };

using namespace std;

bool DirExcluded(const string & dir);

string ToLower(const string & str)
{
  string adjusted = str;
  string::iterator r;
  for(r = adjusted.begin(); r != adjusted.end(); ++r)
    *r = (char)tolower(*r);
  return adjusted;
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
      Dependency,
      DefaultDisabled
    };

    Feature(const string & featureName, const string & optionName, const string & optionValue);

    void Parse(const string & optionName, const string & optionValue);
    void DisplayHelp();
    void DisplayResult();
    bool ProcessArgs(const char * arg);
    void Adjust(string & line);
    bool CheckFiles();
    bool Locate(const char * testDir);
    void RunDependencies();

    string m_featureName;
    string m_displayName;
    string m_directorySymbol;
    string m_simpleDefineName;
    string m_simpleDefineValue;
    map<string, string> m_defines;
    map<string, string> m_defineValues;

    struct CheckFileInfo {
      CheckFileInfo()
        : m_found(false)
        , m_defineName("1")
      { }

      bool Locate(string & testDir);

      bool   m_found;
      string m_fileName;
      string m_fileText;
      string m_defineName;
      string m_defineValue;
    };
    list<CheckFileInfo> m_checkFiles;

    list<string> m_checkDirectories;

    struct FindFileInfo {
      string m_symbol;
      string m_basename;
      string m_subdir;
      string m_fullname;
    };
    list<FindFileInfo> m_findFiles;

    struct IfFeature {
      list<string> m_present;
      list<string> m_absent;
    };
    list<IfFeature> m_ifFeatures;

    string m_breaker;
    string m_directory;
    States m_state;
};


vector<Feature> g_features;
list<string> g_excludeDirList;
vector<string> g_envConfigureList;
map<string, string> g_predefines;
bool g_verbose = false;

///////////////////////////////////////////////////////////////////////

Feature::Feature(const string & featureNameParam,
                 const string & optionName,
                 const string & optionValue)
  : m_featureName(featureNameParam)
  , m_state(Enabled)
{
  Parse(optionName, optionValue);

  const char * env = getenv("INCLUDE");
  if (env == NULL)
    return;

  char * includes = strdup(env);
  char * dir = strtok(includes, ";");
  do {
    if (*dir != '\0') {
      m_checkDirectories.push_back(GetFullPathNameString(dir));

      char * end = &dir[strlen(dir)-1];
      if (*end == '\\')
        end--;
      end -= 7;
      if (*end == '\\' && strnicmp(end+1, "include", 7) == 0) {
        *end = '\0';
        m_checkDirectories.push_back(GetFullPathNameString(dir));
      }
    }
  } while ((dir = strtok(NULL, ";")) != NULL);
  free(includes);
}


void Feature::Parse(const string & optionName, const string & optionValue)
{
  if (optionName == "DISPLAY")
    m_displayName = optionValue;

  else if (optionName == "DEFAULT")
    m_state = ToLower(optionValue) == "disabled" ? DefaultDisabled : Enabled;

  else if (optionName == "DEFINE") {
    string::size_type equal = optionValue.find('=');
    if (equal == string::npos)
      m_simpleDefineName = optionValue;
    else {
      m_simpleDefineName.assign(optionValue, 0, equal);
      m_simpleDefineValue.assign(optionValue, equal+1, INT_MAX);
    }
  }

  else if (optionName == "VERSION") {
    string::size_type equal = optionValue.find('=');
    if (equal != string::npos)
      m_defines.insert(pair<string,string>(optionValue.substr(0, equal), optionValue.substr(equal+1)));
  }

  else if (optionName == "CHECK_FILE") {
    string::size_type comma = optionValue.find(',');
    if (comma == string::npos)
      return;

    CheckFileInfo check;
    string::size_type pipe = optionValue.find('|');
    if (pipe < 0 || pipe > comma)
      check.m_fileName.assign(optionValue, 0, comma);
    else {
      check.m_fileName.assign(optionValue, 0, pipe);
      check.m_fileText.assign(optionValue, pipe+1, comma-pipe-1);
    }

    string::size_type equal = optionValue.find('=', comma);
    if (equal == string::npos)
      check.m_defineName.assign(optionValue, comma+1, INT_MAX);
    else {
      check.m_defineName.assign(optionValue, comma+1, equal-comma-1);
      check.m_defineValue.assign(optionValue, equal+1, INT_MAX);
    }

    m_checkFiles.push_back(check);
  }

  else if (optionName == "DIR_SYMBOL")
    m_directorySymbol = '@' + optionValue + '@';

  else if (optionName == "CHECK_DIR")
    m_checkDirectories.push_back(GetFullPathNameString(optionValue));

  else if (optionName == "FIND_FILE") {
    string::size_type comma1 = optionValue.find(',');
    if (comma1 == string::npos)
      return;

    string::size_type comma2 = optionValue.find(',', comma1+1);
    if (comma2 == string::npos)
      comma2 = INT_MAX-1;

    FindFileInfo find;
    find.m_symbol.assign(optionValue, 0, comma1);
    find.m_basename.assign(optionValue, comma1+1, comma2-comma1-1);
    find.m_subdir.assign(optionValue, comma2+1, INT_MAX);
    if (find.m_subdir.empty())
      find.m_subdir = "...";
    m_findFiles.push_back(find);
  }

  else if (optionName == "IF_FEATURE") {
    IfFeature ifFeature;
    const char * delimiters = "&";
    string::size_type lastPos = optionValue.find_first_not_of(delimiters, 0);
    string::size_type pos = optionValue.find_first_of(delimiters, lastPos);
    while (string::npos != pos || string::npos != lastPos) {
      string str = optionValue.substr(lastPos, pos - lastPos);
      if (str[0] == '!')
        ifFeature.m_absent.push_back(str.substr(1));
      else
        ifFeature.m_present.push_back(str);
      lastPos = optionValue.find_first_not_of(delimiters, pos);
      pos = optionValue.find_first_of(delimiters, lastPos);
    }
    m_ifFeatures.push_back(ifFeature);
  }
}


void Feature::DisplayHelp()
{
  if (m_featureName.empty())
    return;

  cout << "  --disable-" << m_featureName
       << setw(20-m_featureName.length()) << "Disable " << m_displayName << '\n';

  if (!m_checkFiles.empty())
    cout << "  --" << m_featureName << "-dir=dir"
         << setw(30-m_featureName.length()) << "Set directory for " << m_displayName << '\n';
}


void Feature::DisplayResult()
{
  cout << m_displayName << ' ';
  switch (m_state)
  {
    case Feature::Enabled:
      cout << "enabled";
      break;

    case Feature::NotFound :
      cout << "DISABLED due to missing library";
      break;

    case Feature::Disabled :
      cout << "DISABLED by user";
      break;

    case Feature::Dependency :
      cout << "DISABLED due to absence of feature " << m_breaker;
      break;

    case Feature::Blocked :
      cout << "DISABLED due to presence of feature " << m_breaker;
      break;

    default :
      cout << "DISABLED";
  }
  cout << '\n';
}


bool Feature::ProcessArgs(const char * arg)
{
  if (stricmp(arg, ("--no-"     + m_featureName).c_str()) == 0 ||
      stricmp(arg, ("--disable-"+ m_featureName).c_str()) == 0) {
    m_state = Feature::Disabled;
    return true;
  }
  
  if (stricmp(arg, ("--enable-"+ m_featureName).c_str()) == 0) {
    m_state = Feature::Enabled;
    return true;
  }
  
  if (find(g_envConfigureList.begin(), g_envConfigureList.end(), "enable-"+ m_featureName) != g_envConfigureList.end()) {
    m_state = Feature::Enabled;
    return true;
  }
  
  if (find(g_envConfigureList.begin(), g_envConfigureList.end(), "disable-"+ m_featureName) != g_envConfigureList.end() ||
            find(g_envConfigureList.begin(), g_envConfigureList.end(), "no-"+ m_featureName) != g_envConfigureList.end()) {
    m_state = Feature::Disabled;
    return true;
  }
  
  if (strstr(arg, ("--" + m_featureName + "-dir=").c_str()) == arg &&
          !Locate(arg + strlen(("--" + m_featureName + "-dir=").c_str())))
    cerr << m_displayName << " not found in "
          << arg + strlen(("--" + m_featureName+"-dir=").c_str()) << endl;

  return false;
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
  if (m_state == Enabled && undefPos != string::npos) {
    if (!m_simpleDefineName.empty() && CompareName(line, m_simpleDefineName)) {
      line.replace(undefPos, INT_MAX, "#define " + m_simpleDefineName + ' ');
      if (m_simpleDefineValue.empty())
        line += '1';
      else
        line += m_simpleDefineValue;
    }

    map<string,string>::iterator r, s;
    for (r = m_defines.begin(); r != m_defines.end(); ++r) {
      if (CompareName(line, r->first)) {
        s = m_defineValues.find(r->second);
        if (s != m_defineValues.end())
          line.replace(undefPos, INT_MAX, "#define " + r->first + ' ' + s->second);
      }
    }

    for (list<CheckFileInfo>::iterator file = m_checkFiles.begin(); file != m_checkFiles.end(); file++) {
      if (file->m_found && CompareName(line, file->m_defineName)) {
        line.replace(undefPos, INT_MAX, "#define " + file->m_defineName + ' ' + file->m_defineValue);
        break;
      }
    }

    for (list<FindFileInfo>::iterator file = m_findFiles.begin(); file != m_findFiles.end(); file++) {
      if (!file->m_fullname.empty() && CompareName(line, file->m_symbol)) {
        line.replace(undefPos, INT_MAX, "#define " + file->m_symbol + " \"" + file->m_fullname + '"');
        break;
      }
    }
  }

  if (m_directorySymbol[0] != '\0') {
    string::size_type pos = line.find(m_directorySymbol);
    if (pos != string::npos)
      line.replace(pos, m_directorySymbol.length(), m_directory);
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


bool Feature::CheckFiles()
{
  if (m_state != Feature::Enabled)
    return true;
  
  if (m_checkFiles.empty())
    return true;

  list<string>::iterator dir;
  for (dir = m_checkDirectories.begin(); dir != m_checkDirectories.end(); dir++) {
    if (DirExcluded(*dir)) {
      if (g_verbose)
        cout << "Excluded " << *dir << endl;
    }
    else {
      if (Locate(dir->c_str()))
        return true;
    }
  }

  return false;
}


bool Feature::Locate(const char * testDir)
{
  if (m_state != Enabled)
    return true;

  if (!m_directory.empty()) {
    if (g_verbose)
      cout << "No directory for " << m_displayName << endl;
    return true;
  }

  if (m_checkFiles.empty()) {
    if (g_verbose)
      cout << "No files to check for " << m_displayName << endl;
    return true;
  }

  bool noneFound = true;
  for (list<CheckFileInfo>::iterator file = m_checkFiles.begin(); file != m_checkFiles.end(); ++file) {
    string testDirectory = testDir;
    if (file->Locate(testDirectory) && noneFound) {
      char buf[_MAX_PATH];
      _fullpath(buf, testDirectory.c_str(), _MAX_PATH);
      if (DirExcluded(buf)) {
        if (g_verbose)
          cout << "Excluded " << buf << endl;
      }
      else {
        m_directory = buf;
        noneFound = false;
        // No break as want to continue with other CHECK_FILE entries, setting defines
      }
    }
  }

  if (noneFound)
    return false;
  
  if (g_verbose)
    cout << "Located " << m_displayName << " at " << m_directory << endl;

  string::size_type pos;
  while ((pos = m_directory.find('\\')) != string::npos)
    m_directory[pos] = '/';
  pos = m_directory.length()-1;
  if (m_directory[pos] == '/')
    m_directory.erase(pos);

  for (list<FindFileInfo>::iterator file = m_findFiles.begin(); file != m_findFiles.end(); file++) {
    string::size_type elipses = file->m_subdir.find("...");
    if (elipses == string::npos) {
      string filename = m_directory + '/' + file->m_subdir + '/' + file->m_basename;
      if (_access(filename.c_str(), 0) == 0)
        file->m_fullname = filename;
    }
    else {
      string subdir = m_directory + '/';
      subdir.append(file->m_subdir, 0, elipses);
      string filename = file->m_basename;
      if (FindFileInTree(subdir, filename)) {
        while ((pos = filename.find('\\')) != string::npos)
          filename[pos] = '/';
        file->m_fullname = filename;
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

  string filename = testDirectory + m_fileName;
  ifstream file(filename.c_str(), ios::in);
  if (!file.is_open())
    return false;

  if (m_fileText.empty())
    m_found = true;
  else {
    while (file.good()) {
      string line;
      getline(file, line);
      if (line.find(m_fileText) != string::npos) {
        m_found = true;
        break;
      }
    }
  }

  return m_found;
}

string ExcludeDir(const string & _dir)
{
  if (_dir.length() == 0)
    return _dir;

  string dir(GetFullPathNameString(_dir));

  size_t last = dir.length()-1;
  if (dir[last] == '*')
    dir.erase(last, 1);
  else if (dir[last] != '\\')
    dir += '\\';

  g_excludeDirList.push_back(dir);

  dir += '*'; // For cosmetic display
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

  for (r = g_excludeDirList.begin(); r != g_excludeDirList.end(); ++r) {
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
        for (feature = g_features.begin(); feature != g_features.end(); feature++) {
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


bool ProcessHeader(const string & headerFileSpec)
{
  string inFilename, outFilename;
  size_t colon = headerFileSpec.find(':');
  if (colon == string::npos) {
    inFilename = headerFileSpec + ".in";
    outFilename = headerFileSpec;
  }
  else {
    inFilename.assign(headerFileSpec, colon+1, string::npos);
    outFilename.assign(headerFileSpec, 0, colon);
  }

  ifstream in(inFilename.c_str(), ios::in);
  if (!in.is_open()) {
    cerr << "Could not open " << inFilename << endl;
    return false;
  }

  for (map<string, string>::iterator it = g_predefines.begin(); it != g_predefines.end(); ++it) {
    size_t var;
    while ((var = outFilename.find(it->first)) != string::npos &&
            (it->first[1] == '{' || !isalnum(outFilename[var+it->first.length()]))) {
      outFilename.erase(var, it->first.length());
      outFilename.insert(var, it->second);
    }
  }

  string outDir(outFilename, 0, outFilename.find_last_of("/\\"));
  if (_mkdir(outDir.c_str()) < 0) {
    if (errno != EEXIST) {
      cerr << "Could not create directory " << outDir << endl;
      return false;
    }
  }

  ofstream out(outFilename.c_str(), ios::out);  if (!out.is_open()) {
    cerr << "Could not open " << outFilename << endl;
    return false;
  }

  while (!in.eof()) {
    string line;
    getline(in, line);
    if (in.bad()) {
      cerr << "Error reading " << inFilename << endl;
      return false;
    }

    vector<Feature>::iterator feature;
    for (feature = g_features.begin(); feature != g_features.end(); feature++)
      feature->Adjust(line);

    out << line << '\n';
    if (out.bad()) {
      cerr << "Error writing " << outFilename << endl;
      return false;
    }
  }

  cout << "Written " << outFilename << endl;
  return true;
}


bool FeatureEnabled(const string & name)
{
  vector<Feature>::iterator feature;
  for (feature = g_features.begin(); feature != g_features.end(); feature++) {
    if (feature->m_featureName == name && feature->m_state == Feature::Enabled)
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


void Feature::RunDependencies()
{
  if (m_state != Enabled)
    return;

  States newState = Enabled;
  for (list<IfFeature>::iterator it = m_ifFeatures.begin(); it != m_ifFeatures.end(); ++it) {
    if (!AllFeaturesAre(true, it->m_present, m_breaker))
      newState = Dependency;
    else if (!AllFeaturesAre(false, it->m_absent, m_breaker))
      newState = Blocked;
    else {
      newState = Enabled;
      break;
    }
  }
  if (newState != Enabled) {
    m_state = newState;
    return;
  }

  if (m_checkFiles.empty())
    return;

  m_state = NotFound;
  for (list<CheckFileInfo>::iterator file = m_checkFiles.begin(); file != m_checkFiles.end(); file++) {
    if (file->m_found) {
      m_state = Enabled;
      break;
    }
  }
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
    if ((pos = line.find("AC_CONFIG_FILES")) != string::npos ||
        (pos = line.find("AC_CONFIG_HEADERS")) != string::npos) {
      if ((pos = line.find('(', pos)) != string::npos) {
        string::size_type end = line.find(')', pos);
        if (end != string::npos && line.find('[', pos) == string::npos)
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
            for (feature = g_features.begin(); feature != g_features.end(); feature++) {
              if (feature->m_featureName == featureName) {
                found = true;
                break;
              }
            }
            if (found)
              feature->Parse(optionName, optionValue);
            else
              g_features.push_back(Feature(featureName, optionName, optionValue));
          }
        }
      }
    }
  }

  // scan version.h if there is a feature called version and if version.h exists
  for (feature = g_features.begin(); feature != g_features.end(); ++feature) {
    if (feature->m_featureName == "version")
      break;
  }
  if (feature != g_features.end()) {
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
              feature->m_defineValues.insert(pair<string,string>(VersionTags[i], version));
            }
          }
        }
      }
      string version("\"");
      map<string,string>::iterator ver;
      if ((ver = feature->m_defineValues.find(VersionTags[0])) != feature->m_defineValues.end())
        version += ver->second;
      else
        version += "0";
      version += ".";
      if ((ver = feature->m_defineValues.find(VersionTags[1])) != feature->m_defineValues.end())
        version += ver->second;
      else
        version += "0";
      version += ".";
      if ((ver = feature->m_defineValues.find(VersionTags[2])) != feature->m_defineValues.end())
        version += ver->second;
      else
        version += "0";
      version += "\"";
      feature->m_defineValues.insert(pair<string,string>("VERSION", version));
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
    else if (strnicmp(argv[i], EXCLUDE_ENV, sizeof(EXCLUDE_ENV) - 1) == 0) {
        externEnv = argv[i] + sizeof(EXCLUDE_ENV) - 1;
    }
    else if (strnicmp(argv[i], EXTERN_DIR, sizeof(EXTERN_DIR) - 1) == 0){
        externDir = argv[i] + sizeof(EXTERN_DIR) - 1;
    }
    else if (strnicmp(argv[i], EXCLUDE_DIR, sizeof(EXCLUDE_DIR) - 1) == 0) {
      string dir(argv[i] + sizeof(EXCLUDE_DIR) - 1);
      cout << "Excluding \"" << ExcludeDir(dir) << "\" from feature search" << endl;
    }
    else if (argv[i][0] == '-' && argv[i][1] == 'D') {
      string var(argv[i]+2), val;
      size_t equal = var.find('=');
      if (equal != string::npos) {
        val.assign(var, equal+1, string::npos);
        var.erase(equal);
      }
      cout << "Predefine variable \"" << var << "\" as \"" << val << '"' << endl;
      g_predefines['$'+var] = val;
      g_predefines["${"+var+'}'] = val;
    }
    else if (stricmp(argv[i], "-V") == 0 || stricmp(argv[i], "--verbose") == 0)
      g_verbose =  true;
    else if (stricmp(argv[i], "-v") == 0 || stricmp(argv[i], "--version") == 0) {
      cout << "configure version " VERSION "\n";
      return 1;
    }
    else if (stricmp(argv[i], "-h") == 0 || stricmp(argv[i], "--help") == 0) {
      cout << "configure version " VERSION "\n"
              "usage: configure args\n"
              "  -v or --version       Dispaly version\n"
              "  -V or --verbose       Verbose output\n"
              "  --no-search           Do not search disk for libraries.\n"
              "  --extern-dir=dir      Specify where to search disk for libraries.\n"
              "  --exclude-dir=dir     Exclude dir from search path.\n";
              "  --exclude-env=var     Exclude dirs decribed by specified env var from search path.\n";
      for (feature = g_features.begin(); feature != g_features.end(); feature++)
        feature->DisplayHelp();
      return 1;
    }
    else {
      // parse environment variable if it exists
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
          g_envConfigureList.push_back(ToLower(xable));
        }
      }

      // go through features and disable ones that need disabling either from command line or environment
      for (feature = g_features.begin(); feature != g_features.end(); feature++) {
        if (feature->ProcessArgs(argv[i]))
          break;
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
  for (feature = g_features.begin(); feature != g_features.end(); feature++) {
    if (!feature->CheckFiles())
      foundAll = false;
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

  for (feature = g_features.begin(); feature != g_features.end(); feature++)
    feature->RunDependencies();

  int longestNameWidth = 0;
  for (feature = g_features.begin(); feature != g_features.end(); feature++) {
    int length = feature->m_displayName.length();
    if (length > longestNameWidth)
      longestNameWidth = length;
  }

  cout << "\n\nFeatures:\n";
  for (feature = g_features.begin(); feature != g_features.end(); feature++) {
    cout.width(longestNameWidth+3);
    feature->DisplayResult();
  }
  cout << endl;

  for (list<string>::iterator hdr = headers.begin(); hdr != headers.end(); hdr++)
    ProcessHeader(*hdr);

  cout << "Configuration completed.\n";

  return 0;
}
