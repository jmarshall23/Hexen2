#include "Common.hlsl"

struct STriVertex {
  float3 vertex;
  float2 st;
  float3 normal;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);

[shader("closesthit")] void ClosestHit(inout HitInfo payload,
                                       Attributes attrib) {
  float3 barycentrics =
      float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

  uint vertId = 3 * PrimitiveIndex();
  float3 hitColor = normalize(float3(abs(BTriVertex[vertId + 0].st.x), abs(BTriVertex[vertId + 0].st.y), 0.0)) * 0.5;

  payload.colorAndDistance = float4(hitColor, RayTCurrent());
}
