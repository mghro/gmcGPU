
cbuffer VertexMatrix : register(b0)
{
  float4x4 vmatrix;
  float    zoomFactor;
  float    slicePos;
  int      viewAxis;
}

struct ContourVS_INPUT
{
  float4 Pos : POSITION;
  float4 Col : COLOR;
};

struct ContourVS_OUTPUT
{
  float4 Pos : SV_POSITION;
  float4 Col : COLOR;
};

/*_____________________________________________________________________________________________

  Vertex Shader - User drawn lines, uses same transform as dose display
_______________________________________________________________________________________________*/

ContourVS_OUTPUT LineDrawUserVS(ContourVS_INPUT input)
{
  ContourVS_OUTPUT output;

  output.Pos = mul(vmatrix, input.Pos);

  output.Col = input.Col;

  return output;
}


