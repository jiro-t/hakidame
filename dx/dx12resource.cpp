#include "dx12resource.hpp"

namespace ino::d3d {

::Microsoft::WRL::ComPtr<ID3D12Resource> CreateResource(UINT64 bufferSize, D3D12_RESOURCE_STATES initialResourceState , D3D12_RESOURCE_FLAGS Flags)
{
	D3D12_HEAP_PROPERTIES heapProp = {
		.Type = D3D12_HEAP_TYPE_DEFAULT,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 1,
		.VisibleNodeMask = 1
	};

	D3D12_RESOURCE_DESC resourceDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = bufferSize,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {
			.Count = 1,
			.Quality = 0,
		},
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = Flags
	};

	::Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
	HRESULT hr = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		initialResourceState,
		nullptr,
		IID_PPV_ARGS(&buffer));
	if (hr != S_OK)
		return FALSE;

	return buffer;
}

void texture::Create(UINT width, UINT height,DXGI_FORMAT format,D3D12_RESOURCE_FLAGS flag) {
	D3D12_HEAP_PROPERTIES heapProp = {
		.Type = D3D12_HEAP_TYPE_CUSTOM,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_L0,
		.CreationNodeMask = 0,
		.VisibleNodeMask = 0
	};

	D3D12_RESOURCE_DESC resourceDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Width = width,
		.Height = height,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = format,
		.SampleDesc = {.Count = 1,.Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN , 
		.Flags = flag
	};

	device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
#ifdef _DEBUG
	buffer->SetName(L"texture");
#endif

	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	};
	device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&heap));


	D3D12_CPU_DESCRIPTOR_HANDLE handle_srv{};
	D3D12_SHADER_RESOURCE_VIEW_DESC resourct_view_desc{};

	resourct_view_desc.Format = format;
	resourct_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	resourct_view_desc.Texture2D.MipLevels = 1;
	resourct_view_desc.Texture2D.MostDetailedMip = 0;
	resourct_view_desc.Texture2D.PlaneSlice = 0;
	resourct_view_desc.Texture2D.ResourceMinLODClamp = 0.0F;
	resourct_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	handle_srv = heap->GetCPUDescriptorHandleForHeapStart();
	device->CreateShaderResourceView(buffer.Get(), &resourct_view_desc, handle_srv);
}

void texture::Map(void* src, UINT width, UINT height, UINT num_channel)
{
	D3D12_BOX box = { 0, 0, 0, width, height, 1 };
	buffer->WriteToSubresource(0, &box, src, num_channel * width, num_channel * width * height);
}

void texture::Set(ID3D12GraphicsCommandList* cmdList, UINT reg_id) {
	ID3D12DescriptorHeap* heaps[] = { heap.Get() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	cmdList->SetGraphicsRootDescriptorTable(reg_id, heap->GetGPUDescriptorHandleForHeapStart());
}

void renderTexture::Create(UINT width, UINT height, D3D12_RESOURCE_FLAGS flag)
{
	D3D12_HEAP_PROPERTIES heapProp = {
		.Type = D3D12_HEAP_TYPE_CUSTOM,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_L0,
		.CreationNodeMask = 0,
		.VisibleNodeMask = 0
	};

	D3D12_RESOURCE_DESC resourceDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Width = width,
		.Height = height,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = rtvFormat,
		.SampleDesc = {.Count = 1,.Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN ,
		.Flags = flag
	};
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};
	device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ,
		nullptr, IID_PPV_ARGS(&buffer));
	device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&renderTargetHeap));

	resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	D3D12_CLEAR_VALUE clearValue = {
		.Format = DXGI_FORMAT_D32_FLOAT,
		.DepthStencil = {.Depth = 1.f,.Stencil = 0 }
	};
	device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue, IID_PPV_ARGS(&depthBuffer));
	device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&depthHeap));
#ifdef _DEBUG
	buffer->SetName(L"offscreen buffer");
#endif

	rtvHandle = renderTargetHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = rtvFormat;
	device->CreateRenderTargetView(buffer.Get(), &rtvDesc, rtvHandle);

	D3D12_DEPTH_STENCIL_VIEW_DESC depthDesc = {};
	depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthDesc.Flags = D3D12_DSV_FLAG_NONE;
	device->CreateDepthStencilView(depthBuffer.Get(), &depthDesc, depthHeap->GetCPUDescriptorHandleForHeapStart() );

	scissor.left = 0;
	scissor.right = width;
	scissor.top = 0;
	scissor.bottom = height;

	//rendered resource
	descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptor_heap_desc.NumDescriptors = 1;
	descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&afterHeap));

	D3D12_SHADER_RESOURCE_VIEW_DESC resourct_view_desc{};
	resourct_view_desc.Format = rtvFormat;
	resourct_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	resourct_view_desc.Texture2D.MipLevels = 1;
	resourct_view_desc.Texture2D.MostDetailedMip = 0;
	resourct_view_desc.Texture2D.PlaneSlice = 0;
	resourct_view_desc.Texture2D.ResourceMinLODClamp = 0.0F;
	resourct_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(buffer.Get(), &resourct_view_desc, afterHeap->GetCPUDescriptorHandleForHeapStart());
}

void renderTexture::Set(ID3D12GraphicsCommandList* cmdList, UINT reg_id)
{
	ID3D12DescriptorHeap* heaps[] = { afterHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	cmdList->SetGraphicsRootDescriptorTable(reg_id, afterHeap->GetGPUDescriptorHandleForHeapStart());
}

void renderTexture::RenderTarget(ID3D12GraphicsCommandList* cmdList)
{
	D3D12_CPU_DESCRIPTOR_HANDLE scvHandle = D3D12_CPU_DESCRIPTOR_HANDLE(depthHeap->GetCPUDescriptorHandleForHeapStart());

	cmdList->RSSetScissorRects(1, &scissor);
	cmdList->OMSetRenderTargets(1, &rtvHandle, false, &scvHandle);
}

void ibo::Create(const uint32_t* indecies, UINT byteSize) {

	D3D12_HEAP_PROPERTIES prop = {
		.Type = D3D12_HEAP_TYPE_UPLOAD,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 1,
		.VisibleNodeMask = 1,
	};

	D3D12_RESOURCE_DESC desc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = byteSize,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {
			.Count = 1,
			.Quality = 0,
		},
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	};

	device->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer)
	);
	//buffer = CreateResource(byteSize,D3D12_RESOURCE_STATE_GENERIC_READ);
#ifdef _DEBUG
	buffer->SetName(L"index buffer");
#endif

	UINT8* pIndexDataBegin;
	D3D12_RANGE readRange;// We do not intend to read from this resource on the CPU.
	readRange.Begin = 0;
	readRange.End = 0;
	buffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
	memcpy(pIndexDataBegin, indecies, byteSize);
	buffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	view.BufferLocation = buffer->GetGPUVirtualAddress();
	view.Format = DXGI_FORMAT_R32_UINT;
	view.SizeInBytes = byteSize;

	index_count = byteSize / sizeof(uint32_t);
}

void ibo::Draw(ID3D12GraphicsCommandList* cmdList) const
{
	cmdList->IASetIndexBuffer(&view);
	cmdList->DrawIndexedInstanced(index_count,1,0,0,0);
}

void vbo::Create(const void* verts, UINT stride, UINT byteSize) {

	D3D12_HEAP_PROPERTIES prop = {
		.Type = D3D12_HEAP_TYPE_UPLOAD,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 1,
		.VisibleNodeMask = 1,
	};

	D3D12_RESOURCE_DESC desc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = byteSize,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {
			.Count = 1,
			.Quality = 0,
		},
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	};

	device->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer)
	);
	//buffer = CreateResource(byteSize,D3D12_RESOURCE_STATE_GENERIC_READ);
#ifdef _DEBUG
	buffer->SetName(L"vertex buffer");
#endif

	UINT8* pVertexDataBegin;
	D3D12_RANGE readRange;// We do not intend to read from this resource on the CPU.
	readRange.Begin = 0;
	readRange.End = 0;
	buffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	memcpy(pVertexDataBegin, verts, byteSize);
	buffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	view.BufferLocation = buffer->GetGPUVirtualAddress();
	view.StrideInBytes = stride;
	view.SizeInBytes = byteSize;
	vert_count = byteSize / (stride);
	vert_stride = stride;
}

void vbo::Draw(ID3D12GraphicsCommandList* cmdList) const
{
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &view);
	cmdList->DrawInstanced(vert_count, 1, 0, 0);
}

void vbo::Draw(ID3D12GraphicsCommandList* cmdList, ibo const& index) const
{
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1,&view);
	index.Draw(cmdList);
}


}