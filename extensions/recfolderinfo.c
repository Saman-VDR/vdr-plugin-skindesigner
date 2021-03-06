#include "recfolderinfo.h"


class cFolderInfoInternList;

class cRecordingsFolderInfo::cFolderInfoIntern:public cListObject {
private:
  cFolderInfoIntern         *_parent;
  cList<cFolderInfoIntern>  *_subFolders;

  cString   _name;
  time_t    _latest;
  int       _count;
  cString   _latestFileName;

  void UpdateData(const cRecording *Recording);
  cFolderInfoIntern *FindSubFolder(const char *Name) const;

public:
  cFolderInfoIntern(cFolderInfoIntern *Parent, const char *Name);
  virtual ~cFolderInfoIntern(void);

  // split Name and find folder-info in tree
  // if "Add", missing folders are created
  cFolderInfoIntern *Find(const char *Name, bool Add);

  void Add(const cRecording *Recording);

  cRecordingsFolderInfo::cFolderInfo *GetInfo(void) const;

  cString FullName(void) const;
  cString ToText(void) const;
  cString DebugOutput(void) const;
};


cRecordingsFolderInfo::cFolderInfo::cFolderInfo(const char *Name, const char *FullName, time_t Latest, int Count, const char *LatestFileName)
{
  this->Name = Name;
  this->FullName = FullName;
  this->Latest = Latest;
  this->Count = Count;
  this->LatestFileName= LatestFileName;
}


cRecordingsFolderInfo::cRecordingsFolderInfo()
   : _root(NULL)
{
  Rebuild();
}

cRecordingsFolderInfo::~cRecordingsFolderInfo(void)
{
  delete _root;
  _root = NULL;
}

void cRecordingsFolderInfo::Rebuild(void)
{
   cFolderInfoIntern *info;
   
   delete _root;
   _root = new cFolderInfoIntern(NULL, "");
   
#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
   LOCK_RECORDINGS_READ;
   const cRecordings* recordings = Recordings;

   for (const cRecording *rec = recordings->First(); rec; rec = recordings->Next(rec)) 
   {
      info = _root->Find(rec->Folder(), true);
      info->Add(rec);
   }   
#else
   cString folder;
   cThreadLock RecordingsLock(&Recordings);

   // re-get state with lock held

   Recordings.StateChanged(_recState);

   for (cRecording *rec = Recordings.First(); rec; rec = Recordings.Next(rec)) 
   {
#  if APIVERSNUM < 20102

      const char *recName = rec->Name();
      if (const char *s = strrchr(recName, FOLDERDELIMCHAR)) 
      {
         folder = recName;
         folder.Truncate(s - recName);
      }
      else
         folder = "";
#  else
      folder = rec->Folder();
#  endif
      info = _root->Find(*folder, true);
      info->Add(rec);
      }
#endif
}

cRecordingsFolderInfo::cFolderInfo  *cRecordingsFolderInfo::Get(const char *Folder)
{
   cMutexLock lock(&_rootLock);
   
#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
   static cStateKey recordingsKey;
   const cRecordings* recordings = cRecordings::GetRecordingsRead(recordingsKey);
   int recStateChanged = recordings != 0;
   if (recordings) recordingsKey.Remove();
#else
   int recStateChanged = Recordings.StateChanged(_recState);
#endif
   
   if (recStateChanged || (_root == NULL))
      Rebuild();
   
   cFolderInfoIntern *info = _root->Find(Folder, false);
   if (info == NULL)
      return NULL;
   
   return info->GetInfo();
}

cString cRecordingsFolderInfo::DebugOutput(void) const
{
  cMutexLock lock(&_rootLock);

  return _root->DebugOutput();
}


cRecordingsFolderInfo::cFolderInfoIntern::cFolderInfoIntern(cFolderInfoIntern *Parent, const char *Name)
:_parent(Parent)
,_name(Name)
,_latest(0)
,_count(0)
,_latestFileName("")
{
  _subFolders = new cList<cFolderInfoIntern>();
}

cRecordingsFolderInfo::cFolderInfoIntern::~cFolderInfoIntern(void)
{
  delete _subFolders;
  _subFolders = NULL;
}

cRecordingsFolderInfo::cFolderInfoIntern *cRecordingsFolderInfo::cFolderInfoIntern::Find(const char *Name, bool Add)
{
  cFolderInfoIntern *info = NULL;
  if (Add)
     info = this;

  if (Name && *Name) {
     static char delim[2] = { FOLDERDELIMCHAR, 0 };
     char *strtok_next;
     cFolderInfoIntern *next;
     char *folder = strdup(Name);
     info = this;
     for (char *t = strtok_r(folder, delim, &strtok_next); t; t = strtok_r(NULL, delim, &strtok_next)) {
         next = info->FindSubFolder(t);
         if (next == NULL) {
            if (!Add) {
               info = NULL;
               break;
               }

            next = new cFolderInfoIntern(info, t);
            info->_subFolders->Add(next);
            }
         info = next;
         }
     free(folder);
     }

  return info;
}

void cRecordingsFolderInfo::cFolderInfoIntern::UpdateData(const cRecording *Recording)
{
  // count every recording
  _count++;

  // update date if newer
  time_t recdate = Recording->Start();
  if (_latest < recdate) {
     _latest = recdate;
     _latestFileName = Recording->FileName();
     }
}

cRecordingsFolderInfo::cFolderInfoIntern *cRecordingsFolderInfo::cFolderInfoIntern::FindSubFolder(const char *Name) const
{
  for (cRecordingsFolderInfo::cFolderInfoIntern *info = _subFolders->First(); info; info = _subFolders->Next(info)) {
      if (strcmp(info->_name, Name) == 0)
         return info;
      }
  return NULL;
}

void cRecordingsFolderInfo::cFolderInfoIntern::Add(const cRecording *Recording)
{
  if (Recording == NULL)
     return;

  // update this and all parent folders
  for (cFolderInfoIntern *p = this; p; p = p->_parent)
     p->UpdateData(Recording);
}

cRecordingsFolderInfo::cFolderInfo *cRecordingsFolderInfo::cFolderInfoIntern::GetInfo(void) const
{
  return new cRecordingsFolderInfo::cFolderInfo(*_name, *FullName(), _latest, _count, *_latestFileName);
}

cString cRecordingsFolderInfo::cFolderInfoIntern::FullName(void) const
{
  static char delim[2] = { FOLDERDELIMCHAR, 0 };

  cString name = _name;
  for (cFolderInfoIntern *p = _parent; p; p = p->_parent) {
      // don't add FOLDERDELIMCHAR at start of FullName
      if (p->_parent == NULL)
         break;
      name = cString::sprintf("%s%s%s", *p->_name, delim, *name);
      }
  return name;
}

cString cRecordingsFolderInfo::cFolderInfoIntern::ToText(void) const
{
  return cString::sprintf("%s (%d recordings, latest: %s)\n", *FullName(), _count, *ShortDateString(_latest));
}

cString cRecordingsFolderInfo::cFolderInfoIntern::DebugOutput(void) const
{
  cString output = ToText();
  for (cFolderInfoIntern *i = _subFolders->First(); i; i = _subFolders->Next(i))
      output = cString::sprintf("%s%s", *output, *i->DebugOutput());
  return output;
}