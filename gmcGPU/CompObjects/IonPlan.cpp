#include "stdafx.h"
#include "DirectXMath.h"
#include "UniArray.h"
#include "Heapster.h"
#include "FileIO.h"
#include "AppConfigInstance.h"
#include "JsonParser.h"
#include "IonPlan.h"

#define DEG_TO_RAD         (0.01745329251F)
#define RSP_DATA_SCALE     (1000.0)  // Scale factor used before added to json file
#define DEVICE_HALF_WIDTH  (400.0)   // MLW check

CIonPlan::CIonPlan(void) :
  m_frameReferenceUID ("1.22.333.4444"), // dcmtk can't handle blank string, make sure dummy available
  m_numProtonsPlan    (0.0),
  m_numFractions      (0),
  m_flagProne         (false),
  m_flagSOBP          (false),
  m_flagHCL           (false)
{
  return;
}

CIonPlan::~CIonPlan(void)
{

  Clear();

  return;
}

void CIonPlan::Clear(void)
{
  SafeDeleteArray(m_voxGridHU.data);
  SafeDeleteArray(m_voxGridRSP.data);

  return;
}

struct SControlParseParams
{
  bool  angleChange;
  float gantryAngle;
  float supportAngle;
  float isocenter[3];
  float isocenterQA[3];

  DirectX::XMFLOAT3X3 beamMatrix;
};

bool CIonPlan::ParseJSON(FILE* file)
{
  Json::Value* root;

  CJsonParser jparser;
  bool status = jparser.ParseFile(file);

  if (status == false)
  {
    AfxMessageBox("Error parsing JSON Plan file.");
    return false;
  }

  root = jparser.GetRoot();

  ReadMetaData(root);

  CountPlanObjectsJson(root);

  FillPlanJson(root);

  bool status2 = ParseGrids(root);

#ifdef DBG_0 // Save plan objects
  //SaveEnergies(root);
  //SaveDevices();
  SaveApertures();
#endif

  return status2;
}

void CIonPlan::ReadMetaData(Json::Value* root)
{
  if ((*root).isMember("Date"))
  {
    m_studyDate = (*root)["Date"].asCString();
  }
  else
  {
    m_studyDate = "1776_07_04";
  }

  m_planName = (*root)["RTPlanName"].asCString();
  m_patientName = (*root)["PatientName"].asCString();
  m_patientID = (*root)["PatientID"].asCString();

  if ((*root).isMember("RTPlanDescription"))
  {
    m_astroidID = (*root)["RTPlanDescription"].asCString();
    m_astroidID = m_astroidID.Right(m_astroidID.GetLength() - m_astroidID.Find(':') - 1);
    m_astroidID = m_astroidID.Left(m_astroidID.Find(')'));
  }
  else
  {
    m_astroidID = "0123458789876543210";
  }

  if ((*root).isMember("RTPlanLabel"))
  {
    m_planLabel = (*root)["RTPlanLabel"].asCString();
  }
  else
  {
    m_planLabel = "PlanLabelBlank";
  }

  // Used in dose file name so must remove replace bad charactes
  if ((*root).isMember("SeriesDescription"))
  {
    m_seriesDescription = (*root)["SeriesDescription"].asCString();
  }
  else
  {
    m_seriesDescription = m_planLabel;
  }
  m_seriesDescription.Replace('\\', '-');
  m_seriesDescription.Replace('/', '-');
  m_seriesDescription.Replace(':', '-');
  m_seriesDescription.Replace('&', '-');
  m_seriesDescription.Replace('*', '-');
  m_seriesDescription.Replace('?', '-');
  m_seriesDescription.Replace(';', '-');

  m_studyUID  = (*root)["StudyInstanceUID"].asCString();
  m_seriesUID = (*root)["SeriesInstanceUID"].asCString();
  m_planUID   = (*root)["SOPInstanceUID"].asCString();

  return;
}

void CIonPlan::CountPlanObjectsJson(Json::Value* root)
{
  unsigned int numControlPts         = 0;
  unsigned int numSpots              = 0;
  unsigned int numAperturePts        = 0;
  unsigned int numApertures          = 0;
  unsigned int numDevices            = 0;
  unsigned int numDeviceIndexes      = 0;
  unsigned int numCompensators       = 0;
  unsigned int numCompensatorDepths  = 0;
  unsigned int numSOBPGrids          = 0;
  unsigned int numDepthDoses         = 0;
  unsigned int numLETs               = 0;

  Json::Value jBeams;

  jBeams = (*root)["IonBeamSequence"];

  unsigned int numBeams = jBeams.size();

#ifdef DBG_0 // Limit num beams
  numBeams = 1;
#define DBG_BEAM_LIMIT
#endif

  // Assumes all beams in HCL plan are consistent
  if (jBeams[0].isMember("beam2patient"))
  {
    m_flagHCL = true;
  }

  for (unsigned int indexBeam = 0; indexBeam < numBeams; ++indexBeam)
  {

    if (jBeams[indexBeam].isMember("IonBlockSequence")) // For backwards compatibility to old mgh json format
    {
      Json::Value jBlocks = jBeams[indexBeam]["IonBlockSequence"];

      int numBlocks = jBlocks.size();

      for (int indexBlock = 0; indexBlock < numBlocks; ++indexBlock)
      {

        Json::Value jBlockData = jBlocks[indexBlock]["BlockData"];

        numAperturePts += jBlockData.size() / 2 + 1;
        numApertures += 1; // Always holds for old json files

#ifdef DBG_0
        numApertures -= 1;
#endif
      }

      numDevices += numBlocks;

      numDeviceIndexes = numDevices;
    }

    Json::Value jControlPts = jBeams[indexBeam]["IonControlPointSequence"];

    int numControlPtsBeam = jControlPts.size();

    for (int indexControlPt = 0; indexControlPt < numControlPtsBeam; ++indexControlPt)
    {

      if (jControlPts[indexControlPt].isMember("RangeShifterSettingsSequence"))
      {
        Json::Value jShifter = jControlPts[indexControlPt]["RangeShifterSettingsSequence"];
        numDevices += jShifter.size();
      }

      numDepthDoses += jControlPts[indexControlPt]["DepthPDD"].size() + 1;

      numLETs += jControlPts[indexControlPt]["DepthLET"].size();

      Json::Value jScanPositions = jControlPts[indexControlPt]["ScanSpotPositionMap"];

      numSpots += jScanPositions.size() / 2;

    }

    numControlPts += numControlPtsBeam;


    if (jBeams[indexBeam].isMember("BeamShapeIndex"))
    {

      int numDeviceControlPts = jBeams[indexBeam]["BeamShapeIndex"].size();

      for (int indexCntrlPt = 0; indexCntrlPt < numDeviceControlPts; ++indexCntrlPt)
      {
        numDeviceIndexes += jBeams[indexBeam]["BeamShapeIndex"][indexCntrlPt].size();
      }
    }

    if (jBeams[indexBeam].isMember("ShapeSequence"))
    {

      Json::Value  jDevicesBeam = jBeams[indexBeam]["ShapeSequence"];

      int numDevicesBeam = jDevicesBeam.size();

      for (int indexDevice = 0; indexDevice < numDevicesBeam; ++indexDevice)
      {

        if (jDevicesBeam[indexDevice].isMember("ApertureSequence"))
        {
          Json::Value  jCompensatorSequence = jDevicesBeam[indexDevice]["ApertureSequence"];

          int numAperturesDevice = jCompensatorSequence.size();

          for (int indexAperture = 0; indexAperture < numAperturesDevice; ++indexAperture)
          {
            Json::Value jBlockData = jCompensatorSequence[indexAperture]["Data"];
            numAperturePts += jBlockData.size() / 2 + 1;  // + 1 because want last point to close with first point for interior algorithm
          }

          numApertures += numAperturesDevice;
        }

        if (jDevicesBeam[indexDevice].isMember("RangeCompensatorSequence"))
        {
          Json::Value  jCompensatorSequence = jDevicesBeam[indexDevice]["RangeCompensatorSequence"];

          int numCompensatorsDevice = jCompensatorSequence.size();

          for (int indexCompensator = 0; indexCompensator < numCompensatorsDevice; ++indexCompensator)
          {
            Json::Value jBlockData = jCompensatorSequence[indexCompensator]["Data"];
            numCompensatorDepths += jCompensatorSequence[indexCompensator]["CompensatorRows"].asInt() * 
              jCompensatorSequence[indexCompensator]["CompensatorColumns"].asInt();
          }

          numCompensators += numCompensatorsDevice;
        }

        if (jDevicesBeam[indexDevice].isMember("CompensatorThicknessData"))
        {
          Json::Value  depthData = jDevicesBeam[indexDevice]["CompensatorThicknessData"];
          numCompensatorDepths += depthData.size();

          ++numCompensators;
        }
      }

      numDevices += numDevicesBeam;
    } // Shape Sequence

    if (jBeams[indexBeam].isMember("RectiGrid"))
    {
      m_flagSOBP = true;
      ++numSOBPGrids;
    }
  }

  Json::Value jFractions = (*root)["FractionGroupSequence"];

  unsigned int numFractionGroups = jFractions.size();
  unsigned int numFractionBeams = 0;

  for (unsigned int fractionIndex = 0; fractionIndex < numFractionGroups; ++fractionIndex)
  {
    numFractionBeams += jFractions[fractionIndex]["NumberOfBeams"].asInt();
  }

  // Allocate memeory for plan shader objects
  m_arrayBeams.Alloc(numBeams);
  m_arrayMetaBeams.Alloc(numBeams);
  m_arrayControlPts.Alloc(numControlPts);
  m_arraySpots.Alloc(numSpots);
  m_arrayDevices.Alloc(numDevices);
  m_arrayDeviceIndexes.Alloc(numDeviceIndexes);
  m_arrayApertures.Alloc(numApertures);
  m_arrayAperturePts.Alloc(numAperturePts);
  m_arrayCompensators.Alloc(numCompensators);
  m_arrayCompensatorDepths.Alloc(numCompensatorDepths);
  m_arraySOBPGrids.Alloc(numSOBPGrids);
  m_arrayDepthDoses.Alloc(numDepthDoses);
  m_arrayAlphas.Alloc(numDepthDoses);
  m_arrayLETs.Alloc(numLETs);
  m_arrayFractionGroups.Alloc(numFractionGroups);
  m_arrayFractionBeams.Alloc(numFractionBeams);

  if (jFractions[0].isMember("ReferencedBeamDoseSequence"))
  {
    m_arrayFractionBeamDoses.Alloc(numBeams);
  }

  return;
}

void CIonPlan::FillPlanJson(Json::Value* root)
{
  SControlParseParams controlParams;
  unsigned int seed = m_arraySpots.m_numItems;
  int indexDevIndexesBeam;

  SShaderControlPt* controlPt     = m_arrayControlPts(0);
  SShaderSpot* spot               = m_arraySpots(0);
  SShaderDevice* device           = m_arrayDevices(0);
  int* indexDevice                = m_arrayDeviceIndexes(0);
  SShaderAperture* aperture       = m_arrayApertures(0);
  SPt2* aperturePt                = m_arrayAperturePts(0);
  SShaderCompensator* compensator = m_arrayCompensators(0);
  float* compensatorDepth         = m_arrayCompensatorDepths(0);
  SSOBPGrid* sobpGrid             = m_arraySOBPGrids(0);
  float* depthDose                = m_arrayDepthDoses(0);
  float* alpha                    = m_arrayAlphas(0);
  float* let                      = m_arrayLETs(0);

  // Zero out and count again so these can be used as object indexes
  unsigned int numControlPts        = 0;
  unsigned int numSpots             = 0;
  unsigned int numDevices           = 0;
  unsigned int numDeviceIndexes     = 0;
  unsigned int numAperturePts       = 0;
  unsigned int numApertures         = 0;
  unsigned int numCompensators      = 0;
  unsigned int numCompensatorDepths = 0;
  unsigned int numSOBPGrids         = 0;
  unsigned int numDepthDoses        = 0;
  unsigned int numLETs              = 0;

  m_numProtonsPlan = 0.0;

  Json::Value jBeams = (*root)["IonBeamSequence"];

  forArrayI(m_arrayBeams, beam, indexBeam)
  {
    bool transformControlPt  = true;
    beam->numDevices    = 0;
    beam->indexDevices  = numDevices;
    indexDevIndexesBeam = numDevices;

    memset(&controlParams, 0, sizeof(SControlParseParams));
    XMStoreFloat3x3(&controlParams.beamMatrix, DirectX::XMMatrixIdentity());

    m_arrayMetaBeams[indexBeam].beamName   = jBeams[indexBeam]["BeamName"].asCString();
    m_arrayMetaBeams[indexBeam].beamNumber = jBeams[indexBeam]["BeamNumber"].asInt();

    Json::Value jSad = jBeams[indexBeam]["VirtualSourceAxisDistances"];

    for (unsigned int sadIndex = 0; sadIndex < jSad.size(); ++sadIndex)
    {
      beam->sad[sadIndex] = jSad[sadIndex].asFloat();
    }

    if (beam->sad[0] < beam->sad[1])
    {
      beam->protonInitZ = beam->sad[0];
      beam->sadMaxAxis  = 1;
      beam->sadScale    = (beam->sad[1] - beam->sad[0]) / beam->sad[1];
    }
    else
    {
      beam->protonInitZ = beam->sad[1];
      beam->sadMaxAxis  = 0;
      beam->sadScale    = (beam->sad[0] - beam->sad[1]) / beam->sad[0];
    }

    if (jBeams[indexBeam].isMember("IonBlockSequence"))  // For backwards compatibility to old mgh json format
    {
      Json::Value jBlocks = jBeams[indexBeam]["IonBlockSequence"];

      int numBlocks = jBlocks.size();

      for (int indexBlock = 0; indexBlock < numBlocks; ++indexBlock, ++device)
      {
        device->indexApertures = numApertures;

        float blockThickness = jBlocks[indexBlock]["BlockThickness"].asFloat();
        float isocenterBlockDistance = jBlocks[indexBlock]["IsocenterToBlockTrayDistance"].asFloat();

        device->numApertures = 1; // Always holds for old files
#ifdef DBG_0
        device->numApertures = 0;
#endif

        device->indexMtrl  = MTRL_COPPER; // Always holds for old files
        device->zTop       = isocenterBlockDistance + blockThickness;
        device->zBottom    = isocenterBlockDistance;
        device->halfRangeX = DEVICE_HALF_WIDTH;
        device->halfRangeY = DEVICE_HALF_WIDTH;

        for (int ap = 0; ap < device->numApertures; ++ap, ++aperture)
        {
          Json::Value jBlockData = jBlocks[indexBlock]["BlockData"];

          aperture->indexMtrl = MTRL_AIR;

          aperture->numAperturePts = jBlockData.size() / 2; 

          aperture->xMax = -FLT_MAX;
          aperture->xMin =  FLT_MAX;
          aperture->yMax = -FLT_MAX;
          aperture->yMin =  FLT_MAX;

          for (unsigned int dataIndex = 0; dataIndex < jBlockData.size(); ++dataIndex)
          {
            aperturePt->x = jBlockData[dataIndex].asFloat();
            ++dataIndex;
            aperturePt->y = jBlockData[dataIndex].asFloat();

            if (aperturePt->x > aperture->xMax)
            {
              aperture->xMax = aperturePt->x;
            }
            if (aperturePt->x < aperture->xMin)
            {
              aperture->xMin = aperturePt->x;
            }
            if (aperturePt->y > aperture->yMax)
            {
              aperture->yMax = aperturePt->y;
            }
            if (aperturePt->y < aperture->yMin)
            {
              aperture->yMin = aperturePt->y;
            }

            ++aperturePt;
          }

          // Set last point to first point for interior algorithm
          aperturePt->x = jBlockData[0].asFloat();
          aperturePt->y = jBlockData[1].asFloat();
          ++aperturePt;

          aperture->indexAperturePts = numAperturePts;
          numAperturePts += aperture->numAperturePts + 1; // + 1 because want last point to close with first point for interior algorithm
        }

        numApertures += device->numApertures;
      }

      numDevices += numBlocks;
      beam->numDevices += numBlocks;
    }

    Json::Value jControlPts = jBeams[indexBeam]["IonControlPointSequence"];

    beam->numControlPts   = jControlPts.size();
    beam->indexControlPts = numControlPts;
    beam->indexSpots      = numSpots;
    beam->numSpots        = 0;

    if (jBeams[indexBeam].isMember("RectiX"))
    {
      Json::Value jSOBPGridX = jBeams[indexBeam]["RectiX"];
      Json::Value jSOBPGridY = jBeams[indexBeam]["RectiY"];
      sobpGrid->numVoxX = jSOBPGridX.size();
      sobpGrid->numVoxY = jSOBPGridY.size();
      sobpGrid->numVoxGrid = sobpGrid->numVoxX * sobpGrid->numVoxY;
      sobpGrid->upperLeft[0] = jSOBPGridX[0].asFloat();
      sobpGrid->upperLeft[1] = jSOBPGridY[0].asFloat();
      sobpGrid->voxSizeX = jSOBPGridX[1].asFloat() - sobpGrid->upperLeft[0];
      sobpGrid->voxSizeY = jSOBPGridY[1].asFloat() - sobpGrid->upperLeft[1];

      ++sobpGrid;
    }

    Json::Value jIndexDevicesBeam;
    if (jBeams[indexBeam].isMember("BeamShapeIndex"))
    {
      jIndexDevicesBeam = jBeams[indexBeam]["BeamShapeIndex"];
    }

    if (jBeams[indexBeam].isMember("beam2patient"))
    {


      controlParams.beamMatrix._11 = jBeams[indexBeam]["beam2patient"][0][0].asFloat();
      controlParams.beamMatrix._12 = jBeams[indexBeam]["beam2patient"][0][1].asFloat();
      controlParams.beamMatrix._13 = jBeams[indexBeam]["beam2patient"][0][2].asFloat();

      controlParams.beamMatrix._21 = jBeams[indexBeam]["beam2patient"][1][0].asFloat();
      controlParams.beamMatrix._22 = jBeams[indexBeam]["beam2patient"][1][1].asFloat();
      controlParams.beamMatrix._23 = jBeams[indexBeam]["beam2patient"][1][2].asFloat();

      controlParams.beamMatrix._31 = jBeams[indexBeam]["beam2patient"][2][0].asFloat();
      controlParams.beamMatrix._32 = jBeams[indexBeam]["beam2patient"][2][1].asFloat();
      controlParams.beamMatrix._33 = jBeams[indexBeam]["beam2patient"][2][2].asFloat();

      transformControlPt = false;
    }

    for (int indexControlPt = 0; indexControlPt < beam->numControlPts; ++indexControlPt, ++controlPt)
    {

      controlPt->indexBeam          = indexBeam;
      controlPt->indexDeviceIndexes = numDeviceIndexes;

      controlPt->spotSize[0] = jControlPts[indexControlPt]["ScanningSpotSize"][0].asFloat();
      controlPt->spotSize[1] = jControlPts[indexControlPt]["ScanningSpotSize"][1].asFloat();

#ifdef DBG_0
      controlPt->spotSize[0] = 30.0;
      controlPt->spotSize[1] = 30.0;
#endif

      if (jControlPts[indexControlPt].isMember("GantryAngle") && transformControlPt)
      {
        controlParams.gantryAngle  = jControlPts[indexControlPt]["GantryAngle"].asFloat();
        controlParams.angleChange = true;

#ifdef DBG_0
        controlParams.gantryAngle = 0.0;
#endif
      }

      if (jControlPts[indexControlPt].isMember("PatientSupportAngle") && transformControlPt)
      {
        controlParams.supportAngle = jControlPts[indexControlPt]["PatientSupportAngle"].asFloat();
        controlParams.angleChange = true;

#ifdef DBG_0
        controlParams.supportAngle = 0.0;
#endif
      }

      if (controlParams.angleChange)
      {
        CalcControlPtMatrix(&controlParams);

        controlParams.angleChange = false;
      }

      if (jControlPts[indexControlPt].isMember("IsocenterPosition"))
      {
        controlParams.isocenter[0] = jControlPts[indexControlPt]["IsocenterPosition"][0].asFloat();
        controlParams.isocenter[1] = jControlPts[indexControlPt]["IsocenterPosition"][1].asFloat();
        controlParams.isocenter[2] = jControlPts[indexControlPt]["IsocenterPosition"][2].asFloat();
      }

      controlPt->beamMatrix = controlParams.beamMatrix;

      controlPt->isocenter[0] = controlParams.isocenter[0];
      controlPt->isocenter[1] = controlParams.isocenter[1];
      controlPt->isocenter[2] = controlParams.isocenter[2];

      if (jControlPts[indexControlPt].isMember("SurfaceEntryPoint"))
      {
        float surfacePoint[3];

        surfacePoint[0] = jControlPts[indexControlPt]["SurfaceEntryPoint"][0].asFloat();
        surfacePoint[1] = jControlPts[indexControlPt]["SurfaceEntryPoint"][1].asFloat();
        surfacePoint[2] = jControlPts[indexControlPt]["SurfaceEntryPoint"][2].asFloat();

        float dist;
        float totalDist = 0.0;
        dist = surfacePoint[0] - controlPt->isocenter[0];
        totalDist += dist * dist;
        dist = surfacePoint[1] - controlPt->isocenter[1];
        totalDist += dist * dist;
        dist = surfacePoint[2] - controlPt->isocenter[2];
        totalDist += dist * dist;

        controlParams.isocenterQA[0] = 0.0;
        controlParams.isocenterQA[1] = sqrt(totalDist);
        controlParams.isocenterQA[2] = 0.0;
      }

      controlPt->isocenterQA[0] = controlParams.isocenterQA[0];
      controlPt->isocenterQA[1] = controlParams.isocenterQA[1];
      controlPt->isocenterQA[2] = controlParams.isocenterQA[2];

      if (jControlPts[indexControlPt].isMember("RangeShifterSettingsSequence"))   // For backwards compatibility to old mgh json format
      {

        Json::Value jRangeShifters = jControlPts[indexControlPt]["RangeShifterSettingsSequence"];
        int numShifters = jRangeShifters.size();


        //TRACE("RS %d %d\n", indexControlPt, numShifters);

        for (int indexShifters = 0; indexShifters < numShifters; ++indexShifters, ++device)
        {
          device->zTop      = jRangeShifters[indexShifters]["ztop"].asFloat();
          device->zBottom   = jRangeShifters[indexShifters]["zbot"].asFloat();
          device->indexMtrl = MTRL_LUCITE;  // Always holds for old files

          device->numApertures   = 0;
          device->indexApertures = 0;
          device->halfRangeX     = DEVICE_HALF_WIDTH;
          device->halfRangeY     = DEVICE_HALF_WIDTH;
        }

        numDevices += numShifters;
        beam->numDevices += numShifters;
      }

      Json::Value jScanPositions = jControlPts[indexControlPt]["ScanSpotPositionMap"];
      Json::Value jScanWeights   = jControlPts[indexControlPt]["ScanSpotMetersetWeights"];
      Json::Value jScanEquivRad  = jControlPts[indexControlPt]["SpotMapEquivRadius"];
      Json::Value jAlphas        = jControlPts[indexControlPt]["alpha"];


      controlPt->numSpots = jScanPositions.size() / 2;

      controlPt->indexSpots = numSpots;
      numSpots += controlPt->numSpots;
      beam->numSpots += controlPt->numSpots;

      float equivRadius;
      int ptIndex;
      for (int spotIndex = 0; spotIndex < controlPt->numSpots; ++spotIndex, ++spot)
      {

        ptIndex = 2 * spotIndex;
        spot->spotCenterX = jScanPositions[ptIndex].asFloat();
        ++ptIndex;
        spot->spotCenterY = jScanPositions[ptIndex].asFloat();

#ifdef DBG_0
        spot->spotCenterX = 0.0;
        spot->spotCenterY = 0.0;
#endif
        if (m_flagSOBP)
        {
          // DBG_0
          spot->weight = jControlPts[indexControlPt]["ScanSpotMetersetWeights"].asFloat();
        }
        else
        {
          spot->weight = jScanWeights[spotIndex].asFloat();
        }

        m_numProtonsPlan += spot->weight;

        equivRadius = jScanEquivRad[spotIndex].asFloat();
        spot->spotHaloFactor = 1 - exp(-equivRadius * equivRadius / (SIGMA_HALO_SQRDX2));

        spot->indexControlPt = numControlPts;
      }

      controlPt->numDepthDoses  = jControlPts[indexControlPt]["DepthPDD"].size();

      if (controlPt->numDepthDoses)
      {
        ++controlPt->numDepthDoses; //  +1 for wrap around

        controlPt->numLETs = jControlPts[indexControlPt]["DepthLET"].size();
        controlPt->indexDepthDoses = numDepthDoses;
        controlPt->indexLETs = numLETs;
        numDepthDoses += controlPt->numDepthDoses;
        numLETs += controlPt->numLETs;

        controlPt->maxDepthDoseDepth = jControlPts[indexControlPt]["DepthPDD"][jControlPts[indexControlPt]["DepthPDD"].size() - 1].asFloat();
        controlPt->depthSpacingDepthDose = (controlPt->maxDepthDoseDepth - jControlPts[indexControlPt]["DepthPDD"][0].asFloat())
          / (jControlPts[indexControlPt]["DepthPDD"].size() - 1);

        *depthDose = 0.0;
        ++depthDose;
        *alpha = 0.0;
        ++alpha;

        float depthDoseVal;
        float doseSum = 0.0;
        float alphaSum = 0.0;

#ifdef DBG_0 // Save depth dose curve to file
        FILE* fd;
        fd = fopen("..\\AppResults\\DepthDose.txt", "w");
#endif
        for (int depthIndex = 1; depthIndex < controlPt->numDepthDoses; ++depthIndex, ++depthDose, ++alpha)
        {
          depthDoseVal = jControlPts[indexControlPt]["DosePDD"][depthIndex].asFloat();

          doseSum += depthDoseVal;

          alphaSum += depthDoseVal * jControlPts[indexControlPt]["alpha"][depthIndex - 1].asFloat();

          *depthDose = doseSum * controlPt->depthSpacingDepthDose;
          *alpha = alphaSum * controlPt->depthSpacingDepthDose;

#ifdef DBG_0
          fprintf(fd, "%f %f %f\n", controlPt->depthSpacingDepthDose/2 + controlPt->depthSpacingDepthDose*(depthIndex - 1), jControlPts[indexControlPt]["DosePDD"][depthIndex - 1].asFloat(), *depthDose);
        }
        fclose(fd);
#else
        }
#endif

        controlPt->r80 = jControlPts[indexControlPt]["R80"].asFloat();

        controlPt->maxDepthLET = jControlPts[indexControlPt]["DepthLET"][jControlPts[indexControlPt]["DepthLET"].size() - 1].asFloat();
        controlPt->depthSpacingLET = (controlPt->maxDepthLET - jControlPts[indexControlPt]["DepthLET"][0].asFloat())
          / (jControlPts[indexControlPt]["DepthLET"].size() - 1);

        for (int indexLETs = 0; indexLETs < controlPt->numLETs; ++indexLETs, ++let)
        {
          *let = jControlPts[indexControlPt]["LETd"][indexLETs].asFloat();
        }

      }

      if (jIndexDevicesBeam.size())
      {

        Json::Value controlPtIndexes = jIndexDevicesBeam[indexControlPt];

        controlPt->numDeviceIndexes = controlPtIndexes.size();

        for (int indexDevIndexes = 0; indexDevIndexes < controlPt->numDeviceIndexes; ++indexDevIndexes)
        {
          *indexDevice = indexDevIndexesBeam + controlPtIndexes[indexDevIndexes].asInt();
          ++indexDevice;
        }

        numDeviceIndexes += controlPt->numDeviceIndexes;
      }

      ++numControlPts;
    } // control point loop

    if (jBeams[indexBeam].isMember("BeamEmittance"))
    {
      beam->emittance = jBeams[indexBeam]["BeamEmittance"].asFloat();
    }
    else
    {
      beam->emittance = 0.7;
    }

#ifdef DBG_0 // Emittance = 0
    beam->emittance = 0.00;
#endif

    if (jBeams[indexBeam].isMember("ShapeSequence"))
    {

      Json::Value  jDevicesBeam = jBeams[indexBeam]["ShapeSequence"];

      int numDevicesBeam = jDevicesBeam.size();

      for (int indexDevice = 0; indexDevice < numDevicesBeam; ++indexDevice, ++device)
      {

        Json::Value jDevice = jDevicesBeam[indexDevice];
        CString material = jDevice["material"].asCString();

        if (material == "LUCITE")
        {
          device->indexMtrl = MTRL_LUCITE;
        }
        else if(material == "COPPER")
        {
          device->indexMtrl = MTRL_COPPER;
        }
        else if (material == "KRYPTO")
        {
          device->indexMtrl = MTRL_KRYPTO;
        }
        else
        {
          AfxMessageBox("Unknown Material");

        }


        device->indexApertures    = 0;
        device->numApertures      = 0;
        device->indexCompensators = 0;
        device->numCompensators   = 0;

        device->zTop       = jDevice["ztop"].asFloat();
        device->zBottom    = jDevice["zbot"].asFloat();
        device->halfRangeX = DEVICE_HALF_WIDTH;
        device->halfRangeY = DEVICE_HALF_WIDTH;

        if (jDevicesBeam[indexDevice].isMember("ApertureSequence"))
        {
          Json::Value  jApertureSequence = jDevicesBeam[indexDevice]["ApertureSequence"];

          device->indexApertures = numApertures;

          device->numApertures = jApertureSequence.size();
#ifdef DBG_0
          device->numApertures = 0;
#endif

          for (int indexAperture = 0; indexAperture < device->numApertures; ++indexAperture, ++aperture)
          {
            Json::Value jBlockData = jApertureSequence[indexAperture]["Data"];

            aperture->indexMtrl = MTRL_AIR;

            aperture->numAperturePts = jBlockData.size() / 2;

            TRACE("Aperture %d %d %d\n", indexBeam, indexAperture, aperture->numAperturePts);

            aperture->xMax = -FLT_MAX;
            aperture->xMin =  FLT_MAX;
            aperture->yMax = -FLT_MAX;
            aperture->yMin =  FLT_MAX;

            for (unsigned int dataIndex = 0; dataIndex < jBlockData.size(); ++dataIndex)
            {
              aperturePt->x = jBlockData[dataIndex].asFloat();
              ++dataIndex;
              aperturePt->y = jBlockData[dataIndex].asFloat();

#ifdef DBG_0
              aperturePt->x /= 2.0;
              aperturePt->y /= 2.0;
#endif

              if (aperturePt->x > aperture->xMax)
              {
                aperture->xMax = aperturePt->x;
              }
              if (aperturePt->x < aperture->xMin)
              {
                aperture->xMin = aperturePt->x;
              }
              if (aperturePt->y > aperture->yMax)
              {
                aperture->yMax = aperturePt->y;
              }
              if (aperturePt->y < aperture->yMin)
              {
                aperture->yMin = aperturePt->y;
              }

              ++aperturePt;
            }

            // Set last point to first point for interior algorithm
            aperturePt->x = jBlockData[0].asFloat();
            aperturePt->y = jBlockData[1].asFloat();
            ++aperturePt;

            aperture->indexAperturePts = numAperturePts;
            numAperturePts += aperture->numAperturePts + 1;  // + 1 because want last point to close with first point for interior algorithm
          }

          numApertures += device->numApertures;

        } // if aperture sequence

        if (jDevicesBeam[indexDevice].isMember("RangeCompensatorSequence"))
        {
          Json::Value  jCompensatorSequence = jDevicesBeam[indexDevice]["RangeCompensatorSequence"];

          device->indexCompensators = numCompensators;

          device->numCompensators = jCompensatorSequence.size();
#ifdef DBG_0
          device->numCompensators = 0;
#endif

          for (int indexCompensator = 0; indexCompensator < device->numCompensators; ++indexCompensator, ++compensator)
          {
            compensator->numRows    = jCompensatorSequence[indexCompensator]["CompensatorRows"].asInt();
            compensator->numColumns = jCompensatorSequence[indexCompensator]["CompensatorColumns"].asInt();

            compensator->pixSizeX = jCompensatorSequence[indexCompensator]["CompensatorPixelSpacing"][0].asFloat();
            compensator->pixSizeY = jCompensatorSequence[indexCompensator]["CompensatorPixelSpacing"][1].asFloat();

            compensator->numPixels = jCompensatorSequence[indexCompensator]["CompensatorThicknessData"].size();

            TRACE("Compensator %d %d %d\n", indexBeam, indexCompensator, compensator->numPixels);

            // Compensator position is top left (xMin, yMax)
            compensator->xMin = jCompensatorSequence[indexCompensator]["CompensatorPosition"][0].asFloat();
            compensator->yMax = jCompensatorSequence[indexCompensator]["CompensatorPosition"][1].asFloat();
            compensator->xMax = compensator->xMin + compensator->pixSizeX * compensator->numColumns;
            compensator->yMin = compensator->yMax - compensator->pixSizeY * compensator->numRows;


            Json::Value jDepthData = jCompensatorSequence[indexCompensator]["CompensatorThicknessData"];

            for (unsigned int depthIndex = 0; depthIndex < compensator->numPixels; ++depthIndex)
            {
              *compensatorDepth = jDepthData[depthIndex].asFloat();

              ++compensatorDepth;
            }

            compensator->indexPixelDepths = numCompensatorDepths;
            numCompensatorDepths += compensator->numPixels;

          }

          numCompensators += device->numCompensators;

        } // if compensator sequence

        if (jDevicesBeam[indexDevice].isMember("CompensatorThicknessData"))
        {
          //  Json::Value  depthData = jDevicesBeam[indexDevice]["CompensatorThicknessData"];
          //  numCompensatorDepths += depthData.size();

          //  ++numCompensators;
          //}
          Json::Value  jCompensatorSequence = jDevicesBeam[indexDevice]["RangeCompensatorSequence"];

          device->indexCompensators = numCompensators;

          device->numCompensators = 1;
#ifdef DBG_0
          device->numCompensators = 0;
#endif
          //if (transformControlPt)
          //{
          //  compensator->numRows    = jDevicesBeam[indexDevice]["CompensatorRows"].asInt();
          //  compensator->numColumns = jDevicesBeam[indexDevice]["CompensatorColumns"].asInt();
          //}
          //else
          //{
          //  compensator->numColumns = jDevicesBeam[indexDevice]["CompensatorRows"].asInt();
          //  compensator->numRows    = jDevicesBeam[indexDevice]["CompensatorColumns"].asInt();
          //}

          compensator->numRows = jDevicesBeam[indexDevice]["CompensatorRows"].asInt();
          compensator->numColumns = jDevicesBeam[indexDevice]["CompensatorColumns"].asInt();

          compensator->pixSizeX = jDevicesBeam[indexDevice]["CompensatorPixelSpacing"][0].asFloat();
          compensator->pixSizeY = jDevicesBeam[indexDevice]["CompensatorPixelSpacing"][1].asFloat();

          Json::Value jDepthData = jDevicesBeam[indexDevice]["CompensatorThicknessData"];
          compensator->numPixels = jDepthData.size();

          TRACE("Compensator %d %d\n", indexBeam, compensator->numPixels);

          // Compensator position is top left (xMin, yMax)
          compensator->xMin = jDevicesBeam[indexDevice]["CompensatorPosition"][0].asFloat();
          compensator->yMax = jDevicesBeam[indexDevice]["CompensatorPosition"][1].asFloat();
          compensator->xMax = compensator->xMin + compensator->pixSizeX * compensator->numColumns;
          compensator->yMin = compensator->yMax - compensator->pixSizeY * compensator->numRows;

          for (unsigned int depthIndex = 0; depthIndex < compensator->numPixels; ++depthIndex)
          {
            *compensatorDepth = jDepthData[depthIndex].asFloat();

            ++compensatorDepth;
          }

#ifdef DBG_0
          CString fileName;
          fileName.Format("%s\\AppResults\\Compensator.txt", g_configInstance->m_directoryBase);
          FILE* fd = fopen(fileName, "w");
          float* depth = compensatorDepth - compensator->numPixels;
          for (int y = compensator->numRows - 1; y >= 0; --y)
          {
            for (int x = 0; x < compensator->numColumns; ++x)
            {
              fprintf(fd, "%6.2f ", *(depth + y * compensator->numColumns + x));
              //++depth;
            }
            fprintf(fd, "\n");
          }
          fclose(fd);
#endif

#ifdef DBG_0
          CString fileName;
          fileName.Format("%s\\AppResults\\CompensatorModel.txt", g_configInstance->m_directoryBase);
          FILE* fd = fopen(fileName, "w");
          for (int y = 0; y < compensator->numRows/2; ++y)
          {
            for (int x = 0; x < compensator->numColumns; ++x)
            {
              //fprintf(fd, "%6.2f,\n", 50.0);
              fprintf(fd, "%6.2f ", 100.0);

            }
            fprintf(fd, "\n");
          }
          for (int y = compensator->numRows / 2; y < compensator->numRows; ++y)
          {
            for (int x = 0; x < compensator->numColumns /2; ++x)
            {
              fprintf(fd, "%6.2f ", 50.0);
            }
            for (int x = compensator->numColumns / 2; x < compensator->numColumns; ++x)
            {
              fprintf(fd, "%6.2f ", 10.0);
            }
            fprintf(fd, "\n");
          }
          fclose(fd);
#endif
          compensator->indexPixelDepths = numCompensatorDepths;
          numCompensatorDepths += compensator->numPixels;

          ++numCompensators;
          ++compensator;

        } // if compensator sequence


      } // device loop

      numDevices += numDevicesBeam;

    }  // if shape sequence

  } // beam loop

  Json::Value jFractions = (*root)["FractionGroupSequence"];

  unsigned int numFractionGroups = jFractions.size();
  unsigned int numFractionBeams  = 0;

  SFractionGroup* planFraction = m_arrayFractionGroups(0);
  int* fractionBeam = m_arrayFractionBeams(0);
  for (unsigned int fractionIndex = 0; fractionIndex < numFractionGroups; ++fractionIndex)
  {
    planFraction->fractionNum         = jFractions[fractionIndex]["FractionGroupNumber"].asInt();
    if (jFractions[fractionIndex].isMember("FractionGroupDescription"))
    {
      planFraction->description       = jFractions[fractionIndex]["FractionGroupDescription"].asCString(); 
    }
            
    planFraction->numFractionsPlanned = jFractions[fractionIndex]["NumberOfFractionsPlanned"].asInt();
    planFraction->numBeams            = jFractions[fractionIndex]["NumberOfBeams"].asInt();

#ifdef DBG_BEAM_LIMIT
    if (planFraction->numBeams > m_arrayBeams.m_numItems)
    {
      planFraction->numBeams = m_arrayBeams.m_numItems;
    }
#endif

#ifdef DBG_0
    planFraction->numFractionsPlanned = 1;
#endif
   
    Json::Value fractionBeams = jFractions[fractionIndex]["ReferencedBeamSequence"];

    int fractionBeamNum;
    for (int fractionBeamIndex = 0; fractionBeamIndex < planFraction->numBeams; ++fractionBeamIndex)
    {
      fractionBeamNum = fractionBeams[fractionBeamIndex].asInt();
      *fractionBeam = fractionBeamNum;
      ++fractionBeam;

      for (unsigned int indexBeam = 0; indexBeam < m_arrayBeams.m_numItems; ++indexBeam)
      {
        if (m_arrayMetaBeams[indexBeam].beamNumber == fractionBeamNum)
        {
          m_arrayMetaBeams[indexBeam].fractionIndex = fractionIndex;
          m_arrayBeams[indexBeam].numFractions += planFraction->numFractionsPlanned;

          break;
        }
      }
    }

    if (jFractions[fractionIndex].isMember("ReferencedBeamDoseSequence"))
    {
      Json::Value beamDoses = jFractions[fractionIndex]["ReferencedBeamDoseSequence"];

      for (int indexBeam = 0; indexBeam < planFraction->numBeams; ++indexBeam)
      {
        m_arrayFractionBeamDoses[indexBeam] = beamDoses[indexBeam].asFloat();
      }

    }


    //if (jFractions[fractionIndex].isMember("ReferencedBeamDoseSequence"))
    //{
    //  Json::Value beamDoses = jFractions[fractionIndex]["ReferencedBeamDoseSequence"];

    //  if (beamDoses[0].isValidIndex(0)) // New json file format to support ReferencedBeamDoseSequence
    //  {
    //    for (int indexBeam = 0; indexBeam < planFraction->numBeams; ++indexBeam)
    //    {
    //      m_arrayFractionBeamDoses[indexBeam] = beamDoses[indexBeam]["BeamDose"].asFloat();
    //    }
    //  }
    //  else // First json file format to support ReferencedBeamDoseSequence
    //  {
    //    for (int indexBeam = 0; indexBeam < planFraction->numBeams; ++indexBeam)
    //    {
    //      m_arrayFractionBeamDoses[indexBeam] = beamDoses[indexBeam].asFloat();
    //    }
    //  }
    //}

    m_numFractions += planFraction->numFractionsPlanned;

    planFraction->indexFractionBeam = numFractionBeams;
    numFractionBeams += planFraction->numBeams;

    ++planFraction;
  }

  return;
}

void CIonPlan::CalcControlPtMatrix(SControlParseParams* params)
{
  float azAngle;
  float decAngle;
  float cosAz;
  float sinAz;
  float cosDec;
  float sinDec;

  azAngle  = params->supportAngle * DEG_TO_RAD;
  decAngle = params->gantryAngle * DEG_TO_RAD;

  cosAz  =  cos(azAngle);
  sinAz  = -sin(azAngle);
  cosDec =  cos(decAngle);
  sinDec =  sin(decAngle);

  params->beamMatrix._11 =  cosDec * cosAz;
  params->beamMatrix._12 = -sinAz;
  params->beamMatrix._13 =  sinDec * cosAz;

  params->beamMatrix._21 =  cosDec * sinAz;
  params->beamMatrix._22 =  cosAz;
  params->beamMatrix._23 =  sinDec * sinAz;

  params->beamMatrix._31 = -sinDec;
  params->beamMatrix._32 =  0.0;
  params->beamMatrix._33 =  cosDec;

  return;
}

bool CIonPlan::ParsePlanDICOM(LPCTSTR planFile)
{
  OFString deliveryType;
  DcmFileFormat fileformat;

  OFCondition status = fileformat.loadFile(planFile);

  if (!status.good())
  {
    return false;
  }

  DRTIonPlanIOD dicomPlan;

  status = dicomPlan.read(*fileformat.getDataset());

  if (!status.good())
  {
    return false;
  }

  OFString patientsName;
  status = dicomPlan.getPatientName(patientsName);

  if (!status.good())
  {
    return false;
  }

  CountPlanObjectsDicom(dicomPlan);

  //AllocateMemory();

  FillPlanDicom(dicomPlan);

  DRTFractionGroupSequence& fractionSequence = dicomPlan.getFractionGroupSequence();
  fractionSequence.gotoFirstItem();

  unsigned int numFractionGroups = fractionSequence.getNumberOfItems();
  m_arrayFractionGroups.Alloc(numFractionGroups);

  forArray(m_arrayFractionGroups, planFraction)
  {
    DRTFractionGroupSequence::Item& fractionItem = fractionSequence.getCurrentItem();

    fractionItem.getFractionGroupNumber(planFraction->fractionNum);
    fractionItem.getNumberOfFractionsPlanned(planFraction->numFractionsPlanned);
    fractionItem.getNumberOfBeams(planFraction->numBeams);

    fractionItem.getReferencedBeamSequence();

    DRTReferencedBeamSequenceInRTFractionSchemeModule& beamSequence = fractionItem.getReferencedBeamSequence();
    beamSequence.gotoFirstItem();

    
    int fractionBeamNum;
    for (int indexFractionBeam = 0; indexFractionBeam < planFraction->numBeams; ++indexFractionBeam)
    {
      DRTReferencedBeamSequenceInRTFractionSchemeModule::Item & fractionBeam = beamSequence.getCurrentItem();
      fractionBeam.getReferencedBeamNumber(fractionBeamNum);

      for (unsigned int indexBeam = 0; indexBeam < m_arrayBeams.m_numItems; ++indexBeam)
      {
        if (m_arrayMetaBeams[indexBeam].beamNumber == fractionBeamNum)
        {
          m_arrayMetaBeams[indexBeam].fractionIndex = indexFractionBeam;
          m_arrayBeams[indexBeam].numFractions += planFraction->numFractionsPlanned;
          break;
        }
      }
      
      beamSequence.gotoNextItem();
    }

    m_numFractions += planFraction->numFractionsPlanned;

    ++planFraction;
    fractionSequence.gotoNextItem();
  }

  return true;
}

bool CIonPlan::CountPlanObjectsDicom(DRTIonPlanIOD& plan)
{
  OFString deliveryType;
  int numBlockPts;
  unsigned int numBeams       = 0;
  unsigned int numControlPts  = 0;
  unsigned int numSpots       = 0;
  unsigned int numDevices     = 0;
  unsigned int numAperturePts = 0;
  unsigned int numApertures   = 0;

  DRTIonBeamSequence& beamSequence = plan.getIonBeamSequence();

  // Iterate over Beam Sequence Items;
  beamSequence.gotoFirstItem();
  while (beamSequence.getCurrentItem().isValid())
  {

    DRTIonBeamSequence::Item& beamItem = beamSequence.getCurrentItem();

    beamItem.getTreatmentDeliveryType(deliveryType);

    if (deliveryType != "TREATMENT")
    {
      beamSequence.gotoNextItem();
      continue;
    }

    ++numBeams;

    // Range Shifters
    DRTIonRangeCompensatorSequence& rangeShifterSequence = beamItem.getIonRangeCompensatorSequence();

    // Blocks
    DRTIonBlockSequence& blockSequence = beamItem.getIonBlockSequence();

    while (blockSequence.getCurrentItem().isValid())
    {
      DRTIonBlockSequence::Item& blockItem = blockSequence.getCurrentItem();

      blockItem.getBlockNumberOfPoints(numBlockPts);
      numAperturePts += (numBlockPts + 1);

      numApertures += 1;

      blockSequence.gotoNextItem();
    }

    numDevices += rangeShifterSequence.getNumberOfItems() + blockSequence.getNumberOfItems();

    DRTIonControlPointSequence& controlPtSequence = beamItem.getIonControlPointSequence();
    controlPtSequence.gotoFirstItem();

    int numControlPts = controlPtSequence.getNumberOfItems();

    DRTIonControlPointSequence::Item* currentItem;
    int currentNum;
    double currentWeight;
    double nextWeight;

    controlPtSequence.getCurrentItem(currentItem);
    currentItem->getNumberOfScanSpotPositions(currentNum);
    currentItem->getCumulativeMetersetWeight(currentWeight);

    for(int index = 0; index < numControlPts - 1; ++index)
    {
      controlPtSequence.gotoNextItem();
      controlPtSequence.getCurrentItem(currentItem);

      currentItem->getCumulativeMetersetWeight(nextWeight);

      if (currentWeight != nextWeight)
      {
        ++numControlPts;
        currentItem->getNumberOfScanSpotPositions(currentNum);
        numSpots += currentNum;
      }

      currentWeight = nextWeight;

    }

    beamItem.getFinalCumulativeMetersetWeight(nextWeight);
    if (currentWeight != nextWeight)
    {
      ++numControlPts;
      currentItem->getNumberOfScanSpotPositions(currentNum);
      numSpots += currentNum;
    }

    beamSequence.gotoNextItem();
  }

  m_arrayBeams.Alloc(numBeams);
  m_arrayMetaBeams.Alloc(numBeams);

  return true;
}

void CIonPlan::FillPlanDicom(DRTIonPlanIOD& plan) // MLW update for generalized sites
{

  Float64 value;
  OFString deliveryType;
  OFString beamName;
  SControlParseParams controlParams;
  unsigned int seed = m_arraySpots.m_numItems;

  SShaderControlPt* controlPt = m_arrayControlPts(0);
  SShaderSpot* spot           = m_arraySpots(0);
  SShaderDevice* device       = m_arrayDevices(0);
  SShaderAperture* aperture   = m_arrayApertures(0);
  SPt2* aperturePt            = m_arrayAperturePts(0);
  float* depthDose            = m_arrayDepthDoses(0);
  float* alpha                = m_arrayAlphas(0);
  float* let                  = m_arrayLETs(0);

  // Zero out and count again so these can be used as object indexes
  unsigned int numDevices       = 0;
  unsigned int numAperturePts   = 0;
  unsigned int numApertures     = 0;
  unsigned int numDepthDoses    = 0;
  unsigned int numLETs          = 0;

  unsigned int numBeams         = 0;
  unsigned int numControlPts    = 0;
  unsigned int numSpots         = 0;

  m_numProtonsPlan = 0.0;

  // MLW todo: get patient name, id, study date, desc, etc.

  DRTIonBeamSequence& beamSequence = plan.getIonBeamSequence();

  beamSequence.gotoFirstItem();

  //for (int indexBeam = 0; indexBeam < m_numBeams; ++indexBeam, ++beam)
  forArrayI(m_arrayBeams, beam, indexBeam)
  {

    DRTIonBeamSequence::Item& beamItem = beamSequence.getCurrentItem();

    beamItem.getBeamNumber(m_arrayMetaBeams[indexBeam].beamNumber);

    beamItem.getBeamName(beamName);
    m_arrayMetaBeams[indexBeam].beamName = beamName.c_str();

    beamItem.getTreatmentDeliveryType(deliveryType);

    if (deliveryType != "TREATMENT")
    {
      beamSequence.gotoNextItem();
      continue;
    }

    controlParams.gantryAngle  = 0.0;
    controlParams.supportAngle = 0.0;
    controlParams.isocenter[0] = 0.0;
    controlParams.isocenter[1] = 0.0;
    controlParams.isocenter[2] = 0.0;
    XMStoreFloat3x3(&controlParams.beamMatrix, DirectX::XMMatrixIdentity());
    controlParams.angleChange = false;

    for (int sadIndex = 0; sadIndex < 2; ++sadIndex)
    {
      beamItem.getVirtualSourceAxisDistances(beam->sad[sadIndex], sadIndex);
    }

    if (beam->sad[0] < beam->sad[1])
    {
      beam->protonInitZ = beam->sad[0];
      beam->sadMaxAxis  = 1;
      beam->sadScale    = (beam->sad[1] - beam->protonInitZ) / beam->sad[1];
    }
    else
    {
      beam->protonInitZ = beam->sad[1];
      beam->sadMaxAxis  = 0;
      beam->sadScale    = (beam->sad[0] - beam->protonInitZ) / beam->sad[0];
    }

    // Blocks
    DRTIonBlockSequence& blockSequence = beamItem.getIonBlockSequence();
    blockSequence.gotoFirstItem();
    int numBlocks = blockSequence.getNumberOfItems();
    beam->indexDevices = numDevices;

    while (blockSequence.getCurrentItem().isValid())
    {
      DRTIonBlockSequence::Item& blockItem = blockSequence.getCurrentItem();  

      double blockThickness;
      float isocenterBlockDistance;
      blockItem.getBlockThickness(blockThickness);
      blockItem.getIsocenterToBlockTrayDistance(isocenterBlockDistance);

      device->numApertures = 1; // MLW
      device->indexApertures = numApertures;

      device->indexMtrl  = MTRL_COPPER; // MLW
      device->zTop       = isocenterBlockDistance + (float)blockThickness;
      device->zBottom    = isocenterBlockDistance;
      device->halfRangeX = DEVICE_HALF_WIDTH;
      device->halfRangeY = DEVICE_HALF_WIDTH;

      for (int ap = 0; ap < device->numApertures; ++ap, ++aperture)
      {
        Sint32 numApPts;
        blockItem.getBlockNumberOfPoints(numApPts);

        OFVector<Float64> apData(numApPts);
        blockItem.getBlockData(apData);

        aperture->indexMtrl      = MTRL_AIR;
        aperture->numAperturePts = numApPts / 2;

        aperture->xMax = -FLT_MAX;
        aperture->xMin =  FLT_MAX;
        aperture->yMax = -FLT_MAX;
        aperture->yMin =  FLT_MAX;

        for (int dataIndex = 0; dataIndex < aperture->numAperturePts; ++dataIndex)
        {
          aperturePt->x = apData[dataIndex];
          ++dataIndex;
          aperturePt->y = apData[dataIndex];

          if (aperturePt->x > aperture->xMax)
          {
            aperture->xMax = aperturePt->x;
          }
          if (aperturePt->x < aperture->xMin)
          {
            aperture->xMin = aperturePt->x;
          }
          if (aperturePt->y > aperture->yMax)
          {
            aperture->yMax = aperturePt->y;
          }
          if (aperturePt->y < aperture->yMin)
          {
            aperture->yMin = aperturePt->y;
          }

          ++aperturePt;
        }

        // Set last point to first point for interior algorithm
        aperturePt->x = (float) apData[0];
        aperturePt->y = (float) apData[1];
        ++aperturePt;

        aperture->indexAperturePts = numAperturePts;
        numAperturePts += aperture->numAperturePts + 1;
      }

      numApertures += device->numApertures;

      blockSequence.gotoNextItem();
    }
    beam->numDevices = numBlocks;
    numDevices += numBlocks;

    // Range Shifters
    DRTIonRangeCompensatorSequence& rangeShifterSequence = beamItem.getIonRangeCompensatorSequence();
    rangeShifterSequence.gotoFirstItem();

    while (rangeShifterSequence.getCurrentItem().isValid())
    {

      rangeShifterSequence.gotoFirstItem();
      int numShifters = rangeShifterSequence.getNumberOfItems();

      if (numShifters)
      {

        for (int indexShifters = 0; indexShifters < numShifters; ++indexShifters, ++device)
        {
          DRTIonRangeCompensatorSequence::Item& shifterItem = rangeShifterSequence.getCurrentItem();

          shifterItem.getIsocenterToCompensatorDistances(device->zBottom);
          shifterItem.getCompensatorThicknessData(value);
          device->zTop = device->zBottom + (float) value;

          device->indexMtrl = MTRL_LUCITE; // MLW check

          device->numApertures = 0;
          device->indexApertures = 0;
          device->halfRangeX = DEVICE_HALF_WIDTH;
          device->halfRangeY = DEVICE_HALF_WIDTH;

          rangeShifterSequence.gotoNextItem();
        }

        numDevices += numShifters;
      }

    }

    DRTIonControlPointSequence& controlPtSequence = beamItem.getIonControlPointSequence();
    controlPtSequence.gotoFirstItem();

    beam->numControlPts   = controlPtSequence.getNumberOfItems();
    beam->indexControlPts = numControlPts;
    beam->indexSpots      = numSpots;

    DRTIonControlPointSequence::Item* currentItem;
    int currentNum;
    double currentWeight;
    double nextWeight;

    controlPtSequence.getCurrentItem(currentItem);
    currentItem->getNumberOfScanSpotPositions(currentNum);
    currentItem->getCumulativeMetersetWeight(currentWeight);
    int numSpotsCntrlPt;
    for(int index = 0; index < beam->numControlPts - 1; ++index)
    {
      controlPtSequence.gotoNextItem();
      controlPtSequence.getCurrentItem(currentItem);

      currentItem->getCumulativeMetersetWeight(nextWeight);

      if (currentWeight != nextWeight)
      {
        controlPt->indexBeam = indexBeam;
        numSpotsCntrlPt = FillControlPtDicom(currentItem, controlPt, &controlParams, spot, numControlPts);
        controlPt->indexSpots = numSpots;
        spot += numSpotsCntrlPt;
        numSpots += numSpotsCntrlPt;
        beam->numSpots += controlPt->numSpots;
      }

      currentWeight = nextWeight;
    }

    beamItem.getFinalCumulativeMetersetWeight(nextWeight);
    if (currentWeight != nextWeight)
    {
      controlPt->indexBeam = indexBeam;
      numSpotsCntrlPt = FillControlPtDicom(currentItem, controlPt, &controlParams, spot, numControlPts); // MLW check the num =
      controlPt->indexSpots = numSpots;
      spot += numSpotsCntrlPt;
      numSpots += numSpotsCntrlPt;
      beam->numSpots += controlPt->numSpots;
    }

    controlPtSequence.gotoNextItem();

  } // beam loop

  return;
}

unsigned int CIonPlan::FillControlPtDicom(DRTIonControlPointSequence::Item* controlPtItem, SShaderControlPt* cntrlPt,
    SControlParseParams* controlParams, SShaderSpot* spot, unsigned int cntrlPtIndex)
{

  double value;

  //controlPtItem->getNominalBeamEnergy(value);
  //cntrlPt->energy = value;

  controlPtItem->getScanningSpotSize(cntrlPt->spotSize[0], 0);
  controlPtItem->getScanningSpotSize(cntrlPt->spotSize[1], 1);

  //DcmTagKey element(0x300A, 0x011E);
  OFCondition result;
  result = controlPtItem->getGantryAngle(value);
  if (result.good())
  {
    controlParams->gantryAngle  = (float) value;
    controlParams->angleChange = true;
  }

  result = controlPtItem->getPatientSupportAngle(value);
  if (result.good())
  {
    controlParams->supportAngle = (float) value;
    controlParams->angleChange = true;
  }

  if (controlParams->angleChange)
  {
    CalcControlPtMatrix(controlParams);

    controlParams->angleChange = false;
  }

  result = controlPtItem->getIsocenterPosition(value, 0);
  if (result.good())
  {
    controlParams->isocenter[0] = (float) value;
    controlPtItem->getIsocenterPosition(value, 1);
    controlParams->isocenter[1] = (float) value;
    controlPtItem->getIsocenterPosition(value, 2);
    controlParams->isocenter[2] = (float) value;
  }

  cntrlPt->beamMatrix = controlParams->beamMatrix;

  cntrlPt->isocenter[0] = controlParams->isocenter[0];
  cntrlPt->isocenter[1] = controlParams->isocenter[1];
  cntrlPt->isocenter[2] = controlParams->isocenter[2];

  controlPtItem->getNumberOfScanSpotPositions(cntrlPt->numSpots);
    
  int numSpots;
  controlPtItem->getNumberOfScanSpotPositions(numSpots);

  for (int ptIndex = 0; ptIndex < 2*numSpots; ++ptIndex, ++spot)
  {
    controlPtItem->getScanSpotPositionMap(spot->spotCenterX, ptIndex);
    ++ptIndex;
    controlPtItem->getScanSpotPositionMap(spot->spotCenterY, ptIndex);

    controlPtItem->getScanSpotMetersetWeights(spot->weight, ptIndex / 2);

    m_numProtonsPlan += spot->weight;

    spot->indexControlPt = cntrlPtIndex;
  }

  return numSpots;
}

void CIonPlan::SaveEnergies(Json::Value* root)
{
  int indexPlanPt;
  float beamEnergy;
  Json::Value jBeams;

  jBeams = (*root)["IonBeamSequence"];

  CString fileName;
  fileName.Format("%s\\AppResults\\Energies.txt", g_configInstance->m_directoryBase);
  FILE* fd = fopen(fileName, "w");
  fprintf(fd, "0.0 0.0\n");

  forArrayI(m_arrayBeams, beam, indexBeam)
  {

    Json::Value jControlPts = jBeams[indexBeam]["IonControlPointSequence"];

    for (int indexControlPt = 0; indexControlPt < beam->numControlPts; ++indexControlPt)
    {
      beamEnergy = jControlPts[indexControlPt]["NominalBeamEnergy"].asFloat();
      indexPlanPt = beam->indexControlPts + indexControlPt;
      SShaderControlPt* controlPt = m_arrayControlPts(indexPlanPt);

      fprintf(fd, "%f %f %f\n", beamEnergy, m_arrayDepthDoses[controlPt->indexDepthDoses + controlPt->numDepthDoses - 1], beamEnergy * 160.021);
    }
  }

  fclose(fd);

  return;
}

bool CIonPlan::ParseGrids(Json::Value* root)
{

  Json::Value jVoxelGrid;

  jVoxelGrid = (*root)["GMCGrid"];

  Json::Value value;

  value = jVoxelGrid["Nxyz"];

  m_voxGridRSP.dims[0] = value[0].asInt();
  m_voxGridRSP.dims[1] = value[1].asInt();
  m_voxGridRSP.dims[2] = value[2].asInt();

  value = jVoxelGrid["TopLeftBottomCorner"];

  m_voxGridRSP.voxelOrigin[0] = value[0].asFloat();
  m_voxGridRSP.voxelOrigin[1] = value[1].asFloat();
  m_voxGridRSP.voxelOrigin[2] = value[2].asFloat();

  value = jVoxelGrid["Directions"];

  if (value[0].asFloat() < 0.0) // MLw todo: check, expand
  {
    m_flagProne = true;
  }

  value = jVoxelGrid["Deltas"];

  m_voxGridRSP.voxelSize[0] = value[0].asFloat();
  m_voxGridRSP.voxelSize[1] = value[1].asFloat();
  m_voxGridRSP.voxelSize[2] = value[2].asFloat();

  if (m_flagProne)
  {
    //m_voxGridRSP.voxelOrigin[0] -= ((m_voxGridRSP.dims[0] - 1) * m_voxGridRSP.voxelSize[0]);
    //m_voxGridRSP.voxelOrigin[1] -= ((m_voxGridRSP.dims[1] - 1) * m_voxGridRSP.voxelSize[1]);
    m_voxGridRSP.unitVecs[0][0] = -1.0;
    m_voxGridRSP.unitVecs[1][1] = -1.0;
    m_voxGridHU.unitVecs[0][0]  = -1.0;
    m_voxGridHU.unitVecs[1][1]  = -1.0;
  }

  m_voxGridRSP.numVoxels   = m_voxGridRSP.dims[0] * m_voxGridRSP.dims[1] * m_voxGridRSP.dims[2];
  m_voxGridRSP.voxelVolume = m_voxGridRSP.voxelSize[0] * m_voxGridRSP.voxelSize[1] * m_voxGridRSP.voxelSize[2];

  if (jVoxelGrid.isMember("FrameOfReferenceUID"))
  {
    m_frameReferenceUID = jVoxelGrid["FrameOfReferenceUID"].asCString();
  }

  value = jVoxelGrid["ImageStack"];
  m_blobName = value.asCString();

  if (jVoxelGrid.isMember("RSPScale"))
  {
    value = jVoxelGrid["RSPScale"];
    m_rspScale = value.asFloat();
  }
  else
  {
    m_rspScale = RSP_DATA_SCALE;
  }

  if (jVoxelGrid.isMember("RSPType")) // MLW enumerate
  {
    m_rspType = 1;
  }
  else
  {
    m_rspType = 0;
  }

  //jVoxelGrid = (*root)["DoseGrid"];

  //value = jVoxelGrid["Nxyz"];

  //m_voxGridDoseExternal.dims[0] = value[0].asInt();
  //m_voxGridDoseExternal.dims[1] = value[1].asInt();
  //m_voxGridDoseExternal.dims[2] = value[2].asInt();

  //value = jVoxelGrid["TopLeftBottomCorner"];

  //m_voxGridDoseExternal.voxelOrigin[0] = value[0].asFloat();
  //m_voxGridDoseExternal.voxelOrigin[1] = value[1].asFloat();
  //m_voxGridDoseExternal.voxelOrigin[2] = value[2].asFloat();

  //value = jVoxelGrid["Deltas"];

  //m_voxGridDoseExternal.voxelSize[0] = value[0].asFloat();
  //m_voxGridDoseExternal.voxelSize[1] = value[1].asFloat();
  //m_voxGridDoseExternal.voxelSize[2] = value[2].asFloat();

  //m_voxGridDoseExternal.numVoxels   = m_voxGridDoseExternal.dims[0] * m_voxGridDoseExternal.dims[1] * m_voxGridDoseExternal.dims[2];
  //m_voxGridDoseExternal.voxelVolume = m_voxGridDoseExternal.voxelSize[0] * m_voxGridDoseExternal.voxelSize[1] * m_voxGridDoseExternal.voxelSize[2];

  return true;
}

bool CIonPlan::ParseBlobs(LPCTSTR blobPath)
{
  char c;
  char integer[8];
  char* cPtr;
  int blobDims[3];

  // Remove any path from json blob name
  int index = m_blobName.ReverseFind('/');
  if (index >= 0)
  {
    m_blobName = m_blobName.Right(m_blobName.GetLength() - index - 1);
  }

  // Create local blob name from local path and pure blob name
  CString blobName;
  blobName.Format("%s\\%s", blobPath, m_blobName);

  FILE* fd = fopen(blobName, "rb");

  if (fd == NULL)
  {
    return false;
  }

  while (fgetc(fd) != '(')
  {
  }

  cPtr = integer;
  while ((c = fgetc(fd)) != ',')
  {
    *cPtr = c;
    ++cPtr;
  }
  *cPtr = NULL;
  blobDims[2] = atoi(integer);

  cPtr = integer;
  while ((c = fgetc(fd)) != ',')
  {
    *cPtr = c;
    ++cPtr;
  }
  *cPtr = NULL;
  blobDims[1] = atoi(integer);

  cPtr = integer;
  while ((c = fgetc(fd)) != ')')
  {
    *cPtr = c;
    ++cPtr;
  }
  *cPtr = NULL;
  blobDims[0] = atoi(integer);

  // Blobs should be same size as GMC calc grid in JSON file
  if (memcmp(blobDims, m_voxGridRSP.dims, 3 * sizeof(int)))
  {
    return false;
  }

  // Since JSON file copy calc info into image info (have checked sizes match) 
  memcpy(&m_voxGridHU, &m_voxGridRSP, sizeof(SVoxGridGeometry));

  m_voxGridHU.data = new float[m_voxGridHU.numVoxels];

  short* fileData = new short[m_voxGridHU.numVoxels];

  fseek(fd, 128, SEEK_SET);

  fread(fileData, sizeof(short), m_voxGridHU.numVoxels, fd);

  float* fPtr = m_voxGridHU.data;
  short* fileEnd = fileData + m_voxGridHU.numVoxels;

  float min = ((float)(*fileData));
  float max = ((float)(*fileData));

  for (short* dataPtr = fileData; dataPtr < fileEnd; ++dataPtr, ++fPtr)
  {
    *fPtr = ((float)(*dataPtr));
    if (*fPtr > max)
    {
      max = *fPtr;
    }
    else if (*fPtr < min)
    {
      min = *fPtr;
    }
  }

  TRACE("HU min/max %5.2f %5.2f\n", min, max);
  fclose(fd);

  m_voxGridHU.bounds[0] = min;
  m_voxGridHU.bounds[1] = max;

  delete[] fileData;

  blobName.Replace("HU", "RSP");

  fd = fopen(blobName, "rb");

  while (fgetc(fd) != '(')
  {
  }

  cPtr = integer;
  while ((c = fgetc(fd)) != ',')
  {
    *cPtr = c;
    ++cPtr;
  }
  *cPtr = NULL;
  blobDims[2] = atoi(integer);

  cPtr = integer;
  while ((c = fgetc(fd)) != ',')
  {
    *cPtr = c;
    ++cPtr;
  }
  *cPtr = NULL;
  blobDims[1] = atoi(integer);

  cPtr = integer;
  while ((c = fgetc(fd)) != ')')
  {
    *cPtr = c;
    ++cPtr;
  }
  *cPtr = NULL;
  blobDims[0] = atoi(integer);

  // Blobs should be same size as GMC calc grid in JSON file
  if (memcmp(blobDims, m_voxGridRSP.dims, 3 * sizeof(int)))
  {
    return false;
  }

  m_voxGridRSP.data = new float[m_voxGridRSP.numVoxels];

  fseek(fd, 128, SEEK_SET);

  if (m_rspType)
  {
    ReadUShort(fd, &m_voxGridRSP, m_rspScale);
  }
  else
  {
    ReadShort(fd, &m_voxGridRSP, m_rspScale);
  }

  fclose(fd);

#ifdef DBG_0
  CalcImageBounds(&m_voxGridRSP);
#endif

  return true;
}


void CIonPlan::ReadShort(FILE* fd, SDataGrid* grid, float scale)
{

  short* fileData = new short[grid->numVoxels];

  fread(fileData, sizeof(short), grid->numVoxels, fd);

  float min;
  float max;

  min = ((float)*fileData) / scale;
  max = ((float)*fileData) / scale;

  float* fPtr = grid->data;
  short* fileEnd = fileData + grid->numVoxels;

  for (short* dataPtr = fileData; dataPtr < fileEnd; ++dataPtr, ++fPtr)
  {

    *fPtr = ((float)(*dataPtr)) / scale;

    if (*fPtr > max)
    {
      max = *fPtr;
    }
    else if (*fPtr < min)
    {
      min = *fPtr;
    }
  }

  grid->bounds[1] = max;
  grid->bounds[0] = min;

  TRACE("RSPS min/max %5.2f %5.2f\n", min, max);

  delete[] fileData;
}

void CIonPlan::ReadUShort(FILE* fd, SDataGrid* grid, float scale)
{

  unsigned short* fileData = new unsigned short[grid->numVoxels];

  fread(fileData, sizeof(unsigned short), grid->numVoxels, fd);

  float min;
  float max;

  min = ((float)*fileData) / scale;
  max = ((float)*fileData) / scale;

  float* fPtr = grid->data;
  unsigned short* fileEnd = fileData + grid->numVoxels;

  for (unsigned short* dataPtr = fileData; dataPtr < fileEnd; ++dataPtr, ++fPtr)
  {

    *fPtr = ((float)(*dataPtr)) / scale;

    if (*fPtr > max)
    {
      max = *fPtr;
    }
    else if (*fPtr < min)
    {
      min = *fPtr;
    }
  }

  grid->bounds[1] = max;
  grid->bounds[0] = min;

  TRACE("RSPU min/max %5.2f %5.2f\n", min, max);

  delete[] fileData;
}

// Save Aperture perimeters so they can be displayed/checked
void CIonPlan::SaveApertures(void)
{
  CFileIO fileIO;
  int apNum = 0;
  CString fileNameAperture;
  FILE* fd;
  SPt2* apPt = m_arrayAperturePts(0);

  forArray(m_arrayApertures, aperture)
  {

    fileNameAperture.Format("Aperture%d.txt", apNum);
    fd = fileIO.OpenFile(FILE_DEST_DATA, fileNameAperture, "w");

    //fprintf(fd, "    X,        Y\n"); // Header: include if saving as csv file

    for (int index = 0; index < aperture->numAperturePts + 1; ++index, ++apPt)
    {
      fprintf(fd, "%8.2f, %8.2f\n", apPt->x, apPt->y);
    }

    fileIO.CloseFile();
    ++apNum;
  }

  return;
}

void CIonPlan::SaveDevices(void)
{
  CFileIO fileIO;
  CString fileName("Devices.txt");
  SShaderDevice* device;
  SShaderAperture* aperture;
  SPt2* aperturePt;

  FILE* fd = fileIO.OpenFile(FILE_DEST_DATA, fileName, "w");

  forArrayI(m_arrayBeams, beam, indexBeam)
  {
    forArrayIN(m_arrayControlPts, controlPt, beam->indexControlPts, beam->numControlPts)
    {
      forArrayIN(m_arrayDeviceIndexes, indexDevice, controlPt->indexDeviceIndexes, controlPt->numDeviceIndexes)
      {
        device = m_arrayDevices(*indexDevice);

        if (device->numApertures)
        {
          aperture = m_arrayApertures(device->indexApertures);
          aperturePt = m_arrayAperturePts(aperture->indexAperturePts);

          fprintf(fd, "%3d %6.2f %6.2f %2d %4d %6.2f %6.2f\n", *indexDevice, device->zTop, device->zBottom,
            device->numApertures, aperture->numAperturePts, aperturePt->x, aperturePt->y);
        }
        else
        {
          fprintf(fd, "%3d %6.2f %6.2f %2d\n", *indexDevice, device->zTop, device->zBottom, device->numApertures);
        }

      }
    }
  }

  fileIO.CloseFile();

  return;

}

void CIonPlan::CalcImageBounds(SDataGrid* grid)
{
  CFileIO fileIO;
  int    numPosRSPimg;
  float  rspImgMin;
  float  rspImgMax;
  float* imgStart;
  float* imgEnd;

  CString fileName;
  fileName.Format("ImageStats.txt");
  FILE* fd = fileIO.OpenFile(FILE_DEST_DATA, fileName, "w");

  int numVoxImage = grid->dims[0] * grid->dims[1];

  imgEnd = grid->data;
  for (int indexImage = 0; indexImage < grid->dims[2]; ++indexImage)
  {
    imgStart = imgEnd;
    imgEnd += numVoxImage;

    rspImgMin = *imgStart;
    rspImgMax = *imgStart;

    numPosRSPimg = 0;

    for (float* data = imgStart; data < imgEnd; ++data)
    {
      if (*data > rspImgMax)
      {
        rspImgMax = *data;
      }
      else if (*data < rspImgMin)
      {
        rspImgMin = *data;
      }

      if (*data > RSP_BLANK)
      {
        ++numPosRSPimg;
      }
    }

    fprintf(fd, "%4d %10.6f %10.6f %8d\n", indexImage, rspImgMin, rspImgMax, numPosRSPimg);
  }

  fileIO.CloseFile();

  return;
}