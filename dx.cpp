#include "dx.hpp"

namespace ino::d3d
{

#ifdef _DEBUG
::Microsoft::WRL::ComPtr<ID3D12Debug> d3dDebuger = nullptr;
#endif
::Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
::Microsoft::WRL::ComPtr<ID3D12Device2> device = nullptr;
::Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets[num_swap_buffers] = {};
::Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[num_swap_buffers] = {};
UINT currentBackBufferIndex = 0;
::Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderTargetDescriptorHeap = nullptr;
UINT renderTargetDescriptorSize = 0;
#ifdef USE_STENCIL_BUFFER
::Microsoft::WRL::ComPtr<ID3D12Resource> stencilBuffer;
::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> stencilDescriptorHeap;
UINT stencilDescriptorSize;
#endif


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
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&

				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				hr = dxgiAdapter1.As(&dxgiAdapter4);
			}
		}
	}

	return dxgiAdapter4;
}

::Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice(::Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
{
	::Microsoft::WRL::ComPtr<ID3D12Device2> d3d12Device2;

	//UUID experimentalFeatures[] = { D3D12ExperimentalShaderModels,D3D12RaytracingPrototype };
	//bool supportsDXR = D3D12EnableExperimentalFeatures(2, experimentalFeatures, NULL, NULL) == S_OK;
	HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&d3d12Device2));

	// Enable debug messages in debug mode.
#if defined(_DEBUG)
	::Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
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
	return d3d12Device2;
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
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
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
	::Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain,
	::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < num_swap_buffers; ++i)
	{
		::Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;

		swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
		device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		renderTargets[i] = backBuffer;

		rtvHandle.ptr += renderTargetDescriptorSize;
	}
}

HRESULT init(HWND hwnd, bool useWarp, int width, int height)
{
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

	renderTargetDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, num_swap_buffers);
	renderTargetDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CreateRenderTargetViews(device, swapChain, renderTargetDescriptorHeap);
#ifdef USE_STENCIL_BUFFER
	//stencil buffer
	stencilDescriptorHeap = CreateDescriptorHeap(device,D3D12_DESCRIPTOR_HEAP_TYPE_DSV,1);
	stencilDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	D3D12_CLEAR_VALUE clearValue = { 
		.Format = DXGI_FORMAT_D32_FLOAT,
		.DepthStencil = { .Depth = 1.f,.Stencil = 0 }
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

	device->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&stencilBuffer)
	);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {
		.Format = DXGI_FORMAT_D32_FLOAT,
		.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
		.Flags = D3D12_DSV_FLAG_NONE
	};
	device->CreateDepthStencilView(
		stencilBuffer.Get(),
		&dsvDesc,
		stencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
#endif

	for (int i = 0; i < num_swap_buffers; ++i)
	{
		//bit_cast<int>(a);
		result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i]));
		if (!SUCCEEDED(result))return result;
	}

	result = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	return result;
}

void flush(ID3D12GraphicsCommandList** commandLists, UINT pipe_count)
{
	HRESULT result;

	commandQueue->ExecuteCommandLists(pipe_count, reinterpret_cast<ID3D12CommandList* const*>(commandLists));
	result = commandQueue->Signal(fence.Get(), ++frameFenceValues[currentBackBufferIndex]);

	UINT syncInterval = 1;// g_VSync ? 1 : 0;
	UINT presentFlags = 0;// allowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0;

	result = swapChain->Present(syncInterval, presentFlags);

	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

	//wait
	static const DWORD timeout = 10000;
	if (fence->GetCompletedValue() < frameFenceValues[currentBackBufferIndex])
	{
		result = fence->SetEventOnCompletion(frameFenceValues[currentBackBufferIndex], fenceEvent);
		::WaitForSingleObject(fenceEvent, timeout);
	}
	commandAllocators[currentBackBufferIndex]->Reset();
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