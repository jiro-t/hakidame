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
RaytracingAccelerationStructure Scene : register(t0);\
Texture2D<float4>               BackGround    : register(t1);\
ByteAddressBuffer               Indices : register(t2);\
ByteAddressBuffer               Vertices : register(t3);\
RWTexture2D<float4>             RenderTarget  : register(u0);\
ConstantBuffer<SceneParam>      SceneParam    : register(b0);\
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
	//D3D12_RESOURCE_DESC desc = {};
	//desc.DepthOrArraySize = 1;
	//desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	//desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	//desc.Width = d3d::screen_width;
	//desc.Height = d3d::screen_height;
	//desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	//desc.MipLevels = 1;
	//desc.SampleDesc.Count = 1;
	//desc.SampleDesc.Quality = 0;

	//const D3D12_HEAP_PROPERTIES heapProp =
	//{
	//	D3D12_HEAP_TYPE_DEFAULT,
	//	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	//	D3D12_MEMORY_POOL_UNKNOWN,
	//	0, 0
	//};

	//hr = dxrDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&dxr::frameBuffer));
	//desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//hr = dxrDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&dxr::accumulationBuffer));

	//	//Create Pipeline
	//	int paramCount;
	//	D3D12_ROOT_PARAMETER rootParam = {};
	//	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	////	rootParam.DescriptorTable.NumDescriptorRanges = _countof(ranges);
	////	rootParam.DescriptorTable.pDescriptorRanges = ranges;
	//
	//	D3D12_ROOT_PARAMETER rootParams[1] = { rootParam };
	//
	//	D3D12_STATIC_SAMPLER_DESC staticSampler = {};
	//	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	//	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	staticSampler.MipLODBias = 0.f;
	//	staticSampler.MaxAnisotropy = 1;
	//	staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	//	staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	//	staticSampler.MinLOD = 0.f;
	//	staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	//	staticSampler.ShaderRegister = 0;
	//	staticSampler.RegisterSpace = 0;
	//	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	//
	//	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	//	rootDesc.NumParameters = _countof(rootParams);
	//	rootDesc.pParameters = rootParams;
	//	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	//	rootDesc.NumStaticSamplers = 1;
	//	rootDesc.pStaticSamplers = &staticSampler;
	//
	//	ID3DBlob* sig;
	//	ID3DBlob* error;
	//	hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error);
	//	ID3D12RootSignature* pRootSig;
	//	hr = d3d::device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&d3d::dxr::globalRootSignature));
	//
	//	std::vector<D3D12_STATE_SUBOBJECT> subObjs;
	//	struct EntryType {
	//		std::wstring entryPoint;
	//		D3D12_STATE_SUBOBJECT_TYPE state;
	//	};
	//
	//	EntryType entries[] = {
	//		{L"RayGen",D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY},
	//		{L"Miss",D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY},
	//		{L"HitGroup",D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY},
	//		{L"MissShadow",D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY},
	//		{L"HitGroupShadow",D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY},
	//
	//		{L"AnyHit",D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY},
	//		{L"AnyHitShadow",D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP},
	//		{L"ClosestHit",D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP},
	//	};
	/*
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
	*/

	// Upload shader table to GPU
	//D3DResources::UploadToGPU(d3d, dxr.shaderTableData.data(), dxr.shaderTable, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

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

void dxr_pipeline::Create()
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

		D3D12_ROOT_PARAMETER param[6] = {};
		param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[0].DescriptorTable.NumDescriptorRanges = 1;
		param[0].DescriptorTable.pDescriptorRanges = &range[0];
		param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		param[1].Descriptor.ShaderRegister = 0;
		param[1].Descriptor.RegisterSpace = 0;
		param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		param[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[2].DescriptorTable.NumDescriptorRanges = 1;
		param[2].DescriptorTable.pDescriptorRanges = &range[1];
		param[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		param[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		param[3].Descriptor.ShaderRegister = 2;
		param[3].Descriptor.RegisterSpace = 0;
		param[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		param[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		param[4].Descriptor.ShaderRegister = 3;
		param[4].Descriptor.RegisterSpace = 0;
		param[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		param[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		param[5].Descriptor.ShaderRegister = 0;
		param[5].Descriptor.RegisterSpace = 0;
		param[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


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
		dxrDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&globalRootSignature));
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
		dxrDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&localRootSignature));
	}
/*
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
			= ((sizeof(RayGenConstantBuffer) - 1) / sizeof(UINT32) + 1);
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
*/
	//Create pipeline object
	D3D12_STATE_SUBOBJECT subObj = {};

	D3D12_EXPORT_DESC exports[3] = {
		{ L"OnGenerateRay", nullptr, D3D12_EXPORT_FLAG_NONE },
		{ L"OnClosestHit",  nullptr, D3D12_EXPORT_FLAG_NONE },
		{ L"OnMiss",        nullptr, D3D12_EXPORT_FLAG_NONE },
	};

	// グローバルルートシグニチャの設定.
	D3D12_GLOBAL_ROOT_SIGNATURE globalRootSig = {};
	globalRootSig.pGlobalRootSignature = globalRootSignature.Get();
	subObj.pDesc = globalRootSignature.Get();
	subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subObjs.push_back(subObj);

	// ローカルルートシグニチャの設定.
	D3D12_LOCAL_ROOT_SIGNATURE localRootSig = {};
	localRootSig.pLocalRootSignature = localRootSignature.Get();
	subObj.pDesc = localRootSignature.Get();
	subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subObjs.push_back(subObj);

	// DXILライブラリの設定.
	dxilLib.DXILLibrary = { simple_ray_shader, sizeof(simple_ray_shader) };
	dxilLib.NumExports = _countof(exports);
	dxilLib.pExports = exports;
	subObj.pDesc = &dxilLib;
	subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subObjs.push_back(subObj);

	// ヒットグループの設定.
	hitGroup.ClosestHitShaderImport = L"OnClosestHit";
	hitGroup.HitGroupExport = L"MyHitGroup";
	hitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	subObj.pDesc = &hitGroup;
	subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	subObjs.push_back(subObj);

	// シェーダ設定.
	shaderConfig.MaxPayloadSizeInBytes = sizeof(DX::XMVECTOR) + sizeof(DX::XMVECTOR);
	shaderConfig.MaxAttributeSizeInBytes = sizeof(DX::XMVECTOR);
	subObj.pDesc = &shaderConfig;
	subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	subObjs.push_back(subObj);

	// パイプライン設定.
	pipelineConfig.MaxTraceRecursionDepth = 1;
	subObj.pDesc = &pipelineConfig;
	subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	subObjs.push_back(subObj);

	D3D12_STATE_OBJECT_DESC desc = {
		.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
		.NumSubobjects = subObjs.size(),
		.pSubobjects = &(subObjs[0])
	};
	dxrDevice->CreateStateObject(&desc, IID_PPV_ARGS(&rtpso));
}

}