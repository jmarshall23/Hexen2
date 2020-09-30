#define KERNEL_SIZE         13
#define HALF_KERNEL_SIZE    6

#define NUM_THREADS_X       1024
#define NUM_THREADS_Y       1
#define NUM_THREADS_Z       1

static const float k_sample_weights[KERNEL_SIZE] = {
  0.002216,
  0.008764,
  0.026995,
  0.064759,
  0.120985,
  0.176033,
  0.199471,
  0.176033,
  0.120985,
  0.064759,
  0.026995,
  0.008764,
  0.002216,
};

Texture2D<float4>   BufferIn  : register(t0);
Texture2D<float4>   AlbedoBufferIn  : register(t1);
RWTexture2D<float4> BufferOut : register(u2);

cbuffer UniformBlock0 : register(b3)
{
	int xoffset;
	int yoffset;
};

Texture2D<float4>   uiTextureBuffer  : register(t4);

groupshared float4  g_shared_input[NUM_THREADS_X];

//! @fn hblur_main
//!
//!
[numthreads(NUM_THREADS_X, NUM_THREADS_Y, NUM_THREADS_Z)]
void hblur_main(uint3 gid : SV_GroupID, uint gindex : SV_GroupIndex)
{
  int realOffset = xoffset;
  if(realOffset > 0)
  {
	realOffset = xoffset - HALF_KERNEL_SIZE;
  }
  int2 coord = int2(realOffset + gindex, yoffset + gid.y);  

  g_shared_input[gindex] =  BufferIn.Load(int3(coord, 0));  

  GroupMemoryBarrierWithGroupSync();

  float4 value = 0;
  for (int i = -HALF_KERNEL_SIZE; i < HALF_KERNEL_SIZE; ++i) {
    int index = gindex + i;
    if (index >= 0 && index < NUM_THREADS_X) {
      value += g_shared_input[index] * k_sample_weights[i + HALF_KERNEL_SIZE];
    }    
  }

  BufferOut[coord] = value;
}

//! @fn vblur_main
//!
//!
[numthreads(NUM_THREADS_X, NUM_THREADS_Y, NUM_THREADS_Z)]
void vblur_main(uint3 gid : SV_GroupID, uint gindex : SV_GroupIndex)
{
  int realOffset = xoffset;
  if(realOffset > 0)
  {
	realOffset = xoffset - HALF_KERNEL_SIZE;
  }
  int2 coord = int2(realOffset + gid.x, yoffset + gindex);

  g_shared_input[gindex] =  BufferIn.Load(int3(coord, 0));

  GroupMemoryBarrierWithGroupSync();

  float4 value = 0;
  for (int i = -HALF_KERNEL_SIZE; i < HALF_KERNEL_SIZE; ++i) {
    int index = gindex + i;
    if (index >= 0 && index < NUM_THREADS_X) {
      value += g_shared_input[index] * k_sample_weights[i + HALF_KERNEL_SIZE];
    }    
  }

  float4 uiTexture = uiTextureBuffer.Load(int3(coord, 0));
  float4 albeodTexture = AlbedoBufferIn.Load(int3(coord, 0));
  
  BufferOut[coord] = (value * AlbedoBufferIn.Load(int3(coord, 0)) * (1.0 - uiTexture.w)) + (uiTexture * uiTexture.w);

}
