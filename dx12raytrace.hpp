#pragma once

#include "dx.hpp"
#include <algorithm>

#include <vector>

namespace ino::d3d
{

constexpr UINT NUM_BLAS = 2;          // Triangle + AABB(Num BottomLevelAccelerationStructure:BLAS)
constexpr float AABB_WIDTH = 2;      // AABB width.
constexpr float AABB_DISTANCE = 2;
/*
// Raytracing scene
RayGenConstantBuffer m_rayGenCB;

// Geometry
typedef UINT16 Index;
struct Vertex { float v1, v2, v3; };
ComPtr<ID3D12Resource> m_indexBuffer;
ComPtr<ID3D12Resource> m_vertexBuffer;

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

enum class BottomLevelASType {
    Triangle = 0,
    AABB,
    UNDEFINED
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
        auto& instanceDesc = instanceDescs[static_cast<int>(BottomLevelASType::Triangle)];
        instanceDesc = {};
        instanceDesc.InstanceMask = 1;
        instanceDesc.InstanceContributionToHitGroupIndex = 0;
        instanceDesc.AccelerationStructure = bottomLevelASaddresses[static_cast<int>(BottomLevelASType::Triangle)];

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
        auto& instanceDesc = instanceDescs[static_cast<int>(BottomLevelASType::AABB)];
        instanceDesc = {};
        instanceDesc.InstanceMask = 1;

        // Set hit group offset to beyond the shader records for the triangle AABB.
        instanceDesc.InstanceContributionToHitGroupIndex = static_cast<int>(BottomLevelASType::AABB) * static_cast<int>(RayType::UNDEFINED);
        instanceDesc.AccelerationStructure = bottomLevelASaddresses[static_cast<int>(BottomLevelASType::AABB)];

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

//    CreateRaytracingPipelineStateObject();
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

class dxr_pipeline {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
    // Root signatures
    ::Microsoft::WRL::ComPtr<ID3D12RootSignature> globalRootSignature;
    ::Microsoft::WRL::ComPtr<ID3D12RootSignature> localRootSignature;


    ::Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
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

    void Create(
        D3D12_INPUT_ELEMENT_DESC* elementDescs,
        UINT elemntCount,
        BOOL enableDepth,
        D3D12_BLEND_DESC const& blendDesc = defaultBlendDesc(),
        D3D12_RASTERIZER_DESC const& rasterDesc = defaultRasterizerDesc())
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
            //Create root signature
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

        // Local Root Signature
        // This is a root signature that enables a shader to have unique arguments that come from shader tables.
        {
            params[globalParamCount+static_cast<int>(GlobalRootSignatureParams::OutputViewSlot)].ParameterType
                = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            params[globalParamCount+static_cast<int>(GlobalRootSignatureParams::OutputViewSlot)].ShaderVisibility
                = D3D12_SHADER_VISIBILITY_ALL;
            params[globalParamCount+static_cast<int>(GlobalRootSignatureParams::OutputViewSlot)].Descriptor.ShaderRegister
                = 0;
            //D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE
            //Create root signature
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
/*
                .InitAsConstants(SizeOfInUint32(m_rayGenCB), 0, 0);
                CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
                localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
                params[static_cast<int>(LocalRootSignatureParams::CubeConstantSlot)];
                SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &m_raytracingLocalRootSignature);
*/
        }
    }
};

/*
*void D3D12RaytracingHelloWorld::BuildAccelerationStructures()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto commandList = m_deviceResources->GetCommandList();
    auto commandQueue = m_deviceResources->GetCommandQueue();
    auto commandAllocator = m_deviceResources->GetCommandAllocator();

    // Reset the command list for the acceleration structure construction.
    commandList->Reset(commandAllocator, nullptr);

    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.IndexBuffer = m_indexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexCount = static_cast<UINT>(m_indexBuffer->GetDesc().Width) / sizeof(Index);
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
    geometryDesc.Triangles.Transform3x4 = 0;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexCount = static_cast<UINT>(m_vertexBuffer->GetDesc().Width) / sizeof(Vertex);
    geometryDesc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

    // Mark the geometry as opaque. 
    // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
    // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    // Get required sizes for an acceleration structure.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = 1;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = topLevelInputs;
    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.pGeometryDescs = &geometryDesc;
    m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
    ThrowIfFalse(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    ComPtr<ID3D12Resource> scratchResource;
    AllocateUAVBuffer(device, max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"ScratchResource");

    // Allocate resources for acceleration structures.
    // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
    // Default heap is OK since the application doesn稚 need CPU read/write access to them. 
    // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
    // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
    //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
    //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
    {
        D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
    
        AllocateUAVBuffer(device, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_bottomLevelAccelerationStructure, initialResourceState, L"BottomLevelAccelerationStructure");
        AllocateUAVBuffer(device, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_topLevelAccelerationStructure, initialResourceState, L"TopLevelAccelerationStructure");
    }

    // Create an instance desc for the bottom-level acceleration structure.
    ComPtr<ID3D12Resource> instanceDescs;
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
    instanceDesc.InstanceMask = 1;
    instanceDesc.AccelerationStructure = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
    AllocateUploadBuffer(device, &instanceDesc, sizeof(instanceDesc), &instanceDescs, L"InstanceDescs");

    // Bottom Level Acceleration Structure desc
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
    {
        bottomLevelBuildDesc.Inputs = bottomLevelInputs;
        bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
        bottomLevelBuildDesc.DestAccelerationStructureData = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
    }

    // Top Level Acceleration Structure desc
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    {
        topLevelInputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
        topLevelBuildDesc.Inputs = topLevelInputs;
        topLevelBuildDesc.DestAccelerationStructureData = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
        topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
    }

    auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
    {
        raytracingCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_bottomLevelAccelerationStructure.Get()));
        raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
    };

    // Build acceleration structure.
    BuildAccelerationStructure(m_dxrCommandList.Get());
    
    // Kick off acceleration structure construction.
    m_deviceResources->ExecuteCommandList();

    // Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
    m_deviceResources->WaitForGpu();
}
*/

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

}