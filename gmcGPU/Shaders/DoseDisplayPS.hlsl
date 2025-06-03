
#define MAX_NUM_COLORS (1024)

struct SImageLevels
{

  float imageMin;
  float imageMax;
  float imageWidth;
  
  float percentage;

  float padImage[4];
};

struct SColorScale
{
  float red;
  float green;
  float blue;
  float alpha; // Not presently used
};

// Constant Buffers
cbuffer ColorScale : register(b0)
{
  SColorScale colorScale[MAX_NUM_COLORS];
  int numColors;

  int padColor[3];
}

cbuffer ImageLevel : register(b1)
{
  SImageLevels imageLevels;
}

cbuffer DisplayParams : register(b2)
{
  row_major float4x4 fieldTransform;

  float doseMin;
  float doseMax;

  int  padField[6];
}

// Resource Buffers
Texture3D<float> imageTexture : register(t0);
Texture3D<float> doseTexture  : register(t1);

// Samplers
SamplerState     imgSampler   : register(s0);

struct ImagePS_INPUT
{
  float4 Pos       : SV_POSITION;
  float2 Tex       : TEXCOORD0;
  float3 imgVolPos : POSITION;
};

/*_____________________________________________________________________________________________

 Pixel Shader - Display transort results (dose, LET, composite) overlayed on image data 
_______________________________________________________________________________________________*/

float4 DoseDisplayPS(ImagePS_INPUT input) : SV_Target
{
  float imageValue;
  float4 output;

  imageValue = imageTexture.Sample(imgSampler, input.imgVolPos);

#ifdef DBG_0
  imageValue = 1.0;
#endif

  if (imageValue < imageLevels.imageMin)
  {
    imageValue = 0.0;
  }
  else if (imageValue > imageLevels.imageMax)
  {
    imageValue = 1.0;
  }
  else
  {
    imageValue = (imageValue - imageLevels.imageMin) / imageLevels.imageWidth;
  }

  float3 doseVolPos;
  doseVolPos[0] = input.imgVolPos[0] - fieldTransform[3][0];
  doseVolPos[1] = input.imgVolPos[1] - fieldTransform[3][1];
  doseVolPos[2] = input.imgVolPos[2] - fieldTransform[3][2];

  float3 doseVec;
  doseVec[0] = fieldTransform[0][0] * doseVolPos[0] + fieldTransform[0][1] * doseVolPos[1] + fieldTransform[0][2] * doseVolPos[2];
  doseVec[1] = fieldTransform[1][0] * doseVolPos[0] + fieldTransform[1][1] * doseVolPos[1] + fieldTransform[1][2] * doseVolPos[2];
  doseVec[2] = fieldTransform[2][0] * doseVolPos[0] + fieldTransform[2][1] * doseVolPos[1] + fieldTransform[2][2] * doseVolPos[2];

  float fieldValue = doseTexture.Sample(imgSampler, doseVec);

  if (doseVec[0] < 0 || doseVec[0] > 1 || doseVec[1] < 0 || doseVec[1] > 1 || doseVec[2] < -.01 || doseVec[2] > 1)
  {
    output = float4(imageValue, imageValue, imageValue, 1);
  }
  else if (fieldValue >= doseMin)
  {
    int colorIndex;
    int maxColorIndex = numColors - 1;

    colorIndex = (int)(fieldValue * maxColorIndex / doseMax);
    if (colorIndex > maxColorIndex)
    {
      colorIndex = maxColorIndex;
    }

#ifdef DBG_0
    colorIndex = doseVec[0] * numColors; imageValue = 1.0; // Maps colormap along x axis
#endif

    output = float4(imageValue * colorScale[colorIndex].red, imageValue * colorScale[colorIndex].green, imageValue * colorScale[colorIndex].blue, 1.0);
  }
  else
  {
    output = float4(imageValue, imageValue, imageValue, 1);
  }

  return output;
}


