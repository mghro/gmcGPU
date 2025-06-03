#pragma once

/*_____________________________________________________________________________________________

    CUniArray - fast, lightweight array traversal

    Note: m_railHigh required 
       for(pItem = m_pItemFirst; pItem <= m_pItemLast; ++pItem)
         fails if m_pItemFirst = m_pItemLast = NULL, loop executes once, pItem reference fails
       for(pItem = m_pItemFirst; pItem < m_railHigh; ++pItem) works, no loop execution

_______________________________________________________________________________________________*/

template <typename T> class  CUniArray
{
public:

  CUniArray(void) :
    m_dataAlloc    (NULL),
    m_dataExternal (NULL),
    m_pItemFirst   (NULL),
    m_pItemLast    (NULL),
    m_railHigh     (NULL),
    m_loop0        (NULL),
    m_loop1        (NULL),
    m_numItems     (0) 
  {};

  CUniArray(int numItems) : CUniArray()
  {
    Alloc(numItems);

    return;
  }

  CUniArray(int numItems, T initVal) : CUniArray(numItems)
  {

    SetItemAll(initVal);

    return;
  }

  ~CUniArray(void)
  {

    if (m_dataAlloc)
    {
      delete[] m_dataAlloc;
    }

    return;
  }

  void Clear(void)
  {

    if (m_dataAlloc)
    {
      delete[] m_dataAlloc;
    }

    m_dataAlloc    = NULL;
    m_dataExternal = NULL;
    m_pItemFirst   = NULL;
    m_pItemLast    = NULL;
    m_railHigh     = NULL;
    m_numItems     = 0;

    return;
  }

  void Init(int numItems, T initVal)
  {
    Alloc(numItems);

    SetItemAll(initVal);

    return;
  }

  T* Alloc(unsigned int numItems)
  {
    if (m_dataAlloc)
    {
      delete[] m_dataAlloc;
      m_dataAlloc = NULL;
    }

    if (numItems)
    {
      m_dataAlloc = new T[numItems];
    }

    if (m_dataAlloc)
    {
      m_pItemFirst = m_dataAlloc;
      m_railHigh   = m_pItemFirst + numItems;
      m_pItemLast  = m_railHigh - 1;

      m_numItems  = numItems;
    }

    return m_dataAlloc;
  }

  void WrapData(T* data, int numItems)
  {
    m_dataExternal = data;

    m_pItemFirst = m_dataExternal;
    m_railHigh   = m_pItemFirst + numItems;
    m_pItemLast  = m_railHigh - 1;

    m_numItems  = numItems;

    return;
  }

  void ZeroMem(void)
  {
    memset(m_pItemFirst, 0, sizeof(T) * m_numItems);

    return;
  }

  inline unsigned int GetSizeItems(void)
  {
    return sizeof(T);
  }

 inline unsigned int GetSizeBytes(void)
  {
    return (m_numItems * sizeof(T));
  }

  inline T Item(unsigned int itemNum)
  {
    return m_pItemFirst[itemNum];
  }

  inline T ItemFirst(void)
  {
    return *m_pItemFirst;
  }

  inline T ItemLast(void)
  {
    return *m_pItemLast;
  }

  inline void SetItem(unsigned int itemNum, T input)
  {
    m_pItemFirst[itemNum] = input;
  }

  void SetItemAll(T initVal)
  {
    for (T* pItem = m_pItemFirst; pItem < m_railHigh; ++pItem)
    {
      *pItem = initVal;
    }

    return;
  }

  inline T* ItemPtr(unsigned int itemNum)
  {
    return (m_pItemFirst + itemNum);
  }

  inline T* ItemPtrFirst(void)
  {
    return m_pItemFirst;
  }

  inline T* ItemPtrLast(void)
  {
    return m_pItemLast;
  }

  // Note: ClearItems() will not compile if T not a pointer type, (a good thing)
  void ClearItems(void)
  {
    for (T* pItem = m_pItemFirst; pItem < m_railHigh; ++pItem)
    {
      if (*pItem)
      {
        delete *pItem;
      }
    }
  }

  T& operator[](int index)
  {
    return m_pItemFirst[index];
  }

  T* operator() (int index) 
  {
    return m_pItemFirst + index;
  }

  T* m_dataAlloc;
  T* m_dataExternal;

  T* m_pItemFirst;
  T* m_pItemLast;

  T* m_railHigh;
  T* m_loop0;
  T* m_loop1;

  unsigned int m_numItems;

};


#ifdef UARRAY_DBG
#include "UniLoopDbg.h"
#else
#include "UniLoop.h"
#endif
