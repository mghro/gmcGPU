#include "stdafx.h"
#include "BitmapUtils.h"

void LoadTransparentBitmap(CDC* baseDC, UINT bitmapID, CBitmap* destBitmap, DWORD bkColor)
{

  if (bkColor == 0xFFFFFFFF)
  {
    bkColor = GetSysColor(COLOR_BTNFACE);
  }

  BITMAP bmap;
  CBitmap srcBitmap;
  srcBitmap.LoadBitmapA(bitmapID);
  srcBitmap.GetBitmap(&bmap);

  CDC srcDC;
  srcDC.CreateCompatibleDC(baseDC);
  CBitmap* srcHoldBitmap = srcDC.SelectObject(&srcBitmap);


  CDC destDC;
  destDC.CreateCompatibleDC(baseDC);
  destBitmap->CreateCompatibleBitmap(baseDC, bmap.bmWidth, bmap.bmHeight);
  CBitmap* destHoldBitmap = destDC.SelectObject(destBitmap);

  destDC.FillSolidRect(0, 0, bmap.bmWidth, bmap.bmHeight, bkColor);
  destDC.TransparentBlt(0, 0, bmap.bmWidth, bmap.bmHeight, &srcDC, 0, 0, bmap.bmWidth, bmap.bmHeight, srcDC.GetPixel(0, 0));

  srcDC.SelectObject(srcHoldBitmap);
  destDC.SelectObject(destHoldBitmap);

  return;
}

/*
void LoadTransparentBitmap(CDC* baseDC, UINT bitmapID, CBitmap* destBitmap)
{

  BITMAP bmap;
  CBitmap srcBitmap;
  srcBitmap.LoadBitmapA(bitmapID);
  srcBitmap.GetBitmap(&bmap);

  CDC srcDC;
  srcDC.CreateCompatibleDC(baseDC);

  CImage img;

  img.Attach(*srcDC.GetCurrentBitmap());

  img.Save(_T(".\\test.bmp"), Gdiplus::ImageFormatBMP);

  CBitmap* srcHoldBitmap = srcDC.SelectObject(&srcBitmap);
  srcHoldBitmap->GetBitmap(&bmap);


  CDC destDC;
  destDC.CreateCompatibleDC(baseDC);
  COLORREF cf = baseDC->GetBkColor();
  destBitmap->CreateCompatibleBitmap(baseDC, bmap.bmWidth, bmap.bmHeight);
  CBitmap* destHoldBitmap = destDC.SelectObject(destBitmap);
  destDC.FillSolidRect(0, 0, bmap.bmWidth, bmap.bmHeight, GetSysColor(COLOR_BTNFACE));

  destDC.TransparentBlt(0, 0, bmap.bmWidth, bmap.bmHeight, &srcDC, 0, 0, bmap.bmWidth, bmap.bmHeight, srcDC.GetPixel(0, 0));

  srcDC.SelectObject(srcHoldBitmap);
  destDC.SelectObject(destHoldBitmap);

  return;
}
*/

void LoadTransparentBitmap(CWnd* wnd, UINT bitmapID, CBitmap* destBitmap, DWORD bkColor)
{
  if (bkColor == 0xFFFFFFFF)
  {
    bkColor = GetSysColor(COLOR_BTNFACE);
  }

  CDC* baseDC = wnd->GetDC();
  CRect rect;
  wnd->GetClientRect(&rect);

  BITMAP bmap;
  CBitmap srcBitmap;
  srcBitmap.LoadBitmapA(bitmapID);
  srcBitmap.GetBitmap(&bmap);

  CDC srcDC;
  srcDC.CreateCompatibleDC(baseDC);
  CBitmap* srcHoldBitmap = srcDC.SelectObject(&srcBitmap);

  CDC destDC;
  destDC.CreateCompatibleDC(baseDC);
  destBitmap->CreateCompatibleBitmap(baseDC, rect.Width(), rect.Height());
  CBitmap* destHoldBitmap = destDC.SelectObject(destBitmap);
  destDC.FillSolidRect(0, 0, rect.Width(), rect.Height(), bkColor);

  destDC.TransparentBlt(0, 0, rect.Width(), rect.Height(), &srcDC, 0, 0, bmap.bmWidth, bmap.bmHeight, srcDC.GetPixel(0, 0));

  srcDC.SelectObject(srcHoldBitmap);
  destDC.SelectObject(destHoldBitmap);

  return;
}