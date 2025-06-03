#include "stdafx.h"
#include "DirectX.h"
#include "Vector.h"
#include "TileConfig.h"
#include "EngineVis.h"
#include "Viewer.h"

#define WIN_LEVEL_SCROLL_MIN_DEFAULT    (-3000)
#define WIN_LEVEL_SCROLL_MAX_DEFAULT    (3000)
#define WIN_LEVEL_WIDTH_DEFAULT         (2048)
#define SCROLL_IMAGE_WIDTH              (1024)

BEGIN_MESSAGE_MAP(CViewer, CView)

  ON_WM_PAINT()
  ON_WM_VSCROLL()
  ON_WM_HSCROLL()

  ON_WM_MOUSEMOVE()
  ON_WM_LBUTTONDOWN()
  ON_WM_LBUTTONUP()
  ON_WM_RBUTTONDOWN()
  ON_WM_RBUTTONUP()

  ON_WM_NCRBUTTONDOWN()
  ON_WM_NCMOUSEMOVE()
  ON_WM_NCRBUTTONUP()

END_MESSAGE_MAP()

CViewer::CViewer(CWnd* parent, CRect* rect, DWORD scrollBars) : CView(),
  m_tileConfigCurrent    (NULL),
  m_translationHorz      (0.0),
  m_translationVert      (0.0),
  m_zoomFactor           (1.0),
  m_synchStatus          (true),
  m_scrollBarPosImage    (0),
  m_activeMouseResponder (MOUSE_RSPNDR_NOOP),
  m_uiWnds               (NULL)
{

  m_mouseTranslation = { 0, 0 };

  Create(NULL, NULL, WS_CHILDWINDOW | scrollBars, *rect, parent, 0, NULL);

  return;
}

CViewer::~CViewer(void)
{
  return;
}

bool CViewer::Initialize(CEngineVis* engineVis, CWnd** uiWnds)
{

  m_engineVis = engineVis;
  m_uiWnds    = uiWnds;

  CRect rect;
  GetClientRect(&rect);
  m_numViewerPixHorz = rect.Width();
  m_numViewerPixVert = rect.Height();

  CreateColorMap();

  SetScrollRange(SB_VERT, 0, m_numViewerPixVert - 1);
  SetScrollPos(SB_VERT, 0, TRUE);

  m_winLevelScale = 1.0;

  m_winLevelMin = (WIN_LEVEL_SCROLL_MAX_DEFAULT + WIN_LEVEL_SCROLL_MIN_DEFAULT) / 2 - WIN_LEVEL_WIDTH_DEFAULT / 2; 
  m_winLevelMax = m_winLevelMin + WIN_LEVEL_WIDTH_DEFAULT;

  m_winLevelScrollInfo.nMin  = WIN_LEVEL_SCROLL_MIN_DEFAULT;
  m_winLevelScrollInfo.nMax  = WIN_LEVEL_SCROLL_MAX_DEFAULT;
  m_winLevelScrollInfo.nPage = WIN_LEVEL_WIDTH_DEFAULT;
  m_winLevelScrollInfo.nPos  = m_winLevelMin;
  m_winLevelScrollInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;

  SetScrollInfo(SB_HORZ, &m_winLevelScrollInfo, 1);

  m_engineVis->SetWindowLevels(m_winLevelMin, m_winLevelMax);

  DefineTileConfigs();

  return true;
}

void CViewer::DefineTileConfigs(void)
{
  CTileConfig* config;

  m_arrayTileConfigs.Alloc(NUM_TILE_CONFIGS);

  config = m_arrayTileConfigs(0);

  int axisInit0[] = { 0 };
  config->CalcTileGeometry(1, 1, m_numViewerPixHorz, m_numViewerPixVert, axisInit0);
  config->m_numTilesPrimary = 1;

  ++config;
  int axisInit1[] = { 0, 1, 2, 0 };
  config->CalcTileGeometry(2, 2, m_numViewerPixHorz, m_numViewerPixVert, axisInit1);
  config->m_numTilesPrimary = 3;

  ++config;
  int axisInit2[] = { 0, 0 };
  config->CalcTileGeometry(1, 2, m_numViewerPixHorz, m_numViewerPixVert, axisInit2);
  config->m_numTilesPrimary = 1;

  ++config;
  int axisInit3[] = { 0, 0 };
  config->CalcTileGeometry(2, 1, m_numViewerPixHorz, m_numViewerPixVert, axisInit2);
  config->m_numTilesPrimary = 1;

  return;
}

void CViewer::CreateColorMap(void)
{
  SColorMap colorMap;
  SColor4* colors = colorMap.colors;
  colorMap.numColors = 1024;

  // Blue to Cyan to Green to Yellow to Red (0 - 1023)
  for (int i = 0; i < 256; ++i)
  {
    colors[i].red = 0.0;
    colors[i].green = i / 256.0;
    colors[i].blue = 1.0;
    colors[i].alpha = 1.0;

    colors[256 + i].red = 0.0;
    colors[256 + i].green = 1.0;
    colors[256 + i].blue = (256 - i) / 256.0;
    colors[256 + i].alpha = 1.0;

    colors[512 + i].red = i / 256.0;
    colors[512 + i].green = 1.0;
    colors[512 + i].blue = 0;
    colors[512 + i].alpha = 1.0;

    colors[768 + i].red = 1.0;
    colors[768 + i].green = (256 - i) / 256.0;
    colors[768 + i].blue = 0.0;
    colors[768 + i].alpha = 1.0;
  }

  m_engineVis->CreateColorMapBuffer(&colorMap);

  return;
}

void CViewer::CreateColorMap2(void)
{
  SColorMap colorMap;

  float rgb[] =
  {
    0.2422,0.1504,0.6603, 1.0,
    0.2444,0.1534,0.6728, 1.0,
    0.2464,0.1569,0.6847, 1.0,
    0.2484,0.1607,0.6961, 1.0,
    0.2503,0.1648,0.7071, 1.0,
    0.2522,0.1689,0.7179, 1.0,
    0.2540,0.1732,0.7286, 1.0,
    0.2558,0.1773,0.7393, 1.0,
    0.2576,0.1814,0.7501, 1.0,
    0.2594,0.1854,0.7610, 1.0,
    0.2611,0.1893,0.7719, 1.0,
    0.2628,0.1932,0.7828, 1.0,
    0.2645,0.1972,0.7937, 1.0,
    0.2661,0.2011,0.8043, 1.0,
    0.2676,0.2052,0.8148, 1.0,
    0.2691,0.2094,0.8249, 1.0,
    0.2704,0.2138,0.8346, 1.0,
    0.2717,0.2184,0.8439, 1.0,
    0.2729,0.2231,0.8528, 1.0,
    0.2740,0.2280,0.8612, 1.0,
    0.2749,0.2330,0.8692, 1.0,
    0.2758,0.2382,0.8767, 1.0,
    0.2766,0.2435,0.8840, 1.0,
    0.2774,0.2489,0.8908, 1.0,
    0.2781,0.2543,0.8973, 1.0,
    0.2788,0.2598,0.9035, 1.0,
    0.2794,0.2653,0.9094, 1.0,
    0.2798,0.2708,0.9150, 1.0,
    0.2802,0.2764,0.9204, 1.0,
    0.2806,0.2819,0.9255, 1.0,
    0.2809,0.2875,0.9305, 1.0,
    0.2811,0.2930,0.9352, 1.0,
    0.2813,0.2985,0.9397, 1.0,
    0.2814,0.3040,0.9441, 1.0,
    0.2814,0.3095,0.9483, 1.0,
    0.2813,0.3150,0.9524, 1.0,
    0.2811,0.3204,0.9563, 1.0,
    0.2809,0.3259,0.9600, 1.0,
    0.2807,0.3313,0.9636, 1.0,
    0.2803,0.3367,0.9670, 1.0,
    0.2798,0.3421,0.9702, 1.0,
    0.2791,0.3475,0.9733, 1.0,
    0.2784,0.3529,0.9763, 1.0,
    0.2776,0.3583,0.9791, 1.0,
    0.2766,0.3638,0.9817, 1.0,
    0.2754,0.3693,0.9840, 1.0,
    0.2741,0.3748,0.9862, 1.0,
    0.2726,0.3804,0.9881, 1.0,
    0.2710,0.3860,0.9898, 1.0,
    0.2691,0.3916,0.9912, 1.0,
    0.2670,0.3973,0.9924, 1.0,
    0.2647,0.4030,0.9935, 1.0,
    0.2621,0.4088,0.9946, 1.0,
    0.2591,0.4145,0.9955, 1.0,
    0.2556,0.4203,0.9965, 1.0,
    0.2517,0.4261,0.9974, 1.0,
    0.2473,0.4319,0.9983, 1.0,
    0.2424,0.4378,0.9991, 1.0,
    0.2369,0.4437,0.9996, 1.0,
    0.2311,0.4497,0.9995, 1.0,
    0.2250,0.4559,0.9985, 1.0,
    0.2189,0.4620,0.9968, 1.0,
    0.2128,0.4682,0.9948, 1.0,
    0.2066,0.4743,0.9926, 1.0,
    0.2006,0.4803,0.9906, 1.0,
    0.1950,0.4861,0.9887, 1.0,
    0.1903,0.4919,0.9867, 1.0,
    0.1869,0.4975,0.9844, 1.0,
    0.1847,0.5030,0.9819, 1.0,
    0.1831,0.5084,0.9793, 1.0,
    0.1818,0.5138,0.9766, 1.0,
    0.1806,0.5191,0.9738, 1.0,
    0.1795,0.5244,0.9709, 1.0,
    0.1785,0.5296,0.9677, 1.0,
    0.1778,0.5349,0.9641, 1.0,
    0.1773,0.5401,0.9602, 1.0,
    0.1768,0.5452,0.9560, 1.0,
    0.1764,0.5504,0.9516, 1.0,
    0.1755,0.5554,0.9473, 1.0,
    0.1740,0.5605,0.9432, 1.0,
    0.1716,0.5655,0.9393, 1.0,
    0.1686,0.5705,0.9357, 1.0,
    0.1649,0.5755,0.9323, 1.0,
    0.1610,0.5805,0.9289, 1.0,
    0.1573,0.5854,0.9254, 1.0,
    0.1540,0.5902,0.9218, 1.0,
    0.1513,0.5950,0.9182, 1.0,
    0.1492,0.5997,0.9147, 1.0,
    0.1475,0.6043,0.9113, 1.0,
    0.1461,0.6089,0.9080, 1.0,
    0.1446,0.6135,0.9050, 1.0,
    0.1429,0.6180,0.9022, 1.0,
    0.1408,0.6226,0.8998, 1.0,
    0.1383,0.6272,0.8975, 1.0,
    0.1354,0.6317,0.8953, 1.0,
    0.1321,0.6363,0.8932, 1.0,
    0.1288,0.6408,0.8910, 1.0,
    0.1253,0.6453,0.8887, 1.0,
    0.1219,0.6497,0.8862, 1.0,
    0.1185,0.6541,0.8834, 1.0,
    0.1152,0.6584,0.8804, 1.0,
    0.1119,0.6627,0.8770, 1.0,
    0.1085,0.6669,0.8734, 1.0,
    0.1048,0.6710,0.8695, 1.0,
    0.1009,0.6750,0.8653, 1.0,
    0.0964,0.6789,0.8609, 1.0,
    0.0914,0.6828,0.8562, 1.0,
    0.0855,0.6865,0.8513, 1.0,
    0.0789,0.6902,0.8462, 1.0,
    0.0713,0.6938,0.8409, 1.0,
    0.0628,0.6972,0.8355, 1.0,
    0.0535,0.7006,0.8299, 1.0,
    0.0433,0.7039,0.8242, 1.0,
    0.0328,0.7071,0.8183, 1.0,
    0.0234,0.7103,0.8124, 1.0,
    0.0155,0.7133,0.8064, 1.0,
    0.0091,0.7163,0.8003, 1.0,
    0.0046,0.7192,0.7941, 1.0,
    0.0019,0.7220,0.7878, 1.0,
    0.0009,0.7248,0.7815, 1.0,
    0.0018,0.7275,0.7752, 1.0,
    0.0046,0.7301,0.7688, 1.0,
    0.0094,0.7327,0.7623, 1.0,
    0.0162,0.7352,0.7558, 1.0,
    0.0253,0.7376,0.7492, 1.0,
    0.0369,0.7400,0.7426, 1.0,
    0.0504,0.7423,0.7359, 1.0,
    0.0638,0.7446,0.7292, 1.0,
    0.0770,0.7468,0.7224, 1.0,
    0.0899,0.7489,0.7156, 1.0,
    0.1023,0.7519,0.7088, 1.0,
    0.1141,0.7531,0.7019, 1.0,
    0.1252,0.7552,0.6959, 1.0,
    0.1354,0.7572,0.6881, 1.0,
    0.1448,0.7593,0.6812, 1.0,
    0.1532,0.7614,0.6741, 1.0,
    0.1609,0.7635,0.6671, 1.0,
    0.1678,0.7656,0.6599, 1.0,
    0.1741,0.7678,0.6527, 1.0,
    0.1799,0.7699,0.6454, 1.0,
    0.1853,0.7721,0.6379, 1.0,
    0.1905,0.7743,0.6303, 1.0,
    0.1954,0.7765,0.6225, 1.0,
    0.2003,0.7787,0.6146, 1.0,
    0.2061,0.7808,0.6065, 1.0,
    0.2118,0.7828,0.5983, 1.0,
    0.2178,0.7849,0.5899, 1.0,
    0.2244,0.7869,0.5813, 1.0,
    0.2318,0.7887,0.5725, 1.0,
    0.2401,0.7905,0.5636, 1.0,
    0.2491,0.7922,0.5546, 1.0,
    0.2589,0.7937,0.5454, 1.0,
    0.2695,0.7951,0.5369, 1.0,
    0.2809,0.7964,0.5266, 1.0,
    0.2929,0.7975,0.5179, 1.0,
    0.3052,0.7985,0.5074, 1.0,
    0.3176,0.7994,0.4975, 1.0,
    0.3301,0.8002,0.4876, 1.0,
    0.3424,0.8009,0.4774, 1.0,
    0.3548,0.8016,0.4669, 1.0,
    0.3671,0.8021,0.4563, 1.0,
    0.3795,0.8026,0.4454, 1.0,
    0.3921,0.8029,0.4344, 1.0,
    0.4050,0.8031,0.4233, 1.0,
    0.4184,0.8030,0.4122, 1.0,
    0.4322,0.8028,0.4013, 1.0,
    0.4463,0.8024,0.3904, 1.0,
    0.4608,0.8018,0.3797, 1.0,
    0.4753,0.8011,0.3691, 1.0,
    0.4899,0.8002,0.3586, 1.0,
    0.5044,0.7993,0.3480, 1.0,
    0.5187,0.7982,0.3374, 1.0,
    0.5329,0.7970,0.3267, 1.0,
    0.5470,0.7957,0.3159, 1.0,
    0.5609,0.7943,0.3050, 1.0,
    0.5748,0.7929,0.2941, 1.0,
    0.5886,0.7913,0.2833, 1.0,
    0.6024,0.7896,0.2726, 1.0,
    0.6161,0.7878,0.2622, 1.0,
    0.6297,0.7859,0.2521, 1.0,
    0.6433,0.7839,0.2423, 1.0,
    0.6567,0.7818,0.2329, 1.0,
    0.6701,0.7796,0.2239, 1.0,
    0.6833,0.7773,0.2155, 1.0,
    0.6963,0.7750,0.2075, 1.0,
    0.7091,0.7727,0.1998, 1.0,
    0.7218,0.7703,0.1924, 1.0,
    0.7344,0.7679,0.1852, 1.0,
    0.7468,0.7654,0.1782, 1.0,
    0.7590,0.7629,0.1717, 1.0,
    0.7710,0.7604,0.1658, 1.0,
    0.7829,0.7579,0.1608, 1.0,
    0.7945,0.7554,0.1570, 1.0,
    0.8060,0.7529,0.1546, 1.0,
    0.8172,0.7505,0.1535, 1.0,
    0.8281,0.7481,0.1536, 1.0,
    0.8389,0.7457,0.1546, 1.0,
    0.8495,0.7435,0.1564, 1.0,
    0.8600,0.7413,0.1587, 1.0,
    0.8703,0.7392,0.1615, 1.0,
    0.8804,0.7372,0.1650, 1.0,
    0.8903,0.7353,0.1695, 1.0,
    0.9000,0.7336,0.1749, 1.0,
    0.9093,0.7321,0.1815, 1.0,
    0.9184,0.7308,0.1890, 1.0,
    0.9272,0.7298,0.1973, 1.0,
    0.9357,0.7290,0.2061, 1.0,
    0.9440,0.7285,0.2151, 1.0,
    0.9523,0.7284,0.2237, 1.0,
    0.9606,0.7285,0.2312, 1.0,
    0.9689,0.7292,0.2373, 1.0,
    0.9770,0.7304,0.2418, 1.0,
    0.9842,0.7330,0.2446, 1.0,
    0.9900,0.7365,0.2429, 1.0,
    0.9946,0.7407,0.2394, 1.0,
    0.9966,0.7458,0.2351, 1.0,
    0.9971,0.7513,0.2309, 1.0,
    0.9972,0.7569,0.2267, 1.0,
    0.9971,0.7626,0.2224, 1.0,
    0.9969,0.7683,0.2181, 1.0,
    0.9966,0.7740,0.2138, 1.0,
    0.9962,0.7798,0.2095, 1.0,
    0.9957,0.7856,0.2053, 1.0,
    0.9949,0.7915,0.2012, 1.0,
    0.9938,0.7974,0.1974, 1.0,
    0.9923,0.8034,0.1939, 1.0,
    0.9906,0.8095,0.1906, 1.0,
    0.9885,0.8156,0.1875, 1.0,
    0.9861,0.8218,0.1846, 1.0,
    0.9835,0.8280,0.1817, 1.0,
    0.9807,0.8342,0.1787, 1.0,
    0.9778,0.8404,0.1757, 1.0,
    0.9748,0.8467,0.1726, 1.0,
    0.9720,0.8529,0.1695, 1.0,
    0.9694,0.8591,0.1665, 1.0,
    0.9671,0.8654,0.1636, 1.0,
    0.9651,0.8716,0.1608, 1.0,
    0.9634,0.8778,0.1582, 1.0,
    0.9619,0.8840,0.1557, 1.0,
    0.9608,0.8902,0.1532, 1.0,
    0.9601,0.8963,0.1507, 1.0,
    0.9596,0.9023,0.1480, 1.0,
    0.9595,0.9084,0.1450, 1.0,
    0.9597,0.9143,0.1418, 1.0,
    0.9601,0.9203,0.1382, 1.0,
    0.9608,0.9262,0.1344, 1.0,
    0.9618,0.9320,0.1304, 1.0,
    0.9629,0.9379,0.1261, 1.0,
    0.9642,0.9437,0.1216, 1.0,
    0.9657,0.9494,0.1168, 1.0,
    0.9674,0.9552,0.1116, 1.0,
    0.9692,0.9609,0.1061, 1.0,
    0.9711,0.9667,0.1001, 1.0,
    0.9730,0.9724,0.0938, 1.0,
    0.9749,0.9782,0.0872, 1.0,
    0.9769,0.9839,0.0805, 1.0,
  };

  memcpy(colorMap.colors, rgb, 256 * 4 * sizeof(float));
  colorMap.numColors = 256;

  m_engineVis->CreateColorMapBuffer(&colorMap);

  return;
}

void CViewer::SetNewVolume(int* dims, float* voxelSizes)
{
  CalcTileGeometry(dims, voxelSizes);

  m_tileConfigCurrent = NULL;
  SetTileConfig(0);

  m_scrollBarPosImage = 0;

  SetImagePos();

  SetMouseResponder(MOUSE_RSPNDR_IMAGE);

  return;
}

void CViewer::CalcTileGeometry(int* dims, float* voxelSizes)
{
  forArray(m_arrayTileConfigs, config)
  {
    config->SetImageGeometry(dims, voxelSizes);
  }

  return;
}

void CViewer::SetTileConfig(int configIndex)
{

  if (m_arrayTileConfigs(configIndex) == m_tileConfigCurrent)
  {
    return;
  }

  float prevTileSizeHorzPixels;
  float prevTileSizeVertPixels;

  int viewAxis;
  if (m_tileConfigCurrent)
  {
    prevTileSizeHorzPixels = m_tileConfigCurrent->m_numPixHorzTile;
    prevTileSizeVertPixels = m_tileConfigCurrent->m_numPixVertTile;
    viewAxis               = m_tileConfigCurrent->GetPrimaryView();
  }
  else
  {
    prevTileSizeHorzPixels = 0.0;
    prevTileSizeVertPixels = 0.0;
    viewAxis = 0;
  }

  m_tileConfigCurrent = m_arrayTileConfigs(configIndex);
  m_tileConfigCurrent->SetPrimaryView(viewAxis, m_synchStatus);

  m_engineVis->SetTileConfig(m_tileConfigCurrent);

  if (m_translationHorz)
  {
    m_translationHorz *= (prevTileSizeHorzPixels / m_tileConfigCurrent->m_numPixHorzTile);
  }

  if (m_translationVert)
  {
    m_translationVert *= (prevTileSizeVertPixels / m_tileConfigCurrent->m_numPixVertTile);
  }

  SetImagePos();

  return;
}

void CViewer::SetZoomFactor(float zoomFactor)
{
  m_zoomFactor = zoomFactor;

  m_tileConfigCurrent->SetZoomFactor(zoomFactor);

  return;
}

void CViewer::SetImageTranslation(CPoint* translation)
{

  m_translationHorz = translation->x;
  m_translationVert = translation->y;

  m_tileConfigCurrent->SetImageTranslation(translation);

  return;
}

void CViewer::ResetView(void) // MLW make UI element to connect
{
  CPoint center(0.0, 0.0);

  SetImageTranslation(&center);

  SetZoomFactor(1.0);

  return;
}

void CViewer::SetViewAxisPrimary(int indexAxis)
{

  m_tileConfigCurrent->SetPrimaryView(indexAxis, m_synchStatus);

  SetImagePos();

  return;
}

void CViewer::SetViewAxisSecondary(int indexAxis)
{

  m_tileConfigCurrent->SetSecondaryView(indexAxis, m_synchStatus);

  SetImagePos();

  return;
}

void CViewer::UpdateWinLevelUI(void)
{

  if (m_uiWnds[UI_WND_WIN_LEVEL_MIN])
  {
    CString winLevelInfo;

    winLevelInfo.Format("%d", m_winLevelMin);
    m_uiWnds[UI_WND_WIN_LEVEL_MIN]->SetWindowTextA(winLevelInfo);

    winLevelInfo.Format("%d", m_winLevelMax);
    m_uiWnds[UI_WND_WIN_LEVEL_MAX]->SetWindowTextA(winLevelInfo);
  }

  return;
}

void CViewer::OnVScroll(UINT action, UINT position, CScrollBar* scrollBar)
{
  if ((m_activeMouseResponder == MOUSE_RSPNDR_NOOP) || (action == SB_ENDSCROLL))
  {
    return;
  }

  if (m_scrollBarPosImage != position)
  {
    m_scrollBarPosImage = position;

    SetImagePos();

    m_engineVis->Render();
  }

  return;
}

void CViewer::SetImagePos(void)
{

  float slicePos = ((float)m_scrollBarPosImage) / (m_numViewerPixVert - 1);

  m_tileConfigCurrent->SetImagePos(slicePos, m_synchStatus);

  m_engineVis->CalcSlicePositions(slicePos);

  SetScrollPos(SB_VERT, m_scrollBarPosImage, TRUE);

  return;
}

void CViewer::SetWinLevelType(int type)
{
  if (type == 0)
  {
    m_winLevelScale = 1.0;

    m_winLevelMin = (WIN_LEVEL_SCROLL_MAX_DEFAULT + WIN_LEVEL_SCROLL_MIN_DEFAULT) / 2 - WIN_LEVEL_WIDTH_DEFAULT / 2;
    m_winLevelMax = m_winLevelMin + WIN_LEVEL_WIDTH_DEFAULT;

    m_winLevelScrollInfo.nMin  = WIN_LEVEL_SCROLL_MIN_DEFAULT;
    m_winLevelScrollInfo.nMax  = WIN_LEVEL_SCROLL_MAX_DEFAULT;
    m_winLevelScrollInfo.nPage = WIN_LEVEL_WIDTH_DEFAULT;
    m_winLevelScrollInfo.nPos  = m_winLevelMin;
  }
  else
  {
    m_winLevelScale = 1000.0;

    m_winLevelScrollInfo.nMin  = 0;
    m_winLevelScrollInfo.nMax  = 4000;
    m_winLevelScrollInfo.nPage = 2000;
    m_winLevelScrollInfo.nPos  = 0;

    m_winLevelMin = 0;
    m_winLevelMax = 4;
  }

  SetScrollInfo(SB_HORZ, &m_winLevelScrollInfo, 1);

  m_engineVis->SetWindowLevels(m_winLevelMin, m_winLevelMax);

  return;
}

void CViewer::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{

  int iPos = (int)nPos;

  if ((nPos != m_winLevelMin) && (nSBCode != SB_ENDSCROLL))
  {
    m_winLevelMin = iPos / m_winLevelScale;
    m_winLevelMax = m_winLevelMin + m_winLevelScrollInfo.nPage / m_winLevelScale;

    m_engineVis->SetWindowLevels(m_winLevelMin, m_winLevelMax);

    //TRACE("WINLEVEL min/max %f %f\n", m_winLevelMin, m_winLevelMax);

    m_engineVis->Render();

    SetScrollPos(SB_HORZ, iPos, 1);

    UpdateWinLevelUI();
  }

  return;
}

UINT CViewer::SetMouseResponder(UINT responder)
{
  UINT prevResponder = m_activeMouseResponder;

  m_activeMouseResponder = responder;

  m_engineVis->SetMeasureInfo(NULL);

  m_resultsString.Empty();
  m_uiWnds[UI_WND_PT_DOSE_RESULTS]->SetWindowTextA(m_resultsString);

  if (responder == MOUSE_RSPNDR_DOSE)
  {
    m_engineVis->PrepPointDose();
    m_numDosePts = 0;
    m_doseDiffAvrg = 0.0;
  }

  return prevResponder;
};

void CViewer::OnLButtonDown(UINT nFlags, CPoint point)
{

  m_mousePtDown.x = point.x;
  m_mousePtDown.y = point.y;

  (this->*m_mouseResponders[m_activeMouseResponder]->leftButtonDown)(nFlags, &point);

  m_mousePtPrev.x = point.x;
  m_mousePtPrev.y = point.y;

  return;
}

void CViewer::OnLButtonUp(UINT nFlags, CPoint point)
{

  (this->*m_mouseResponders[m_activeMouseResponder]->leftButtonUp)(nFlags, &point);

  m_mousePtPrev.x = point.x;
  m_mousePtPrev.y = point.y;

  return;
}

void CViewer::OnRButtonDown(UINT nFlags, CPoint point)
{

  m_mousePtDown.x = point.x;
  m_mousePtDown.y = point.y;

  (this->*m_mouseResponders[m_activeMouseResponder]->rightButtonDown)(nFlags, &point);

  (this->*m_mouseResponders[0]->rightButtonDown)(nFlags, &point);

  m_mousePtPrev.x = point.x;
  m_mousePtPrev.y = point.y;

  return;
}

void CViewer::OnRButtonUp(UINT nFlags, CPoint point)
{

  (this->*m_mouseResponders[m_activeMouseResponder]->rightButtonUp)(nFlags, &point);

  m_mousePtPrev.x = point.x;
  m_mousePtPrev.y = point.y;

  return;
}

void CViewer::OnMouseMove(UINT nFlags, CPoint point)
{

  (this->*m_mouseResponders[m_activeMouseResponder]->mouseMove)(nFlags, &point);

  m_mousePtPrev.x = point.x;
  m_mousePtPrev.y = point.y;

  return;
}

void CViewer::NoOp(UINT nFlags, CPoint* point)
{
  return;
}

/*_____________________________________________________________________________________________________________________

  Image Mode Mouse Response    (window level and image selection)

    Right button drag vertical  - Change image
    Left button drag horizontal - Change window level center
    Left button drag vertical   - Change window level width
_______________________________________________________________________________________________________________________*/

void CViewer::LeftButtonDownImage(UINT nFlags, CPoint* point)
{

  m_scrollBarPosImageAnchor = m_scrollBarPosImage;

  return;
}

void CViewer::RightButtonDownImage(UINT nFlags, CPoint* point)
{

  m_winLevelWidthMouseAnchor = m_winLevelScrollInfo.nPage;
  m_winLevelPosMouseAnchor = m_winLevelScrollInfo.nPos;

  return;
}

void CViewer::MouseMoveImage(UINT nFlags, CPoint* point)
{
  
  if (nFlags & MK_LBUTTON) // Left button down  - Image Scroll
  {
    m_scrollBarPosImage = m_scrollBarPosImageAnchor + (point->y - m_mousePtDown.y);

    if (m_scrollBarPosImage < 0)
    {
      m_scrollBarPosImage = 0;
    }
    else if (m_scrollBarPosImage > m_numViewerPixVert)
    {
      m_scrollBarPosImage = m_numViewerPixVert;
    }

    SetImagePos();

    // Allow derived classes to perform specific actions
    //ImageChangeAux();

    m_engineVis->Render();
  }
  else if (nFlags & MK_RBUTTON) // Right button down - Window Level
  {
    CPoint delta = *point - m_mousePtDown;

    if (abs(delta.y) > abs(delta.x)) // Vert movement - window level width
    {

      int width = m_winLevelWidthMouseAnchor + delta.y * (WIN_LEVEL_SCROLL_MAX_DEFAULT - WIN_LEVEL_SCROLL_MIN_DEFAULT) / m_numViewerPixVert;

      if (width < 3)
      {
        width = 3;
      }

      m_winLevelMin = m_winLevelPosMouseAnchor + (m_winLevelWidthMouseAnchor - width) / 2.0F;

      if (m_winLevelMin < WIN_LEVEL_SCROLL_MIN_DEFAULT)
      {
        width -= (WIN_LEVEL_SCROLL_MIN_DEFAULT - m_winLevelMin);
        m_winLevelMin = WIN_LEVEL_SCROLL_MIN_DEFAULT;

      }

      m_winLevelMax = m_winLevelMin + width;

      if (m_winLevelMax > WIN_LEVEL_SCROLL_MAX_DEFAULT)
      {
        width -= (m_winLevelMax - WIN_LEVEL_SCROLL_MAX_DEFAULT);
        m_winLevelMin += (m_winLevelMax - WIN_LEVEL_SCROLL_MAX_DEFAULT);
        m_winLevelMax = WIN_LEVEL_SCROLL_MAX_DEFAULT;
      }

      m_winLevelScrollInfo.nPos = m_winLevelMin;
      m_winLevelScrollInfo.nPage = width;
      m_winLevelScrollInfo.fMask = SIF_POS | SIF_PAGE;

    }
    else // Horz movement - window level position
    {
      int position = m_winLevelPosMouseAnchor + delta.x * (WIN_LEVEL_SCROLL_MAX_DEFAULT - WIN_LEVEL_SCROLL_MIN_DEFAULT) / m_numViewerPixHorz;

      if (delta.x == 0)
        return;

      if (position < WIN_LEVEL_SCROLL_MIN_DEFAULT)
      {
        position = WIN_LEVEL_SCROLL_MIN_DEFAULT;
      }
      else if (position + (int)m_winLevelScrollInfo.nPage > WIN_LEVEL_SCROLL_MAX_DEFAULT)
      {
        position = WIN_LEVEL_SCROLL_MAX_DEFAULT - m_winLevelScrollInfo.nPage;
      }

      m_winLevelMin = position;
      m_winLevelMax = position + (int)m_winLevelScrollInfo.nPage;

      m_winLevelScrollInfo.nPos  = position;
      m_winLevelScrollInfo.fMask = SIF_POS | SIF_PAGE;

    }

    SetScrollInfo(SB_HORZ, &m_winLevelScrollInfo, 1);

    m_engineVis->SetWindowLevels(m_winLevelMin, m_winLevelMax);

    UpdateWinLevelUI();

    m_engineVis->Render();
  }

  return;
}

void CViewer::MouseMovePan(UINT nFlags, CPoint* point)
{

  if (nFlags & MK_LBUTTON)
  { 
    //TRACE("PANPT %d %d %d %d %d %d\n", m_mousePtPrev.x, m_mousePtPrev.y, point->x, point->y, m_mouseTranslation.x, m_mouseTranslation.y);
    m_mouseTranslation += *point - m_mousePtPrev;

    SetImageTranslation(&m_mouseTranslation);
    m_engineVis->Render();
  }

  return;
}

void CViewer::LeftButtonDownDose(UINT nFlags, CPoint* point)
{
  float results[8];
  SPt2  ptInfo[3];
  SPt3  dosePt;

  SDisplayTile* tilePicked = m_tileConfigCurrent->ConvertPoint(point, &m_mouseTranslation, ptInfo);

  float imagePos = tilePicked->vsTileInfo.slicePosFract;

  if (tilePicked->vsTileInfo.viewAxis == 0)
  {
    dosePt.x = imagePos;
    dosePt.y = 1.0F - ptInfo[0].x;
    dosePt.z = 1.0F - ptInfo[0].y;
  }
  else if (tilePicked->vsTileInfo.viewAxis == 1)
  {
    dosePt.x = ptInfo[0].x;
    dosePt.y = imagePos;
    dosePt.z = 1.0F - ptInfo[0].y;
  }
  else
  {
    dosePt.x = ptInfo[0].x;
    dosePt.y = ptInfo[0].y;
    dosePt.z = imagePos;
  }

  m_engineVis->CalcPointDose(&dosePt, ptInfo + 1, tilePicked, results);

  CString resultsString;
  resultsString.Format("D0 %6.2f D1 %6.2f Img %f\r\nDiff %6.2f%%\r\n\r\n", results[0], results[1], results[6], results[2]);

  m_resultsString += resultsString;

  m_doseDiffAvrg = m_doseDiffAvrg * m_numDosePts + fabs(results[2]);
  ++m_numDosePts;
  m_doseDiffAvrg /= m_numDosePts;

  resultsString.Format("Num Pts %d Avrg Diff %6.2f%%", m_numDosePts, m_doseDiffAvrg);
  resultsString = m_resultsString + resultsString;

  m_uiWnds[UI_WND_PT_DOSE_RESULTS]->SetWindowTextA(resultsString);
  CEdit* edit = (CEdit*)m_uiWnds[UI_WND_PT_DOSE_RESULTS];
  edit->LineScroll(edit->GetLineCount());

  return;
}

void CViewer::LeftButtonDownMeasure(UINT nFlags, CPoint* point)
{

  m_infoMeasure.drawTile = m_tileConfigCurrent->FindTile(point);

  m_tileConfigCurrent->ConvertPtScreenPixToShader(point, &m_infoMeasure.origin);

  TRACE("DM %d %d %f %f\n", point->x, point->y, m_infoMeasure.origin.x, m_infoMeasure.origin.y);

  m_engineVis->SetMeasureInfo(&m_infoMeasure);

  return;
}

void CViewer::MouseMoveMeasure(UINT nFlags, CPoint* point)
{
  if (nFlags & MK_LBUTTON)
  {

    CRect* rect = &m_infoMeasure.drawTile->tileRect;

    // Limit second point to tile boundary
    if (point->x > rect->right)
    {
      point->x = rect->right;
    }
    else if (point->x < rect->left)
    {
      point->x = rect->left;
    }

    if (point->y > rect->bottom)
    {
      point->y = rect->bottom;
    }
    else if (point->y < rect->top)
    {
      point->y = rect->top;
    }

    m_tileConfigCurrent->ConvertPtScreenPixToShader(point, &m_infoMeasure.corner);

    m_engineVis->Render();

    int view = m_infoMeasure.drawTile->vsTileInfo.viewAxis;

    float deltaX = (m_infoMeasure.origin.x - m_infoMeasure.corner.x) * m_tileConfigCurrent->m_scaleAbsHorz[view];
    float deltaY = (m_infoMeasure.origin.y - m_infoMeasure.corner.y) * m_tileConfigCurrent->m_scaleAbsVert[view];

    float distance = sqrt(deltaX * deltaX + deltaY * deltaY);

    //float xo = (-1.0F + m_infoMeasure.origin.x);
    m_resultsString.Format("Distance = %7.2f mm\r\nPoint1 %7.2f %7.2f\r\nPoint2 %7.2f %7.2f",
      distance, m_infoMeasure.origin.x, m_infoMeasure.origin.y, m_infoMeasure.corner.x, m_infoMeasure.corner.y);

    m_uiWnds[2]->SetWindowTextA(m_resultsString);
  }

  return;
}


