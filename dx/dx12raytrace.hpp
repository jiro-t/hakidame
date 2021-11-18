#pragma once

#include "dx.hpp"
#include "dx12resource.hpp"

#include <algorithm>

#include <vector>
#include <fstream>

namespace ino::d3d::dxr
{

/* ref:https://github.com/acmarrs/IntroToDXR
struct RtProgram
{
	D3D12ShaderInfo			info = {};
	IDxcBlob* blob = nullptr;

	RtProgram() {}

	RtProgram(D3D12ShaderInfo shaderInfo)
	{
		info = shaderInfo;
	}
};
*/
extern	::Microsoft::WRL::ComPtr<ID3D12Device5> dxrDevice;
extern ::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> dxrCommandList;
extern ::Microsoft::WRL::ComPtr<ID3D12Resource> dxrRenderTarget;
extern ::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dxrRtvHeap;

BOOL InitDXRDevice();

struct VertexMaterial
{
	DirectX::XMVECTOR albedo;
};

struct SceneConstant
{
	DirectX::XMMATRIX proj;
	DirectX::XMVECTOR cameraPos;
	DirectX::XMVECTOR lightPosition;
	DirectX::XMVECTOR lightAmbientColor;
	DirectX::XMVECTOR lightDiffuseColor;
};


class AccelerationStructure
{
	std::vector<::Microsoft::WRL::ComPtr<ID3D12Resource>> bottomLevelAccelerationStructure;
	::Microsoft::WRL::ComPtr<ID3D12Resource> topLevelAccelerationStructure;

	::Microsoft::WRL::ComPtr<ID3D12Resource> scratchResource;
	::Microsoft::WRL::ComPtr<ID3D12Resource> instanceDescs;
public:
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>  geometryDescs;
	std::vector< std::pair<UINT, DirectX::XMMATRIX> >  instanceMat;

	inline ::Microsoft::WRL::ComPtr<ID3D12Resource> blas(UINT id) noexcept { return bottomLevelAccelerationStructure[id]; }
	inline ::Microsoft::WRL::ComPtr<ID3D12Resource> tlas() noexcept { return topLevelAccelerationStructure; }

	void ClearGeometory() noexcept;
	void ClearInstance() noexcept;
	void AddGeometory(StaticMesh mesh);
	void AddInstance(UINT objID, DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity());
	void BuildBlas();
	void BuildTlas();
};

class ShaderRecord
{
public:
	ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize) :
		shaderIdentifier(pShaderIdentifier, shaderIdentifierSize)
	{
	}

	ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize, void* pLocalRootArguments, UINT localRootArgumentsSize) :
		shaderIdentifier(pShaderIdentifier, shaderIdentifierSize),
		localRootArguments(pLocalRootArguments, localRootArgumentsSize)
	{
	}

	void CopyTo(void* dest) const noexcept
	{
		uint8_t* byteDest = static_cast<uint8_t*>(dest);
		memcpy(byteDest, shaderIdentifier.ptr, shaderIdentifier.size);
		if (localRootArguments.ptr)
		{
			memcpy(byteDest + shaderIdentifier.size, localRootArguments.ptr, localRootArguments.size);
		}
	}

	struct PointerWithSize {
		void* ptr;
		UINT size;

		PointerWithSize() noexcept : ptr(nullptr), size(0)  {}
		PointerWithSize(void* _ptr, UINT _size) noexcept : ptr(_ptr), size(_size) {};
	};
	PointerWithSize shaderIdentifier;
	PointerWithSize localRootArguments;
};

class ShaderTable
{
	uint8_t* m_mappedShaderRecords;
	UINT m_shaderRecordSize;

	// Debug support
	std::wstring m_name;
	std::vector<ShaderRecord> m_shaderRecords;

	::Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

	inline UINT aligment(UINT a, UINT b) noexcept { return ((a + b - 1) / b) * b; }
	ShaderTable() noexcept  {}
public:
	ShaderTable(ID3D12Device* device, UINT numShaderRecords, UINT shaderRecordSize, LPCWSTR resourceName = nullptr)
		: m_name(resourceName)
	{
		m_shaderRecordSize = aligment(shaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
		m_shaderRecords.reserve(numShaderRecords);
		UINT bufferSize = numShaderRecords * m_shaderRecordSize;

		D3D12_HEAP_PROPERTIES uploadHeapProperties = {
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1
		};

		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Alignment = 0;
		bufferDesc.Width = bufferSize;
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_resource));
		uint8_t* mappedData;
		D3D12_RANGE readRange = {};
		m_resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData));
		m_mappedShaderRecords = mappedData;
	}

	void push_back(const ShaderRecord& shaderRecord)
	{
		m_shaderRecords.push_back(shaderRecord);
		shaderRecord.CopyTo(m_mappedShaderRecords);
		m_mappedShaderRecords += m_shaderRecordSize;
	}

	inline ::Microsoft::WRL::ComPtr<ID3D12Resource> GetResource() noexcept { return m_resource; }
	inline UINT GetShaderRecordSize() noexcept { return m_shaderRecordSize; }
};

class DxrPipeline {
	// Root signatures
	::Microsoft::WRL::ComPtr<ID3D12RootSignature> globalRootSignature = nullptr;
	::Microsoft::WRL::ComPtr<ID3D12RootSignature> localRootSignature = nullptr;

	::Microsoft::WRL::ComPtr<ID3D12StateObject> rtpso = nullptr;
	::Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> rtpsoInfo = nullptr;

	::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	UINT descriptorSize = 0;

	union AlignedSceneConstant
	{
		SceneConstant constants;
		uint8_t alignmentPadding[D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT];
	};
	AlignedSceneConstant* mappedConstantData;
	::Microsoft::WRL::ComPtr<ID3D12Resource> sceneConstants = nullptr;

	std::vector<D3D12_STATE_SUBOBJECT> subObjs;
	D3D12_DXIL_LIBRARY_DESC dxilLib = {};
	D3D12_HIT_GROUP_DESC hitGroup = {};
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};

	D3D12_RESOURCE_BARRIER barrier = {};

	void* rayGenShaderIdentifier;
	void* missShaderIdentifier;
	void* hitGroupShaderIdentifier;
	::Microsoft::WRL::ComPtr<ID3D12Resource> resMissShaderTable;
	::Microsoft::WRL::ComPtr<ID3D12Resource> resHitGroupShaderTable;
	::Microsoft::WRL::ComPtr<ID3D12Resource> resRayGenShaderTable;
	UINT rayGenShaderTableStride;
	UINT missShaderTableStride;
	UINT hitGroupShaderTableStride;

	SceneConstant sceneCB[num_swap_buffers];
	VertexMaterial materialCB;
public:
	~DxrPipeline()
	{
		if (dxilLib.DXILLibrary.pShaderBytecode)
		{
			delete[] dxilLib.DXILLibrary.pShaderBytecode;
			dxilLib.DXILLibrary.pShaderBytecode = nullptr;
		}
	}
	D3D12_VIEWPORT view = { .MinDepth = 0.f,.MaxDepth = 1.f };
	D3D12_RECT scissor = {};

	void Create();
	void CreateShaderTable(AccelerationStructure& as);
	void Dispatch();
	void CopyToScreen();
};

}