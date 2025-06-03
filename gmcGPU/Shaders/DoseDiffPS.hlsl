
#define MAX_NUM_COLORS (1024)
#define DIFF_MAX       (2.0)

struct SColorScale
{
  float red;
  float green;
  float blue;
  float alpha; // Need this even though not used
};

struct SImageLevels
{

  float imageMin;
  float imageMax;
  float imageWidth;

  float percentage;

  float padImage[4];
};

struct SFieldParams
{
  row_major float4x4 xform;

  float fieldMin;
  float fieldMax;

  int  fieldType;

  int padField[5];
};

// Constant Buffers
cbuffer ColorScale : register(b0)
{
  SColorScale colorScale[MAX_NUM_COLORS];
  int numColors;
  int pad[3];
}

cbuffer ImageLevels : register(b1)
{
  SImageLevels imageLevels;
}

cbuffer FieldParamsPrim : register(b2)
{
  SFieldParams fieldPrim;
}

cbuffer FieldParamsSec : register(b3)
{
  SFieldParams fieldSec;
}

// Resource Buffers
Texture3D<float> imageTexture    : register(t0);
Texture3D<float> doseTexturePrim : register(t1);
Texture3D<float> doseTextureSec  : register(t2);

// Samplers
SamplerState     imgSampler   : register(s0);

struct ImagePS_INPUT
{
  float4 Pos : SV_POSITION;
  float2 Tex : TEXCOORD0;
  float3 imgVolPos : POSITION;
};

/*_____________________________________________________________________________________________

   Pixel Shader - Calculates scaled diff between two fields and displays result
_______________________________________________________________________________________________*/

float4 DoseDiffPS(ImagePS_INPUT input) : SV_Target
{
  float val;
  float4 output;
  float3 doseVolPos;

  val = imageTexture.Sample(imgSampler, input.imgVolPos);

  if (val < imageLevels.imageMin)
  {
    val = 0.0;
  }
  else if (val > imageLevels.imageMax)
  {
    val = 1.0;
  }
  else
  {
    val = (val - imageLevels.imageMin) / imageLevels.imageWidth;
  }

  // Find primary dose value
  doseVolPos[0] = input.imgVolPos[0] - fieldPrim.xform[3][0];
  doseVolPos[1] = input.imgVolPos[1] - fieldPrim.xform[3][1];
  doseVolPos[2] = input.imgVolPos[2] - fieldPrim.xform[3][2];

  float3 doseVecPrim;
  doseVecPrim[0] = fieldPrim.xform[0][0] * doseVolPos[0] + fieldPrim.xform[0][1] * doseVolPos[1] + fieldPrim.xform[0][2] * doseVolPos[2];
  doseVecPrim[1] = fieldPrim.xform[1][0] * doseVolPos[0] + fieldPrim.xform[1][1] * doseVolPos[1] + fieldPrim.xform[1][2] * doseVolPos[2];
  doseVecPrim[2] = fieldPrim.xform[2][0] * doseVolPos[0] + fieldPrim.xform[2][1] * doseVolPos[1] + fieldPrim.xform[2][2] * doseVolPos[2];

  float doseValPrim = doseTexturePrim.Sample(imgSampler, doseVecPrim);

  // Find secondary dose value
  doseVolPos[0] = input.imgVolPos[0] - fieldSec.xform[3][0];
  doseVolPos[1] = input.imgVolPos[1] - fieldSec.xform[3][1];
  doseVolPos[2] = input.imgVolPos[2] - fieldSec.xform[3][2];

  float3 doseVecSec;
  doseVecSec[0] = fieldSec.xform[0][0] * doseVolPos[0] + fieldSec.xform[0][1] * doseVolPos[1] + fieldSec.xform[0][2] * doseVolPos[2];
  doseVecSec[1] = fieldSec.xform[1][0] * doseVolPos[0] + fieldSec.xform[1][1] * doseVolPos[1] + fieldSec.xform[1][2] * doseVolPos[2];
  doseVecSec[2] = fieldSec.xform[2][0] * doseVolPos[0] + fieldSec.xform[2][1] * doseVolPos[1] + fieldSec.xform[2][2] * doseVolPos[2];

  float doseValSec = doseTextureSec.Sample(imgSampler, doseVecSec);

  float doseDiffVal = abs(2*(doseValPrim - doseValSec)) / (doseValPrim + doseValSec);

  if (doseVecSec[0] < 0 || doseVecSec[0] > 1 || doseVecSec[1] < 0 || doseVecSec[1] > 1 || doseVecSec[2] < -.01 || doseVecSec[2] > 1)
  {
    output = float4(val, val, val, 1);
  }
  else
  {
    if (doseValPrim > doseValSec)
    {
      output = float4(0.0, 1.0, 0.0, 1.0);
    }
    else if(doseValPrim < doseValSec)
    {
      output = float4(0.0, 0.0, 1.0, 1.0);
    }
    else
    {
      output = float4(val, val, val, 1);
    }
  }
  //{
  //  int colorIndex;
  //  int maxColorIndex = numColors - 1;

  //  colorIndex = (int)(doseDiffVal * maxColorIndex / DIFF_MAX);
  //  if (colorIndex > maxColorIndex)
  //  {
  //    colorIndex = maxColorIndex;
  //  }
  //  output = float4(val*colorScale[colorIndex].red, val*colorScale[colorIndex].green, val*colorScale[colorIndex].blue, 1.0);
  //}

  return output;
}


