#define _SHADER_

Buffer<float2>  rbData   : register(t0);

RWBuffer<double> ubBounds : register(u0);

[numthreads(1, 1, 1)]
void ArrayMaxCS( uint3 threadID : SV_DispatchThreadID )
{

  float min = rbData[0].x;
  float max = rbData[0].y;

  for (int index = 0; index < 1024; ++index)
  {
    //if (rbData[index].x < min)
    //{
    //  min = rbData[index].x;
    //}

    if (rbData[index].y > max)
    {
      max = rbData[index].y;
      min = rbData[index].x;
    }
  }

  ubBounds[0] = min;
  ubBounds[1] = max;

  return;
}