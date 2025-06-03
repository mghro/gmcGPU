#include "stdafx.h"
#include "Shlwapi.h"
#include "AppConfigInstance.h"
#include "FileIO.h"

static CString s_destFolders[NUM_FILE_DESTS] =
{
  "AppResults",
  "PACS",
  "Dicom",
  "Patients",
  "PACS\\PlaYer"
};

CFileIO::CFileIO(void) :
  m_fd (NULL)
{
  return;
}

CFileIO::~CFileIO(void)
{
  CloseFile();

  return;
}

FILE* CFileIO::OpenFile(EFileDest destIndex, LPCTSTR fileName,LPCTSTR specifier)
{

  m_fileNameAbs.Format("%s\\%s\\%s", g_configInstance->m_directoryBase, s_destFolders[destIndex], fileName);

  m_fd = fopen(m_fileNameAbs, specifier);

  return m_fd;
}

FILE* CFileIO::OpenFile(EFileDest destIndex, LPCTSTR folderName, LPCTSTR fileName, LPCTSTR specifier)
{
  m_folderPathAbs.Format("%s\\%s\\%s", g_configInstance->m_directoryBase, s_destFolders[destIndex], folderName);

  if (PathFileExists(m_folderPathAbs) == false)
  {
    if (CreateDirectory(m_folderPathAbs, NULL) == false)
    {
      return NULL;
    }
  }

  m_fileNameAbs.Format("%s\\%s", m_folderPathAbs, fileName);

  m_fd = fopen(m_fileNameAbs, specifier);

  return m_fd;
}

void CFileIO::CloseFile(void)
{
  if (m_fd)
  {
    fclose(m_fd);
    m_fd = NULL;
  }

  return;
}

