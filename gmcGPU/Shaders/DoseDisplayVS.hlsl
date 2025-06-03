


cbuffer VertexMatrix : register(b0)
{
  float4x4 vmatrix;
  float    zoomFactor;
  float    slicePos;
  int      viewAxis;
}

struct VS_INPUT
{
  float4 viewPos  : POSITION0;
  float4 volPos   : POSITION1;
  float2 tex      : TEXCOORD0;
};

struct VS_OUTPUT
{
  float4 pos    : SV_POSITION;
  float2 tex    : TEXCOORD0;
  float3 volPos : POSITION;
};

/*_____________________________________________________________________________________________

   DoseVS - Vertex Shader for dose volume display
_______________________________________________________________________________________________*/

VS_OUTPUT DoseDisplayVS(VS_INPUT input)
{
  VS_OUTPUT output;

  output.pos = mul(vmatrix, input.viewPos);

  output.tex = input.tex;

  output.volPos = input.volPos;

  if (viewAxis == 0)
  {
    output.volPos[0] = slicePos;    // Sagital (x)
  }   
  else if (viewAxis == 1)
  {
    output.volPos[1] = slicePos;    // Coronal (y)
  }
  else
  {
    output.volPos[2] = slicePos;    // Axial (z)
  }
    
  return output;
}


