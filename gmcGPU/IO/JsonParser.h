#pragma once

#include "json.h"

class CJsonParser
{
public:

  CJsonParser(void);
  ~CJsonParser(void);

  bool ParseFile(FILE* file);

  Json::Value* GetRoot(void) { return &m_root; }

private:

  Json::CharReader* m_reader;
  Json::Value       m_root;

};
