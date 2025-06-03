struct ContourPS_INPUT
{
  float4 Pos : SV_POSITION;
  float4 Col : COLOR;
};

/*_____________________________________________________________________________________________

 Pixel Shader - User drawn lines
_______________________________________________________________________________________________*/

float4 LineDrawUserPS(ContourPS_INPUT input) : SV_Target
{
  float4 output;

  output = input.Col;

  return output;
}