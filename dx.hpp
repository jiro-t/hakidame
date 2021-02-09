#ifndef INO_D3D_INCLUDED
#define INO_D3D_INCLUDED

//D3D HEADERS
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

//DX Math
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
class texture;
#ifdef _DEBUG
extern	::Microsoft::WRL::ComPtr<ID3D12Debug> d3dDebuger;
#endif
extern	::Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
extern	::Microsoft::WRL::ComPtr<ID3D12Device2> device;
extern	::Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[num_swap_buffers+1/*DXR Allocator*/];
extern	UINT currentBackBufferIndex;
extern	::Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
extern texture renderTargets[num_swap_buffers];
#ifdef USE_STENCIL_BUFFER
extern texture	stencilBuffer;
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

enum class ShaderTypes {
	VERTEX_SHADER,
	FRAGMENT_SHADER,
	GEOMETORY_SHADER,
	DOMAIN_SHADER,
	HULL_SHADER,
	UNDEFINED
};

}

#endif