#pragma once


// Looping with array
#define forArray(Array, pItem)   \
  for(auto pItem = Array.m_pItemFirst; pItem < Array.m_railHigh; ++pItem)                  // local pItem used for traversal

#define forArrayX(Array, pItem)  \
  for(pItem = Array.m_pItemFirst; pItem < Array.m_railHigh; ++pItem)                       // externally declared pItem used for traversal

#define forArrayS(Array, pItem, step)  \
  for(auto pItem = Array.m_pItemFirst; pItem < Array.m_railHigh; pItem += step)            // traverse with step

#define forArrayI(Array, pItem, indexItem)  \
  int indexItem = 0;                        \
  for(auto pItem = Array.m_pItemFirst; pItem < Array.m_railHigh; ++pItem, ++indexItem)     // traverse with index

#define forArrayN(Array, pItem, numItems)        \
  Array.m_loop1 = Array.m_pItemFirst + numItems; \
  for(auto pItem = Array.m_pItemFirst; pItem < Array.m_loop1; ++pItem)                     // loop over explicit number

#define forArrayIN(Array, pItem, indexStart, numItems)  \
  Array.m_loop0 = Array.m_pItemFirst + indexStart;      \
  Array.m_loop1 = Array.m_loop0 + numItems;             \
  for(auto pItem = Array.m_loop0; pItem < Array.m_loop1; ++pItem)                          // loop over start, number

#define forArrayI2(Array, pItem, indexStart, indexEnd)  \
  Array.m_loop0 = Array.m_pItemFirst + indexStart;      \
  Array.m_loop1 = Array.m_pItemFirst + indexEnd + 1;    \
  for(auto pItem = Array.m_loop0; pItem < Array.m_loop1; ++pItem)                          // loop over explicit interval

// Looping with pointer to array
#define forPrray(pArray, pItem)  \
  for(auto pItem = pArray->m_pItemFirst; pItem < pArray->m_railHigh; ++pItem)              // local pItem used for traversal

#define forPrrayX(pArray, pItem)  \
  for(pItem = pArray->m_pItemFirst; pItem < pArray->m_railHigh; ++pItem)                   // externally declared pItem used for traversal

#define forPrrayS(pArray, pItem, step)  \
  for(auto pItem = pArray->m_pItemFirst; pItem < pArray->m_railHigh; pItem += step)        // traverse with step

#define forPrrayI(pArray, pItem, indexItem)  \
  int indexItem = 0;                         \
  for(auto pItem = pArray->m_pItemFirst; pItem < pArray->m_railHigh; ++pItem, ++indexItem) // travere with index

#define forPrrayN(pArray, pItem, numItems)           \
  pArray->m_loop1 = pArray->m_pItemFirst + numItems; \
  for(auto pItem = pArray->m_pItemFirst; pItem < pArray->m_loop1; ++pItem)                 // loop over explicit number

#define forPrrayIN(pArray, pItem, indexStart, numItems) \
  pArray->m_loop0 = pArray->m_pItemFirst + indexStart;  \
  pArray->m_loop1 = pArray->m_loop0 + numItems;         \
  for(auto pItem = pArray->m_loop0; pItem < pArray->m_loop1; ++pItem)                      // loop over start, number

#define forPrrayI2(pArray, pItem, indexStart, indexEnd)  \
  pArray->m_loop0 = pArray->m_pItemFirst + indexStart;   \
  pArray->m_loop1 = pArray->m_pItemFirst + indexEnd + 1; \
  for(auto pItem = pArray->m_loop0; pItem < pArray->m_loop1; ++pItem)                      // loop over explicit interval