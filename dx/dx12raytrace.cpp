#include "dx12raytrace.hpp"

char path_shader[] = "\
#define MAX_COUNT 0xFFFF\
#define FLT_MAX 3.402823466e+38F\
#define MIN_BOUNCES 3\
#define USE_PCG 1\
#define RIS_CANDIDATES_LIGHTS 8\
#define SHADOW_RAY_IN_RIS 0\
struct Attributes\
{\
	float2 uv;\
};\
\
struct VertexAttributes\
{\
	float3 position;\
	float3 shadingNormal;\
	float3 geometryNormal;\
	float2 uv;\
};\
cbuffer RaytracingDataCB : register(b0)\
{\
	RaytracingData gData;\
}\
\
RWTexture2D<float4> RTOutput						: register(u0);\
RWTexture2D<float4> accumulationBuffer				: register(u1);\
\
RaytracingAccelerationStructure sceneBVH : register(t0);\
\
StructuredBuffer<MaterialData> materials			: register(t1);\
ByteAddressBuffer indices[MAX_COUNT]		: register(t0, space1);\
ByteAddressBuffer vertices[MAX_COUNT]		: register(t0, space2);\
Texture2D<float4> textures[MAX_COUNT]		: register(t0, space3);\
\
[shader(\"closesthit\")]\
void ClosestHit(inout HitInfo payload, Attributes attrib)\
{\
	uint materialID;\
	uint geometryID;\
	unpackInstanceID(InstanceID(), materialID, geometryID);\
\
	float3 barycentrics = float3((1.0f - attrib.uv.x - attrib.uv.y), attrib.uv.x, attrib.uv.y);\
	VertexAttributes vertex = GetVertexAttributes(geometryID, PrimitiveIndex(), barycentrics);\
\
	payload.encodedNormals = encodeNormals(vertex.geometryNormal, vertex.shadingNormal);\
	payload.hitPosition = vertex.position;\
	payload.materialID = materialID;\
	payload.uvs = float16_t2(vertex.uv);\
}\
\
[shader(\"anyhit\")]\
void AnyHit(inout HitInfo payload : SV_RayPayload, Attributes attrib : SV_IntersectionAttributes)\
{\
	if (testOpacityAnyHit(attrib)) IgnoreHit();\
}\
\
[shader(\"anyhit\")]\
void AnyHitShadow(inout ShadowHitInfo payload : SV_RayPayload, Attributes attrib : SV_IntersectionAttributes)\
{\
	if (testOpacityAnyHit(attrib))\
		IgnoreHit();\
	else\
		AcceptHitAndEndSearch();\
}\
\
[shader(\"miss\")]\
void Miss(inout HitInfo payload)\
{\
	payload.materialID = INVALID_ID;\
}\
\
[shader(\"miss\")]\
void MissShadow(inout ShadowHitInfo payload)\
{\
	payload.hasHit = false;\
}\
\
[shader(\"raygeneration\")]\
void RayGen()\
{\
	uint2 LaunchIndex = DispatchRaysIndex().xy;\
	uint2 LaunchDimensions = DispatchRaysDimensions().xy;\
\
	RngStateType rngState = initRNG(LaunchIndex, LaunchDimensions, gData.frameNumber);\
\
	float2 pixel = float2(DispatchRaysIndex().xy);\
	const float2 resolution = float2(DispatchRaysDimensions().xy);\
\
	// Antialiasing (optional)\
	if (gData.enableAntiAliasing) {\
		// Add a random offset to the pixel 's screen coordinates .\
		float2 offset = float2(rand(rngState), rand(rngState));\
		pixel += lerp(-0.5.xx, 0.5.xx, offset);\
	}\
\
	pixel = (((pixel + 0.5f) / resolution) * 2.0f - 1.0f);\
\
	RayDesc ray = generatePrimaryRay(pixel, rngState);\
	HitInfo payload = (HitInfo)0;\
\
	float3 radiance = float3(0.0f, 0.0f, 0.0f);\
	float3 throughput = float3(1.0f, 1.0f, 1.0f);\
\
	// Start the ray tracing loop\
	for (int bounce = 0; bounce < gData.maxBounces; bounce++) {\
		TraceRay(\
			sceneBVH,\
			RAY_FLAG_NONE,\
			0xFF,\
			STANDARD_RAY_INDEX,\
			0,\
			STANDARD_RAY_INDEX,\
			ray,\
			payload);\
\
		if (!payload.hasHit()) {\
			radiance += throughput * loadSkyValue(ray.Direction);\
			break;\
		}\
\
		float3 geometryNormal;\
		float3 shadingNormal;\
		decodeNormals(payload.encodedNormals, geometryNormal, shadingNormal);\
\
		float3 V = -ray.Direction;\
		if (dot(geometryNormal, V) < 0.0f) geometryNormal = -geometryNormal;\
		if (dot(geometryNormal, shadingNormal) < 0.0f) shadingNormal = -shadingNormal;\
\
		MaterialProperties material = loadMaterialProperties(payload.materialID, payload.uvs);\
\
		radiance += throughput * material.emissive;\
\
		Light light;\
		float lightWeight;\
		if (sampleLightRIS(rngState, payload.hitPosition, geometryNormal, light, lightWeight)) {\
\
			float3 lightVector;\
			float lightDistance;\
			getLightData(light, payload.hitPosition, lightVector, lightDistance);\
			float3 L = normalize(lightVector);\
\
			if (SHADOW_RAY_IN_RIS || castShadowRay(payload.hitPosition, geometryNormal, L, lightDistance))\
			{\
				radiance += throughput * evalCombinedBRDF(shadingNormal, L, V, material) * (getLightIntensityAtPoint(light, lightDistance) * lightWeight);\
			}\
		}\
	\
		if (bounce == gData.maxBounces - 1) break;\
\
		// Russian Roulette\
		if (bounce > MIN_BOUNCES) {\
			float rrProbability = min(0.95f, luminance(throughput));\
			if (rrProbability < rand(rngState)) break;\
			else throughput /= rrProbability;\
		}\
\
		int brdfType;\
		if (material.metalness == 1.0f && material.roughness == 0.0f) {\
			brdfType = SPECULAR_TYPE;\
		}\
		else {\
			float brdfProbability = getBrdfProbability(material, V, shadingNormal);\
\
			if (rand(rngState) < brdfProbability) {\
				brdfType = SPECULAR_TYPE;\
				throughput /= brdfProbability;\
			}else {\
				brdfType = DIFFUSE_TYPE;\
				throughput /= (1.0f - brdfProbability);\
			}\
		}\
\
		float3 brdfWeight;\
		float2 u = float2(rand(rngState), rand(rngState));\
		if (!evalIndirectCombinedBRDF(u, shadingNormal, geometryNormal, V, material, brdfType, ray.Direction, brdfWeight)) {\
			break; // Ray was eaten by the surface :(\
		}\
\
		throughput *= brdfWeight;\
		ray.Origin = offsetRay(payload.hitPosition, geometryNormal);\
	}\
\
	float3 previousColor = accumulationBuffer[LaunchIndex].rgb;\
	float3 accumulatedColor = radiance;\
	if (gData.enableAccumulation) accumulatedColor = previousColor + radiance;\
	accumulationBuffer[LaunchIndex] = float4(accumulatedColor, 1.0f);\
\
	RTOutput[LaunchIndex] = float4(linearToSrgb(accumulatedColor / gData.accumulatedFrames * gData.exposureAdjustment), 1.0f);\
}\
";

char ray_shader[] = "\
#define VERTEX_STRIDE   (12 * 4)\
#define INDEX_STRIDE    (3 * 4)\
#define COLOR_OFFSET    (8 * 4)\
\
typedef BuiltInTriangleIntersectionAttributes HitArgs;\
\
struct SceneParam\
{\
	float4x4    InvView;\
	float4x4    InvViewProj;\
};\
\
struct RayPayload\
{\
	float4 Color;\
	float3 RayDir;\
};\
\
struct Vertex\
{\
	float4 Position;\
	float4 Normal;\
	float4 Color;\
};\
\
RWTexture2D<float4>             RenderTarget  : register(u0);\
RaytracingAccelerationStructure Scene : register(t0);\
ByteAddressBuffer               Indices : register(t1);\
ByteAddressBuffer               Vertices : register(t2);\
ConstantBuffer<SceneParam>      SceneParam    : register(b0);\
\
SamplerState                    LinearSampler : register(s0);\
\
uint3 GetIndices(uint triangleIndex)\
{\
	uint address = triangleIndex * INDEX_STRIDE;\
	return Indices.Load3(address);\
}\
\
Vertex GetVertex(uint triangleIndex, float3 barycentrices)\
{\
	uint3 indices = GetIndices(triangleIndex);\
	Vertex v = (Vertex)0;\
\
	[unroll]\
	for (uint i = 0; i < 3; ++i)\
	{\
		uint address = indices[i] * VERTEX_STRIDE;\
		v.Position += asfloat(Vertices.Load3(address)) * barycentrices[i];\
\
		address += COLOR_OFFSET;\
		v.Color += asfloat(Vertices.Load4(address)) * barycentrices[i];\
	}\
\
	return v;\
}\
\
void CalcRay(float2 index, out float3 pos, out float3 dir)\
{\
	float4 orig = float4(0.0f, 0.0f, 0.0f, 1.0f);           // カメラの位置.\
	float4 screen = float4(-2.0f * index + 1.0f, 0.0f, 1.0f); // スクリーンの位置.\
\
	orig = mul(SceneParam.InvView, orig);\
	screen = mul(SceneParam.InvViewProj, screen);\
\
	// w = 1 に射影.\
	screen.xyz /= screen.w;\
\
	// レイの位置と方向を設定.\
	pos = orig.xyz;\
	dir = normalize(screen.xyz - orig.xyz);\
}\
\
[shader(\"raygeneration\")]\
void OnGenerateRay()\
{\
	float2 index = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();\
\
	// レイを生成.\
	float3 rayOrig;\
	float3 rayDir;\
	CalcRay(index, rayOrig, rayDir);\
\
	// ペイロード初期化.\
	RayPayload payload;\
	payload.RayDir = rayDir;\
	payload.Color = float4(0.0f, 0.0f, 0.0f, 0.0f);\
\
	// レイを設定.\
	RayDesc ray;\
	ray.Origin = rayOrig;\
	ray.Direction = rayDir;\
	ray.TMin = 1e-3f;\
	ray.TMax = 10000.0f;\
\
	// レイを追跡.\
	TraceRay(Scene, RAY_FLAG_NONE, ~0, 0, 1, 0, ray, payload);\
\
	// レンダーターゲットに格納.\
	RenderTarget[DispatchRaysIndex().xy] = payload.Color;\
}\
\
[shader(\"closesthit\")]\
void OnClosestHit(inout RayPayload payload, in HitArgs args)\
{\
	// 重心座標を求める.\
	float3 barycentrics = float3(\
		1.0f - args.barycentrics.x - args.barycentrics.y,\
		args.barycentrics.x,\
		args.barycentrics.y);\
\
	// プリミティブ番号取得.\
	uint triangleIndex = PrimitiveIndex();\
\
	// 頂点データ取得.\
	Vertex v = GetVertex(triangleIndex, barycentrics);\
\
	// カラーを格納.\
	payload.Color = v.Color;\
}\
\
[shader(\"miss\")]\
void OnMiss(inout RayPayload payload)\
{\
	// 色を設定.\
	payload.Color = float4(0,0,0,0);\
}\
";

namespace ino::d3d::dxr {

static ::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> dxrCommandList;
static ::Microsoft::WRL::ComPtr<ID3D12Resource> dxrRenderTarget;
static ::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dxrRtvHeap;

struct Vertex
{
	DirectX::XMVECTOR position;
	DirectX::XMVECTOR uv;
};

struct Material
{
	std::string name = "defaultMaterial";
	std::string texturePath = "";
	float  textureResolution = 512;
};

struct MaterialCB
{
	DirectX::XMFLOAT4 resolution;
};

struct ViewCB
{
	DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
	DirectX::XMFLOAT4 viewOriginAndTanHalfFovY = DirectX::XMFLOAT4(0, 0.f, 0.f, 0.f);
	DirectX::XMFLOAT2 resolution = DirectX::XMFLOAT2(1280, 720);
};

D3D12_SHADER_BYTECODE LoadShader(std::wstring_view path)
{
	D3D12_SHADER_BYTECODE ret;
	std::ifstream ifs;
	uint32_t size = 0;

	ifs.open(path.data(), std::ios::binary);
	while (!ifs.eof())
	{
		ifs.read(nullptr, 1);
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

BOOL InitDXRDevice()
{
	//Create DXR commandList
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[currentBackBufferIndex].Get(), nullptr, IID_PPV_ARGS(&dxrCommandList));

	// Create renderTarget
	D3D12_HEAP_PROPERTIES prop = {
		.Type = D3D12_HEAP_TYPE_UPLOAD,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 1,
		.VisibleNodeMask = 1,
	};
	D3D12_RESOURCE_DESC desc = {};
	desc.DepthOrArraySize = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Format = rtvFormat;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	desc.Width = d3d::screen_width;
	desc.Height = d3d::screen_height;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	device->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&dxrRenderTarget));

	D3D12_HEAP_PROPERTIES heapProp = {
		.Type = D3D12_HEAP_TYPE_CUSTOM,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_L0,
		.CreationNodeMask = 0,
		.VisibleNodeMask = 0
	};
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};
	device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&dxrRtvHeap));

	return TRUE;
}

void AccelerationStructure::ClearInstance() noexcept
{
	instanceMat.clear();
}

void AccelerationStructure::ClearGeometory() noexcept
{
	geometryDescs.clear();
}

void AccelerationStructure::AddGeometory(StaticMesh mesh) {
	D3D12_RAYTRACING_GEOMETRY_DESC geom_desc = {};
	geom_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geom_desc.Triangles.IndexBuffer = mesh.ibo.GetGPUVirtualAddress();
	geom_desc.Triangles.IndexCount = mesh.ibo.index_count;
	geom_desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	geom_desc.Triangles.Transform3x4 = 0;
	geom_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	geom_desc.Triangles.VertexCount = mesh.vbo.vert_count;
	geom_desc.Triangles.VertexBuffer.StartAddress = mesh.vbo.GetGPUVirtualAddress();
	geom_desc.Triangles.VertexBuffer.StrideInBytes = mesh.vbo.vert_stride;

	geometryDescs.push_back(geom_desc);
}

void AccelerationStructure::AddInstance(UINT objID, DirectX::XMMATRIX matrix)
{
	instanceMat.push_back( std::make_pair(objID,matrix) );
}

void AccelerationStructure::BuildBlas()
{
	::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	auto& commandAllocator = commandAllocators[currentBackBufferIndex];
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[currentBackBufferIndex].Get(), nullptr, IID_PPV_ARGS(&commandList));

	commandList->Reset(commandAllocator.Get(), nullptr);

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = buildFlags;
	topLevelInputs.NumDescs = 1;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
	if (topLevelPrebuildInfo.ResultDataMaxSizeInBytes <= 0)
		return;

	for (int i = 0; i < geometryDescs.size(); ++i)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = topLevelInputs;
		bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		bottomLevelInputs.Flags = buildFlags;
		bottomLevelInputs.NumDescs = 1;
		bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		bottomLevelInputs.pGeometryDescs = &geometryDescs[i];
		device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
		if (bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes <= 0)
			continue;

		scratchResource = CreateResource(std::max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		bottomLevelAccelerationStructure[i] = CreateResource(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	}

	topLevelAccelerationStructure = CreateResource(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = nullptr;
	for (int i = 0; i < geometryDescs.size(); ++i)
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
		{
			bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
			bottomLevelBuildDesc.DestAccelerationStructureData = bottomLevelAccelerationStructure[i]->GetGPUVirtualAddress();
		}
		dxrCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
	}
	commandList->ResourceBarrier(1, &barrier);

	commandList->Close();
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

	wait();
}

void AccelerationStructure::BuildTlas()
{
	::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	auto& commandAllocator = commandAllocators[currentBackBufferIndex];
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[currentBackBufferIndex].Get(), nullptr, IID_PPV_ARGS(&commandList));

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDesc(instanceMat.size());
	for (auto i:instanceMat)
	{
		D3D12_RAYTRACING_INSTANCE_DESC desc = {};
		memcpy(desc.Transform,&(i.second),sizeof(float)*4*3);
		desc.AccelerationStructure = bottomLevelAccelerationStructure[i.first]->GetGPUVirtualAddress();
		desc.Flags = 0;
		desc.InstanceID = 0;
		desc.InstanceMask = 1;
		desc.InstanceContributionToHitGroupIndex = i.first;
	}
	instanceDescs = CreateUploadResource(sizeof(instanceDesc), &instanceDesc[0]);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	{
		topLevelBuildDesc.DestAccelerationStructureData = topLevelAccelerationStructure->GetGPUVirtualAddress();
		topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
		topLevelBuildDesc.Inputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
	}

	dxrCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = topLevelAccelerationStructure.Get();
	commandList->ResourceBarrier(1, &barrier);

	commandList->Close();
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

	wait();
}

D3D12_RAYTRACING_GEOMETRY_DESC ToGeometoryDesc(StaticMesh mesh)
{
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};

	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.IndexBuffer = mesh.ibo.GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount = mesh.ibo.index_count;
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
	geometryDesc.Triangles.Transform3x4 = 0;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	geometryDesc.Triangles.VertexCount = mesh.vbo.vert_count;
	geometryDesc.Triangles.VertexBuffer.StartAddress = mesh.vbo.GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = mesh.vbo.vert_stride;

	return geometryDesc;
}

void DxrPipeline::Create()
{
	namespace DX = DirectX;

	// Global Root Signature
	{
		D3D12_DESCRIPTOR_RANGE ranges[2] = {};
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[1].NumDescriptors = 2;
		ranges[1].BaseShaderRegister = 1;
		ranges[1].RegisterSpace = 0;
		ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER rootParameters[4] = {};
		rootParameters[0/*OutputViewSlot*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[1/*AccelerationStructure*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParameters[1].Descriptor.ShaderRegister = 0;
		rootParameters[1].Descriptor.RegisterSpace = 0;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[2/*SceneConstantSlot*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[2].Descriptor.ShaderRegister = 0;
		rootParameters[2].Descriptor.RegisterSpace = 0;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[3/*VertexBuffer*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[3].DescriptorTable.pDescriptorRanges = &ranges[1];
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = _countof(rootParameters);
		desc.pParameters = rootParameters;
		desc.NumStaticSamplers = 1;
		desc.pStaticSamplers = &sampler;

		::Microsoft::WRL::ComPtr<ID3DBlob> signature;
		::Microsoft::WRL::ComPtr<ID3DBlob> error;
		D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&globalRootSignature));
	}

	// Local Root Signature
	{
		D3D12_DESCRIPTOR_RANGE ranges[4];
		// 1 buffer - index buffer.
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 1;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		// 1 buffer - current frame vertex buffer.
		ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[1].NumDescriptors = 1;
		ranges[1].BaseShaderRegister = 1;
		ranges[1].RegisterSpace = 1;
		ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		// 1 buffer - previous frame vertex buffer.
		ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[2].NumDescriptors = 1;
		ranges[2].BaseShaderRegister = 2;
		ranges[2].RegisterSpace = 1;
		ranges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		// 1 diffuse texture
		ranges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[3].NumDescriptors = 1;
		ranges[3].BaseShaderRegister = 3;
		ranges[3].RegisterSpace = 1;
		ranges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


		D3D12_ROOT_PARAMETER rootParameters[5];
		rootParameters[0/*SceneConstantSlot*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].Constants.Num32BitValues = sizeof(VertexMaterial);
		rootParameters[0].Constants.RegisterSpace = 0;
		rootParameters[0].Constants.ShaderRegister = 1;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[1/*IndexBuffer*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[1].DescriptorTable.pDescriptorRanges = &ranges[0];
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[2/*VertexBuffer*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[2].DescriptorTable.pDescriptorRanges = &ranges[1];
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[3/*FrameBuffer*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[3].DescriptorTable.pDescriptorRanges = &ranges[2];
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[4/*Texture*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[4].DescriptorTable.pDescriptorRanges = &ranges[3];
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = 1;
		desc.pParameters = rootParameters;
		desc.NumStaticSamplers = 0;
		desc.pStaticSamplers = nullptr;
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		::Microsoft::WRL::ComPtr<ID3DBlob> signature;
		::Microsoft::WRL::ComPtr<ID3DBlob> error;
		D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&localRootSignature));
	}

	//PipelineStateObject
	{	//SubObj:DXIL Lib
		D3D12_STATE_SUBOBJECT subObj = {};
		std::vector< D3D12_EXPORT_DESC> exports;
		{
			D3D12_EXPORT_DESC exp = {};
			exp.Name = L"inoRaygenShader";
			exp.ExportToRename = L"";
			exp.Flags = D3D12_EXPORT_FLAG_NONE;
			exports.push_back(exp);
		}
		{
			D3D12_EXPORT_DESC exp = {};
			exp.Name = L"inoClosestHitShader";
			exp.ExportToRename = L"";
			exp.Flags = D3D12_EXPORT_FLAG_NONE;
			exports.push_back(exp);
		}
		{
			D3D12_EXPORT_DESC exp = {};
			exp.Name = L"inoMissShader";
			exp.ExportToRename = L"";
			exp.Flags = D3D12_EXPORT_FLAG_NONE;
			exports.push_back(exp);
		}

		dxilLib.DXILLibrary = { ray_shader, ARRAYSIZE(ray_shader) };
		dxilLib.NumExports = exports.size();
		dxilLib.pExports = &exports[0];
		subObj.pDesc = &dxilLib;
		subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		subObjs.push_back(subObj);
	}
	{	//SubObj:HitGroup
		D3D12_STATE_SUBOBJECT subObj = {};
		hitGroup.ClosestHitShaderImport = L"inoClosestHitShader";
		hitGroup.HitGroupExport = L"inoHitGroup";
		hitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		subObj.pDesc = &hitGroup;
		subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		subObjs.push_back(subObj);
	}
	{	//SubObj:ShaderConfig
		D3D12_STATE_SUBOBJECT subObj = {};
		shaderConfig.MaxPayloadSizeInBytes = sizeof(DX::XMVECTOR);
		shaderConfig.MaxAttributeSizeInBytes = sizeof(DX::XMVECTOR);
		subObj.pDesc = &shaderConfig;
		subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		subObjs.push_back(subObj);
	}
	{	//SubObj:LocalRootSignature
		D3D12_STATE_SUBOBJECT subObj = {};
		subObj.pDesc = localRootSignature.Get();
		subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		subObjs.push_back(subObj);
	}
	{	//SubObj:GlobalRootSignature
		D3D12_STATE_SUBOBJECT subObj = {};
		subObj.pDesc = globalRootSignature.Get();
		subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
		subObjs.push_back(subObj);
	}
	{	//SubObj:PipelineConfig
		D3D12_STATE_SUBOBJECT subObj = {};
		pipelineConfig.MaxTraceRecursionDepth = 1;
		subObj.pDesc = &pipelineConfig;
		subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
		subObjs.push_back(subObj);
	}

	D3D12_STATE_OBJECT_DESC desc = {
		.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
		.NumSubobjects = static_cast<UINT>(subObjs.size()),
		.pSubobjects = &(subObjs[0])
	};
	device->CreateStateObject(&desc, IID_PPV_ARGS(&rtpso));

	//Create DescriptorHeap
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	// Allocate a heap for 3 descriptors:
	// 2 - vertex and index buffer SRVs
	// 1 - raytracing output texture SRV
	descriptorHeapDesc.NumDescriptors = 3;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask = 0;
	device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//Create Constant Buffer
	auto frameCount = d3d::currentBackBufferIndex;

	// Create the constant buffer memory and map the CPU and GPU addresses
	D3D12_HEAP_PROPERTIES heapProp = {
		.Type = D3D12_HEAP_TYPE_UPLOAD,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 1,
		.VisibleNodeMask = 1,
	};

	// Allocate one constant buffer per frame, since it gets updated every frame.
	D3D12_RESOURCE_DESC constantBufferDesc = {};
	constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	constantBufferDesc.Alignment = 0;
	constantBufferDesc.Width = d3d::currentBackBufferIndex*sizeof(AlignedSceneConstant);
	constantBufferDesc.Height = 1;
	constantBufferDesc.DepthOrArraySize = 1;
	constantBufferDesc.MipLevels = 1;
	constantBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	constantBufferDesc.SampleDesc.Count = 1;
	constantBufferDesc.SampleDesc.Quality = 0;
	constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	constantBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&constantBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&sceneConstants));

	//CD3DX12_RANGE readRange(0, 0);
	sceneConstants->Map(0, nullptr, reinterpret_cast<void**>(&mappedConstantData));

	//Create ShaderTable
	UINT shaderIdentifierSize;
	{
		::Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		rtpso.As(&stateObjectProperties);
		//GetShaderIdentifiers(stateObjectProperties.Get());
		rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"inoRaygenShader");
		missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"inoMissShader");
		hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"inoHitGroup");
		shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	}

	// Ray gen shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable rayGenShaderTable(device.Get(), numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
		rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize));
		resRayGenShaderTable = rayGenShaderTable.GetResource();
	}

	// Miss shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable missShaderTable(device.Get(), numShaderRecords, shaderRecordSize, L"MissShaderTable");
		missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
		resMissShaderTable = missShaderTable.GetResource();
	}

	// Hit group shader table
	{
		struct RootArguments {
			MaterialCB cb;
		} rootArguments;
		memcpy(&rootArguments.cb,&materialCB,sizeof(MaterialCB));

		/*
		UINT numShaderRecords = 0;
		for (auto& bottomLevelASGeometryPair : bottomLevelASGeometries)
		{
			auto& bottomLevelASGeometry = bottomLevelASGeometryPair.second;
			numShaderRecords += static_cast<UINT>(bottomLevelASGeometry.m_geometryInstances.size();
		}

		for (auto& geometryInstance : bottomLevelASGeometry.m_geometryInstances)
		{
			LocalRootSignature::RootArguments rootArgs;
			rootArgs.cb.materialID = geometryInstance.materialID;
			rootArgs.cb.isVertexAnimated = geometryInstance.isVertexAnimated;

			memcpy(&rootArgs.indexBufferGPUHandle, &geometryInstance.ib.gpuDescriptorHandle, sizeof(geometryInstance.ib.gpuDescriptorHandle));
			memcpy(&rootArgs.vertexBufferGPUHandle, &geometryInstance.vb.gpuDescriptorHandle, sizeof(geometryInstance.ib.gpuDescriptorHandle));
			memcpy(&rootArgs.previousFrameVertexBufferGPUHandle, &m_nullVertexBufferGPUhandle, sizeof(m_nullVertexBufferGPUhandle));
			memcpy(&rootArgs.albedoTextureGPUHandle, &geometryInstance.diffuseTexture, sizeof(geometryInstance.diffuseTexture));

			for (auto& hitGroupShaderID : hitGroupShaderIDs_TriangleGeometry)
			{
				hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderID, shaderIDSize, &rootArgs, sizeof(rootArgs)));
			}
		}
		*/

		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);
		ShaderTable hitGroupShaderTable(device.Get(), numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");
		hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));
		resHitGroupShaderTable = hitGroupShaderTable.GetResource();
	}
}

void DxrPipeline::Dispatch()
{
}

}