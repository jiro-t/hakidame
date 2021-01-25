#ifndef INO_D3D_INCLUDED
#define INO_D3D_INCLUDED

//D3D HEADERS
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
//Com
#include <wrl.h>
//ARGV
#include <shellapi.h>

#ifdef _DEBUG
#include <iostream>
#endif
#include <string>

static const int screen_width = 800;
static const int screen_height = 600;
static const int num_swap_buffers = 2;

#define USE_STENCIL_BUFFER

namespace ino::d3d
{

#ifdef _DEBUG
extern	::Microsoft::WRL::ComPtr<ID3D12Debug> d3dDebuger;
#endif
extern	::Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
extern	::Microsoft::WRL::ComPtr<ID3D12Device2> device;
extern	::Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets[num_swap_buffers];
extern	::Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[num_swap_buffers];
extern	UINT currentBackBufferIndex;
extern	::Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
extern	::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderTargetDescriptorHeap;
extern	UINT renderTargetDescriptorSize;
#ifdef USE_STENCIL_BUFFER
extern	::Microsoft::WRL::ComPtr<ID3D12Resource> stencilBuffer;
extern	::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> stencilDescriptorHeap;
extern	UINT stencilDescriptorSize;
#endif

extern	::Microsoft::WRL::ComPtr<ID3D12Fence> fence;
extern	uint64_t fenceValue;
extern	uint64_t frameFenceValues[num_swap_buffers];
extern	HANDLE fenceEvent;

extern	BOOL allowTearing;

	inline void putErrorMsg(::Microsoft::WRL::ComPtr<ID3DBlob> error)
	{
#ifdef _DEBUG
		if (error)
		{
			char* str = reinterpret_cast<char*>(error->GetBufferPointer());
			std::cout << str << std::endl;
			error->Release();
		}
#endif
	}

	HRESULT init(HWND hwnd, bool useWarp, int width, int height);
	void release();
	void flush(ID3D12GraphicsCommandList** commandLists, UINT pipe_count);

	::Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);
	::Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice(::Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter);

	inline bool CheckDXRSupport(::Microsoft::WRL::ComPtr<ID3D12Device2> device)
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 features = {};
		device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features, sizeof(features));
		if (features.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
		{
			return true;
		}

		return false;
	}

	::Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(
		::Microsoft::WRL::ComPtr<ID3D12Device2> device,
		D3D12_COMMAND_LIST_TYPE type);

	::Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(
		HWND hWnd,
		::Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
		uint32_t width, uint32_t height, uint32_t bufferCount);

	::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		::Microsoft::WRL::ComPtr<ID3D12Device2> device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		uint32_t numDescriptors);

	void CreateRenderTargetViews(
		::Microsoft::WRL::ComPtr<ID3D12Device2> device,
		::Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain,
		::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap);

	enum class ShaderTypes {
		VERTEX_SHADER,
		FRAGMENT_SHADER,
		GEOMETORY_SHADER,
		DOMAIN_SHADER,
		HULL_SHADER,
		UNDEFINED
	};

	/*
	Resize
	// resize the swap chain
	swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
	// (re)-create the render target view
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()))))
		return std::runtime_error("Direct3D was unable to acquire the back buffer!");
	if (FAILED(dev->CreateRenderTargetView(backBuffer.Get(), NULL, &renderTargetView)))
		return std::runtime_error("Direct3D was unable to create the render target view!");
	*/

	/*
		//...........DXR
		struct AccelerationStructureBuffers
		{
			::Microsoft::WRL::ComPtr<ID3D12Resource> pScratch;      // Scratch memory for AS builder
			::Microsoft::WRL::ComPtr<ID3D12Resource> pResult;       // Where the AS is
			::Microsoft::WRL::ComPtr<ID3D12Resource> pInstanceDesc; // Hold the matrices of the instances
		};

	AccelerationStructureBuffers D3D12HelloTriangle::CreateBottomLevelAS(vbo verts[]) {
			nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;

			// Adding all vertex buffers and not transforming their position.
			for (const auto& buffer : verts) {
				bottomLevelAS.AddVertexBuffer(buffer.first.Get(), 0, buffer.second,sizeof(verts), 0, 0);
			}

			// The AS build requires some scratch space to store temporary information.
			// The amount of scratch memory is dependent on the scene complexity.
			UINT64 scratchSizeInBytes = 0;
			// The final AS also needs to be stored in addition to the existing vertex
			// buffers. It size is also dependent on the scene complexity.
			UINT64 resultSizeInBytes = 0;

			bottomLevelAS.ComputeASBufferSizes(device.Get(), false, &scratchSizeInBytes,
				&resultSizeInBytes);

			// Once the sizes are obtained, the application is responsible for allocating
			// the necessary buffers. Since the entire generation will be done on the GPU,
			// we can directly allocate those on the default heap
			AccelerationStructureBuffers buffers;
			buffers.pScratch = nv_helpers_dx12::CreateBuffer(
				m_device.Get(), scratchSizeInBytes,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
				nv_helpers_dx12::kDefaultHeapProps);
			buffers.pResult = nv_helpers_dx12::CreateBuffer(
				m_device.Get(), resultSizeInBytes,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
				nv_helpers_dx12::kDefaultHeapProps);

			// Build the acceleration structure. Note that this call integrates a barrier
			// on the generated AS, so that it can be used to compute a top-level AS right
			// after this method.
			bottomLevelAS.Generate(m_commandList.Get(), buffers.pScratch.Get(),
				buffers.pResult.Get(), false, nullptr);

			return buffers;
		}
	}
	*/
}

#endif