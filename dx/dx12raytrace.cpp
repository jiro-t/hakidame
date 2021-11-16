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

char simple_ray_shader[] = "\
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
		D3D12_DESCRIPTOR_RANGE range[2] = {};
		range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		range[0].NumDescriptors = 1;
		range[0].BaseShaderRegister = 0;
		range[0].RegisterSpace = 0;
		range[0].OffsetInDescriptorsFromTableStart = 0;

		range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		range[1].NumDescriptors = 1;
		range[1].BaseShaderRegister = 1;
		range[1].RegisterSpace = 0;
		range[1].OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER param[5] = {};
		//u0
		param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[0].DescriptorTable.NumDescriptorRanges = 1;
		param[0].DescriptorTable.pDescriptorRanges = &range[0];
		param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		//t0
		param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		param[1].Descriptor.ShaderRegister = 0;
		param[1].Descriptor.RegisterSpace = 0;
		param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		//t1
		param[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		param[2].Descriptor.ShaderRegister = 2;
		param[2].Descriptor.RegisterSpace = 0;
		param[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		//t2
		param[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		param[3].Descriptor.ShaderRegister = 3;
		param[3].Descriptor.RegisterSpace = 0;
		param[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		//b0
		param[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		param[4].Descriptor.ShaderRegister = 0;
		param[4].Descriptor.RegisterSpace = 0;
		param[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


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
		desc.NumParameters = _countof(param);
		desc.pParameters = param;
		desc.NumStaticSamplers = 1;
		desc.pStaticSamplers = &sampler;

		::Microsoft::WRL::ComPtr<ID3DBlob> signature;
		::Microsoft::WRL::ComPtr<ID3DBlob> error;
		D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&globalRootSignature));
	}
	// Local Root Signature
	{
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = 0;
		desc.pParameters = nullptr;
		desc.NumStaticSamplers = 0;
		desc.pStaticSamplers = nullptr;
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		::Microsoft::WRL::ComPtr<ID3DBlob> signature;
		::Microsoft::WRL::ComPtr<ID3DBlob> error;
		D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&localRootSignature));
	}

	//Create pipeline object
	D3D12_STATE_SUBOBJECT subObj = {};
	D3D12_EXPORT_DESC exports[3] = {
		{ L"OnGenerateRay", nullptr, D3D12_EXPORT_FLAG_NONE },
		{ L"OnClosestHit",  nullptr, D3D12_EXPORT_FLAG_NONE },
		{ L"OnMiss",        nullptr, D3D12_EXPORT_FLAG_NONE },
	};

	D3D12_GLOBAL_ROOT_SIGNATURE globalRootSig = {};
	globalRootSig.pGlobalRootSignature = globalRootSignature.Get();
	subObj.pDesc = globalRootSignature.Get();
	subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subObjs.push_back(subObj);

	D3D12_LOCAL_ROOT_SIGNATURE localRootSig = {};
	localRootSig.pLocalRootSignature = localRootSignature.Get();
	subObj.pDesc = localRootSignature.Get();
	subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subObjs.push_back(subObj);

	dxilLib.DXILLibrary = { simple_ray_shader, sizeof(simple_ray_shader) };
	dxilLib.NumExports = _countof(exports);
	dxilLib.pExports = exports;
	subObj.pDesc = &dxilLib;
	subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subObjs.push_back(subObj);

	hitGroup.ClosestHitShaderImport = L"OnClosestHit";
	hitGroup.HitGroupExport = L"MyHitGroup";
	hitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	subObj.pDesc = &hitGroup;
	subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	subObjs.push_back(subObj);

	shaderConfig.MaxPayloadSizeInBytes = sizeof(DX::XMVECTOR) + sizeof(DX::XMVECTOR);
	shaderConfig.MaxAttributeSizeInBytes = sizeof(DX::XMVECTOR);
	subObj.pDesc = &shaderConfig;
	subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	subObjs.push_back(subObj);

	pipelineConfig.MaxTraceRecursionDepth = 1;
	subObj.pDesc = &pipelineConfig;
	subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	subObjs.push_back(subObj);

	D3D12_STATE_OBJECT_DESC desc = {
		.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
		.NumSubobjects = static_cast<UINT>(subObjs.size()),
		.pSubobjects = &(subObjs[0])
	};
	device->CreateStateObject(&desc, IID_PPV_ARGS(&rtpso));

	// shader table
	{
		::Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> props;
		rtpso->QueryInterface(IID_PPV_ARGS(&props));

		rayGenShaderIdentifier = props->GetShaderIdentifier(L"OnGenerateRay");
		missShaderIdentifier = props->GetShaderIdentifier(L"OnMiss");
		hitGroupShaderIdentifier = props->GetShaderIdentifier(L"MyHitGroup");
	}
	/*
	// Ray gen shader table
	{
		struct RootArguments {
			RayGenConstantBuffer cb;
		} rootArguments;
		rootArguments.cb = m_rayGenCB;

		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);
		ShaderTable rayGenShaderTable(device, numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
		rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));
		m_rayGenShaderTable = rayGenShaderTable.GetResource();
	}

	// Miss shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable missShaderTable(device, numShaderRecords, shaderRecordSize, L"MissShaderTable");
		missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
		m_missShaderTable = missShaderTable.GetResource();
	}

	// Hit group shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable hitGroupShaderTable(device, numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");
		hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize));
		m_hitGroupShaderTable = hitGroupShaderTable.GetResource();
	}
	*/
}

void DxrPipeline::Dispatch()
{
	::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList;
	auto& commandAllocator = commandAllocators[currentBackBufferIndex];
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[currentBackBufferIndex].Get(), nullptr, IID_PPV_ARGS(&commandList));

	commandList->SetComputeRootSignature(globalRootSignature.Get());
	commandList->SetPipelineState1(rtpso.Get());

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE gen = {};
	D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE miss = {};
	D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE hit = {};

	D3D12_DISPATCH_RAYS_DESC desc = {};
	//desc.HitGroupTable = m_HitGroupTable.GetTableView();
	//desc.MissShaderTable = m_MissTable.GetTableView();
	//desc.RayGenerationShaderRecord = m_RayGenTable.GetRecordView();
	desc.Width = d3d::screen_width;
	desc.Height = d3d::screen_height;
	desc.Depth = 1;

	commandList->DispatchRays(&desc);


	D3D12_RESOURCE_BARRIER pre_barrier[2] = {
		{
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition = {
				.pResource = renderTargets[d3d::currentBackBufferIndex].GetHandle().Get(),
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
				.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST,
			},
		},
		{
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition = {
				.pResource = renderTargets[d3d::currentBackBufferIndex].GetHandle().Get(),
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE,
			},
		},
	};
	commandList->ResourceBarrier(ARRAYSIZE(pre_barrier), pre_barrier);
	commandList->CopyResource(renderTargets[d3d::currentBackBufferIndex].GetHandle().Get(), dxrRenderTarget.Get());

	D3D12_RESOURCE_BARRIER post_barrier[2] = {
		{
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition = {
				.pResource = renderTargets[d3d::currentBackBufferIndex].GetHandle().Get(),
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
				.StateAfter = D3D12_RESOURCE_STATE_PRESENT,
			},
		},
		{
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition = {
				.pResource = renderTargets[d3d::currentBackBufferIndex].GetHandle().Get(),
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE,
				.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			},
		},
	};
	commandList->ResourceBarrier(ARRAYSIZE(post_barrier), post_barrier);

	commandList->Close();
}

}