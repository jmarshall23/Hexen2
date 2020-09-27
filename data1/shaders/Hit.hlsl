#include "Common.hlsl"

struct STriVertex {
  float3 vertex;
  float2 st;
  float3 normal;
  float4 vtinfo;
};

struct SInstanceProperties
{
	int startVertex;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);
Texture2D<float4> MegaTexture : register(t1);
StructuredBuffer<SInstanceProperties> BInstanceProperties : register(t2);

[shader("closesthit")] void ClosestHit(inout HitInfo payload,
                                       Attributes attrib) {
  float3 barycentrics =
      float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

  uint vertId = BInstanceProperties[InstanceID()].startVertex + (3 * PrimitiveIndex());
  float3 hitColor = float3(1, 0, 0);
  
  if(BTriVertex[vertId + 0].vtinfo.x != -1)
  {
	  float u = 0, v = 0;
	  for(int i = 0; i < 3; i++)
	  {
			u += abs(BTriVertex[vertId + i].st.x) * barycentrics[i];
			v += abs(BTriVertex[vertId + i].st.y) * barycentrics[i];
	  }
	  
	  u = frac(u) / 4096;
	  v = frac(1 - v) / 4096;
	  
	  u = u * BTriVertex[vertId + 0].vtinfo.z;
	  v = v * BTriVertex[vertId + 0].vtinfo.w;
	  
	  u = u + (BTriVertex[vertId + 0].vtinfo.x / 4096);
	  v = v + (BTriVertex[vertId + 0].vtinfo.y / 4096);
	  

	  //u *= BTriVertex[vertId + 0].vtinfo.z + (BTriVertex[vertId + 0].vtinfo.x / 4096);
	  //v *= BTriVertex[vertId + 0].vtinfo.w + (BTriVertex[vertId + 0].vtinfo.y / 4096);
	  //hitColor = float3(u, v, 0);
	  hitColor = MegaTexture.Load(int3(u * 4096, v * 4096, 0)).rgb; //normalize(BTriVertex[vertId + 0].vertex) * 4;
  }
  else
  {
	float u = 0, v = 0;
	  for(int i = 0; i < 3; i++)
	  {
			u += abs(BTriVertex[vertId + i].st.x) * barycentrics[i];
			v += abs(BTriVertex[vertId + i].st.y) * barycentrics[i];
	  }
	  
	  u = frac(u);
	  v = frac(1 - v);
	  hitColor = float3(u, v, 0);
  }

  //hitColor = float3(InstanceID(), 0, 0);

  payload.colorAndDistance = float4(hitColor, RayTCurrent());
}
