#include "dx12resource.hpp"

namespace ino::d3d {

void vbo::Create(void* verts, UINT count, UINT stride, UINT byteSize) {
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
	vert_count = count;
}

void vbo::Draw(ID3D12GraphicsCommandList* cmdList) const
{
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &view);
	cmdList->DrawInstanced(3, vert_count / 3, 0, 0);
}

}