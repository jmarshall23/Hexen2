// d3d12_raytrace_entities.cpp
//

#include "d3d12_local.h"
#include "nv_helpers_dx12/BottomLevelASGenerator.h"
#include "nv_helpers_dx12/TopLevelASGenerator.h"
#include <vector>

nv_helpers_dx12::TopLevelASGenerator m_topLevelASGenerator;
AccelerationStructureBuffers m_topLevelASBuffers;

std::vector<entity_t*> r_dxrEntities;

int r_currentDxrEntities = -1;

void GL_CreateTopLevelAccelerationStructs(void) {
	// Add in the BSP world.
	{
		DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity();
		m_topLevelASGenerator.AddInstance(dxrMeshList[0]->buffers.pResult.Get(), matrix, 0, 0);
	}

	// Add in the entities.
	int numProcessedEntities = 1;

	for (int i = 0; i < cl_numvisedicts; i++)
	{
		entity_t *currententity = cl_visedicts[i];

		dxrMesh_t* mesh = (dxrMesh_t*)currententity->model->dxrModel;

		switch (currententity->model->type)
		{
			case mod_alias:
				create_entity_matrix(&currententity->dxrTransform[0], currententity, false);
				m_topLevelASGenerator.AddInstance(mesh->buffers.pResult.Get(), (DirectX::XMMATRIX &)currententity->dxrTransform, i + 1, 0);
				r_currentDxrEntities++;
				break;
		}
	}

	bool onlyUpdate = (numProcessedEntities == r_currentDxrEntities);
	r_currentDxrEntities = numProcessedEntities;
		

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
		m_topLevelASBuffers.pInstanceDesc.Get(), onlyUpdate, m_topLevelASBuffers.pResult.Get());
}