#pragma once

#include "PacsPipe.h"
#include <gdiplus.h>

using namespace Gdiplus;

class CDlgPlanGetPacs : public CDialogEx
{
public:

	CDlgPlanGetPacs(CPACSPipe* pacsPipe);
	~CDlgPlanGetPacs();

  void ExecutePACSThread(void);

  LPCTSTR GetFileNamePlan(void)
  {
    return m_fileNameBasePlan;
  }

  LPCTSTR GetPathNamePlan(void)
  {
    return m_pathNamePlan;
  }

protected:

	BOOL OnInitDialog(void);
	void DoDataExchange(CDataExchange* pDX);

	afx_msg void RetrievePlans(void);
	afx_msg void RetrieveStudy(void);
  afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnOK(void);

  void InitGif(void);
  LRESULT OnThreadComplete(WPARAM wParam, LPARAM lParam);
  void PostThreadPlans(void);
  void PostThreadStudy(void);
  void StartTimer(void);

	CListCtrl m_listCntrl;

	CPACSPipe* m_pacsPipe;

	CString m_patientID;
  CString m_fileNameBasePlan;
  CString m_pathNamePlan;

  int m_numIonPlans;
  int m_planIndex;

  Gdiplus::Image *m_gif;

  //Gdiplus::Image m_gif2;

  ULONG_PTR m_gifTimer;
  ULONG_PTR m_gdiToken;
  UINT      m_frameIndex;
  UINT      m_numFrames;
  const UINT* m_frameDelays;
  CUniArray<unsigned char> m_frameDelayData;
  int m_gifWidth;
  int m_gifHeight;


  int m_threadType;

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnLvnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
};


//#include <functional>
//#include <afxwin.h>
//
//#pragma comment(lib,"gdiplus.lib")
//
//inline void ManageGdiPlusInit(bool release=false) {
//  static int refcount = 0;
//  static ULONG_PTR token;
//  if(release) {
//    if(--refcount == 0) { 
//      Gdiplus::GdiplusShutdown(token); 
//    }
//  } else if(++refcount == 1) {
//    Gdiplus::GdiplusStartupInput startup_input;
//    Gdiplus::GdiplusStartup(&token, &startup_input, 0);
//  }   }
//
//inline void GdiPlusInit()    { ManageGdiPlusInit(false); }
//inline void GdiPlusRelease() { ManageGdiPlusInit(true); }
//
//
//  class SplashWnd : public CWnd {
//  protected:
//    static CString WindowClass() {
//      static CString name;
//      if(name.IsEmpty()) {
//        name = AfxRegisterWndClass(CS_DROPSHADOW, 0, (HBRUSH)GetStockObject(GRAY_BRUSH), 0);
//      }
//      return name;
//    }
//
//    Gdiplus::Image        *m_pImage;
//    UINT                   m_FrameCount;
//    unsigned char         *m_FrameDelayData;
//    const UINT            *m_FrameDelays;
//    UINT                   m_CurFrameIndex;
//    UINT                   m_AnimationTimerId;
//    UINT                   m_ExpireTimerId;
//    CRect                  m_WindowRect;
//
//    DECLARE_MESSAGE_MAP()
//
//    afx_msg void OnLButtonDown(UINT nFlags, CPoint point) {
//      DestroyWindow();
//    }
//
//    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
//      DestroyWindow();
//    }
//
//    afx_msg void OnDestroy() {
//      if(m_AnimationTimerId != UINT(-1)) {
//        KillTimer(m_AnimationTimerId);
//      }
//      if(m_ExpireTimerId != UINT(-1)) {
//        KillTimer(m_ExpireTimerId);
//      }
//
//      CWnd::OnDestroy();
//    }
//
//    afx_msg void OnTimer(UINT nIDEvent) {
//      if(nIDEvent == m_AnimationTimerId) {
//        if(++m_CurFrameIndex >= m_FrameCount) {
//          m_CurFrameIndex = 0;
//        }
//        DrawCurFrame();
//        KillTimer(m_AnimationTimerId);
//        m_AnimationTimerId = SetTimer(1, m_FrameDelays[m_CurFrameIndex], 0);
//        return;
//      }
//      if(nIDEvent == m_ExpireTimerId) {
//        DestroyWindow();
//        return;
//      }   }
//
//    void PostNcDestroy() {
//      if(m_DeleteSelf) {
//        delete this;
//      }
//    }
//
//    void DrawCurFrame() {
//      Gdiplus::Graphics g(m_hWnd);
//      GUID dim_select_id = Gdiplus::FrameDimensionTime;
//      m_pImage->SelectActiveFrame(&dim_select_id, m_CurFrameIndex);
//      g.DrawImage(m_pImage, 0, 0, m_WindowRect.Width(), m_WindowRect.Height());
//    }
//
//  public:
//    // set m_DeleteSelf to true if a SplashWnd is created with new, and you want it to
//    // auto-delete itself when the window expires or is dismissed.
//    bool m_DeleteSelf;
//
//    // file_path    the gif file path
//    // ExpireMs     the time, in milliseconds until the window automatically closes itself
//    // WidthFactor  the fraction of the width of the primary display to use as the splash screen's width
//    // HeightFactor the fraction of the height of the primary display to use as the height
//    // If WidthFactor or HeightFactor are 0, the original image aspect ratio is preserved
//    // If both are 0, the original image size, in pixels is used
//    SplashWnd(CString file_path, DWORD ExpireMs=2000, double WidthFactor=0.4, double HeightFactor=0) {
//      GdiPlusInit();
//
//      WCHAR* fileName = L"C:\\MyProjects\\GMCWork\\gmcGPU\\res\\pace.gif";
//
//      m_pImage = new Gdiplus::Image(fileName);
//
//      // Set up an array of frame times for animated images
//      UINT dimension_count = m_pImage->GetFrameDimensionsCount();
//
//      GUID dimension_id;
//      m_pImage->GetFrameDimensionsList(&dimension_id, 1);
//      m_FrameCount = m_pImage->GetFrameCount(&dimension_id);
//      UINT frame_delay_size = m_pImage->GetPropertyItemSize(PropertyTagFrameDelay);
//
//      m_FrameDelayData = new unsigned char[frame_delay_size];
//      Gdiplus::PropertyItem* frame_delay_item = reinterpret_cast<Gdiplus::PropertyItem*>(m_FrameDelayData);
//      m_pImage->GetPropertyItem(PropertyTagFrameDelay, frame_delay_size, frame_delay_item);
//      m_FrameDelays = reinterpret_cast<const UINT*>(frame_delay_item->value);
//
//      // Figure out the size and location of the splash window
//      int primary_width  = GetSystemMetrics(SM_CXFULLSCREEN);
//      int primary_height = GetSystemMetrics(SM_CYFULLSCREEN);
//
//      int splash_width  = int(primary_width * WidthFactor);
//      int splash_height = int(primary_height * HeightFactor);
//
//      if(splash_width == 0) {
//        if(splash_height == 0) {
//          splash_width  = m_pImage->GetWidth();
//          splash_height = m_pImage->GetHeight();
//        } else {
//          splash_width = primary_width * splash_height / primary_height;
//        }
//      } else if(splash_height == 0) {
//        splash_height = primary_height * splash_width / primary_width;
//      }
//
//      int l = (primary_width - splash_width) / 2;
//      int t = (primary_height - splash_height) / 2;
//      int r = l + splash_width;
//      int b = t + splash_height;
//
//      m_WindowRect.SetRect(
//        (primary_width  - splash_width)  / 2,
//        (primary_height - splash_height) / 2,
//        (primary_width  + splash_width)  / 2,
//        (primary_height + splash_height) / 2);
//
//      // WS_EX_TOPMOST makes the window cover up other, regular windows
//      // WS_EX_TOOLWINDOW prevents an icon for this window in the taskbar
//      // WS_POPUP prevents caption and border from being drawn
//      CreateEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, WindowClass(), _T("Splash"), WS_VISIBLE | WS_POPUP, m_WindowRect, 0, 0);
//
//      // Show the first frame
//      m_CurFrameIndex = 0;
//      DrawCurFrame();
//
//      // Set up the frame-flipping animation timer
//      m_ExpireTimerId = m_AnimationTimerId = UINT(-1);
//      if(m_FrameCount > 1) {
//        m_AnimationTimerId = SetTimer(1, m_FrameDelays[m_CurFrameIndex], 0);
//      }
//      // Set up the expiration timer
//      if(ExpireMs != INFINITE) {
//        m_ExpireTimerId = SetTimer(2, ExpireMs, 0);
//      }
//
//      m_DeleteSelf = false;
//    }
//
//    // Constructor which takes a callback function which will be called when the splash window closes
//    template <typename F>
//    SplashWnd(CString file_path, DWORD ExpireMs, double WidthFactor, double HeightFactor, F DismissCallback)
//      : SplashWnd(file_path, ExpireMs, WidthFactor, HeightFactor)
//    {
//      m_DismissCallback = DismissCallback;
//    }
//
//    ~SplashWnd() {
//      delete [] m_FrameDelayData;
//      delete m_pImage;
//      GdiPlusRelease();
//    }
//  };

  // Message map, usually in an implementation file, but here encapsulated inside the header
  // using an anonymous namespace to prevent possible ODR problems.
  //BEGIN_MESSAGE_MAP(SplashWnd, CWnd)
  //  ON_WM_KEYDOWN()
  //  ON_WM_LBUTTONDOWN()
  //  ON_WM_TIMER()
  //  ON_WM_DESTROY()
  //END_MESSAGE_MAP()


