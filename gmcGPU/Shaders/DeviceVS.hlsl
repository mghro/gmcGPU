
cbuffer VertexMatrix : register(b0)
{
  float4x4 vmatrix;
}


cbuffer ColorVal : register(b1)
{
  float4 color1;
}


struct VS_INPUT
{
  float4 pos   : POSITION;
};

struct VS_OUTPUT
{
  float4 pos   : SV_POSITION;
  float4 color : COLOR;
};

//--------------------------------------------------------------------------------------
//   Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT DeviceVS(VS_INPUT input)
{
  VS_OUTPUT output;

  output.pos = mul(input.pos, vmatrix);

  //output.color = float4(0.0, 0.0, 1.0, 1.0);

  output.color = color1;



  return output;
}



