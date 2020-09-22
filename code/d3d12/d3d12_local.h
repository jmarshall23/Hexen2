// d3d12_local.h
//

#pragma once

#include "d3dx12.h"
#include <wrl/client.h>
#include <dxgi1_4.h>
#include "DXRHelper.h"

extern "C" {
	#include "../quakedef.h"
	#include "../winquake.h"
	#include "../resource.h"
};

using namespace Microsoft::WRL;

#include <exception>

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
_Use_decl_annotations_
inline void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = adapter.Detach();
}

struct dxrVertex_t {
	vec3_t xyz;
	vec2_t st;
	vec3_t normal;
};

struct dxrSurface_t {
	int startVertex;
	int numVertexes;

	int startMegaVertex;
	int startIndex;
	int numIndexes;
};

struct AccelerationStructureBuffers
{
	ComPtr<ID3D12Resource> pScratch;      // Scratch memory for AS builder
	ComPtr<ID3D12Resource> pResult;       // Where the AS is
	ComPtr<ID3D12Resource> pInstanceDesc; // Hold the matrices of the instances
};

struct dxrMesh_t {
	std::vector<dxrVertex_t> meshVertexes;
	std::vector<dxrVertex_t> meshTriVertexes;
	std::vector<int> meshIndexes;
	std::vector<dxrSurface_t> meshSurfaces;

	int startSceneVertex;
	int numSceneVertexes;

	AccelerationStructureBuffers buffers;
};

void GL_FinishVertexBufferAllocation(void);

const int FrameCount = 3;

extern ComPtr<IDXGISwapChain3> m_swapChain;
extern ComPtr<ID3D12Device5> m_device;
extern ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
extern ComPtr<ID3D12CommandAllocator> m_commandAllocator;
extern ComPtr<ID3D12CommandQueue> m_commandQueue;
extern ComPtr<ID3D12RootSignature> m_rootSignature;
extern ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
extern ComPtr<ID3D12PipelineState> m_pipelineState;
extern ComPtr<ID3D12GraphicsCommandList4> m_commandList;

extern HANDLE m_fenceEvent;
extern ComPtr<ID3D12Fence> m_fence;
extern UINT64 m_fenceValue;

extern ComPtr<ID3D12Resource> m_vertexBuffer;

void GL_CreateTopLevelAccelerationStructs(void);