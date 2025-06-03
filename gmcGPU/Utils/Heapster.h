#pragma once

#define ReturnFailNew(obj, type) \
  if( (obj = new type) == NULL) return false;

#define ReturnFailNewArray(obj, type, numItems) \
  if( (obj = new type[numItems]) == NULL) return false;

#define ReturnFailHR(x) \
  if(FAILED(x)) return false;

#define ReturnOnFalse(x) \
  if((x) == false) return false;

#define ReturnOnNull(x) \
  if((x) == NULL) return false;

#define SafeDelete(x)  \
if(x)                  \
{                      \
  delete x;            \
  x = NULL;            \
}

#define SafeDeleteArray(x) \
if(x)                      \
{                          \
  delete[] x;              \
  x = NULL;                \
}

#define SafeDestroyWindow(x) \
if(x)                        \
{                            \
  x->DestroyWindow();        \
  x = NULL;                  \
}

#define SafeRelease(x)  \
if(x)                   \
{                       \
  x->Release();         \
  x = NULL;             \
}

