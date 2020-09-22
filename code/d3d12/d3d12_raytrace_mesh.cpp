// d3d12_raytrace_mesh.cpp
//

#include "d3d12_local.h"
#include "nv_helpers_dx12/BottomLevelASGenerator.h"
#include "nv_helpers_dx12/TopLevelASGenerator.h"
#include <vector>

nv_helpers_dx12::TopLevelASGenerator m_topLevelASGenerator;
std::vector<dxrMesh_t *> dxrMeshList;

AccelerationStructureBuffers m_topLevelASBuffers;

void GL_CreateTopLevelAccelerationStructs(void) {
	DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity();

	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	for (int i = 0; i < dxrMeshList.size(); i++)
	{
		m_topLevelASGenerator.AddInstance(dxrMeshList[i]->buffers.pResult.Get(), matrix, i, 0);
	}

	// As for the bottom-level AS, the building the AS requires some scratch space
	// to store temporary data in addition to the actual AS. In the case of the
	// top-level AS, the instance descriptors also need to be stored in GPU
	// memory. This call outputs the memory requirements for each (scratch,
	// results, instance descriptors) so that the application can allocate the
	// corresponding memory
	UINT64 scratchSize, resultSize, instanceDescsSize;

	m_topLevelASGenerator.ComputeASBufferSizes(m_device.Get(), true, &scratchSize,
		&resultSize, &instanceDescsSize);

	// Create the scratch and result buffers. Since the build is all done on GPU,
	// those can be allocated on the default heap
	m_topLevelASBuffers.pScratch = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nv_helpers_dx12::kDefaultHeapProps);
	m_topLevelASBuffers.pResult = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps);

	// The buffer describing the instances: ID, shader binding information,
	// matrices ... Those will be copied into the buffer by the helper through
	// mapping, so the buffer has to be allocated on the upload heap.
	m_topLevelASBuffers.pInstanceDesc = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), instanceDescsSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	// After all the buffers are allocated, or if only an update is required, we
	// can build the acceleration structure. Note that in the case of the update
	// we also pass the existing AS as the 'previous' AS, so that it can be
	// refitted in place.
	m_topLevelASGenerator.Generate(m_commandList.Get(),
		m_topLevelASBuffers.pScratch.Get(),
		m_topLevelASBuffers.pResult.Get(),
		m_topLevelASBuffers.pInstanceDesc.Get());

	// Flush the command list and wait for it to finish
	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
	m_fenceValue++;
	m_commandQueue->Signal(m_fence.Get(), m_fenceValue);

	m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
	WaitForSingleObject(m_fenceEvent, INFINITE);
}

void GL_LoadBottomLevelAccelStruct(dxrMesh_t* mesh, msurface_t* surfaces, int numSurfaces) {
	glpoly_t* p;

	for (int i = 0; i < numSurfaces; i++)
	{
		msurface_t* fa = &surfaces[i];
		dxrSurface_t surf;

		BuildSurfaceDisplayList(fa);

		surf.startVertex = mesh->meshVertexes.size();
		surf.numVertexes = 0;
		for (p = fa->polys; p; p = p->next) {
			for (int d = 0; d < p->numverts; d++) {
				dxrVertex_t v;
		
				v.xyz[0] = p->verts[d][0];
				v.xyz[1] = p->verts[d][1];
				v.xyz[2] = p->verts[d][2];
				v.st[0] = p->verts[d][0];
				v.st[1] = p->verts[d][1];
		
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
			}
		}
	}

	// Create the vertex buffer.
	{
		const UINT vertexBufferSize = sizeof(dxrVertex_t) * mesh->meshTriVertexes.size();

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
			IID_PPV_ARGS(&mesh->m_vertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		ThrowIfFailed(mesh->m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, &mesh->meshTriVertexes[0], sizeof(dxrVertex_t) * mesh->meshTriVertexes.size());
		mesh->m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		mesh->m_vertexBufferView.BufferLocation = mesh->m_vertexBuffer->GetGPUVirtualAddress();
		mesh->m_vertexBufferView.StrideInBytes = sizeof(dxrVertex_t);
		mesh->m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}


	nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;
	bottomLevelAS.AddVertexBuffer(mesh->m_vertexBuffer.Get(), 0, mesh->meshTriVertexes.size(), sizeof(dxrVertex_t), NULL, 0);

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

void *GL_LoadDXRMesh(msurface_t *surfaces, int numSurfaces)  {
	dxrMesh_t* mesh = new dxrMesh_t();

	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	GL_LoadBottomLevelAccelStruct(mesh, surfaces, numSurfaces);

	// Flush the command list and wait for it to finish
	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
	m_fenceValue++;
	m_commandQueue->Signal(m_fence.Get(), m_fenceValue);

	m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
	WaitForSingleObject(m_fenceEvent, INFINITE);

	dxrMeshList.push_back(mesh);

	return mesh;
}