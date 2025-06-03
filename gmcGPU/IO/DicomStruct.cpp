#include "stdafx.h"
#include "DcmtkGMC.h"
#include "DicomStruct.h"

CDicomStruct::CDicomStruct(void)
{
  return;
}

CDicomStruct::~CDicomStruct(void)
{
  return;
}

bool CDicomStruct::ParseStructFile(LPCTSTR fileNameStruct, CSliceHedra* sliceHedra)
{
  OFCondition status;
  DcmFileFormat fileformat;
  DRTStructureSetIOD rtStruct;
  OFString nameStruct;

  status = fileformat.loadFile(fileNameStruct);
  if (status.bad())
  {
    return false;
  }

  status = rtStruct.read(*fileformat.getDataset());
  if (status.bad())
  {
    return false;
  }

  DRTStructureSetROISequence seqStruct;
  DRTStructureSetROISequence::Item itemStruct;

  seqStruct = rtStruct.getStructureSetROISequence();
  if (seqStruct.gotoFirstItem().bad())
  {
    return false;
  }

  int numItemsSeq;
  int count;
  
  count = 0;
  numItemsSeq = seqStruct.getNumberOfItems();
  do
  {
    itemStruct = seqStruct.getCurrentItem();

    itemStruct.getROIName(nameStruct);

    char buff[64];
    sprintf(buff, "Struct %3d %s\n", count, nameStruct.c_str());
    OutputDebugString(buff);

    ++count;

  } while (seqStruct.gotoNextItem().good() && (count < numItemsSeq) );

  DRTROIContourSequence seqROIContour;
  DRTROIContourSequence::Item itemROIContour;
  DRTContourSequence seqContour;
  DRTContourSequence::Item itemContour;
  OFVector<Float64> contourData;


  seqROIContour = rtStruct.getROIContourSequence();

  if (seqROIContour.gotoFirstItem().bad())
  {
    return false;
  }

  count = 0;
  numItemsSeq = seqROIContour.getNumberOfItems();
  do
  {
    itemROIContour = seqROIContour.getCurrentItem();

    seqContour = itemROIContour.getContourSequence();

    int count1 = 0;
    int numItemsSeq1 = seqContour.getNumberOfItems();
    do
    {
      itemContour = seqContour.getCurrentItem();

      itemContour.getContourData(contourData);

      char buff[64];
      sprintf(buff, "Contour %4d %4d %5d %f\n", count, count1, contourData.size(), contourData.size()/3.0);
      OutputDebugString(buff);

      ++count1;
    } while (seqContour.gotoNextItem().good() && (count1 < numItemsSeq1));


    ++count;

  } while (seqStruct.gotoNextItem().good() && (count < numItemsSeq));

  return true;
}