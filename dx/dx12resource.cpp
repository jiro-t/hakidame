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

void texture::Create(UINT width, UINT height,DXGI_FORMAT format) {
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
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN
	};

	device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));

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

}