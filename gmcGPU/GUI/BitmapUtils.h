#pragma once

void LoadTransparentBitmap(CDC* baseDC, UINT bitmapID, CBitmap* destBitmap, DWORD bkColor = 0xFFFFFFFF);
void LoadTransparentBitmap(CWnd* wnd, UINT bitmapID, CBitmap* destBitmap, DWORD bkColor = 0xFFFFFFFF);