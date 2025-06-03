#include "stdafx.h"
#include "Transport.h"

#define VECCROSS(A, B, C)            \
  C[0] = A[1] * B[2] - A[2] * B[1];  \
  C[1] = A[2] * B[0] - A[0] * B[2];  \
  C[2] = A[0] * B[1] - A[1] * B[0];

#define VECDOT(A, B) \
  (A[0] * B[0] + A[1] * B[1] + A[2] * B[2])

CTextureGroup::CTextureGroup(void)
{
  return;
}

void CTextureGroup::CalcFieldTransform(SVoxGridGeometry* baseGrid)
{
  XMFLOAT4X4* transform = &m_fieldTransform;

  float* horzUnitVec = baseGrid->unitVecs[0];
  float* vertUnitVec = baseGrid->unitVecs[1];
  float* doseHorzUnitVec = m_volumeGrid.unitVecs[0];
  float* doseVertUnitVec = m_volumeGrid.unitVecs[1];
  float dot;

  float scale;
  float imgDepthUnitVec[3];
  float doseDepthUnitVec[3];

  VECCROSS(horzUnitVec, vertUnitVec, imgDepthUnitVec);
  VECCROSS(doseHorzUnitVec, doseVertUnitVec, doseDepthUnitVec);

  scale = (baseGrid->dims[0] * baseGrid->voxelSize[0]) / (m_volumeGrid.dims[0] * m_volumeGrid.voxelSize[0]);
  dot = VECDOT(horzUnitVec, doseHorzUnitVec);
  transform->_11 = dot * scale;
  dot = VECDOT(vertUnitVec, doseHorzUnitVec);
  transform->_12 = dot * scale;
  dot = VECDOT(imgDepthUnitVec, doseHorzUnitVec);
  transform->_13 = dot * scale;

  scale = (baseGrid->dims[1] * baseGrid->voxelSize[1]) / (m_volumeGrid.dims[1] * m_volumeGrid.voxelSize[1]);
  dot = VECDOT(horzUnitVec, doseVertUnitVec);
  transform->_21 = dot * scale;
  dot = VECDOT(vertUnitVec, doseVertUnitVec);
  transform->_22 = dot * scale;
  dot = VECDOT(imgDepthUnitVec, doseVertUnitVec);
  transform->_23 = dot * scale;

  scale = (baseGrid->dims[2] * baseGrid->voxelSize[2]) / (m_volumeGrid.dims[2] * m_volumeGrid.voxelSize[2]);
  dot = VECDOT(horzUnitVec, doseDepthUnitVec);
  transform->_31 = dot * scale;
  dot = VECDOT(vertUnitVec, doseDepthUnitVec);
  transform->_32 = dot * scale;
  dot = VECDOT(imgDepthUnitVec, doseDepthUnitVec);
  transform->_33 = dot * scale;

  float doseVec[3];
  doseVec[0] = m_volumeGrid.voxelOrigin[0] - baseGrid->voxelOrigin[0];
  doseVec[1] = m_volumeGrid.voxelOrigin[1] - baseGrid->voxelOrigin[1];
  doseVec[2] = m_volumeGrid.voxelOrigin[2] - baseGrid->voxelOrigin[2];

  transform->_41 = VECDOT(doseVec, horzUnitVec) / (baseGrid->dims[0] * baseGrid->voxelSize[0]);
  transform->_42 = VECDOT(doseVec, vertUnitVec) / (baseGrid->dims[1] * baseGrid->voxelSize[1]);
  transform->_43 = VECDOT(doseVec, imgDepthUnitVec) / (baseGrid->dims[2] * baseGrid->voxelSize[2]);
  transform->_44 = 1.0;

  return;
}