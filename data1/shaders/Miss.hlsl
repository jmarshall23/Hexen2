#include "Common.hlsl"

[shader("miss")] void Miss(inout HitInfo payload
                           : SV_RayPayload) {
  uint2 launchIndex = DispatchRaysIndex().xy;
  float2 dims = float2(DispatchRaysDimensions().xy);

  float ramp = launchIndex.y / dims.y;
  payload.colorAndDistance = float4(0.0f, 0.0f, 0.0f, -1.0f);
}