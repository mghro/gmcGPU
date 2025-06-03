#include "stdafx.h"
#include "Heapster.h"
#include "JsonParser.h"


CJsonParser::CJsonParser(void) :
  m_reader (NULL)
{
  Json::CharReaderBuilder builder;
  Json::Features features;

  builder.settings_["allowComments"] = features.allowComments_;
  builder.settings_["strictRoot"] = features.strictRoot_;
  builder.settings_["allowDroppedNullPlaceholders"] = features.allowDroppedNullPlaceholders_;
  builder.settings_["allowNumericKeys"] = features.allowNumericKeys_;

  m_reader = builder.newCharReader();

  return;
}

CJsonParser::~CJsonParser(void)
{

  SafeDelete(m_reader);

  return;
}

bool CJsonParser::ParseFile(FILE* file)
{

  fseek(file, 0, SEEK_END);
  auto const size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char* buffer;
  ReturnFailNew(buffer, char[size + 1]);

  // Terminate buffer string
  buffer[size] = 0;

  bool status;
  size_t size2 = fread(buffer, 1, size, file);
  if (size2 == size)
  {
     Json::String errors;

     status = m_reader->parse(buffer, buffer + size, &m_root, &errors);
  }
  else
  {
    status = false;
  }

  delete[] buffer;
  
  return status;
}
