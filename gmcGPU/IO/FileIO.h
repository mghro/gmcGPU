#pragma once

enum EFileDest
{
  FILE_DEST_DATA,
  FILE_DEST_PACS,
  FILE_DEST_DICOM,
  FILE_DEST_PATIENTS,
  FILE_DEST_PACS_PLYR,

  NUM_FILE_DESTS
};

class CFileIO
{
public:

  CFileIO(void);
  ~CFileIO(void);

  FILE* OpenFile(EFileDest destIndex, LPCTSTR fileName, LPCTSTR specifier);
  FILE* OpenFile(EFileDest destIndex, LPCTSTR folderName, LPCTSTR fileName, LPCTSTR specifier);

  void  CloseFile(void);

private:

  CString m_fileNameAbs;
  CString m_folderPathAbs;

  FILE* m_fd;

};

