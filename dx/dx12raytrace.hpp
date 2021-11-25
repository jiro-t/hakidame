#pragma once

#include "dx.hpp"
#include "dx12resource.hpp"

#include <algorithm>

#include <vector>
#include <fstream>

#include "d3dx12.h"

namespace ino::d3d::dxr
{

extern	::Microsoft::WRL::ComPtr<ID3D12Device5> dxrDevice;
extern ::Microsoft::WRL::ComPtr<ID3D12Resource> dxrRenderTarget[3];
extern ::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dxrHeap[3];

BOOL InitDXRDevice();

struct Vertex {
	DirectX::XMVECTOR pos;
	DirectX::XMVECTOR color;
	DirectX::XMVECTOR texcoord;
};

struct SceneConstant
{
	DirectX::XMVECTOR cameraPos;
	DirectX::XMMATRIX invViewProj;
};

struct InstanceConstant
{
	DirectX::XMMATRIX modelMatrix;
	float emit;
	float kr;
};

struct HitArgs {
	D3D12_GPU_DESCRIPTOR_HANDLE indexBufferGPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE vertexBufferGPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE constantBuffer;

	D3D12_GPU_DESCRIPTOR_HANDLE albedoTexture;
};

class Blas {
	::Microsoft::WRL::ComPtr<ID3D12Resource> bottomLevelAccelerationStructure;
	::Microsoft::WRL::ComPtr<ID3D12Resource> scratchResource;
	
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc;
	::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList;
public:
	void Build(StaticMesh& mesh);
	inline ::Microsoft::WRL::ComPtr<ID3D12Resource> Get() noexcept { return bottomLevelAccelerationStructure; }

	inline D3D12_GPU_VIRTUAL_ADDRESS& IndexBuffer() noexcept { return geometryDesc.Triangles.IndexBuffer; }
	inline D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE& VertexBuffer() noexcept { return geometryDesc.Triangles.VertexBuffer; }
};

class Tlas {
	::Microsoft::WRL::ComPtr<ID3D12Resource> topLevelAccelerationStructure;
	::Microsoft::WRL::ComPtr<ID3D12Resource> scratchResource;
	::Microsoft::WRL::ComPtr<ID3D12Resource> instanceDescs;
	std::vector< std::pair<Blas*, InstanceConstant> >  instanceMat;
	std::vector< texture* > instanceTex;

	::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList;
public:
	inline uint32_t GetBlasCount() noexcept { return instanceMat.size(); }
	inline Blas* GetBlas(UINT id) noexcept { return instanceMat[id].first; }
	inline texture* GetAlbedo(UINT id) noexcept { return instanceTex[id]; }
	inline InstanceConstant& GetConstant(UINT id) noexcept { return instanceMat[id].second; }
	void ClearInstance() noexcept;
	void AddInstance(Blas* blas, InstanceConstant cb = { DirectX::XMMatrixIdentity(),0.f,0.3f} );
	void AddInstance(Blas* blas, texture* albedo,InstanceConstant cb = { DirectX::XMMatrixIdentity(),0.f,0.3f });
	void Build();
	inline ::Microsoft::WRL::ComPtr<ID3D12Resource> Get() noexcept { return topLevelAccelerationStructure; }
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

	CD3DX12_STATE_OBJECT_DESC pipelineDesc = { D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

	D3D12_RESOURCE_BARRIER barrier = {};

	void* rayGenShaderIdentifier;
	void* missShaderIdentifier;
	void* hitGroupShaderIdentifier[2];
	::Microsoft::WRL::ComPtr<ID3D12Resource> resMissShaderTable;
	::Microsoft::WRL::ComPtr<ID3D12Resource> resHitGroupShaderTable;
	::Microsoft::WRL::ComPtr<ID3D12Resource> resRayGenShaderTable;
	UINT rayGenShaderTableStride;
	UINT missShaderTableStride;
	UINT hitGroupShaderTableStride;

	SceneConstant sceneCB;
	cbo<SceneConstant>	sceneConstantBuffer;
	//VertexMaterial materialCB;

	::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> dxrCommandList;
	std::vector<HitArgs> hitArgs;
	std::vector< ::Microsoft::WRL::ComPtr<ID3D12Resource> > instanceConstants;
public:
	~DxrPipeline()
	{
	}

	void SetConstantBuffer(void* src, uint32_t size) {
		memcpy(&sceneCB, src, size);
	}
	void Create();
	void CreateShaderTable(Tlas& as);
	void Dispatch(Tlas& as);
	void CopyToScreen(renderTexture& target,uint32_t i);

	::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> Begin()
	{
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[currentBackBufferIndex].Get(), nullptr, IID_PPV_ARGS(&dxrCommandList));
		return dxrCommandList;
	}
	void End()
	{
		dxrCommandList->Close();
	}

	::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetRtvHeap();
};

}