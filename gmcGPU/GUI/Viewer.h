#pragma once

#include "UniArray.h"

class CTileConfig;
struct SDisplayTile;

#define NUM_TILE_CONFIGS   (4)

typedef enum
{
  MOUSE_RSPNDR_IMAGE = 0,
  MOUSE_RSPNDR_PAN,
  MOUSE_RSPNDR_DOSE,
  MOUSE_RSPNDR_MEASURE,
  MOUSE_RSPNDR_NOOP,

  NUM_MOUSE_RESPONDERS

} MouseResponder;

enum
{
  UI_WND_WIN_LEVEL_MIN = 0,
  UI_WND_WIN_LEVEL_MAX,
  UI_WND_PT_DOSE_RESULTS,

  NUM_UI_WNDS
};

struct SSliceScale
{
  float slope;
  float intercept;
};

#include "EngineVis.h"





class CViewer;
typedef void (CViewer::* BFX_PMSG)(UINT, CPoint*);

struct SMouseResponder
{
  BFX_PMSG leftButtonDown;
  BFX_PMSG rightButtonDown;
  BFX_PMSG mouseMove;
  BFX_PMSG leftButtonUp;
  BFX_PMSG rightButtonUp;
};

class CViewer : public CView
{
public:

  CViewer(CWnd* parent, CRect* rect, DWORD scrollBars = 0);
  ~CViewer(void);

  bool Initialize(CEngineVis* engineVis, CWnd** uiWnds);
  void SetNewVolume(int* dims, float* voxelSizes);

  void SetTileConfig(int configIndex);
  UINT SetMouseResponder(UINT responder);
  void SetZoomFactor(float zoom);
  void ResetView(void);
  void SetImagePos(void);
  void SetViewAxisPrimary(int indexAxis);
  void SetViewAxisSecondary(int indexAxis);
  inline void ToggleSynchStatus(void)
  {
    m_synchStatus = !m_synchStatus;
  }

  void SetWinLevelType(int type);

  afx_msg void OnDraw(CDC* pDC) {};

protected:

  afx_msg void OnMouseMove(UINT nFlags, CPoint point);
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
  afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

  afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
  afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);


  // Called on Viewer initialization
  void CreateColorMap(void);
  void CreateColorMap2(void);
  void DefineTileConfigs(void);

  // Called in response to user actions
  void CalcTileGeometry(int* dims, float* voxelSizes);
  void UpdateWinLevelUI(void);
  void SetImageTranslation(CPoint* translation);

  // Image mouse responses
  void LeftButtonDownImage(UINT nFlags, CPoint* point);
  void RightButtonDownImage(UINT nFlags, CPoint* point);
  void MouseMoveImage(UINT nFlags, CPoint* point);

  // Pan mouse Responses
  void MouseMovePan(UINT nFlags, CPoint* point);
  void NoOp(UINT nFlags, CPoint* point);

  // Dose mouse responses
  void LeftButtonDownDose(UINT nFlags, CPoint* point);

  // Measure mouse repsonses
  void LeftButtonDownMeasure(UINT nFlags, CPoint* point);
  void MouseMoveMeasure(UINT nFlags, CPoint* point);

  CEngineVis* m_engineVis;

  int   m_numViewerPixHorz;
  int   m_numViewerPixVert;

  CUniArray<CTileConfig>  m_arrayTileConfigs;
  CTileConfig* m_tileConfigCurrent;

  UINT m_scrollBarPosImage;
  bool m_synchStatus;

  // Change due to user actions
  float m_zoomFactor;
  float m_translationHorz;
  float m_translationVert;

  CPoint m_mousePtDown;
  CPoint m_mousePtPrev;
  CPoint m_mouseTranslation;

  UINT m_activeMouseResponder;

  SMouseResponder m_mouseResponderNoOp =
  {
    (BFX_PMSG)&CViewer::NoOp,
    (BFX_PMSG)&CViewer::NoOp,
    (BFX_PMSG)&CViewer::NoOp,
    (BFX_PMSG)&CViewer::NoOp,
    (BFX_PMSG)&CViewer::NoOp,
  };

  SMouseResponder m_mouseResponderImage =
  {
    (BFX_PMSG)&CViewer::LeftButtonDownImage,
    (BFX_PMSG)&CViewer::RightButtonDownImage,
    (BFX_PMSG)&CViewer::MouseMoveImage,
    (BFX_PMSG)&CViewer::NoOp,
    (BFX_PMSG)&CViewer::NoOp
  };

  SMouseResponder m_mouseResponderPan =
  {
    (BFX_PMSG)&CViewer::NoOp,
    (BFX_PMSG)&CViewer::NoOp,
    (BFX_PMSG)&CViewer::MouseMovePan,
    (BFX_PMSG)&CViewer::NoOp,
    (BFX_PMSG)&CViewer::NoOp
  };

  SMouseResponder m_mouseResponderDose =
  {
    (BFX_PMSG)&CViewer::LeftButtonDownDose,
    (BFX_PMSG)&CViewer::NoOp,
    (BFX_PMSG)&CViewer::NoOp,
    (BFX_PMSG)&CViewer::NoOp,
    (BFX_PMSG)&CViewer::NoOp
  };

  SMouseResponder m_mouseResponderMeasure =
  {
    (BFX_PMSG)&CViewer::LeftButtonDownMeasure,
    (BFX_PMSG)&CViewer::NoOp,
    (BFX_PMSG)&CViewer::MouseMoveMeasure,
    (BFX_PMSG)&CViewer::NoOp,
    (BFX_PMSG)&CViewer::NoOp
  };
  
  SMouseResponder* m_mouseResponders[NUM_MOUSE_RESPONDERS]
  {
    &m_mouseResponderImage,
    &m_mouseResponderPan,
    &m_mouseResponderDose,
    &m_mouseResponderMeasure,
    &m_mouseResponderNoOp
  };

  float m_winLevelMin;
  float m_winLevelMax;
  float m_winLevelScale;

  int m_scrollBarPosImageAnchor;
  int m_winLevelWidthMouseAnchor;
  int m_winLevelPosMouseAnchor;
  int m_scrollWidthStatus;

  SCROLLINFO m_winLevelScrollInfo;

  CWnd** m_uiWnds;

  SMeasureInfo m_infoMeasure;
  CString m_resultsString;
  float m_doseDiffAvrg;
  int m_numDosePts;

  DECLARE_MESSAGE_MAP()
};