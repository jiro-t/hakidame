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

class AccelerationStructure
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>  geometryDescs;
	std::vector<::Microsoft::WRL::ComPtr<ID3D12Resource>>  geometryMat;

	::Microsoft::WRL::ComPtr<ID3D12Resource> bottomLevelAccelerationStructure;
	::Microsoft::WRL::ComPtr<ID3D12Resource> topLevelAccelerationStructure;

	::Microsoft::WRL::ComPtr<ID3D12Resource> scratchResource;
	::Microsoft::WRL::ComPtr<ID3D12Resource> instanceDescs;
public:
	void ClearGeometory()
	{
		geometryMat.clear();
		geometryDescs.clear();
	}

	void AddGeometory(StaticMesh mesh,DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity() ) {
		geometryMat.push_back(CreateUploadResource(sizeof(DirectX::XMMATRIX), &matrix));

		D3D12_RAYTRACING_GEOMETRY_DESC geom_desc = {};
		geom_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geom_desc.Triangles.IndexBuffer = mesh.ibo.GetGPUVirtualAddress();
		geom_desc.Triangles.IndexCount = mesh.ibo.index_count;
		geom_desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
		geom_desc.Triangles.Transform3x4 = geometryMat.back()->GetGPUVirtualAddress();
		geom_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		geom_desc.Triangles.VertexCount = mesh.vbo.vert_count;
		geom_desc.Triangles.VertexBuffer.StartAddress = mesh.vbo.GetGPUVirtualAddress();
		geom_desc.Triangles.VertexBuffer.StrideInBytes = mesh.vbo.vert_stride;

		geometryDescs.push_back(geom_desc);
	}

	void Build()
	{
		::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
		auto& commandAllocator = commandAllocators[currentBackBufferIndex];
		device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,commandAllocators[currentBackBufferIndex].Get(),nullptr,IID_PPV_ARGS(&commandList));

		commandList->Reset(commandAllocator.Get(), nullptr);

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
		topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		topLevelInputs.Flags = buildFlags;
		topLevelInputs.NumDescs = 1;
		topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
		device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
		if(topLevelPrebuildInfo.ResultDataMaxSizeInBytes <= 0)
			return;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = topLevelInputs;
		bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		bottomLevelInputs.Flags = buildFlags;
		bottomLevelInputs.NumDescs = geometryDescs.size();
		bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		bottomLevelInputs.pGeometryDescs = &geometryDescs[0];
		device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
		if(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes <= 0)
			return;

		scratchResource = CreateResource(std::max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		bottomLevelAccelerationStructure = CreateResource(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		topLevelAccelerationStructure = CreateResource(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
		instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
		instanceDesc.InstanceMask = 1;
		instanceDesc.AccelerationStructure = bottomLevelAccelerationStructure->GetGPUVirtualAddress();
		instanceDescs = CreateUploadResource(sizeof(instanceDesc), &instanceDesc);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
		{
			bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
			bottomLevelBuildDesc.DestAccelerationStructureData = bottomLevelAccelerationStructure->GetGPUVirtualAddress();
		}

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
		{
			topLevelBuildDesc.DestAccelerationStructureData = topLevelAccelerationStructure->GetGPUVirtualAddress();
			topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
			topLevelBuildDesc.Inputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
		}
		/*
		dxrCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = bottomLevelAccelerationStructure.Get();
		commandList->ResourceBarrier(1,&barrier);
		dxrCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

		commandList->Close();
		ID3D12CommandList* commandLists[] = { commandList.Get() };
		commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

		wait();
		*/
	}
};

BOOL InitDXRDevice();

class DxrPipeline {
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

	void* rayGenShaderIdentifier;
	void* missShaderIdentifier;
	void* hitGroupShaderIdentifier;
public:
	D3D12_VIEWPORT view = { .MinDepth = 0.f,.MaxDepth = 1.f };
	D3D12_RECT scissor = {};

	void Create();
	void Dispatch();
};

/*
void Excute()
{
	asdx::GfxDevice().SetUploadCommand(pCmd);

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
		res.InvView = asdx::Matrix::Invert(view);
		res.InvViewProj = asdx::Matrix::Invert(proj) * res.InvView;

		m_SceneBuffer.SwapBuffer();
		m_SceneBuffer.Update(&res, sizeof(res));
	}

	// レイトレ実行.
	{
		pCmd->SetComputeRootSignature(m_GlobalRootSig.GetPtr());
		// ステートオブジェクト設定.
		pCmd->SetPipelineState1(m_StateObject.GetPtr());

		// リソースをバインド.
		asdx::SetTable(pCmd, ROOT_PARAM_U0, m_Canvas.GetUAV(), true);
		asdx::SetSRV(pCmd, ROOT_PARAM_T0, m_TLAS.GetResource(), true);
		asdx::SetTable(pCmd, ROOT_PARAM_T1, m_BackGround.GetView(), true);
		asdx::SetSRV(pCmd, ROOT_PARAM_T2, m_IndexSRV.GetPtr(), true);
		asdx::SetSRV(pCmd, ROOT_PARAM_T3, m_VertexSRV.GetPtr(), true);
		asdx::SetCBV(pCmd, ROOT_PARAM_B0, m_SceneBuffer.GetView(), true);

		D3D12_DISPATCH_RAYS_DESC desc = {};
		desc.HitGroupTable = m_HitGroupTable.GetTableView();
		desc.MissShaderTable = m_MissTable.GetTableView();
		desc.RayGenerationShaderRecord = m_RayGenTable.GetRecordView();
		desc.Width = UINT(m_Canvas.GetDesc().Width);
		desc.Height = m_Canvas.GetDesc().Height;
		desc.Depth = 1;

		pCmd->DispatchRays(&desc);

		// バリアを張っておく.
		asdx::BarrierUAV(pCmd, m_Canvas.GetResource());
	}


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
	{
		m_pGraphicsQueue->Sync(m_FrameWaitPoint);
	}
	m_pGraphicsQueue->Execute(1, pCmds);

	m_FrameWaitPoint = m_pGraphicsQueue->Signal();
	Present(0);
	asdx::GfxDevice().FrameSync();
}
*/
}