#include "Common.hlsl"

struct STriVertex {
  float3 vertex;
  float2 st;
  float3 normal;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);
Texture2D<float4> MegaTexture : register(t1);

[shader("closesthit")] void ClosestHit(inout HitInfo payload,
                                       Attributes attrib) {
  float3 barycentrics =
      float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

  uint vertId = 3 * PrimitiveIndex();
  float3 hitColor = MegaTexture.Load(int3(barycentrics.x * 4096, barycentrics.y * 4096, 0)).rgb; //normalize(BTriVertex[vertId + 0].vertex) * 4;

  payload.colorAndDistance = float4(hitColor, RayTCurrent());
}
