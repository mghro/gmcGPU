#include "stdafx.h"
#include "DcmtkGMC.h"
#include "IonPlan.h"
#include "ShaderPlan.h"
#include "DicomDose.h"

CDicomDose::CDicomDose(void)
{

  return;
}

CDicomDose::~CDicomDose(void)
{

  return;
}

void CDicomDose::ParseDoseFile(LPCTSTR doseFile, SDataGrid* dataGrid)
{
  OFString deliveryType;
  DcmFileFormat fileformat;
  SVoxGridGeometry* volGrid = dataGrid;

  OFCondition status = fileformat.loadFile(doseFile);

  // MLW todo: check modality for dose

  if (status.good())
  {
    DRTDoseIOD rtDose;

    status = rtDose.read(*fileformat.getDataset());

    if (status.good())
    {
      OFString patientsName;
      status = rtDose.getPatientName(patientsName);
    }

    OFString tagString;
    rtDose.getNumberOfFrames(tagString);
    volGrid->dims[2] = atoi(tagString.c_str());

    unsigned short shortInt;
    rtDose.getColumns(shortInt);
    volGrid->dims[0] = shortInt;
    rtDose.getRows(shortInt);
    volGrid->dims[1] = shortInt;

    OFVector<double> spacing(3);
    rtDose.getPixelSpacing(spacing);
    volGrid->voxelSize[0] = (float)spacing[0];
    volGrid->voxelSize[1] = (float)spacing[1];
    status = rtDose.getSliceThickness(spacing[2]);

    if (status.good())
    {
      volGrid->voxelSize[2] = (float)spacing[2];
    }
    else
    {
      OFVector<Float64> vec;
      rtDose.getGridFrameOffsetVector(vec);

      int num = (int)vec.size() - 1;
      volGrid->voxelSize[2] = (float) ((vec[num] - vec[0]) / num); 
    }
 
    volGrid->voxelVolume = volGrid->voxelSize[0] * volGrid->voxelSize[1] * volGrid->voxelSize[2];
    volGrid->numVoxels = volGrid->dims[0] * volGrid->dims[1] * volGrid->dims[2];

    rtDose.getImagePositionPatient(spacing);
    volGrid->voxelOrigin[0] = (float)spacing[0];
    volGrid->voxelOrigin[1] = (float)spacing[1];
    volGrid->voxelOrigin[2] = (float)spacing[2];

    //OFVector<Float64> orient(6);
    //rtDose.getImageOrientationPatient(orient);
    //volGrid->unitVecs[0][0] = orient[1];
    //volGrid->unitVecs[0][1] = orient[2];
    //volGrid->unitVecs[0][2] = orient[3];
    //volGrid->unitVecs[1][0] = orient[4];
    //volGrid->unitVecs[1][1] = orient[5];
    //volGrid->unitVecs[1][2] = orient[6];
    //// MLW todo: crossprod

    double doseScaling;
    rtDose.getDoseGridScaling(doseScaling);

#ifdef DBG_0
    float sigma[8] = {
      15.101,
      8.116,
      7.134,
      14.083,
      13.896,
      12.033,
      12.26,
      12.26
    };
    doseScaling *= (sigma[6] / 100.0);
#endif

    rtDose.getBitsStored(shortInt);
    int numBits = shortInt;
    rtDose.getPixelRepresentation(shortInt);
    int pixRep = shortInt;

    DcmPixelData& pixelData = rtDose.getPixelData();

    dataGrid->data = new float[volGrid->numVoxels];
    memset(dataGrid->data, 0, volGrid->numVoxels * sizeof(float));

    double sum = 0.0;
    float* fData = dataGrid->data;
    if (pixRep == 0 && numBits == 32) // MLW todo: check all possible, template
    {
      unsigned int* dicomData = NULL;

      unsigned short* dicomData2 = NULL;

      // Returns pointer to data but short
      pixelData.getUint16Array(dicomData2);

      //OFCondition cnd = pixelData.getUint32Array(dicomData); // Does not work

      // Cast short pointer to regular pointer
      dicomData = (unsigned int*)dicomData2;

      dataGrid->bounds[0] = (float) ((*dicomData)  * doseScaling);
      dataGrid->bounds[1] = dataGrid->bounds[0];

      for (unsigned int* data = dicomData; data < dicomData + volGrid->numVoxels; ++data)
      {
        *fData = (float) ((*data)  * doseScaling);

        if (*fData < dataGrid->bounds[0])
        {
          dataGrid->bounds[0] = *fData;
        }
        else if (*fData > dataGrid->bounds[1])
        {
          dataGrid->bounds[1] = *fData;
        }

        sum += *fData;
        ++fData;
      }
    }
    //else if (pixRep == 0 && numBits == 16) // MLW todo: check all possible, template
    //{
    //  unsigned int* dicomData = NULL;

    //  unsigned short* dicomData2 = NULL;
    //  pixelData.getUint16Array(dicomData2);

    //  //OFCondition cnd = pixelData.getUint32Array(dicomData);

    //  dataGrid->bounds[0] = (float)((*dicomData2) * doseScaling);
    //  dataGrid->bounds[1] = dataGrid->bounds[0];

    //  for (unsigned short* data = dicomData2; data < dicomData2 + volGrid->numVoxels; ++data)
    //  {
    //    *fData = (float)((*data) * doseScaling);

    //    if (*fData < dataGrid->bounds[0])
    //    {
    //      dataGrid->bounds[0] = *fData;
    //    }
    //    else if (*fData > dataGrid->bounds[1])
    //    {
    //      dataGrid->bounds[1] = *fData;
    //    }

    //    sum += *fData;
    //    ++fData;
    //  }
    //}
    else
    {
      unsigned short* dicomData = NULL;
      pixelData.getUint16Array(dicomData);

      dataGrid->bounds[0] = (float)((*dicomData) * doseScaling);
      dataGrid->bounds[1] = dataGrid->bounds[0];


      for (unsigned short* data = dicomData; data < dicomData + volGrid->numVoxels; ++data)
      {
        *fData = (float)((*data) * doseScaling);

        if (*fData < dataGrid->bounds[0])
        {
          dataGrid->bounds[0] = *fData;
        }
        else if (*fData > dataGrid->bounds[1])
        {
          dataGrid->bounds[1] = *fData;
        }

        sum += *fData;
        ++fData;
      }
    }

    TRACE("ExternalDose Bounds %f %f Sum %6.4e %6.4e\n", dataGrid->bounds[0], dataGrid->bounds[1], sum);
  }

  return;
}

void CDicomDose::BuildDoseFile(CIonPlan* ionPlan, unsigned short* doseData, SVoxGridGeometry* doseGrid,
  LPCTSTR fileName, LPCTSTR seriesDescr, float storeScale, int beamIndex)
{
  OFCondition status;
  DRTDoseIOD  rtDose;
  CString     stringValue;
  COleDateTime now = COleDateTime::GetCurrentTime();

  // 0008
  rtDose.setSpecificCharacterSet("ISO_IR 100");
  stringValue.Format("%4d%02d%02d", now.GetYear(), now.GetMonth(), now.GetDay());
  rtDose.setInstanceCreationDate(OFString(stringValue));
  rtDose.setStudyDate(OFString(stringValue));
  rtDose.setSeriesDate(OFString(stringValue));
  stringValue.Format("%02d%02d%02d", now.GetHour(), now.GetDay(), now.GetSecond());
  rtDose.setInstanceCreationTime(OFString(stringValue));
  rtDose.setStudyTime(OFString(stringValue));
  rtDose.setSeriesTime(OFString(stringValue));
  rtDose.setSOPClassUID("1.2.840.10008.5.1.4.1.1.481.2");
  stringValue.Preallocate(65);
  dcmGenerateUniqueIdentifier(stringValue.GetBuffer(), "2.16.840.1.114460.178.1");
  stringValue.ReleaseBuffer();
  rtDose.setSOPInstanceUID(OFString(stringValue));
  rtDose.setInstanceCreationTime(OFString(stringValue));
  rtDose.setModality("RTDOSE");
  status = rtDose.setManufacturer("MGH RO");
  rtDose.setInstitutionName("MGH");
  rtDose.setSeriesDescription(seriesDescr);
  rtDose.setManufacturerModelName("MGH RO GMC");

  // 0010
  rtDose.setPatientName(OFString(ionPlan->m_patientName));
  rtDose.setPatientID(OFString(ionPlan->m_patientID));

  // 0012
  rtDose.setPatientIdentityRemoved("NO");

  // 0018
  stringValue.Format("%f", doseGrid->voxelSize[2]);
  rtDose.setSliceThickness(OFString(stringValue));

  // 0020
  status = rtDose.setStudyInstanceUID(OFString(ionPlan->m_studyUID));
  if (status.bad())
  {
    AfxMessageBox("Bad study UID.\nNo dose file saved.");
    return;
  }

  stringValue.Format("%s.8", ionPlan->m_seriesUID); // MLW ok?
  rtDose.setSeriesInstanceUID(OFString(stringValue));
  if (status.bad())
  {
    AfxMessageBox("Bad series UID.\nNo dose file saved.");
    return;
  }

  rtDose.setStudyID("HFS1");
  rtDose.setSeriesNumber("1");
  rtDose.setInstanceNumber("0");
  char slash = 0x5C; // backslash
  stringValue.Format("%f%c%f%c%f", doseGrid->voxelOrigin[0], slash, doseGrid->voxelOrigin[1], slash, doseGrid->voxelOrigin[2]);
  rtDose.setImagePositionPatient(OFString(stringValue));
  stringValue.Format("%f%c%f%c%f%c%f%c%f%c%f", doseGrid->unitVecs[0][0], slash, doseGrid->unitVecs[0][1], slash, doseGrid->unitVecs[0][2], slash,
                                               doseGrid->unitVecs[1][0], slash, doseGrid->unitVecs[1][1], slash, doseGrid->unitVecs[1][2]);
  rtDose.setImageOrientationPatient(OFString(stringValue));
  rtDose.setFrameOfReferenceUID(OFString(ionPlan->m_frameReferenceUID));

  // 0028
  rtDose.setSamplesPerPixel(1);
  rtDose.setPhotometricInterpretation("MONOCHROME2");
  stringValue.Format("%d", doseGrid->dims[2]);
  rtDose.setNumberOfFrames(OFString(stringValue));
  stringValue.Format("(3004,000C)");
  rtDose.setFrameIncrementPointer(OFString(stringValue));
  rtDose.setRows(doseGrid->dims[1]);
  rtDose.setColumns(doseGrid->dims[0]);
  stringValue.Format("%f%c%f", doseGrid->voxelSize[0], slash, doseGrid->voxelSize[1]);
  rtDose.setPixelSpacing(OFString(stringValue));
  rtDose.setBitsAllocated(16);
  rtDose.setBitsStored(16);
  rtDose.setHighBit(15);
  rtDose.setPixelRepresentation(0);

  // 3004
  rtDose.setDoseUnits("GY");
  rtDose.setDoseType("EFFECTIVE");
  if (beamIndex)
  {
    rtDose.setDoseSummationType("BEAM");
  }
  else
  {
    rtDose.setDoseSummationType("PLAN");
  }
  
  stringValue = "0";
  CString add;
  for (int i = 1; i < doseGrid->dims[2]; ++i)
  {
    add.Format("%c%f", slash, i * doseGrid->voxelSize[2]);
    stringValue += add;
  }
  rtDose.setGridFrameOffsetVector(OFString(stringValue));

  stringValue.Format("%f", storeScale);
  rtDose.setDoseGridScaling(OFString(stringValue));

  // 300C
  DRTReferencedRTPlanSequence& planSequence = rtDose.getReferencedRTPlanSequence();
  DRTReferencedRTPlanSequence::Item* planItem;
  planSequence.addItem(planItem);
  planItem->setReferencedSOPClassUID("1.2.840.10008.5.1.4.1.1.481.8");
  status = planItem->setReferencedSOPInstanceUID(OFString(ionPlan->m_planUID));

  if (beamIndex)
  {
    DRTReferencedRTPlanSequence& planSequence = rtDose.getReferencedRTPlanSequence();
    DRTReferencedFractionGroupSequence& fractionGroupSequence = planItem->getReferencedFractionGroupSequence();
    DRTReferencedFractionGroupSequence::Item* fractionGroupItem;
    fractionGroupSequence.addItem(fractionGroupItem);

    fractionGroupItem->setReferencedFractionGroupNumber("1");
    DRTReferencedBeamSequenceInRTDoseModule& beamSequence = fractionGroupItem->getReferencedBeamSequence();
    DRTReferencedBeamSequenceInRTDoseModule::Item* beamItem;
    beamSequence.addItem(beamItem);
    stringValue.Format("%d", beamIndex);
    beamItem->setReferencedBeamNumber(OFString(stringValue));
  } 

  DcmDataset dataSet;
  status = rtDose.write(dataSet);
  if (status.bad())
  {
    AfxMessageBox("Problem uilding dose file.");
    return;
  }

  // 7FE0
  status = dataSet.putAndInsertUint16Array(DCM_PixelData, doseData, doseGrid->numVoxels);
  if (status.bad())
  {
    AfxMessageBox("Problem storing dose data.");
    return;
  }

  // Write to file
  status = dataSet.saveFile(fileName, EXS_LittleEndianImplicit, EET_ExplicitLength);
  if (status.bad())
  {
    AfxMessageBox("Problem saving dose file.");
    return;
  }

  return;
}

bool CDicomDose::ReadPlanBasic(LPCTSTR fileNamePlan, SDicomInfo* info)
{
  OFString stringInput;
  DcmFileFormat fileformat;

  OFCondition status = fileformat.loadFile(fileNamePlan);
  if (status.bad())
  {
    return false;
  }

  DRTPlanIOD rtPlan;

  status = rtPlan.read(*fileformat.getDataset());
  if (status.bad())
  {
    return false;
  }

  rtPlan.getPatientID(stringInput);
  info->idPatient = stringInput.data();

  rtPlan.getStudyInstanceUID(stringInput);
  info->uidStudy = stringInput.data();

  rtPlan.getSeriesInstanceUID(stringInput);
  info->uidSeries = stringInput.data();

  rtPlan.getSOPInstanceUID(stringInput);
  info->uidInstance = stringInput.data();

  return true;
}

void CDicomDose::SumBeamDoses(CIonPlan* ionPlan, CString beamFolder)
{

  WIN32_FIND_DATA fileData;
  HANDLE fileHandle;
  CString fileNameFull;
  float sobpFactor = 1.0;
  float* beamFactor = NULL;
  

  CString parseObject = beamFolder + "\\*";

  fileHandle = FindFirstFile(parseObject, &fileData);  // Find file "."
  FindNextFile(fileHandle, &fileData);                 // Find file ".."

  SDataGrid gridSum;

  bool firstDoseFile = true;


  if (ionPlan->m_flagSOBP)
  {
    beamFactor = ionPlan->m_arrayFractionBeamDoses(0);
  }


  while (FindNextFile(fileHandle, &fileData))
  {

    if (strstr(fileData.cFileName, "dcm"))
    {
      SDataGrid gridBeam = {};

      fileNameFull = beamFolder + "\\" + fileData.cFileName;
      ParseDoseFile(fileNameFull, &gridBeam);

      if (firstDoseFile)
      {
        // Allocate dose sum based on first beam dose size (assumes all beam dose cubes same size)
        gridSum = gridBeam;

        gridSum.data = new float[gridBeam.numVoxels];
        memset(gridSum.data, 0, gridBeam.numVoxels * sizeof(float));

        firstDoseFile = false;
      }

      if (ionPlan->m_flagSOBP)
      {
        sobpFactor = *beamFactor;
        ++beamFactor;
      }

      SumDose(&gridSum, &gridBeam, sobpFactor);

      delete[] gridBeam.data;

    }
  }

  unsigned short* sumDose = new unsigned short[gridSum.numVoxels];
  float scaleFactor = 1000;

  CString fileNameDose = beamFolder + "\\SumDoses.dcm";

  ConvertDose(sumDose, gridSum.data, gridSum.numVoxels, scaleFactor);
  delete[] gridSum.data;

  BuildDoseFile(ionPlan, sumDose, &gridSum, fileNameDose, "BeamSum", 1.0/scaleFactor, 0);

  delete[] sumDose;

}

void CDicomDose::SumDose(SDataGrid* gridSum, SDataGrid* gridBeam, float scaleFactor)
{
  float* sum;
  float* beam;
  float* end;

  sum = gridSum->data;
  end = gridBeam->data + gridBeam->numVoxels;
  for (beam = gridBeam->data; beam < end; ++sum, ++beam)
  {
    *sum += *beam * scaleFactor;
  }
  
  return;
}

void CDicomDose::ConvertDose(unsigned short* doseShort, float* doseFloat, int numVoxels, float scaleFactor)
{
  unsigned short* end = doseShort + numVoxels;
  float* doseF = doseFloat;

  for (unsigned short* doseS = doseShort; doseS < end; ++doseS, ++doseF)
  {
    if (doseS - doseShort == 60927)
    {
      int i = 5;
    }
    *doseS = (unsigned short) (*doseF * scaleFactor + 0.5);
    if ((doseS - doseShort) % 256 == 0)
    {
      TRACE("%d\n", doseS - doseShort);
    }
  }

  return;
}