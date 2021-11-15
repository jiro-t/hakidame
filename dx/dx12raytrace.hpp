#pragma once

#include "dx.hpp"
#include "dx12resource.hpp"

#include <algorithm>

#include <vector>
#include <fstream>

namespace ino::d3d::dxr
{

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

::Microsoft::WRL::ComPtr<ID3D12Resource> shaderTable = nullptr;
uint32_t										shaderTableRecordSize = 0;
std::vector<uint8_t>							shaderTableData;

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

struct AccelerationStructureBuffer
{
	::Microsoft::WRL::ComPtr<ID3D12Resource>          scratch;
	::Microsoft::WRL::ComPtr<ID3D12Resource>          structure;
	DXR_BUILD_DESC                  m_BuildDesc;
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>  geometryDesc;

	ID3D12Resource* result = nullptr;
	ID3D12Resource* instanceDesc = nullptr;	// only used in top-level AS

	void Release()
	{
		if (result) { result->Release(); }
		if (instanceDesc) { instanceDesc->Release(); }
	}

	void Init()
	{
		// 高速化機構の入力設定.
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = flags;
		inputs.NumDescs = count;
		inputs.pGeometryDescs = m_GeometryDesc.data();
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

		// ビルド前情報を取得.
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
		if (prebuildInfo.ResultDataMaxSizeInBytes == 0)
		{
			return;
		}

		// スクラッチバッファ生成.
		CreateBufferUAV(dxrDevice,prebuildInfo.ScratchDataSizeInBytes, scratch.Get(),	D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		scratch->SetName(L"BlasScratch");

		// 高速化機構用バッファ生成.
		CreateBufferUAV(dxrDevice,prebuildInfo.ResultDataMaxSizeInBytes, structure.Get(),D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
		structure->SetName(L"Blas");

		// ビルド設定.
		memset(&m_BuildDesc, 0, sizeof(m_BuildDesc));
		m_BuildDesc.Inputs = inputs;
		m_BuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
		m_BuildDesc.DestAccelerationStructureData = structure->GetGPUVirtualAddress();
	}

	void addGeometory() {
		D3D12_RAYTRACING_GEOMETRY_DESC geom_desc;
		//geom_desc.Triangles = ;
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs;
		//inputs.pGeometryDescs
	}
};

struct BottomLevelAs {};
struct TopLevelAs {};

AccelerationStructureBuffer						TLAS;
std::vector<AccelerationStructureBuffer>		BLASes;
uint64_t										tlasSize;


BOOL InitDXRDevice();

void DrawDXR(D3D12_RAYTRACING_GEOMETRY_DESC geom[],UINT num_geom)
{
	for (int i = 0; i < num_geom; ++i)
	{
		geom[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	}
	//CreateBLAS Buffer
	//AccelerationStructureBuffers blas;
	//CreateTLAS Buffer
	//AccelerationStructureBuffers tlas;

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
	//blas.accelerationStructure = CreateResource(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, initialResourceState);
	//tlas.accelerationStructure = CreateResource(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, initialResourceState);

	// Create an instance desc for the bottom-level acceleration structure.
	::Microsoft::WRL::ComPtr<ID3D12Resource> instanceDescs;
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
	instanceDesc.InstanceMask = 1;
	//instanceDesc.AccelerationStructure = blas.accelerationStructure->GetGPUVirtualAddress();
	AllocateUploadBuffer(device.Get(), &instanceDesc, sizeof(instanceDesc), &instanceDescs);

	// Bottom Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	{
		bottomLevelBuildDesc.Inputs = bottomLevelInputs;
		bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
		//bottomLevelBuildDesc.DestAccelerationStructureData = blas.accelerationStructure->GetGPUVirtualAddress();
	}

	// Top Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	{
		topLevelInputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
		topLevelBuildDesc.Inputs = topLevelInputs;
		//topLevelBuildDesc.DestAccelerationStructureData = tlas.accelerationStructure->GetGPUVirtualAddress();
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
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
	// Root signatures
	::Microsoft::WRL::ComPtr<ID3D12RootSignature> globalRootSignature;
	::Microsoft::WRL::ComPtr<ID3D12RootSignature> localRootSignature;

	::Microsoft::WRL::ComPtr<ID3D12StateObject> rtpso = nullptr;
	::Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> rtpsoInfo = nullptr;

	std::vector<D3D12_STATE_SUBOBJECT> subObjs;
	D3D12_DXIL_LIBRARY_DESC dxilLib = {};
	D3D12_HIT_GROUP_DESC hitGroup = {};
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};

	D3D12_RESOURCE_BARRIER barrier = {};

	// Descriptors
	//::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	//UINT descriptorsAllocated;
	//UINT descriptorSize;
public:
	D3D12_VIEWPORT view = { .MinDepth = 0.f,.MaxDepth = 1.f };
	D3D12_RECT scissor = {};

	dxr_pipeline();
	~dxr_pipeline();

	void Create();
};

void BuildShaderTables()
{
/*
	m_RecordSize = RoundUp(
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + localRootArgumentSize,
		D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

	auto bufferSize = m_RecordSize * recordCount;
	CreateUploadBuffer(pDevice, bufferSize, m_Resource.GetAddress());

	uint8_t* ptr = nullptr;
	m_Resource->Map(0, nullptr, reinterpret_cast<void**>(&ptr));

	if (localRootArgumentSize == 0)
	{
		for (auto i = 0u; i < recordCount; ++i)
		{
			auto record = records[i];
			memcpy(ptr, record.ShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			ptr += m_RecordSize;
		}
	}
	else
	{
		for (auto i = 0u; i < recordCount; ++i)
		{
			auto record = records[i];
			memcpy(ptr, record.ShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			memcpy(ptr + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, record.LocalRootArguments, localRootArgumentSize);
			ptr += m_RecordSize;
		}
	}

	m_Resource->Unmap(0, nullptr);
*/

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

/*
	if (m_pGraphicsQueue == nullptr)
	{ return; }

	auto idx  = GetCurrentBackBufferIndex();
	auto pCmd = m_GfxCmdList.Reset();

	asdx::GfxDevice().SetUploadCommand(pCmd);

	// シーンバッファ更新.
	{
		// ビュー行列.
		auto view = asdx::Matrix::CreateLookAt(
			asdx::Vector3(0.0f, 0.0f, -2.0f),
			asdx::Vector3(0.0f, 0.0f, 0.0f),
			asdx::Vector3(0.0f, 1.0f, 0.0f));

		// 射影行列.
		auto proj = asdx::Matrix::CreatePerspectiveFieldOfView(
			asdx::ToRadian(37.5f),
			float(m_Width) / float(m_Height),
			1.0f,
			1000.0f);

		SceneParam res = {};
		res.InvView     = asdx::Matrix::Invert(view);
		res.InvViewProj = asdx::Matrix::Invert(proj) * res.InvView;

		m_SceneBuffer.SwapBuffer();
		m_SceneBuffer.Update(&res, sizeof(res));
	}

	// レイトレ実行.
	{
		// グローバルルートシグニチャ設定.
		pCmd->SetComputeRootSignature(m_GlobalRootSig.GetPtr());

		// ステートオブジェクト設定.
		pCmd->SetPipelineState1(m_StateObject.GetPtr());

		// リソースをバインド.
		asdx::SetTable(pCmd, ROOT_PARAM_U0, m_Canvas      .GetUAV(), true);
		asdx::SetSRV  (pCmd, ROOT_PARAM_T0, m_TLAS   .GetResource(), true);
		asdx::SetTable(pCmd, ROOT_PARAM_T1, m_BackGround .GetView(), true);
		asdx::SetSRV  (pCmd, ROOT_PARAM_T2, m_IndexSRV   .GetPtr (), true);
		asdx::SetSRV  (pCmd, ROOT_PARAM_T3, m_VertexSRV  .GetPtr (), true);
		asdx::SetCBV  (pCmd, ROOT_PARAM_B0, m_SceneBuffer.GetView(), true);

		D3D12_DISPATCH_RAYS_DESC desc = {};
		desc.HitGroupTable              = m_HitGroupTable.GetTableView();
		desc.MissShaderTable            = m_MissTable    .GetTableView();
		desc.RayGenerationShaderRecord  = m_RayGenTable  .GetRecordView();
		desc.Width                      = UINT(m_Canvas.GetDesc().Width);
		desc.Height                     = m_Canvas.GetDesc().Height;
		desc.Depth                      = 1;

		pCmd->DispatchRays(&desc);

		// バリアを張っておく.
		asdx::BarrierUAV(pCmd, m_Canvas.GetResource());
	}

	// レイトレ結果をバックバッファにコピー.
	{
		asdx::ScopedTransition b0(pCmd, m_Canvas.GetResource(), asdx::STATE_UAV, asdx::STATE_COPY_SRC);
		asdx::ScopedTransition b1(pCmd, m_ColorTarget[idx].GetResource(), asdx::STATE_PRESENT, asdx::STATE_COPY_DST);
		pCmd->CopyResource(m_ColorTarget[idx].GetResource(), m_Canvas.GetResource());
	}

	pCmd->Close();

	ID3D12CommandList* pCmds[] = {
		pCmd
	};

	// 前フレームの描画の完了を待機.
	if (m_FrameWaitPoint.IsValid())
	{ m_pGraphicsQueue->Sync(m_FrameWaitPoint); }

	// コマンドを実行.
	m_pGraphicsQueue->Execute(1, pCmds);

	// 待機点を発行.
	m_FrameWaitPoint = m_pGraphicsQueue->Signal();

	// 画面に表示.
	Present(0);

	// フレーム同期.
	asdx::GfxDevice().FrameSync();
*/
}

}