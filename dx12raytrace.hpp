#pragma once

#include "dx.hpp"
#include <algorithm>

#include <vector>

namespace ino::d3d 
{

constexpr UINT NUM_BLAS = 2;          // Triangle + AABB(Num BottomLevelAccelerationStructure:BLAS)
constexpr float AABB_WIDTH = 2;      // AABB width.
constexpr float AABB_DISTANCE = 2;

// DirectX Raytracing (DXR) attributes
::Microsoft::WRL::ComPtr<ID3D12Device5> dxrDevice;
::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> dxrCommandList;
::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> dxrGraphCommandList;
::Microsoft::WRL::ComPtr<ID3D12StateObject> dxrStateObject;

// Root signatures
::Microsoft::WRL::ComPtr<ID3D12RootSignature> raytracingGlobalRootSignature;
::Microsoft::WRL::ComPtr<ID3D12RootSignature> raytracingLocalRootSignature;

enum BottomLevelASType {
    Triangle = 0,
    AABB,       // Procedural geometry with an application provided AABB.
    BottomASCount
};

enum RayType{
        Radiance = 0,   // ~ Primary, reflected camera/view rays calculating color for each hit.
        Shadow,         // ~ Shadow/visibility rays, only testing for occlusion
        RayCount
};

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

template <class InstanceDescType, class BLASPtrType>
void BuildBotomLevelASInstanceDescs(BLASPtrType* bottomLevelASaddresses, ::Microsoft::WRL::ComPtr<ID3D12Resource>* instanceDescsResource)
{
    using namespace DirectX;

    std::vector<InstanceDescType> instanceDescs;
    instanceDescs.resize(NUM_BLAS);

    // Width of a bottom-level AS geometry.
    // Make the plane a little larger than the actual number of primitives in each dimension.
    const XMUINT3 NUM_AABB = XMUINT3(700, 1, 700);
    const XMFLOAT3 fWidth = XMFLOAT3(
        NUM_AABB.x * AABB_WIDTH + (NUM_AABB.x - 1) * AABB_DISTANCE,
        NUM_AABB.y * AABB_WIDTH + (NUM_AABB.y - 1) * AABB_DISTANCE,
        NUM_AABB.z * AABB_WIDTH + (NUM_AABB.z - 1) * AABB_DISTANCE);
    const XMVECTOR vWidth = XMLoadFloat3(&fWidth);

    // Bottom-level AS with a single plane.
    {
        auto& instanceDesc = instanceDescs[BottomLevelASType::Triangle];
        instanceDesc = {};
        instanceDesc.InstanceMask = 1;
        instanceDesc.InstanceContributionToHitGroupIndex = 0;
        instanceDesc.AccelerationStructure = bottomLevelASaddresses[BottomLevelASType::Triangle];

        // Calculate transformation matrix.
        XMFLOAT3 a(-0.35f, 0.0f, -0.35f);
        const XMVECTOR vBasePosition = vWidth * XMLoadFloat3(&a);

        // Scale in XZ dimensions.
        XMMATRIX mScale = XMMatrixScaling(fWidth.x, fWidth.y, fWidth.z);
        XMMATRIX mTranslation = XMMatrixTranslationFromVector(vBasePosition);
        XMMATRIX mTransform = mScale * mTranslation;
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), mTransform);
    }

    // Create instanced bottom-level AS with procedural geometry AABBs.
    // Instances share all the data, except for a transform.
    {
        auto& instanceDesc = instanceDescs[BottomLevelASType::AABB];
        instanceDesc = {};
        instanceDesc.InstanceMask = 1;

        // Set hit group offset to beyond the shader records for the triangle AABB.
        instanceDesc.InstanceContributionToHitGroupIndex = BottomLevelASType::AABB * RayType::RayCount;
        instanceDesc.AccelerationStructure = bottomLevelASaddresses[BottomLevelASType::AABB];

        // Move all AABBS above the ground plane.
        XMFLOAT3 a(0, AABB_WIDTH / 2, 0);
        XMMATRIX mTranslation = XMMatrixTranslationFromVector(XMLoadFloat3(&a));
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), mTranslation);
    }
    UINT64 bufferSize = static_cast<UINT64>(instanceDescs.size() * sizeof(instanceDescs[0]));
    AllocateUploadBuffer(device.Get(), instanceDescs.data(), bufferSize, &(*instanceDescsResource));
};

struct AccelerationStructureBuffers
{
    ::Microsoft::WRL::ComPtr<ID3D12Resource> scratch;
    ::Microsoft::WRL::ComPtr<ID3D12Resource> accelerationStructure;
    ::Microsoft::WRL::ComPtr<ID3D12Resource> instanceDesc;    // Used only for top-level AS
	UINT64                 ResultDataMaxSizeInBytes;
};

BOOL InitDXRDevice()
{
    // Initialize raytracing pipeline.
    HRESULT hr = S_OK;
    // Create raytracing interfaces: raytracing device and commandlist.
    {
        hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[num_swap_buffers].Get(), nullptr, IID_PPV_ARGS(&dxrGraphCommandList));
        if (hr != S_OK) return FALSE;
        hr = dxrGraphCommandList->Close();
        if (hr != S_OK) return FALSE;
        hr = device->QueryInterface(IID_PPV_ARGS(&dxrDevice));
        if (hr != S_OK) return FALSE;
        hr = dxrGraphCommandList->QueryInterface(IID_PPV_ARGS(&dxrCommandList));
        if (hr != S_OK) return FALSE;
    }

    enum GlobalRootSignatureParams {
        OutputViewSlot = 0,
        AccelerationStructureSlot,
        SceneConstantSlot,
        VertexBuffersSlot,
        Count
    };

    // Create root signatures for the shaders.
    // Global Root Signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    {
        D3D12_DESCRIPTOR_RANGE range;
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        range.NumDescriptors = 1;
        range.BaseShaderRegister = 0;
        range.RegisterSpace = 0;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        //CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
        //UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
//        CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootSignatureParams::Count];
//        rootParameters[GlobalRootSignatureParams::OutputViewSlot].InitAsDescriptorTable(1, &UAVDescriptor);
//        rootParameters[GlobalRootSignatureParams::AccelerationStructureSlot].InitAsShaderResourceView(0);
//        CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
//        SerializeAndCreateRaytracingRootSignature(globalRootSignatureDesc, &m_raytracingGlobalRootSignature);
    }

    // Local Root Signature
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    {
//        CD3DX12_ROOT_PARAMETER rootParameters[LocalRootSignatureParams::Count];
//        rootParameters[LocalRootSignatureParams::ViewportConstantSlot].InitAsConstants(SizeOfInUint32(m_rayGenCB), 0, 0);
//        CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
//        localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
//        SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &m_raytracingLocalRootSignature);
    }
    // Create a raytracing pipeline state object which defines the binding of shaders, state and resources to be used during raytracing.
//    CreateRaytracingPipelineStateObject();
    // Create a heap for descriptors.
//    CreateDescriptorHeap();
    // Build geometry to be used in the sample.
//    BuildGeometry();
    // Build raytracing acceleration structures from the generated geometry.
//    BuildAccelerationStructures();
    // Build shader tables, which define shaders and their local root arguments.
//    BuildShaderTables();
    // Create an output 2D texture to store the raytracing result to.
//    CreateRaytracingOutputResource();

    return TRUE;
}

//BotttomLevel
AccelerationStructureBuffers BuildBottomLevelAS(const D3D12_RAYTRACING_GEOMETRY_DESC geometryDescs[],UINT sizeGeometoryDescs, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags)
{
    auto commandList = dxrGraphCommandList;
    ::Microsoft::WRL::ComPtr<ID3D12Resource> scratch;
    ::Microsoft::WRL::ComPtr<ID3D12Resource> bottomLevelAS;

    // Get the size requirements for the scratch and AS buffers.
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;
    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelInputs.Flags = buildFlags;
    bottomLevelInputs.NumDescs = sizeGeometoryDescs;
    bottomLevelInputs.pGeometryDescs = geometryDescs;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
    dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
    if(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0)
        return AccelerationStructureBuffers();

    // Create a scratch buffer. //D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    scratch = CreateResource(bottomLevelPrebuildInfo.ScratchDataSizeInBytes,D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    bottomLevelAS = CreateResource(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    // bottom-level AS desc.
    {
        bottomLevelBuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
        bottomLevelBuildDesc.DestAccelerationStructureData = bottomLevelAS->GetGPUVirtualAddress();
    }

    // Build the acceleration structure.
    dxrCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

    AccelerationStructureBuffers bottomLevelASBuffers;
    bottomLevelASBuffers.accelerationStructure = bottomLevelAS;
    bottomLevelASBuffers.scratch = scratch;
    bottomLevelASBuffers.ResultDataMaxSizeInBytes = bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes;
    return bottomLevelASBuffers;
}

//TopLevel
AccelerationStructureBuffers BuildTopLevelAS(AccelerationStructureBuffers bottomLevelAS[],D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags)
{
    ::Microsoft::WRL::ComPtr<ID3D12Resource> scratch;
    ::Microsoft::WRL::ComPtr<ID3D12Resource> topLevelAS;

    // Get required sizes for an acceleration structure.
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = NUM_BLAS;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    if ( topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0 )
        return AccelerationStructureBuffers();

    scratch = CreateResource(topLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    topLevelAS = CreateResource(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    // Create instance descs for the bottom-level acceleration structures.
    ::Microsoft::WRL::ComPtr<ID3D12Resource> instanceDescsResource;
    {
        D3D12_RAYTRACING_INSTANCE_DESC instanceDescs[NUM_BLAS] = {};
        D3D12_GPU_VIRTUAL_ADDRESS bottomLevelASaddresses[NUM_BLAS] =
        {
            bottomLevelAS[0].accelerationStructure->GetGPUVirtualAddress(),
            bottomLevelAS[1].accelerationStructure->GetGPUVirtualAddress()
        };
        BuildBotomLevelASInstanceDescs<D3D12_RAYTRACING_INSTANCE_DESC>(bottomLevelASaddresses, &instanceDescsResource);
    }

    // Top-level AS desc
    {
        topLevelBuildDesc.DestAccelerationStructureData = topLevelAS->GetGPUVirtualAddress();
        topLevelInputs.InstanceDescs = instanceDescsResource->GetGPUVirtualAddress();
        topLevelBuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
    }

    // Build acceleration structure.
    dxrCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

    AccelerationStructureBuffers topLevelASBuffers;
    topLevelASBuffers.accelerationStructure = topLevelAS;
    topLevelASBuffers.instanceDesc = instanceDescsResource;
    topLevelASBuffers.scratch = scratch;
    topLevelASBuffers.ResultDataMaxSizeInBytes = topLevelPrebuildInfo.ResultDataMaxSizeInBytes;
    return topLevelASBuffers;
}

}