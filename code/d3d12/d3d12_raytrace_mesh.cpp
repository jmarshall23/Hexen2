// d3d12_raytrace_mesh.cpp
//

#include "d3d12_local.h"
#include "nv_helpers_dx12/BottomLevelASGenerator.h"
#include "nv_helpers_dx12/TopLevelASGenerator.h"
#include <vector>

std::vector<dxrMesh_t *> dxrMeshList;


ComPtr<ID3D12Resource> m_vertexBuffer;
D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
std::vector<dxrVertex_t> sceneVertexes;

void GL_LoadBottomLevelAccelStruct(dxrMesh_t* mesh, msurface_t* surfaces, int numSurfaces) {
	glpoly_t* p;

	mesh->startSceneVertex = sceneVertexes.size();

	for (int i = 0; i < numSurfaces; i++)
	{
		msurface_t* fa = &surfaces[i];
		dxrSurface_t surf;

		float x, y, w, h;
		GL_FindMegaTile(fa->texinfo->texture->name, x, y, w, h);

		int materialInfo = 1;

		//if(strstr(fa->texinfo->texture->name, "rtex018")) {
		//	materialInfo = 0;
		//}

		// HACK! 
		if(x == -1) {
			GL_FindMegaTile("rtex080", x, y, w, h);
			materialInfo = 0; 
		}

		BuildSurfaceDisplayList(fa);

		if(fa->bmodelindex > 0 && currentmodel->name[0] != '*') {
			continue;
		}

		if (fa->flags & SURF_DRAWSKY) {
			continue;
		}

		surf.startVertex = mesh->meshVertexes.size();
		surf.numVertexes = 0;
		for (p = fa->polys; p; p = p->next) {
			for (int d = 0; d < p->numverts; d++) {
				dxrVertex_t v;
		
				v.xyz[0] = p->verts[d][0];
				v.xyz[1] = p->verts[d][1];
				v.xyz[2] = p->verts[d][2];
				v.st[0] = p->verts[d][3];
				v.st[1] = p->verts[d][4];
				v.st[2] = materialInfo;
				v.vtinfo[0] = x;
				v.vtinfo[1] = y;
				v.vtinfo[2] = w;
				v.vtinfo[3] = h;
		
				mesh->meshVertexes.push_back(v);
				surf.numVertexes++;
			}
		}

		surf.numIndexes = 0;
		surf.startIndex = mesh->meshIndexes.size();
		for (int d = 0; d < surf.numVertexes - 1; d++)
		{
			mesh->meshIndexes.push_back(surf.startVertex + 0);
			mesh->meshIndexes.push_back(surf.startVertex + d + 1);
			mesh->meshIndexes.push_back(surf.startVertex + d);
			surf.numIndexes+=3;
		}

		mesh->meshSurfaces.push_back(surf);
	}

	// TODO: Use a index buffer here : )
	{
		for (int i = 0; i < mesh->meshSurfaces.size(); i++)
		{
			mesh->meshSurfaces[i].startMegaVertex = mesh->meshTriVertexes.size();

			for (int d = 0; d < mesh->meshSurfaces[i].numIndexes; d++)
			{
				int indexId = mesh->meshSurfaces[i].startIndex + d;
				int idx = mesh->meshIndexes[indexId];

				mesh->meshTriVertexes.push_back(mesh->meshVertexes[idx]);
				sceneVertexes.push_back(mesh->meshVertexes[idx]);
				mesh->numSceneVertexes++;
			}
		}
	}

	// Write Obj test
	//if (m->numTris > 300)
	//{
	//	FILE* f = fopen("test.obj", "wb");
	//
	//	for (int i = 0; i < mesh->numSceneVertexes; i++) {
	//		fprintf(f, "v %f %f %f\n", sceneVertexes[mesh->startSceneVertex + i].xyz[0], sceneVertexes[mesh->startSceneVertex + i].xyz[2], -sceneVertexes[mesh->startSceneVertex + i].xyz[1]);
	//	}
	//
	//	for (int i = 0; i < mesh->numSceneVertexes; i += 3) {
	//		int idx1 = i + 0;
	//		int idx2 = i + 1;
	//		int idx3 = i + 2;
	//
	//		fprintf(f, "f %d %d %d\n", idx1 + 1, idx2 + 1, idx3 + 1);
	//	}
	//
	//	fclose(f);
	//}

	// Calculate the normals
	{
		for(int i = 0; i < mesh->numSceneVertexes; i+=3)
		{
			float* v0 = &sceneVertexes[mesh->startSceneVertex + i + 0].xyz[0];
			float* v1 = &sceneVertexes[mesh->startSceneVertex + i + 1].xyz[0];
			float* v2 = &sceneVertexes[mesh->startSceneVertex + i + 2].xyz[0];
	
			vec3_t e1, e2, normal;
			VectorSubtract(v1, v0, e1);
			VectorSubtract(v2, v0, e2);
			CrossProduct(e1, e2, normal);
			VectorNormalize(normal);
	
			memcpy(sceneVertexes[mesh->startSceneVertex + i + 0].normal, normal, sizeof(float) * 3);
			memcpy(sceneVertexes[mesh->startSceneVertex + i + 1].normal, normal, sizeof(float) * 3);
			memcpy(sceneVertexes[mesh->startSceneVertex + i + 2].normal, normal, sizeof(float) * 3);
		}
	}
}

void *GL_LoadDXRMesh(msurface_t *surfaces, int numSurfaces)  {
	dxrMesh_t* mesh = new dxrMesh_t();
	
	mesh->meshId = dxrMeshList.size();
	
	GL_LoadBottomLevelAccelStruct(mesh, surfaces, numSurfaces);

	dxrMeshList.push_back(mesh);

	return mesh;
}

void GL_AliasVertexToDxrVertex(trivertx_t inVert, stvert_t stvert, dxrVertex_t &vertex, float x, float y, float w, float h) {
	memset(&vertex, 0, sizeof(dxrVertex_t));
	vertex.xyz[0] = inVert.v[0];
	vertex.xyz[1] = inVert.v[1];
	vertex.xyz[2] = inVert.v[2];
	vertex.st[0] = (stvert.s + 0.5) / w;
	vertex.st[1] = (stvert.t + 0.5) / h;
	vertex.st[2] = 0;

	//assert(vertex.st[0] > 1 || vertex.st[1] > 0);
	if(vertex.st[0] > 1 || vertex.st[1] > 1 || w == -1 || h == -1) {
		vertex.vtinfo[0] = -1;
		vertex.vtinfo[1] = -1;
		vertex.vtinfo[2] = -1;
		vertex.vtinfo[3] = -1;
	}
	else
	{
		vertex.vtinfo[0] = x;
		vertex.vtinfo[1] = y;
		vertex.vtinfo[2] = w;
		vertex.vtinfo[3] = h;
	}
}

void *GL_LoadDXRAliasMesh(const char* name, int numVertexes, trivertx_t* vertexes, int numTris, mtriangle_t* triangles, stvert_t* stverts) {
	dxrMesh_t* mesh = new dxrMesh_t();
	
	mesh->meshId = dxrMeshList.size();
	mesh->startSceneVertex = sceneVertexes.size();
	mesh->numSceneVertexes = 0;

	float x, y, w, h;
	GL_FindMegaTile(COM_SkipPath((char *)name), x, y, w, h);
	
	// TODO: Use a index buffer here : )
	for (int d = 0; d < numTris; d++)
	{
		{
			dxrVertex_t v;
			GL_AliasVertexToDxrVertex(vertexes[triangles[d].vertindex[0]], stverts[triangles[d].vertindex[0]], v, x, y, w, h);
			mesh->meshTriVertexes.push_back(v);
			sceneVertexes.push_back(v);
			mesh->numSceneVertexes++;
		}

		{
			dxrVertex_t v;
			GL_AliasVertexToDxrVertex(vertexes[triangles[d].vertindex[1]], stverts[triangles[d].vertindex[1]], v, x, y, w, h);
			mesh->meshTriVertexes.push_back(v);
			sceneVertexes.push_back(v);
			mesh->numSceneVertexes++;
		}

		{
			dxrVertex_t v;
			GL_AliasVertexToDxrVertex(vertexes[triangles[d].vertindex[2]], stverts[triangles[d].vertindex[2]], v, x, y, w, h);
			mesh->meshTriVertexes.push_back(v);
			sceneVertexes.push_back(v);
			mesh->numSceneVertexes++;
		}
	}

	dxrMeshList.push_back(mesh);

	return mesh;
}

void GL_FinishVertexBufferAllocation(void) {
//	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	// Create the vertex buffer.
	{
		const UINT vertexBufferSize = sizeof(dxrVertex_t) * sceneVertexes.size();

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, &sceneVertexes[0], sizeof(dxrVertex_t) * sceneVertexes.size());
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(dxrVertex_t);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	for(int i = 0; i < dxrMeshList.size(); i++)
	{
		dxrMesh_t* mesh = dxrMeshList[i];

		nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;
		bottomLevelAS.AddVertexBuffer(m_vertexBuffer.Get(), mesh->startSceneVertex * sizeof(dxrVertex_t), mesh->numSceneVertexes, sizeof(dxrVertex_t), NULL, 0);

		// Adding all vertex buffers and not transforming their position.
		//for (const auto& buffer : vVertexBuffers) {
		//	bottomLevelAS.AddVertexBuffer(buffer.first.Get(), 0, buffer.second,
		//		sizeof(Vertex), 0, 0);
		//}

		// The AS build requires some scratch space to store temporary information.
		// The amount of scratch memory is dependent on the scene complexity.
		UINT64 scratchSizeInBytes = 0;
		// The final AS also needs to be stored in addition to the existing vertex
		// buffers. It size is also dependent on the scene complexity.
		UINT64 resultSizeInBytes = 0;

		bottomLevelAS.ComputeASBufferSizes(m_device.Get(), false, &scratchSizeInBytes,
			&resultSizeInBytes);

		// Once the sizes are obtained, the application is responsible for allocating
		// the necessary buffers. Since the entire generation will be done on the GPU,
		// we can directly allocate those on the default heap	
		mesh->buffers.pScratch = nv_helpers_dx12::CreateBuffer(m_device.Get(), scratchSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, nv_helpers_dx12::kDefaultHeapProps);
		mesh->buffers.pResult = nv_helpers_dx12::CreateBuffer(m_device.Get(), resultSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nv_helpers_dx12::kDefaultHeapProps);

		// Build the acceleration structure. Note that this call integrates a barrier
		// on the generated AS, so that it can be used to compute a top-level AS right
		// after this method.

		bottomLevelAS.Generate(m_commandList.Get(), mesh->buffers.pScratch.Get(), mesh->buffers.pResult.Get(), false, nullptr);
	}

	// Flush the command list and wait for it to finish
	//m_commandList->Close();
	//ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	//m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
	//m_fenceValue++;
	//m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
	//
	//m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
	//WaitForSingleObject(m_fenceEvent, INFINITE);
}