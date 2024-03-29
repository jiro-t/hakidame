﻿#include "dx.hpp"
#include "dx12resource.hpp"

#include <filesystem>

#include "..\resource.h"
#include <fstream>

//LPDxcCreateInstance dllDxcCreateInstance;

namespace ino::d3d
{

int screen_width = 800;
int screen_height = 600;

#ifdef _DEBUG
::Microsoft::WRL::ComPtr<ID3D12Debug> d3dDebuger = nullptr;
#endif
::Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
::Microsoft::WRL::ComPtr<ID3D12Device5> device = nullptr;
texture renderTargets[num_swap_buffers];
::Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[num_swap_buffers] = {};
UINT currentBackBufferIndex = 0;
::Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;

::Microsoft::WRL::ComPtr<IDxcLibrary> lib;
::Microsoft::WRL::ComPtr<IDxcCompiler> compiler;

texture depthBuffer[num_swap_buffers];


::Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
uint64_t fenceValue = 0;
uint64_t frameFenceValues[num_swap_buffers] = {};
HANDLE fenceEvent = 0;

BOOL allowTearing = FALSE;

::Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp)
{
	::Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;

#ifdef _DEBUG
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	HRESULT hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));

	::Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter1;
	::Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter4;

	if (useWarp)
	{
		hr = dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1));
		hr = dxgiAdapter1.As(&dxgiAdapter4);
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)) &&

				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				hr = dxgiAdapter1.As(&dxgiAdapter4);
			}
		}
	}

	return dxgiAdapter4;
}

::Microsoft::WRL::ComPtr<ID3D12Device5> CreateDevice(::Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
{
	::Microsoft::WRL::ComPtr<ID3D12Device5> d3d12Device5;

	UUID experimentalFeatures[] = { D3D12ExperimentalShaderModels/*,D3D12RaytracingPrototype*/ };
	bool ext = D3D12EnableExperimentalFeatures(1, experimentalFeatures, NULL, NULL) == S_OK;
	//if (ext == false)
	//	return nullptr;

	HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&d3d12Device5));

	// Enable debug messages in debug mode.
#if defined(_DEBUG)
	::Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3d12Device5.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {
			.DenyList = {
				.NumSeverities = _countof(Severities),
				.pSeverityList = Severities,
				.NumIDs = _countof(DenyIds),
				.pIDList = DenyIds,
			},
		};

		hr = pInfoQueue->PushStorageFilter(&NewFilter);
	}
#endif
	return d3d12Device5;
}

::Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(
	::Microsoft::WRL::ComPtr<ID3D12Device2> device,
	D3D12_COMMAND_LIST_TYPE type)
{
	::Microsoft::WRL::ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

	D3D12_COMMAND_QUEUE_DESC desc = {
		.Type = type,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
		.NodeMask = 0,
	};

	HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue));

	return d3d12CommandQueue;
}

::Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(
	HWND hWnd,
	::Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
	uint32_t width, uint32_t height, uint32_t bufferCount)
{
	::Microsoft::WRL::ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	::Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	HRESULT result = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
		.Width = width,
		.Height = height,
		.Format = rtvFormat,
		.Stereo = FALSE,
		.SampleDesc = { 1, 0 },
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = bufferCount,
		.Scaling = DXGI_SCALING_STRETCH,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
	};
	// It is recommended to always allow tearing if tearing support is available.
	//DXGI_FEATURE_PRESENT_ALLOW_TEARING
	::Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
	if (CreateDXGIFactory1(IID_PPV_ARGS(&factory5)) >= 0)
		factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));

	swapChainDesc.Flags = allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	::Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
	result = dxgiFactory4->CreateSwapChainForHwnd(commandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
	//result = dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	result = swapChain1.As(&dxgiSwapChain4);

	return dxgiSwapChain4;
}

::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
	::Microsoft::WRL::ComPtr<ID3D12Device2> device,
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	uint32_t numDescriptors)
{
	::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;

	HRESULT result = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));

	return descriptorHeap;
}

void CreateRenderTargetViews(
	::Microsoft::WRL::ComPtr<ID3D12Device2> device,
	::Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain)
{
	for (int i = 0; i < num_swap_buffers; ++i)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(renderTargets[i].GetCpuHeapHandle());

		::Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;

		swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
		device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		renderTargets[i].GetHandle() = backBuffer;
	}
}

//void outputDll(UINT idr,std::wstring filename)
//{
//	HRSRC resInfo = FindResource(0,MAKEINTRESOURCE(idr),L"DLL");
//	HGLOBAL resData = LoadResource(0, resInfo);
//	LPVOID pvResData = LockResource(resData);
//
//	std::wstring path = std::filesystem::current_path();
//	if (std::filesystem::exists(path + filename))
//		return;
//
//	std::ofstream ofs(path + filename, std::ios::binary);
//	DWORD size = SizeofResource(0, resInfo);
//	ofs.write((char*)pvResData, size);
//	ofs.close();
//
//	UnlockResource(resData);
//	FreeResource(resData);
//
//	return;
//}

HRESULT init(HWND hwnd, bool useWarp, int width, int height)
{
	////write out dll files
	//outputDll(IDR_DLL1,L"\\dxcompiler.dll");
	//outputDll(IDR_DLL2, L"\\dxil.dll");
	////Load dxil.dll dxcompiler.dll
	//HMODULE module = LoadLibrary(L"dxcompiler.dll");
	//if (module == NULL)
	//	return S_FALSE;
	//dllDxcCreateInstance = reinterpret_cast<LPDxcCreateInstance>(GetProcAddress(module,"DxcCreateInstance"));
	//BOOL FreeLibrary(
	//	HMODULE hModule   // DLLモジュールのハンドル
	//);
	HRESULT result = NULL;
#ifdef _DEBUG
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&d3dDebuger));
	if (FAILED(result))
	{
		return result;
	}
	d3dDebuger->EnableDebugLayer();
#endif

	::Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(useWarp);

	device = CreateDevice(dxgiAdapter4);
	commandQueue = CreateCommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	swapChain = CreateSwapChain(hwnd, commandQueue, width, height, num_swap_buffers);
	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

	for (std::size_t i = 0; i < num_swap_buffers; ++i)
		renderTargets[i].GetHeap() = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
	CreateRenderTargetViews(device, swapChain);

	//stencil buffer
	for (std::size_t i = 0; i < num_swap_buffers; ++i)
	{
		depthBuffer[i].GetHeap() = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
		D3D12_CLEAR_VALUE clearValue = {
			.Format = DXGI_FORMAT_D32_FLOAT,
			.DepthStencil = {.Depth = 1.f,.Stencil = 0 }
		};
		D3D12_HEAP_PROPERTIES prop = {
			.Type = D3D12_HEAP_TYPE_DEFAULT,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1
		};
		D3D12_RESOURCE_DESC desc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = 0,
			.Width = static_cast<UINT64>(width),
			.Height = static_cast<UINT64>(height),
			.DepthOrArraySize = 1,
			.MipLevels = 0,
			.Format = DXGI_FORMAT_D32_FLOAT,
			.SampleDesc = {.Count = 1,.Quality = 0},
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
		};

		result = device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&depthBuffer[i].GetHandle())
		);

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {
			.Format = DXGI_FORMAT_D32_FLOAT,
			.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
			.Flags = D3D12_DSV_FLAG_NONE
		};
		device->CreateDepthStencilView(
			depthBuffer[i].GetHandle().Get(),
			&dsvDesc,
			depthBuffer[i].GetCpuHeapHandle());
	}

	for (int i = 0; i < num_swap_buffers; ++i)
	{
		result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i]));
		if (!SUCCEEDED(result))return result;
	}

	result = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	screen_width = width;
	screen_height = height;

	DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), &lib);
	DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), &compiler);

	return result;
}

void wait()
{
	// Schedule a Signal command in the GPU queue.
	UINT64 fenceValue = frameFenceValues[currentBackBufferIndex];
	if (SUCCEEDED(commandQueue->Signal(fence.Get(), fenceValue)))
	{
		// Wait until the Signal has been processed.
		if (SUCCEEDED(fence->SetEventOnCompletion(fenceValue, fenceEvent)))
		{
			WaitForSingleObjectEx(fence.Get(), INFINITE, FALSE);

			// Increment the fence value for the current frame.
			frameFenceValues[currentBackBufferIndex]++;
		}
	}
}

void excute(ID3D12GraphicsCommandList** commandLists, UINT pipe_count)
{
	HRESULT result = S_OK;

	commandQueue->ExecuteCommandLists(pipe_count, reinterpret_cast<ID3D12CommandList* const*>(commandLists));
	result = commandQueue->Signal(fence.Get(), ++frameFenceValues[currentBackBufferIndex]);
}

void begin()
{
	commandAllocators[currentBackBufferIndex]->Reset();
}

void end()
{
	HRESULT result = S_OK;

	UINT syncInterval = 1;// g_VSync ? 1 : 0;
	UINT presentFlags = 0;// allowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0;

	result = swapChain->Present(syncInterval, presentFlags);
	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();
}

void release()
{
	HRESULT result = NULL;
	//signal
	result = commandQueue->Signal(fence.Get(), ++fenceValue);

	//wait
	static const DWORD timeOut = 10000;
	if (fence->GetCompletedValue() < fenceValue)
	{
		result = fence->SetEventOnCompletion(fenceValue, fenceEvent);
		::WaitForSingleObject(fenceEvent, timeOut);
	}
	::CloseHandle(fenceEvent);
}

}