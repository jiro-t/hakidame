#pragma once

#include "dx.hpp"
#include <algorithm>

namespace ino::d3d {

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

	void Create()
	{
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
			.Width = std::max((UINT)256,(UINT)sizeof(T)),
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

	void Set(ID3D12GraphicsCommandList* cmdList,std::add_const_t<std::add_rvalue_reference_t<T>> data)
	{
		if (pDataBegin == nullptr)
		{
			UINT size = std::max( (UINT)256, (UINT)sizeof(T) );
			D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {
				.BufferLocation = buffer->GetGPUVirtualAddress(),
				.SizeInBytes = size
			};

			device->CreateConstantBufferView(&bufferDesc, heap->GetCPUDescriptorHandleForHeapStart());

			buffer->Map(0, nullptr, reinterpret_cast<void**>(&pDataBegin));
		}
		memcpy(pDataBegin, &data, sizeof(data));

		cmdList->SetGraphicsRootConstantBufferView(0, buffer->GetGPUVirtualAddress());
	}
};

class texture {
	::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	::Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
public:
	void Create(){
		D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		};
		device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&heap));

		D3D12_CPU_DESCRIPTOR_HANDLE handle_srv{};
		D3D12_SHADER_RESOURCE_VIEW_DESC resourct_view_desc{};

		resourct_view_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		resourct_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		resourct_view_desc.Texture2D.MipLevels = 1;
		resourct_view_desc.Texture2D.MostDetailedMip = 0;
		resourct_view_desc.Texture2D.PlaneSlice = 0;
		resourct_view_desc.Texture2D.ResourceMinLODClamp = 0.0F;
		resourct_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		handle_srv = heap->GetCPUDescriptorHandleForHeapStart();
		device->CreateShaderResourceView(buffer.Get(), &resourct_view_desc, handle_srv);
	}
	void Set(ID3D12GraphicsCommandList* cmdList,std::size_t reg_id,void *src,std::size_t size) {
		cmdList->SetGraphicsRootDescriptorTable(reg_id, heap->GetGPUDescriptorHandleForHeapStart());
	}
};

class vbo {
	::Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
	D3D12_VERTEX_BUFFER_VIEW view;
	size_t vert_count;

public:
	void Create(void* verts, UINT count, UINT stride, UINT byteSize);
	void Draw(ID3D12GraphicsCommandList* cmdList) const;
};

}