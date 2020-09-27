#include "Common.hlsl"

struct ShadowHitInfo {
  bool isHit;
};

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
RaytracingAccelerationStructure SceneBVH : register(t3);

float3 QuakeCoords(float3 xyz) {
	return float3(xyz.x, -xyz.z, xyz.y);
}

float attenuation(float r, float f, float d) {
	return pow(max(0.0, 1.0 - (d / r)), f + 1.0);
}

bool IsLightShadowed(float3 worldOrigin, float3 lightDir, float distance)
{	
	 // Fire a shadow ray. The direction is hard-coded here, but can be fetched
     // from a constant-buffer
     RayDesc ray;
     ray.Origin = worldOrigin;
     ray.Direction = lightDir;
     ray.TMin = 0.01;
     ray.TMax = distance;
     bool hit = true;
     
     // Initialize the ray payload
     ShadowHitInfo shadowPayload;
     shadowPayload.isHit = false;
     
     // Trace the ray
     TraceRay(
     	// Acceleration structure
     	SceneBVH,
     	// Flags can be used to specify the behavior upon hitting a surface
     	RAY_FLAG_NONE,
     	// Instance inclusion mask, which can be used to mask out some geometry to
     	// this ray by and-ing the mask with a geometry mask. The 0xFF flag then
     	// indicates no geometry will be masked
     	0xFF,
     	// Depending on the type of ray, a given object can have several hit
     	// groups attached (ie. what to do when hitting to compute regular
     	// shading, and what to do when hitting to compute shadows). Those hit
     	// groups are specified sequentially in the SBT, so the value below
     	// indicates which offset (on 4 bits) to apply to the hit groups for this
     	// ray. In this sample we only have one hit group per object, hence an
     	// offset of 0.
     	1,
     	// The offsets in the SBT can be computed from the object ID, its instance
     	// ID, but also simply by the order the objects have been pushed in the
     	// acceleration structure. This allows the application to group shaders in
     	// the SBT in the same order as they are added in the AS, in which case
     	// the value below represents the stride (4 bits representing the number
     	// of hit groups) between two consecutive objects.
     	0,
     	// Index of the miss shader to use in case several consecutive miss
     	// shaders are present in the SBT. This allows to change the behavior of
     	// the program when no geometry have been hit, for example one to return a
     	// sky color for regular rendering, and another returning a full
     	// visibility value for shadow rays. This sample has only one miss shader,
     	// hence an index 0
     	1,
     	// Ray information to trace
     	ray,
     	// Payload associated to the ray, which will be used to communicate
     	// between the hit/miss shaders and the raygen
     	shadowPayload);
		
	return shadowPayload.isHit;
}

[shader("closesthit")] void ClosestHit(inout HitInfo payload,
                                       Attributes attrib) {
  float3 barycentrics =
      float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

  uint vertId = BInstanceProperties[InstanceID()].startVertex + (3 * PrimitiveIndex());
  float3 hitColor = float3(1, 0, 0);
  
  // Find the world - space hit position
  float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
  
  float ndotl = 1;
  float3 debug = float3(1, 1, 1);
  //if(InstanceID() == 0) // For now only light the world geometry
  {	  
	  float3 normal = BTriVertex[vertId + 0].normal;
	  
	  bool isBackFacing = dot(normal, WorldRayDirection()) > 0.f;
	  if (isBackFacing)
			normal = -normal;
	  
	  float3 lightPos = QuakeCoords(float3(-891.667, 469.175, 1863.288 ));
	  float3 centerLightDir = lightPos - worldOrigin;
	  float lightDistance = length(centerLightDir);
	  float falloff = attenuation(2000, 1.0, lightDistance);
	  
	  
	  bool isShadowed = dot(normal, centerLightDir) < 0;	  
	  if(!isShadowed)
	  {
		    if(IsLightShadowed(worldOrigin, normalize(centerLightDir), lightDistance))
			{
				ndotl = 0.0f;
			}
			else
			{
				ndotl = falloff * 1.5f; // normalize(centerLightDir); //max(0.f, dot(normal, normalize(centerLightDir))); 
			}
	  }
	  else
	  {
		 ndotl = 0;
	  }
	//  debug = normal;
  }
  
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
  ndotl = max(0.2, ndotl);

  payload.colorAndDistance = float4(hitColor * ndotl * debug, RayTCurrent());
}
