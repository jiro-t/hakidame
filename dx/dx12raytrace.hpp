#pragma once

#include "dx.hpp"
#include <algorithm>

#include <vector>
#include <fstream>

namespace ino::d3d::dxr
{

struct AccelerationStructureBuffer
{
	ID3D12Resource* result = nullptr;
	ID3D12Resource* instanceDesc = nullptr;	// only used in top-level AS

	void Release()
	{
		if (result) { result->Release(); }
		if (instanceDesc) { instanceDesc->Release(); }
	}
};

/*
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
struct HitInfo
{
	DirectX::XMFLOAT4 encodedNormals;

	DirectX::XMFLOAT4 hitPosition;
	uint16_t materialID;

	DirectX::XMFLOAT2 uvs;

	bool hasHit() {
		return materialID != 0;
	}
};

struct ShadowHitInfo
{
	bool hasHit;
};

D3D12_SHADER_BYTECODE LoadShader(std::wstring_view path)
{
	D3D12_SHADER_BYTECODE ret;
	std::ifstream ifs;
	uint32_t size = 0;

	ifs.open(path.data(), std::ios::binary);
	while (!ifs.eof())
	{
		ifs.read(nullptr,1);
		size++;
	}
	ifs.seekg(ifs.beg);
	byte* p = new byte[size];

	uint32_t i = 0;
	while (!ifs.eof())
	{
		ifs.read(reinterpret_cast<char*>(p) + i, 1);
		++i;
	}
	ret.pShaderBytecode = p;
	return ret;
}

ByteCode LoadShader(void* src, size_t src_size)
{
	ByteCode ret = { nullptr , src_size };
	ret.data = std::shared_ptr<byte>(new byte[src_size]);
	memcpy(ret.data.get(), src, src_size);
	return ret;
}

AccelerationStructureBuffer						TLAS;
std::vector<AccelerationStructureBuffer>		BLASes;
uint64_t										tlasSize;
//LinearGPUAllocator								scratchBuffersCache;

::Microsoft::WRL::ComPtr<ID3D12RootSignature> globalRootSignature = nullptr;
::Microsoft::WRL::ComPtr<ID3D12Resource> shaderTable = nullptr;
uint32_t										shaderTableRecordSize = 0;
std::vector<uint8_t>							shaderTableData;

//RtProgram										rtProgram;
::Microsoft::WRL::ComPtr<ID3D12StateObject> rtpso = nullptr;
::Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> rtpsoInfo = nullptr;

::Microsoft::WRL::ComPtr<ID3D12Resource> frameBuffer = nullptr;
::Microsoft::WRL::ComPtr<ID3D12Resource> accumulationBuffer = nullptr;
/*
uint32_t										frameNumber = 0;
uint32_t										accumulatedFrames = 0;
int												maxBounces = 8;
bool											enableAntiAliasing = true;
float											exposureAdjustment = 0.8f;
float											skyIntensity = 3.0f;
bool											enableAccumulation = true;
bool											enableDirectLighting = true;
DirectX::XMMATRIX								lastView;
bool											enableSun = true;
bool											enableHeadlight = false;
float											sunIntensity = 1.0f;
float											headlightIntensity = 10.0f;
float											focusDistance = 10.0f;
float											apertureSize = 0.0f;
bool											forceAccumulationReset = false;
float											sunAzimuth = 295.0f;
float											sunElevation = 78.0f;
*/

constexpr float AABB_WIDTH = 2;      // AABB width.
constexpr float AABB_DISTANCE = 2;
/*
// Raytracing scene
RayGenConstantBuffer m_rayGenCB;

// Acceleration structure
ComPtr<ID3D12Resource> m_accelerationStructure;
ComPtr<ID3D12Resource> m_bottomLevelAccelerationStructure;
ComPtr<ID3D12Resource> m_topLevelAccelerationStructure;

// Raytracing output
ComPtr<ID3D12Resource> m_raytracingOutput;
D3D12_GPU_DESCRIPTOR_HANDLE m_raytracingOutputResourceUAVGpuDescriptor;
UINT m_raytracingOutputResourceUAVDescriptorHeapIndex;

// Shader tables
static const wchar_t* c_hitGroupName;
static const wchar_t* c_raygenShaderName;
static const wchar_t* c_closestHitShaderName;
static const wchar_t* c_missShaderName;
ComPtr<ID3D12Resource> m_missShaderTable;
ComPtr<ID3D12Resource> m_hitGroupShaderTable;
ComPtr<ID3D12Resource> m_rayGenShaderTable;
*/
// DirectX Raytracing (DXR) attributes
::Microsoft::WRL::ComPtr<ID3D12Device5> dxrDevice;
::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> dxrCommandList;
::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> dxrGraphCommandList;
::Microsoft::WRL::ComPtr<ID3D12StateObject> dxrStateObject;

::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dxrDescriptorHeap;
UINT dxrDescriptorsAllocated;
UINT dxrDescriptorSize;

enum class BottomLevelASType {
	Triangle = 0,
	AABB,
	NUM_BLAS
};

enum class RayType{
	Radiance = 0,
	Shadow,
	UNDEFINED
};

enum class GlobalRootSignatureParams {
	OutputViewSlot = 0,
	AccelerationStructureSlot,
	SceneConstantSlot,
	VertexBuffersSlot,
	UNDEFINED
};

enum class LocalRootSignatureParams {
	CubeConstantSlot = 0,
	UNDEFINED
};

struct Viewport
{
	float left;
	float top;
	float right;
	float bottom;
};

struct RayGenConstantBuffer
{
	Viewport viewport;
	Viewport stencil;
};

class dxr_pipeline;

void AllocateUploadBuffer(ID3D12Device* pDevice, void* pData, UINT64 datasize, ID3D12Resource** ppResource)//, const wchar_t* resourceName = nullptr)
{
	D3D12_HEAP_PROPERTIES heapProp = {
		.Type = D3D12_HEAP_TYPE_UPLOAD,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 1,
		.VisibleNodeMask = 1
	};

	D3D12_RESOURCE_DESC resourceDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = datasize,
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

	 pDevice->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(ppResource));

	void* pMappedData;
	(*ppResource)->Map(0, nullptr, &pMappedData);
	memcpy(pMappedData, pData, datasize);
	(*ppResource)->Unmap(0, nullptr);
}

struct AccelerationStructureBuffers
{
	::Microsoft::WRL::ComPtr<ID3D12Resource> scratch;
	::Microsoft::WRL::ComPtr<ID3D12Resource> accelerationStructure;
	::Microsoft::WRL::ComPtr<ID3D12Resource> instanceDesc;
	UINT64                 ResultDataMaxSizeInBytes = 0;
};

inline uint32_t alignment(uint32_t al, uint32_t byte)
{
	return byte + byte % al;
};

BOOL InitDXRDevice()
{
	HRESULT hr = S_OK;

	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[currentBackBufferIndex].Get(), nullptr, IID_PPV_ARGS(&dxrGraphCommandList));
	if (hr != S_OK) return FALSE;
	hr = dxrGraphCommandList->Close();
	if (hr != S_OK) return FALSE;
	hr = device->QueryInterface(IID_PPV_ARGS(&dxrDevice));
	if (hr != S_OK) return FALSE;
	hr = dxrGraphCommandList->QueryInterface(IID_PPV_ARGS(&dxrCommandList));
	if (hr != S_OK) return FALSE;

	//Create Buffers
	D3D12_RESOURCE_DESC desc = {};
	desc.DepthOrArraySize = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	desc.Width = d3d::screen_width;
	desc.Height = d3d::screen_height;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	const D3D12_HEAP_PROPERTIES heapProp =
	{
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0, 0
	};

	hr = device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&dxr::frameBuffer));
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	hr = device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&dxr::accumulationBuffer));

	//Create Pipeline
	int paramCount;
	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
//	rootParam.DescriptorTable.NumDescriptorRanges = _countof(ranges);
//	rootParam.DescriptorTable.pDescriptorRanges = ranges;

	D3D12_ROOT_PARAMETER rootParams[1] = { rootParam };

	D3D12_STATIC_SAMPLER_DESC staticSampler = {};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.MipLODBias = 0.f;
	staticSampler.MaxAnisotropy = 1;
	staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSampler.MinLOD = 0.f;
	staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	staticSampler.ShaderRegister = 0;
	staticSampler.RegisterSpace = 0;
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.NumParameters = _countof(rootParams);
	rootDesc.pParameters = rootParams;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	rootDesc.NumStaticSamplers = 1;
	rootDesc.pStaticSamplers = &staticSampler;

	ID3DBlob* sig;
	ID3DBlob* error;
	hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error);
	ID3D12RootSignature* pRootSig;
	hr = d3d::device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&d3d::dxr::globalRootSignature));

	std::vector<D3D12_STATE_SUBOBJECT> subObjs;
	struct EntryType {
		std::wstring entryPoint;
		D3D12_STATE_SUBOBJECT_TYPE state;
	};

	EntryType entries[] = {
		{L"RayGen",D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY},
		{L"Miss",D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY},
		{L"HitGroup",D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY},
		{L"MissShadow",D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY},
		{L"HitGroupShadow",D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY},

		{L"AnyHit",D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY},
		{L"AnyHitShadow",D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP},
		{L"ClosestHit",D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP},
	};

	//shaderObjects
	for (int i = 0;i < _countof(entries);++i)
	{
		D3D12_EXPORT_DESC exportDesc = {};
		exportDesc.Name = entries[i].entryPoint.c_str();
		exportDesc.ExportToRename = entries[i].entryPoint.c_str();
		exportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

		D3D12_DXIL_LIBRARY_DESC	libDesc = {};
		libDesc.DXILLibrary = LoadShader(L"shaders\\PathTracer.hlsl");
		libDesc.NumExports = 1;
		libDesc.pExports = &exportDesc;

		D3D12_STATE_SUBOBJECT obj = {};
		obj.Type = entries[i].state;
		obj.pDesc = &libDesc;

		subObjs.push_back(obj);
	}

	// shader list
	const WCHAR* shaderExports[] = { 
		entries[0].entryPoint.c_str(),
		entries[1].entryPoint.c_str(),
		entries[2].entryPoint.c_str(),
		entries[3].entryPoint.c_str(),
		entries[4].entryPoint.c_str(),
	};
	D3D12_RAYTRACING_SHADER_CONFIG shaderDesc = {};
	shaderDesc.MaxPayloadSizeInBytes = std::max(sizeof(HitInfo), sizeof(ShadowHitInfo));
	shaderDesc.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;

	D3D12_STATE_SUBOBJECT shaderConfigObject = {};
	shaderConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	shaderConfigObject.pDesc = &shaderDesc;
	subObjs.push_back(shaderConfigObject);

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderPayloadAssociation = {};
	shaderPayloadAssociation.NumExports = _countof(shaderExports);
	shaderPayloadAssociation.pExports = shaderExports;
	shaderPayloadAssociation.pSubobjectToAssociate = &subObjs.back();

	D3D12_STATE_SUBOBJECT shaderPayloadAssociationObject = {};
	shaderPayloadAssociationObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	shaderPayloadAssociationObject.pDesc = &shaderPayloadAssociation;
	subObjs.push_back(shaderPayloadAssociationObject);

	D3D12_STATE_SUBOBJECT globalRootSig;
	globalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	globalRootSig.pDesc = dxr::globalRootSignature.Get();
	subObjs.push_back(globalRootSig);

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = 1;

	D3D12_STATE_SUBOBJECT pipelineConfigObject = {};
	pipelineConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipelineConfigObject.pDesc = &pipelineConfig;
	subObjs.push_back(pipelineConfigObject);

	D3D12_STATE_OBJECT_DESC pipelineDesc = {};
	pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	pipelineDesc.NumSubobjects = static_cast<UINT>(subObjs.size());
	pipelineDesc.pSubobjects = &subObjs.front();

	hr = d3d::device->CreateStateObject(&pipelineDesc, IID_PPV_ARGS(&dxr::rtpso));
	hr = dxr::rtpso->QueryInterface(IID_PPV_ARGS(&dxr::rtpsoInfo));

	//ShaderTable
	uint32_t shaderIdSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	dxr::shaderTableRecordSize = shaderIdSize;
	dxr::shaderTableRecordSize = alignment(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, dxr::shaderTableRecordSize);

	uint32_t shaderTableSize = ( dxr::shaderTableRecordSize * _countof(shaderExports) );
	shaderTableSize = alignment(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT, shaderTableSize);
	dxr::shaderTableData.reserve(shaderTableSize);

	uint8_t* pData = dxr::shaderTableData.data();
	for (auto entry : shaderExports)
	{
		memcpy(pData, dxr::rtpsoInfo->GetShaderIdentifier(entry), shaderIdSize);
		pData += dxr::shaderTableRecordSize;
	}

	// Upload shader table to GPU
	//D3DResources::UploadToGPU(d3d, dxr.shaderTableData.data(), dxr.shaderTable, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	return TRUE;
}

D3D12_RAYTRACING_GEOMETRY_DESC ToGeometoryDesc(vbo v)
{
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.IndexBuffer = 0;//m_indexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount = 0;//static_cast<UINT>(m_indexBuffer->GetDesc().Width) / sizeof(Index);
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
	geometryDesc.Triangles.Transform3x4 = 0;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	geometryDesc.Triangles.VertexCount = v.vert_count;
	geometryDesc.Triangles.VertexBuffer.StartAddress = v.GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = v.vert_stride;

	return geometryDesc;
}

void DrawDXR(D3D12_RAYTRACING_GEOMETRY_DESC geom[],UINT num_geom)
{
	for (int i = 0; i < num_geom; ++i)
	{
		geom[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	}
	//CreateBLAS Buffer
	AccelerationStructureBuffers blas;
	//CreateTLAS Buffer
	AccelerationStructureBuffers tlas;

//    auto device = m_deviceResources->GetD3DDevice();
//    auto commandList = m_deviceResources->GetCommandList();
//    auto commandQueue = m_deviceResources->GetCommandQueue();
//    auto commandAllocator = m_deviceResources->GetCommandAllocator();

	// Reset the command list for the acceleration structure construction.
	dxrCommandList->Reset(commandAllocators[currentBackBufferIndex].Get(), nullptr);

	// Get required sizes for an acceleration structure.
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = buildFlags;
	topLevelInputs.NumDescs = 1;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
	if(topLevelPrebuildInfo.ResultDataMaxSizeInBytes <= 0)
		return;//no geometry

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = topLevelInputs;
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.pGeometryDescs = geom;
	dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
	if(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes <= 0)
		return;//no geometry

	//allocate resource
	::Microsoft::WRL::ComPtr<ID3D12Resource> scratchResource = CreateResource(
		(std::max)(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	blas.accelerationStructure = CreateResource(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, initialResourceState);
	tlas.accelerationStructure = CreateResource(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, initialResourceState);

	// Create an instance desc for the bottom-level acceleration structure.
	::Microsoft::WRL::ComPtr<ID3D12Resource> instanceDescs;
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
	instanceDesc.InstanceMask = 1;
	instanceDesc.AccelerationStructure = blas.accelerationStructure->GetGPUVirtualAddress();
	AllocateUploadBuffer(device.Get(), &instanceDesc, sizeof(instanceDesc), &instanceDescs);

	// Bottom Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	{
		bottomLevelBuildDesc.Inputs = bottomLevelInputs;
		bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
		bottomLevelBuildDesc.DestAccelerationStructureData = blas.accelerationStructure->GetGPUVirtualAddress();
	}

	// Top Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	{
		topLevelInputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
		topLevelBuildDesc.Inputs = topLevelInputs;
		topLevelBuildDesc.DestAccelerationStructureData = tlas.accelerationStructure->GetGPUVirtualAddress();
		topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
	}

	dxrCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
	//        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_bottomLevelAccelerationStructure.Get()));
	dxrCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

	dxrCommandList->Close();
	ID3D12CommandList* commandLists[] = { dxrCommandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	wait();
}

class dxr_pipeline {
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
	// Root signatures
	::Microsoft::WRL::ComPtr<ID3D12RootSignature> globalRootSignature;
	::Microsoft::WRL::ComPtr<ID3D12RootSignature> localRootSignature;

	::Microsoft::WRL::ComPtr<ID3DBlob> signature;
	//::Microsoft::WRL::ComPtr<ID3DBlob> shader[static_cast<int>(ShaderTypes::UNDEFINED)];
	::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	D3D12_ROOT_PARAMETER* params = nullptr;
	BOOL use_depth = FALSE;

	D3D12_RESOURCE_BARRIER barrier = {};

	// Descriptors
	::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	UINT descriptorsAllocated;
	UINT descriptorSize;

	LPCSTR constexpr getCstrShaderType(ShaderTypes type) const;
	void resetCommand();
public:
	D3D12_VIEWPORT view = { .MinDepth = 0.f,.MaxDepth = 1.f };
	D3D12_RECT scissor = {};

	dxr_pipeline();
	~dxr_pipeline();

	void LoadShader(ShaderTypes type, std::wstring_view path, LPCSTR entryPoint);
	void LoadShader(ShaderTypes type, void* src, size_t src_size, LPCSTR entryPoint);

	void CreateSampler(UINT count, D3D12_FILTER filter[], D3D12_TEXTURE_ADDRESS_MODE warp[]);

	void Create()
	{
		UINT globalParamCount = static_cast<int>(GlobalRootSignatureParams::UNDEFINED);
		UINT localParamCount = static_cast<int>(LocalRootSignatureParams::UNDEFINED);
		params = new D3D12_ROOT_PARAMETER[globalParamCount + localParamCount];
		ZeroMemory(params, sizeof(D3D12_ROOT_PARAMETER) * (globalParamCount + localParamCount));

		// Global Root Signature
		{
			D3D12_DESCRIPTOR_RANGE range = {
				.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
				.NumDescriptors = 1,
				.BaseShaderRegister = 0,
				.RegisterSpace = 0,
				.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
			};
			params[static_cast<int>(GlobalRootSignatureParams::AccelerationStructureSlot)].ParameterType 
				= D3D12_ROOT_PARAMETER_TYPE_SRV;
			params[static_cast<int>(GlobalRootSignatureParams::AccelerationStructureSlot)].ShaderVisibility 
				= D3D12_SHADER_VISIBILITY_ALL;
			params[static_cast<int>(GlobalRootSignatureParams::AccelerationStructureSlot)].Descriptor.ShaderRegister 
				= 0;

			params[static_cast<int>(GlobalRootSignatureParams::OutputViewSlot)].ParameterType
				= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[static_cast<int>(GlobalRootSignatureParams::OutputViewSlot)].ShaderVisibility
				= D3D12_SHADER_VISIBILITY_ALL;
			params[static_cast<int>(GlobalRootSignatureParams::OutputViewSlot)].DescriptorTable.pDescriptorRanges 
				= &range;
			params[static_cast<int>(GlobalRootSignatureParams::OutputViewSlot)].DescriptorTable.NumDescriptorRanges
				= 1;
			//D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE
			D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {
				.NumParameters = globalParamCount,
				.pParameters = params,
				.NumStaticSamplers = 0,
				.pStaticSamplers = nullptr,
				.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE
			};

			::Microsoft::WRL::ComPtr<ID3DBlob> signature;
			::Microsoft::WRL::ComPtr<ID3DBlob> error;
			D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
			device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&globalRootSignature));
		}

		cbo<RayGenConstantBuffer> rayGen;
		rayGen.Create();
		{
			params[globalParamCount + static_cast<int>(LocalRootSignatureParams::CubeConstantSlot)].ParameterType
				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			params[globalParamCount + static_cast<int>(LocalRootSignatureParams::CubeConstantSlot)].ShaderVisibility
				= D3D12_SHADER_VISIBILITY_ALL;
			params[globalParamCount + static_cast<int>(LocalRootSignatureParams::CubeConstantSlot)].Constants.Num32BitValues
				= ((sizeof(RayGenConstantBuffer) - 1) / sizeof(UINT32)+1);
			params[globalParamCount + static_cast<int>(LocalRootSignatureParams::CubeConstantSlot)].Constants.ShaderRegister
				= 0;
			params[globalParamCount + static_cast<int>(LocalRootSignatureParams::CubeConstantSlot)].Constants.RegisterSpace
				= 0;

			D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {
				.NumParameters = localParamCount,
				.pParameters = params + globalParamCount,
				.NumStaticSamplers = 0,
				.pStaticSamplers = nullptr,
				.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE
			};

			::Microsoft::WRL::ComPtr<ID3DBlob> signature;
			::Microsoft::WRL::ComPtr<ID3DBlob> error;
			D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
			device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&localRootSignature));
		}
	}
};

// Build shader tables.
// This encapsulates all shader records - shaders and the arguments for their local root signatures.
void BuildShaderTables()
{
	auto device = ino::d3d::device;

	void* rayGenShaderIdentifier;
	void* missShaderIdentifier;
	void* hitGroupShaderIdentifier;

	auto GetShaderIdentifiers = [&](auto* stateObjectProperties)
	{
		rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"raygen");//c_raygenShaderName);
		missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"miss");// c_missShaderName);
		hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"hit");// c_hitGroupName);
	};

	// Get shader identifiers.
	UINT shaderIdentifierSize;
	{
		::Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		if (stateObjectProperties == stateObjectProperties)
			return;
		//m_dxrStateObject.As(&stateObjectProperties);
		GetShaderIdentifiers(stateObjectProperties.Get());
		shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	}

	// Ray gen shader table
	{
		/*
		struct RootArguments {
			RayGenConstantBuffer cb;
		} rootArguments;
		rootArguments.cb = m_rayGenCB;

		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);
		ShaderTable rayGenShaderTable(device, numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
		rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));
		m_rayGenShaderTable = rayGenShaderTable.GetResource();
		*/
	}

	// Miss shader table
	{
		/*
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable missShaderTable(device, numShaderRecords, shaderRecordSize, L"MissShaderTable");
		missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
		m_missShaderTable = missShaderTable.GetResource();
		*/
	}

	// Hit group shader table
	{

/*
	//shader_table
	uint8_t* m_mappedShaderRecords;
	UINT m_shaderRecordSize;

	// Debug support
	std::wstring m_name;
	std::vector<ShaderRecord> m_shaderRecords;
*/
		/*
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable hitGroupShaderTable(device, numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");
		hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize));
		m_hitGroupShaderTable = hitGroupShaderTable.GetResource();
		*/
	}
}

//-----------------What that----------------------------------
//
//------------------------------------------------------------

/*
	auto commandList = m_deviceResources->GetCommandList();

	auto DispatchRays = [&](auto* commandList, auto* stateObject, auto* dispatchDesc)
	{
		// Since each shader table has only one shader record, the stride is same as the size.
		dispatchDesc->HitGroupTable.StartAddress = m_hitGroupShaderTable->GetGPUVirtualAddress();
		dispatchDesc->HitGroupTable.SizeInBytes = m_hitGroupShaderTable->GetDesc().Width;
		dispatchDesc->HitGroupTable.StrideInBytes = dispatchDesc->HitGroupTable.SizeInBytes;
		dispatchDesc->MissShaderTable.StartAddress = m_missShaderTable->GetGPUVirtualAddress();
		dispatchDesc->MissShaderTable.SizeInBytes = m_missShaderTable->GetDesc().Width;
		dispatchDesc->MissShaderTable.StrideInBytes = dispatchDesc->MissShaderTable.SizeInBytes;
		dispatchDesc->RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable->GetGPUVirtualAddress();
		dispatchDesc->RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderTable->GetDesc().Width;
		dispatchDesc->Width = m_width;
		dispatchDesc->Height = m_height;
		dispatchDesc->Depth = 1;
		commandList->SetPipelineState1(stateObject);
		commandList->DispatchRays(dispatchDesc);
	};

	commandList->SetComputeRootSignature(m_raytracingGlobalRootSignature.Get());

	// Bind the heaps, acceleration structure and dispatch rays.
	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	commandList->SetDescriptorHeaps(1, m_descriptorHeap.GetAddressOf());
	commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, m_raytracingOutputResourceUAVGpuDescriptor);
	commandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, m_topLevelAccelerationStructure->GetGPUVirtualAddress());
	DispatchRays(m_dxrCommandList.Get(), m_dxrStateObject.Get(), &dispatchDesc);
*/

//void CreateAs(auto vertexcies)
//{
//}

void ExcuteRaytrace()
{
	auto commandList = dxrCommandList;
	auto device = dxrDevice;


	auto DispatchRays = [&](auto* commandList, auto* stateObject, auto* dispatchDesc)
	{
		dispatchDesc->HitGroupTable.StartAddress = m_hitGroupShaderTable->GetGPUVirtualAddress();
		dispatchDesc->HitGroupTable.SizeInBytes = m_hitGroupShaderTable->GetDesc().Width;
		dispatchDesc->HitGroupTable.StrideInBytes = dispatchDesc->HitGroupTable.SizeInBytes;
		dispatchDesc->MissShaderTable.StartAddress = m_missShaderTable->GetGPUVirtualAddress();
		dispatchDesc->MissShaderTable.SizeInBytes = m_missShaderTable->GetDesc().Width;
		dispatchDesc->MissShaderTable.StrideInBytes = dispatchDesc->MissShaderTable.SizeInBytes;
		dispatchDesc->RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable->GetGPUVirtualAddress();
		dispatchDesc->RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderTable->GetDesc().Width;
		dispatchDesc->Width = m_width;
		dispatchDesc->Height = m_height;
		dispatchDesc->Depth = 1;
		commandList->SetPipelineState1(stateObject);
		commandList->DispatchRays(dispatchDesc);
	};

//	device->CreateRootSignature(1, g_pRaytracing, ARRAYSIZE(g_pRaytracing), IID_PPV_ARGS(&m_raytracingGlobalRootSignature));
//	commandList->SetComputeRootSignature(dxrGlobalRootSignature.Get());

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	// Allocate a heap for 3 descriptors:
	// 2 - vertex and index buffer SRVs
	// 1 - raytracing output texture SRV
	descriptorHeapDesc.NumDescriptors = 3;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask = 0;
	device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&dxrDescriptorHeap));
	dxrDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Bind the heaps, acceleration structure and dispatch rays.    
	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
//	commandList->SetDescriptorHeaps(1, dxrDescriptorHeap.GetAddressOf());
//	commandList->SetComputeRootDescriptorTable(0, m_raytracingOutputResourceUAVGpuDescriptor);
//	commandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, m_topLevelAccelerationStructure->GetGPUVirtualAddress());
//	DispatchRays(m_dxrCommandList.Get(), m_dxrStateObject.Get(), &dispatchDesc);
}

}