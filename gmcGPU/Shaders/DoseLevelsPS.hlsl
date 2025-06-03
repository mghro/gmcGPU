
#define MAX_NUM_COLORS (1024)

struct SFieldLevels
{
  float min;
  float max;
  float width;
  float doseMin;
  float doseMax;
  float pad[3];
};

struct SColorScale
{
  float red;
  float green;
  float blue;
  float alpha; // Need this
};

// Constant Buffers
cbuffer ColorScale : register(b0)
{
  SColorScale colorScale[MAX_NUM_COLORS];
  int numColors;
  int pad[3];
}

cbuffer FieldLevel : register(b1)
{
  SFieldLevels fieldLevels;
}

cbuffer DoseParams : register(b2)
{
  row_major float4x4 doseMatrix;
}

// Resource Buffers
Texture3D<float> imageTexture : register(t1);
Texture3D<float> doseTexture  : register(t2);

// Samplers
SamplerState     imgSampler   : register(s0);

struct ImagePS_INPUT
{
  float4 Pos       : SV_POSITION;
  float2 Tex       : TEXCOORD0;
  float3 imgVolPos : POSITION;
};

//--------------------------------------------------------------------------------------
//  Image Pixel Shader
//--------------------------------------------------------------------------------------
float4 DoseLevelsPS(ImagePS_INPUT input) : SV_Target
{
  float val;
  float4 output;

  val = imageTexture.Sample(imgSampler, input.imgVolPos);

  if (val < fieldLevels.min)
  {
    val = 0.0;
  }
  else if (val > fieldLevels.max)
  {
    val = 1.0;
  }
  else
  {
    val = (val - fieldLevels.min) / fieldLevels.width;
  }

  float3 doseVolPos;
  doseVolPos[0] = input.imgVolPos[0] - doseMatrix[3][0];
  doseVolPos[1] = input.imgVolPos[1] - doseMatrix[3][1];
  doseVolPos[2] = input.imgVolPos[2] - doseMatrix[3][2];

  float3 doseVec;
  doseVec[0] = doseMatrix[0][0] * doseVolPos[0] + doseMatrix[0][1] * doseVolPos[1] + doseMatrix[0][2] * doseVolPos[2];
  doseVec[1] = doseMatrix[1][0] * doseVolPos[0] + doseMatrix[1][1] * doseVolPos[1] + doseMatrix[1][2] * doseVolPos[2];
  doseVec[2] = doseMatrix[2][0] * doseVolPos[0] + doseMatrix[2][1] * doseVolPos[1] + doseMatrix[2][2] * doseVolPos[2];

  float doseVal = doseTexture.Sample(imgSampler, doseVec);

  if (doseVec[0] < 0 || doseVec[0] > 1 || doseVec[1] < 0 || doseVec[1] > 1 || doseVec[2] < -.01 || doseVec[2] > 1)
  {
    output = float4(val, val, val, 1);
    
  }
  else
  {
    if (doseVal < fieldLevels.doseMin)
    {
      output = float4(val, val, val, 1);
    }
    else
    {
      int colorIndex;

      if (doseVal >= fieldLevels.doseMax)
      {
        colorIndex = 1023;
      }
      else
      {
        colorIndex = (int)(((int)( 10 * doseVal / fieldLevels.doseMax)) / 10.0 * 1023);
      }

      output = float4(val *colorScale[colorIndex].red, val*colorScale[colorIndex].green, val *colorScale[colorIndex].blue, 1.0);
    }
  }

  return output;
}


