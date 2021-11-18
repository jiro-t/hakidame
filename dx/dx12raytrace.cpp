#include "dx12raytrace.hpp"

char simple_ray_shader[] = "\
struct Viewport\n\
{\n\
	float left;\n\
	float top;\n\
	float right;\n\
	float bottom;\n\
};\n\
\
struct RayGenConstantBuffer\n\
{\n\
	Viewport viewport;\n\
	Viewport stencil;\n\
};\n\
\
RaytracingAccelerationStructure Scene : register(t0, space0);\n\
RWTexture2D<float4> RenderTarget : register(u0);\n\
ConstantBuffer<RayGenConstantBuffer> g_rayGenCB : register(b0);\n\
\
typedef BuiltInTriangleIntersectionAttributes MyAttributes;\n\
struct RayPayload\n\
{\
	float4 color;\n\
};\n\
\
bool IsInsideViewport(float2 p, Viewport viewport)\n\
{\n\
	return (p.x >= viewport.left && p.x <= viewport.right)\n\
		&& (p.y >= viewport.top && p.y <= viewport.bottom);\n\
}\n\
\
[shader(\"raygeneration\")]\n\
void inoRaygenShader()\n\
{\n\
	float2 lerpValues = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();\n\
\
	// Orthographic projection since we're raytracing in screen space.\n\
	float3 rayDir = float3(0, 0, 1);\n\
	float3 origin = float3(\n\
		lerp(g_rayGenCB.viewport.left, g_rayGenCB.viewport.right, lerpValues.x),\n\
		lerp(g_rayGenCB.viewport.top, g_rayGenCB.viewport.bottom, lerpValues.y),\n\
		0.0f);\n\
\
	if (IsInsideViewport(origin.xy, g_rayGenCB.stencil))\n\
	{\n\
		// Trace the ray.\n\
		// Set the ray's extents.\n\
		RayDesc ray;\n\
		ray.Origin = origin;\n\
		ray.Direction = rayDir;\n\
		// Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.\n\
		// TMin should be kept small to prevent missing geometry at close contact areas.\n\
		ray.TMin = 0.001;\n\
		ray.TMax = 10000.0;\n\
		RayPayload payload = { float4(0, 0, 0, 0) };\n\
		TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);\n\
\
		// Write the raytraced color to the output texture.\n\
		RenderTarget[DispatchRaysIndex().xy] = payload.color;\n\
	}\n\
	else\n\
	{\n\
		// Render interpolated DispatchRaysIndex outside the stencil window\n\
		RenderTarget[DispatchRaysIndex().xy] = float4(lerpValues, 0, 1);\n\
	}\n\
}\
\
[shader(\"closesthit\")]\n\
void inoClosestHitShader(inout RayPayload payload, in MyAttributes attr)\n\
{\n\
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);\n\
	payload.color = float4(barycentrics, 1);\n\
}\n\
\
[shader(\"miss\")]\n\
void inoMissShader(inout RayPayload payload)\n\
{\n\
	payload.color = float4(0, 0, 0, 1);\n\
}\n";

/*
Color TracePath(Ray ray, count depth) {
	if (depth >= MaxDepth) {
		return Black;  // Bounced enough times.
	}

	ray.FindNearestObject();
	if (ray.hitSomething == false) {
		return Black;  // Nothing was hit.
	}

	Material material = ray.thingHit->material;
	Color emittance = material.emittance;

	// Pick a random direction from here and keep going.
	Ray newRay;
	newRay.origin = ray.pointWhereObjWasHit;

	// This is NOT a cosine-weighted distribution!
	newRay.direction = RandomUnitVectorInHemisphereOf(ray.normalWhereObjWasHit);

	// Probability of the newRay
	const float p = 1 / (2 * PI);

	// Compute the BRDF for this ray (assuming Lambertian reflection)
	float cos_theta = DotProduct(newRay.direction, ray.normalWhereObjWasHit);
	Color BRDF = material.reflectance / PI;

	// Recursively trace reflected light sources.
	Color incoming = TracePath(newRay, depth + 1);

	// Apply the Rendering Equation here.
	return emittance + (BRDF * incoming * cos_theta / p);
}

void Render(Image finalImage, count numSamples) {
	foreach(pixel in finalImage) {
		foreach(i in numSamples) {
			Ray r = camera.generateRay(pixel);
			pixel.color += TracePath(r, 0);
		}
		pixel.color /= numSamples;  // Average samples.
	}
}*/

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
};\
\
struct Vertex\
{\
	float4 Position;\
	float4 Normal;\
	float4 Color;\
};\
TriangleHitGroup inoHitGroup =\
{\
	\"\",\
	\"inoClosestHitShader\",\
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
ConstantBuffer<PrimitiveConstantBuffer> l_materialCB : register(b0, space1);\
StructuredBuffer<Index> indices : register(t0, space1);\
StructuredBuffer<VertexPositionNormalTextureTangent> vertices : register(t1, space1);\
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
void inoRaygenShader()\
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
void inoClosestHitShader(inout RayPayload payload, in HitArgs args)\
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
void inoMissShader(inout RayPayload payload)\
{\
	// 色を設定.\
	payload.Color = float4(0,0,0,0);\
}\
";

namespace ino::d3d::dxr {

static ::Microsoft::WRL::ComPtr<ID3D12Device5> dxrDevice;
static ::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> dxrCommandList;
static ::Microsoft::WRL::ComPtr<ID3D12Resource> dxrRenderTarget;
static ::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dxrRtvHeap;

struct Vertex
{
	DirectX::XMVECTOR position;
	DirectX::XMVECTOR uv;
};

struct ViewCB
{
	DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
	DirectX::XMFLOAT4 viewOriginAndTanHalfFovY = DirectX::XMFLOAT4(0, 0.f, 0.f, 0.f);
	DirectX::XMFLOAT2 resolution = DirectX::XMFLOAT2(1280, 720);
};

BOOL InitDXRDevice()
{
	device->QueryInterface(IID_PPV_ARGS(&dxrDevice));

	// Create renderTarget
	D3D12_HEAP_PROPERTIES prop = {
		.Type = D3D12_HEAP_TYPE_DEFAULT,
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

	device->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&dxrRenderTarget));

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
	geom_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
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
	auto& commandAllocator = commandAllocators[currentBackBufferIndex];
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[currentBackBufferIndex].Get(), nullptr, IID_PPV_ARGS(&dxrCommandList));

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = buildFlags;
	topLevelInputs.NumDescs = 1;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
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
		dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
		if (bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes <= 0)
			continue;

		scratchResource = CreateResource(std::max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		bottomLevelAccelerationStructure.push_back(CreateResource(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
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
	dxrCommandList->ResourceBarrier(1, &barrier);

	dxrCommandList->Close();
	ID3D12CommandList* commandLists[] = { dxrCommandList.Get() };
	commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

	wait();
}

void AccelerationStructure::BuildTlas()
{
	::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	auto& commandAllocator = commandAllocators[currentBackBufferIndex];
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[currentBackBufferIndex].Get(), nullptr, IID_PPV_ARGS(&dxrCommandList));

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
	dxrCommandList->ResourceBarrier(1, &barrier);

	dxrCommandList->Close();
	ID3D12CommandList* commandLists[] = { dxrCommandList.Get() };
	commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

	wait();
}

D3D12_SHADER_BYTECODE CompileLibShader(void* src, uint32_t size) noexcept
{
	::Microsoft::WRL::ComPtr <IDxcBlob> bc;

	::Microsoft::WRL::ComPtr<IDxcLibrary> lib;
	DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), &lib);
	::Microsoft::WRL::ComPtr<IDxcCompiler> compiler;
	DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), &compiler);
	::Microsoft::WRL::ComPtr<IDxcOperationResult> result;

	::Microsoft::WRL::ComPtr <IDxcBlobEncoding> enc;
	lib->CreateBlobWithEncodingOnHeapCopy(src, size, 932, &enc);
#ifdef _DEBUG
	std::cout << (char*)enc->GetBufferPointer() << std::endl;
#endif
	compiler->Compile(enc.Get(), L"", L"", L"lib_6_3", nullptr, 0, nullptr, 0, nullptr, &result);
	result->GetResult(&bc);
	::Microsoft::WRL::ComPtr <IDxcBlobEncoding> err;
	if (!bc)
	{
		result->GetErrorBuffer(&err);
#ifdef _DEBUG
		std::cout << (char*)err->GetBufferPointer() << std::endl;
#endif
	}

	D3D12_SHADER_BYTECODE ret = {};
	ret.BytecodeLength = bc->GetBufferSize();
	void* p = new byte[bc->GetBufferSize()];
	memcpy(p, bc->GetBufferPointer(), bc->GetBufferSize());
	ret.pShaderBytecode = p;

	return ret;
}

void DxrPipeline::Create()
{
	namespace DX = DirectX;

	// Global Root Signature
	{
		D3D12_DESCRIPTOR_RANGE ranges[1] = {};
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;// 1 output texture
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

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

		//rootParameters[3/*VertexBuffer*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		//rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
		//rootParameters[3].DescriptorTable.pDescriptorRanges = &ranges[1];
		//rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

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
		D3D12_DESCRIPTOR_RANGE ranges[2];
		// 1 buffer - index and vertex buffer.
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
		//// 1 diffuse texture
		//ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		//ranges[2].NumDescriptors = 1;
		//ranges[2].BaseShaderRegister = 3;
		//ranges[2].RegisterSpace = 1;
		//ranges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


		D3D12_ROOT_PARAMETER rootParameters[3];
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

		//rootParameters[4/*Texture*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		//rootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
		//rootParameters[4].DescriptorTable.pDescriptorRanges = &ranges[3];
		//rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

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

		dxilLib.DXILLibrary = CompileLibShader(simple_ray_shader, sizeof(simple_ray_shader));
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
		shaderConfig.MaxAttributeSizeInBytes = 2 * sizeof(float);
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
	dxrDevice->CreateStateObject(&desc, IID_PPV_ARGS(&rtpso));

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
	constantBufferDesc.Width = (d3d::currentBackBufferIndex+1)*sizeof(AlignedSceneConstant);
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
}

void DxrPipeline::CreateShaderTable(AccelerationStructure& as)
{
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
		rayGenShaderTableStride = rayGenShaderTable.GetShaderRecordSize();
		resRayGenShaderTable = rayGenShaderTable.GetResource();
	}

	// Miss shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable missShaderTable(device.Get(), numShaderRecords, shaderRecordSize, L"MissShaderTable");
		missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
		missShaderTableStride = missShaderTable.GetShaderRecordSize();
		resMissShaderTable = missShaderTable.GetResource();
	}

	// Hit group shader table
	{
		struct RootArguments {
			VertexMaterial cb;

			D3D12_GPU_DESCRIPTOR_HANDLE indexBuffer;
			D3D12_GPU_DESCRIPTOR_HANDLE vertexBuffer;
		} rootArguments;

		int materialID = 1;
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);
		ShaderTable hitGroupShaderTable(device.Get(), as.geometryDescs.size(), shaderRecordSize, L"HitGroupShaderTable");

		for (auto& geometryInstance : as.geometryDescs)
		{
			memcpy(&rootArguments.cb, &materialCB, sizeof(VertexMaterial));

			UINT shaderIDSize = shaderIdentifierSize + sizeof(RootArguments);

			memcpy(&rootArguments.indexBuffer, &geometryInstance.Triangles.IndexBuffer, sizeof(geometryInstance.Triangles.IndexBuffer));
			memcpy(&rootArguments.vertexBuffer, &geometryInstance.Triangles.VertexBuffer.StartAddress, sizeof(geometryInstance.Triangles.VertexBuffer.StartAddress));
			//memcpy(&rootArguments.albedoTextureGPUHandle, &geometryInstance.diffuseTexture, sizeof(geometryInstance.diffuseTexture));

			hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIDSize, &rootArguments, sizeof(RootArguments)));
			++materialID;
		}

		hitGroupShaderTableStride = hitGroupShaderTable.GetShaderRecordSize();
		resHitGroupShaderTable = hitGroupShaderTable.GetResource();
	}
}

void DxrPipeline::Dispatch()
{
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[currentBackBufferIndex].Get(), nullptr, IID_PPV_ARGS(&dxrCommandList));

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	dispatchDesc.HitGroupTable.StartAddress = resHitGroupShaderTable->GetGPUVirtualAddress();
	dispatchDesc.HitGroupTable.SizeInBytes = resHitGroupShaderTable->GetDesc().Width;
	dispatchDesc.HitGroupTable.StrideInBytes = hitGroupShaderTableStride;
	dispatchDesc.MissShaderTable.StartAddress = resMissShaderTable->GetGPUVirtualAddress();
	dispatchDesc.MissShaderTable.SizeInBytes = resMissShaderTable->GetDesc().Width;
	dispatchDesc.MissShaderTable.StrideInBytes = missShaderTableStride;
	dispatchDesc.RayGenerationShaderRecord.StartAddress = resRayGenShaderTable->GetGPUVirtualAddress();
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = resRayGenShaderTable->GetDesc().Width;
	dispatchDesc.Width = screen_width;
	dispatchDesc.Height = screen_height;
	dispatchDesc.Depth = 1;
	dxrCommandList->SetPipelineState1(rtpso.Get());

	dxrCommandList->DispatchRays(&dispatchDesc);
}

void DxrPipeline::CopyToScreen()
{
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
	dxrCommandList->ResourceBarrier(ARRAYSIZE(pre_barrier), pre_barrier);
	dxrCommandList->CopyResource(renderTargets[d3d::currentBackBufferIndex].GetHandle().Get(), dxrRenderTarget.Get());

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
	dxrCommandList->ResourceBarrier(ARRAYSIZE(post_barrier), post_barrier);
	dxrCommandList->Close();
}

}