#include "Common.hlsl"

// #DXR Extra: Perspective Camera
cbuffer CameraParams : register(b0)
{
  float4x4 view;
  float4x4 projection;
  float4x4 viewI;
  float4x4 projectionI;
}

// Raytracing output texture, accessed as a UAV
RWTexture2D<float4> gOutput : register(u0);
RWTexture2D<float4> gLightOutput : register(u1);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0);

HitInfo FirePrimaryRay() {
	// Initialize the ray payload
  HitInfo payload;
  payload.colorAndDistance = float4(0, 0, 0, 0);
  
  // Get the location within the dispatched 2D grid of work items
  // (often maps to pixels, so this could represent a pixel coordinate).
  uint2 launchIndex = DispatchRaysIndex().xy;
  float2 dims = float2(DispatchRaysDimensions().xy);
  float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);
  // Define a ray, consisting of origin, direction, and the min-max distance
  // values
  // #DXR Extra: Perspective Camera
  float aspectRatio = dims.x / dims.y;
  
  RayDesc ray;
  float4 target = mul(projectionI, float4(d.x, d.y, 1, 1));
  
  ray.Origin = mul(viewI, float4(0, 0, 0, 1));
  ray.Direction = mul(viewI, float4(target.xyz, 0));
  ray.TMin = 0;
  ray.TMax = 100000;

  // Trace the ray
  TraceRay(
      // Parameter name: AccelerationStructure
      // Acceleration structure
      SceneBVH,

      // Parameter name: RayFlags
      // Flags can be used to specify the behavior upon hitting a surface
      RAY_FLAG_NONE,

      // Parameter name: InstanceInclusionMask
      // Instance inclusion mask, which can be used to mask out some geometry to
      // this ray by and-ing the mask with a geometry mask. The 0xFF flag then
      // indicates no geometry will be masked
      0xFF,

      // Parameter name: RayContributionToHitGroupIndex
      // Depending on the type of ray, a given object can have several hit
      // groups attached (ie. what to do when hitting to compute regular
      // shading, and what to do when hitting to compute shadows). Those hit
      // groups are specified sequentially in the SBT, so the value below
      // indicates which offset (on 4 bits) to apply to the hit groups for this
      // ray. In this sample we only have one hit group per object, hence an
      // offset of 0.
      0,

      // Parameter name: MultiplierForGeometryContributionToHitGroupIndex
      // The offsets in the SBT can be computed from the object ID, its instance
      // ID, but also simply by the order the objects have been pushed in the
      // acceleration structure. This allows the application to group shaders in
      // the SBT in the same order as they are added in the AS, in which case
      // the value below represents the stride (4 bits representing the number
      // of hit groups) between two consecutive objects.
      0,

      // Parameter name: MissShaderIndex
      // Index of the miss shader to use in case several consecutive miss
      // shaders are present in the SBT. This allows to change the behavior of
      // the program when no geometry have been hit, for example one to return a
      // sky color for regular rendering, and another returning a full
      // visibility value for shadow rays. This sample has only one miss shader,
      // hence an index 0
      0,

      // Parameter name: Ray
      // Ray information to trace
      ray,

      // Parameter name: Payload
      // Payload associated to the ray, which will be used to communicate
      // between the hit/miss shaders and the raygen
      payload);
  gOutput[launchIndex] = float4(payload.colorAndDistance.rgb, 1.f);
  gLightOutput[launchIndex] = float4(payload.lightColor.rgb, 1.f);
  
  return payload;
}

[shader("raygeneration")] void RayGen() {
	HitInfo hit = FirePrimaryRay();

	if(hit.lightColor.w == 1 && hit.worldNormal.z != 0 && hit.worldNormal.x == 0 && hit.worldNormal.y == 0)
	{
		  uint2 launchIndex = DispatchRaysIndex().xy;
          float2 dims = float2(DispatchRaysDimensions().xy);
		  float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);
		  float4 target = mul(projectionI, float4(d.x, d.y, 1, 1));  
		  float4 Direction = mul(viewI, float4(target.xyz, 0));
		
		  RayDesc ray;
		  HitInfo payload;
		  payload.colorAndDistance = float4(0, 0, 0, 0);

		  ray.Origin = hit.worldOrigin;
		  ray.Direction = reflect(Direction, hit.worldNormal);
		  ray.TMin = 0;
		  ray.TMax = 100000;

		  // Trace the ray
		  TraceRay(
			  // Parameter name: AccelerationStructure
			  // Acceleration structure
			  SceneBVH,

			  // Parameter name: RayFlags
			  // Flags can be used to specify the behavior upon hitting a surface
			  RAY_FLAG_NONE,

			  // Parameter name: InstanceInclusionMask
			  // Instance inclusion mask, which can be used to mask out some geometry to
			  // this ray by and-ing the mask with a geometry mask. The 0xFF flag then
			  // indicates no geometry will be masked
			  0xFF,

			  // Parameter name: RayContributionToHitGroupIndex
			  // Depending on the type of ray, a given object can have several hit
			  // groups attached (ie. what to do when hitting to compute regular
			  // shading, and what to do when hitting to compute shadows). Those hit
			  // groups are specified sequentially in the SBT, so the value below
			  // indicates which offset (on 4 bits) to apply to the hit groups for this
			  // ray. In this sample we only have one hit group per object, hence an
			  // offset of 0.
			  0,

			  // Parameter name: MultiplierForGeometryContributionToHitGroupIndex
			  // The offsets in the SBT can be computed from the object ID, its instance
			  // ID, but also simply by the order the objects have been pushed in the
			  // acceleration structure. This allows the application to group shaders in
			  // the SBT in the same order as they are added in the AS, in which case
			  // the value below represents the stride (4 bits representing the number
			  // of hit groups) between two consecutive objects.
			  0,

			  // Parameter name: MissShaderIndex
			  // Index of the miss shader to use in case several consecutive miss
			  // shaders are present in the SBT. This allows to change the behavior of
			  // the program when no geometry have been hit, for example one to return a
			  // sky color for regular rendering, and another returning a full
			  // visibility value for shadow rays. This sample has only one miss shader,
			  // hence an index 0
			  0,

			  // Parameter name: Ray
			  // Ray information to trace
			  ray,

			  // Parameter name: Payload
			  // Payload associated to the ray, which will be used to communicate
			  // between the hit/miss shaders and the raygen
			  payload);
			  
		   gOutput[launchIndex] = lerp(gOutput[launchIndex], float4(payload.colorAndDistance.rgb, 1.f), 0.3);
		   gLightOutput[launchIndex] = lerp(gLightOutput[launchIndex], float4(payload.lightColor.rgb, 1.f), 0.3);
	}
}
