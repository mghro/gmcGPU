#pragma once

template <typename T> class  CUniArray;

struct SPACSServer
{
  CString serverName;
  CString serverIPAddress;
  CString serverPort;

  CUniArray<CString> aeNames;
};
