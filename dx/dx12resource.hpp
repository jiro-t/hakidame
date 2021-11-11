#pragma once

#include "dx.hpp"
#include <utility>
#include <memory>

namespace ino::d3d {

::Microsoft::WRL::ComPtr<ID3D12Resource> CreateResource(
	UINT64 bufferSize,
	D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON,
	D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE);

template<typename T>
class cbo {
	::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	::Microsoft::WRL::ComPtr<ID3D12Resource> buffer;

	void* pDataBegin = nullptr;
public:
	~cbo(){
		if (pDataBegin)
			buffer->Unmap(0, nullptr);
	}

	void Create(){
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		};

		device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

		D3D12_HEAP_PROPERTIES heapProp = {
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1
		};

		D3D12_RESOURCE_DESC resourceDesc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
			.Width = std::max<UINT>((UINT)256,(UINT)sizeof(T)),
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE,
		};

		device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&buffer));
	}
	void Set(ID3D12GraphicsCommandList* cmdList, T data, UINT reg)
	{
		if (pDataBegin == nullptr)
		{
			UINT size = std::max<UINT>((UINT)256, (UINT)sizeof(T));
			D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {
				.BufferLocation = buffer->GetGPUVirtualAddress(),
				.SizeInBytes = size
			};

			device->CreateConstantBufferView(&bufferDesc, heap->GetCPUDescriptorHandleForHeapStart());

			buffer->Map(0, nullptr, reinterpret_cast<void**>(&pDataBegin));
		}
		memcpy(pDataBegin, &data, sizeof(data));

		cmdList->SetGraphicsRootConstantBufferView(reg, buffer->GetGPUVirtualAddress());
	}
};

class texture {
	::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	::Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
public:
	void Create(UINT width, UINT height,DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);
	void Map(void* src, UINT width, UINT height, UINT num_channel = 4);
	void Set(ID3D12GraphicsCommandList* cmdList, UINT reg_id);

	inline D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHeapHandle() { return heap->GetCPUDescriptorHandleForHeapStart(); }
	inline ::Microsoft::WRL::ComPtr<ID3D12Resource>& GetHandle() { return buffer; }
	inline ::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& GetHeap() { return heap; }
};

class renderTexture {
	::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderTargetHeap;
	::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> afterHeap;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	::Microsoft::WRL::ComPtr<ID3D12Resource> buffer;

	D3D12_RECT scissor;
public:
	void Create(UINT width, UINT height, DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);
	void Set(ID3D12GraphicsCommandList* cmdList, UINT reg_id);
	void RenderTarget(ID3D12GraphicsCommandList* cmdList);

	inline ::Microsoft::WRL::ComPtr<ID3D12Resource>& GetHandle() { return buffer; }
	inline D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() { return rtvHandle; }
};

class ibo {
	::Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
	D3D12_INDEX_BUFFER_VIEW view = {};
public:
	UINT index_count = 0;
	void Create(const uint32_t* indecies, UINT byteSize);
	void Draw(ID3D12GraphicsCommandList* cmdList) const;

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() { return buffer->GetGPUVirtualAddress(); }
};

class vbo {
	::Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
	D3D12_VERTEX_BUFFER_VIEW view = {};
public:
	UINT vert_count = 0;
	UINT vert_stride = 0;

	void Create(const void* verts, UINT stride, UINT byteSize);
	void Draw(ID3D12GraphicsCommandList* cmdList) const;
	void Draw(ID3D12GraphicsCommandList* cmdList,ibo const& index) const;

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() { return buffer->GetGPUVirtualAddress(); }
};

struct ByteCode {
	std::shared_ptr<byte> data = nullptr;
	uint32_t size = 0;
};

struct StaticMesh {
	d3d::vbo vbo;
	d3d::ibo ibo;

	void Draw(ID3D12GraphicsCommandList* cmdList) const { vbo.Draw(cmdList, ibo); };
};

}