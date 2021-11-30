#include "dx12raytrace.hpp"

char simple_ray_shader[] = "\
struct Vertex{\n\
	float4 pos;\n\
	float4 color;\n\
	float4 texcoord;\n\
};\n\
struct SceneConstantBuffer\n\
{\n\
	float4 cameraPos;\n\
	float4x4 invViewProj;\n\
	float time;\n\
};\n\
struct InstanceConstantBuffer\n\
{\n\
	float4x4 modelMatrix; \n\
	float emit;\n\
	float kr;\n\
	int materialID;\n\
};\n\
\
RWTexture2D<float4> RenderTarget : register(u0);\n\
RWTexture2D<float4> normalRenderTarget : register(u1);\n\
RWTexture2D<float4> worldRenderTarget : register(u2);\n\
RaytracingAccelerationStructure Scene : register(t0, space0);\n\
ConstantBuffer<SceneConstantBuffer> sceneCB : register(b0);\n\
\
ByteAddressBuffer Indices : register(t1, space0);\n\
StructuredBuffer<Vertex> Vertices : register(t2, space0); \n\
ConstantBuffer<InstanceConstantBuffer> instanceCB : register(b1);\n\
Texture2D<float4> albedo[] : register(t3,space0);\n\
SamplerState                    LinearSampler : register(s0); \
\n\
static const int SAMPLE_NUM = 8;\n\
\n\
float rand(float2 texCoord, int Seed)\n\
{\n\
	return frac(sin(dot(texCoord.xy+float2(sceneCB.time,sceneCB.time), float2(12.9898, 78.233)) + Seed) * 43758.5453); \n\
}\n\
typedef BuiltInTriangleIntersectionAttributes inoAttributes;\n\
struct RayPayload\n\
{\
	float4 color;\n\
	float3 rayOrig;\n\
	float3 rayDir;\n\
	float3 normal;\n\
	float3 worldPos;\n\
	uint depth;\n\
};\n\
\
RayPayload shootRay(RayPayload payload)\n\
{\n\
	RayDesc ray; \n\
	ray.Origin = payload.rayOrig; \n\
	ray.Direction = payload.rayDir; \n\
	ray.TMin = 0.001; \n\
	ray.TMax = 1000.0; \n\
\n\
	TraceRay(Scene, RAY_FLAG_NONE, ~0, 0, 1, 0, ray, payload); \n\
	return payload;\n\
}\n\
[shader(\"raygeneration\")]\n\
void inoRaygenShader()\n\
{\n\
\n\
	float2 lerpValues = ((float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions())*2.0 -1.0;\n\
	float2 screenPos = lerpValues;\n\
	screenPos.y = -screenPos.y;\n\
	float4 world = mul(float4(screenPos, 0, 1), sceneCB.invViewProj);\n\
	world.xyz /= world.w;\n\
	float3 origin = sceneCB.cameraPos.xyz;\n\
	float3 rayDir = normalize(world.xyz - origin);\n\
\
	RayPayload payload = { float4(0, 0, 0, 0),origin,rayDir,float3(0,0,0),float3(0,0,0),0 };\n\
	payload = shootRay(payload);\n\
	RenderTarget[DispatchRaysIndex().xy] = float4(payload.color.xyz/SAMPLE_NUM,1);\n\
	normalRenderTarget[DispatchRaysIndex().xy] = float4(abs(payload.normal.xyz),1);\n\
	worldRenderTarget[DispatchRaysIndex().xy] = float4(payload.worldPos.xyz,1);\n\
}\n\
\
float4 HitAttribute(float4 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)\n\
{\n\
	return vertexAttribute[0] +\n\
		attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +\n\
		attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);\n\
}\n\
[shader(\"closesthit\")]\n\
void inoClosestHitShader(inout RayPayload payload, in inoAttributes attr)\n\
{\n\
	if(payload.depth > 3)\n\
		return;\n\
	uint indexSizeInBytes = 4;\n\
	uint indicesPerTriangle = 3;\n\
	uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;\n\
	uint baseIndex = PrimitiveIndex() * triangleIndexStride;\n\
	const uint3 indices = Indices.Load3(baseIndex);\n\
\n\
	float4 vertexPosition[3] = {\n\
		Vertices[indices[0]].pos,\n\
		Vertices[indices[1]].pos,\n\
		Vertices[indices[2]].pos\n\
	};\n\
	float4 vertexColor[3] = {\n\
		Vertices[indices[0]].color,\n\
		Vertices[indices[1]].color,\n\
		Vertices[indices[2]].color\n\
	};\n\
\n\
	float3 norm = normalize(mul(instanceCB.modelMatrix,cross(\n\
		(vertexPosition[1] - vertexPosition[0]).xyz, \n\
		(vertexPosition[2] - vertexPosition[0]).xyz ))); \n\
\n\
	float4 color = float4(0,0,0,0);\n\
	float4 vColor = HitAttribute(vertexColor,attr);\n\
	float4 emit = vColor*instanceCB.emit;\n\
	payload.depth += 1;\n\
	float3 newPos = mul(instanceCB.modelMatrix,HitAttribute(vertexPosition,attr)).xyz;\n\
	for(int i = 0;i < SAMPLE_NUM;++i)\n\
	{\n\
		float theta = (rand(newPos.xy, 0+i)*2.0-1.0)*3.14159263;\n\
		float phi = rand(newPos.xy, SAMPLE_NUM + i)*3.14159263; \n\
		float3 newDir = normalize(float3(cos(theta)*cos(phi),cos(theta)*sin(phi),sin(theta))*0.3 + norm); \n\
		float p = 1 / (2 * 3.14159264);\n\
		float cos_theta = dot(abs(newDir),abs(norm));\n\
\n\
		payload.rayDir = newDir; \n\
		payload.rayOrig = newPos + newDir*0.001; \n\
		payload = shootRay(payload); \n\
		color += emit + (cos_theta*instanceCB.kr*vColor/(3.14159264))*payload.color / p; \n\
	}\n\
	payload.color = color;\n\
	payload.normal = norm;\n\
	payload.worldPos = newPos;\n\
}\n\
[shader(\"closesthit\")]\n\
void inoClosestHitShaderWithTexture(inout RayPayload payload, in inoAttributes attr)\n\
{\n\
	if(payload.depth > 3)\n\
		return;\n\
	uint indexSizeInBytes = 4;\n\
	uint indicesPerTriangle = 3;\n\
	uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;\n\
	uint baseIndex = PrimitiveIndex() * triangleIndexStride;\n\
	const uint3 indices = Indices.Load3(baseIndex);\n\
\n\
	float4 vertexPosition[3] = {\n\
		Vertices[indices[0]].pos,\n\
		Vertices[indices[1]].pos,\n\
		Vertices[indices[2]].pos\n\
	};\n\
	float4 vertexNormal[3] = {\n\
		Vertices[indices[0]].color,\n\
		Vertices[indices[1]].color,\n\
		Vertices[indices[2]].color\n\
	};\n\
	float4 vertexTexcoord[3] = {\n\
		Vertices[indices[0]].texcoord,\n\
		Vertices[indices[1]].texcoord,\n\
		Vertices[indices[2]].texcoord\n\
	};\n\
\n\
	float3 norm = normalize(mul(instanceCB.modelMatrix,HitAttribute(vertexNormal,attr))); \n\
\n\
	float4 color = float4(0,0,0,0);\n\
	float2 uv = HitAttribute(vertexTexcoord,attr).xy;\n\
	float4 vColor = float4(albedo[instanceCB.materialID].SampleLevel(LinearSampler, uv,0.0f).xyz,1);\n\
	float4 emit = vColor*instanceCB.emit;\n\
	payload.depth += 1;\n\
	float3 newPos = mul(instanceCB.modelMatrix,HitAttribute(vertexPosition,attr)).xyz;\n\
	for(int i = 0;i < SAMPLE_NUM;++i)\n\
	{\n\
		float theta = (rand(newPos.xy, 0+i)*2.0-1.0)*3.14159263;\n\
		float phi = rand(newPos.xy, SAMPLE_NUM + i)*3.14159263; \n\
		float3 newDir = normalize(float3(cos(theta)*cos(phi),cos(theta)*sin(phi),sin(theta))*0.3 + norm); \n\
		float p = 1 / (2 * 3.14159264);\n\
		float cos_theta = dot(abs(newDir),abs(norm));\n\
\n\
		payload.rayDir = newDir; \n\
		payload.rayOrig = newPos + newDir*0.001; \n\
		payload = shootRay(payload); \n\
		color += emit + cos_theta*(cos_theta*instanceCB.kr*vColor/(3.14159264))*payload.color / p; \n\
	}\n\
	payload.color = color;\n\
	payload.normal = norm;\n\
	payload.worldPos = newPos;\n\
}\n\
\
[shader(\"miss\")]\n\
void inoMissShader(inout RayPayload payload)\n\
{\n\
	payload.color = float4(0,0,0,0);\n\
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

namespace ino::d3d::dxr {

static ::Microsoft::WRL::ComPtr<ID3D12Device5> dxrDevice;
static ::Microsoft::WRL::ComPtr<ID3D12Resource> dxrRenderTarget[3];
static ::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dxrHeap[3] = {};

BOOL InitDXRDevice()
{
	device->QueryInterface(IID_PPV_ARGS(&dxrDevice));

	// Create renderTarget
	for (int i = 0; i < 3; ++i)
	{
		D3D12_HEAP_PROPERTIES prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(rtvFormat, screen_width, screen_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		device->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&dxrRenderTarget[i]));

		D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		};
		device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&dxrHeap[i]));

		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		device->CreateUnorderedAccessView(dxrRenderTarget[i].Get(), nullptr, &UAVDesc, dxrHeap[i]->GetCPUDescriptorHandleForHeapStart());
	}

	return TRUE;
}

void Blas::Build(StaticMesh& mesh)
{
	auto& commandAllocator = commandAllocators[currentBackBufferIndex];
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[currentBackBufferIndex].Get(), nullptr, IID_PPV_ARGS(&commandList));

	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE/*D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE*/;
	geometryDesc.Triangles.IndexBuffer = mesh.ibo.GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount = mesh.ibo.index_count;
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	geometryDesc.Triangles.Transform3x4 = 0;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.VertexCount = mesh.vbo.vert_count;
	geometryDesc.Triangles.VertexBuffer.StartAddress = mesh.vbo.GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = mesh.vbo.vert_stride;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInput = {};
	bottomLevelInput.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInput.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	bottomLevelInput.NumDescs = 1;
	bottomLevelInput.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInput.pGeometryDescs = &geometryDesc;
	dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInput, &bottomLevelPrebuildInfo);
	if (bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes <= 0)
		return;
	bottomLevelAccelerationStructure = CreateResource(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	scratchResource = CreateResource(bottomLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	{
		bottomLevelBuildDesc.Inputs = bottomLevelInput;
		bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
		bottomLevelBuildDesc.DestAccelerationStructureData = bottomLevelAccelerationStructure->GetGPUVirtualAddress();
	}
	commandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

	auto barrier(CD3DX12_RESOURCE_BARRIER::UAV(bottomLevelAccelerationStructure.Get()));

	commandList->ResourceBarrier(1, &barrier);

	commandList->Close();
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

	wait();
}

void Tlas::ClearInstance() noexcept
{
	instanceMat.clear();
	instanceTex.clear();
}
void Tlas::AddInstance(Blas* blas, InstanceConstant cb)
{
	cb.modelMatrix = DirectX::XMMatrixTranspose(cb.modelMatrix);
	instanceMat.push_back(std::make_pair(blas,cb));
	instanceTex.push_back(nullptr);
}
void Tlas::AddInstance(Blas* blas, texture* albedo, InstanceConstant cb)
{
	cb.modelMatrix = DirectX::XMMatrixTranspose(cb.modelMatrix);
	instanceMat.push_back(std::make_pair(blas, cb));
	instanceTex.push_back(albedo);
}
void Tlas::Build()
{
	auto& commandAllocator = commandAllocators[currentBackBufferIndex];
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[currentBackBufferIndex].Get(), nullptr, IID_PPV_ARGS(&commandList));

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.NumDescs = 1024;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
	if (topLevelPrebuildInfo.ResultDataMaxSizeInBytes <= 0)
		return;

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDesc;
	int instanceID = 0;
	for (auto& i : instanceMat)
	{
		D3D12_RAYTRACING_INSTANCE_DESC desc = {};
		memcpy(desc.Transform, &(i.second.modelMatrix), sizeof(float) * 4 * 3);
		desc.AccelerationStructure = i.first->Get()->GetGPUVirtualAddress();
		desc.Flags = 0;
		desc.InstanceID = instanceID;
		desc.InstanceMask = 0xFF;
		desc.InstanceContributionToHitGroupIndex = instanceID;
		instanceID++;

		instanceDesc.push_back(desc);
	}
	if(!scratchResource)
		scratchResource = CreateResource(topLevelPrebuildInfo.ScratchDataSizeInBytes,D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	if(!topLevelAccelerationStructure)
		topLevelAccelerationStructure = CreateResource(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	instanceDescs = CreateUploadResource(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDesc.size(), &instanceDesc[0]);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	{
		D3D12_RAYTRACING_INSTANCE_DESC* ptr = nullptr;
		auto hr = instanceDescs->Map(0, nullptr, reinterpret_cast<void**>(&ptr));
		memcpy(ptr, &instanceDesc[0], sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDesc.size());
		instanceDescs->Unmap(0, nullptr);

		topLevelBuildDesc.DestAccelerationStructureData = topLevelAccelerationStructure->GetGPUVirtualAddress();
		topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
		topLevelBuildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		topLevelBuildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		topLevelBuildDesc.Inputs.NumDescs = instanceID;
		topLevelBuildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		topLevelBuildDesc.Inputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
	}

	commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = topLevelAccelerationStructure.Get();
	commandList->ResourceBarrier(1, &barrier);

	commandList->Close();
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

	wait();
}

D3D12_SHADER_BYTECODE CompileLibShader(void* src, uint32_t size) noexcept
{
	::Microsoft::WRL::ComPtr <IDxcBlob> bc;
	::Microsoft::WRL::ComPtr<IDxcOperationResult> result;

	::Microsoft::WRL::ComPtr <IDxcBlobEncoding> enc;
	lib->CreateBlobWithEncodingOnHeapCopy(src, size, 932, &enc);

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
		static D3D12_DESCRIPTOR_RANGE ranges[4] = {};
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;// 1 output texture
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;// 1 output texture
		ranges[1].NumDescriptors = 1;
		ranges[1].BaseShaderRegister = 1;
		ranges[1].RegisterSpace = 0;
		ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;// 1 output texture
		ranges[2].NumDescriptors = 1;
		ranges[2].BaseShaderRegister = 2;
		ranges[2].RegisterSpace = 0;
		ranges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		ranges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[3].NumDescriptors = 1;
		ranges[3].BaseShaderRegister = 3;
		ranges[3].RegisterSpace = 0;
		ranges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER rootParameters[6] = {};
		rootParameters[0/*OutputViewSlot*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[1/*OutputViewSlot*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[1].DescriptorTable.pDescriptorRanges = &ranges[1];
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[2/*OutputViewSlot*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[2].DescriptorTable.pDescriptorRanges = &ranges[2];
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[3/*AccelerationStructure*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParameters[3].Descriptor.ShaderRegister = 0;
		rootParameters[3].Descriptor.RegisterSpace = 0;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[4/*SceneConstantSlot*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[4].Constants.Num32BitValues = sizeof(SceneConstant)/sizeof(uint32_t);
		rootParameters[4].Constants.RegisterSpace = 0;
		rootParameters[4].Constants.ShaderRegister = 0;
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[5/*Texture*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[5].DescriptorTable.pDescriptorRanges = &ranges[3];
		rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

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
#ifdef _DEBUG
		if (error)
		{
			std::cout << (char*)error->GetBufferPointer() << std::endl;
		}
#endif
		device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&globalRootSignature));
	}

	// Local Root Signature
	{
		D3D12_DESCRIPTOR_RANGE ranges[1];
		// 1 diffuse texture
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 3;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


		D3D12_ROOT_PARAMETER rootParameters[3];
		rootParameters[0/*IndexBuffer*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParameters[0].Descriptor.ShaderRegister = 1;
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[1/*VertexBuffer*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParameters[1].Descriptor.ShaderRegister = 2;
		rootParameters[1].Descriptor.RegisterSpace = 0;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[2/*InstanceConstantSlot*/].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[2].Constants.Num32BitValues = sizeof(InstanceConstant)/sizeof(uint32_t);
		rootParameters[2].Constants.ShaderRegister = 1;
		rootParameters[2].Constants.RegisterSpace = 0;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = _countof(rootParameters);
		desc.pParameters = rootParameters;
		desc.NumStaticSamplers = 0;
		desc.pStaticSamplers = 0;
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		::Microsoft::WRL::ComPtr<ID3DBlob> signature;
		::Microsoft::WRL::ComPtr<ID3DBlob> error;
		D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
#ifdef _DEBUG
		if (error)
		{
			std::cout << (char*)error->GetBufferPointer() << std::endl;
		}
#endif
		device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&localRootSignature));
	}

	//PipelineStateObject
	{	//SubObj:DXIL Lib
		static std::wstring shader_names[4] = {
			L"inoRaygenShader",
			L"inoClosestHitShader",
			L"inoClosestHitShaderWithTexture",
			L"inoMissShader"
		};
		auto lib = pipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
		auto byteCode = CompileLibShader(simple_ray_shader, sizeof(simple_ray_shader));
		lib->SetDXILLibrary(&byteCode);
		{
			for (std::wstring const& s : shader_names)
				lib->DefineExport(s.c_str());
		}
	}
	{	//SubObj:HitGroup
		auto hitGroup = pipelineDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
		hitGroup->SetClosestHitShaderImport(L"inoClosestHitShader");
		hitGroup->SetHitGroupExport(L"inoHitGroup");
		hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

		auto hitGroup2 = pipelineDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
		hitGroup2->SetClosestHitShaderImport(L"inoClosestHitShaderWithTexture");
		hitGroup2->SetHitGroupExport(L"inoHitGroup2");
		hitGroup2->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
	}
	{	//SubObj:ShaderConfig
		auto shaderConfig = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
		UINT payloadSize = (4+3+3+3+3) * sizeof(float) + sizeof(uint32_t);   // float4 color
		UINT attributeSize = 2 * sizeof(float); // float2 barycentrics
		shaderConfig->Config(payloadSize, attributeSize);
	}
	{	//SubObj:LocalRootSignature
		auto localRootSignatureSub = pipelineDesc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
		localRootSignatureSub->SetRootSignature(localRootSignature.Get());
		// Shader association
		//auto rootSignatureAssociation = pipelineDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
		//rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignatureSub);
		//rootSignatureAssociation->AddExport(L"inoClosestHitShader");
	}
	{	//SubObj:GlobalRootSignature
		auto globalRootSignatureSub = pipelineDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
		globalRootSignatureSub->SetRootSignature(globalRootSignature.Get());
	}
	{	//SubObj:PipelineConfig
		auto pipelineConfig = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
		UINT maxRecursionDepth = 24; // ~ primary rays only. 
		pipelineConfig->Config(maxRecursionDepth);
	}
	dxrDevice->CreateStateObject(pipelineDesc, IID_PPV_ARGS(&rtpso));

	//Buffer Object
	sceneConstantBuffer.Create();

	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
}

void DxrPipeline::CreateShaderTable(Tlas& as)
{
	Sleep(24);
	//Create ShaderTable
	UINT shaderIdentifierSize;
	{
		::Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		rtpso.As(&stateObjectProperties);
		rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"inoRaygenShader");
		missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"inoMissShader");
		hitGroupShaderIdentifier[0] = stateObjectProperties->GetShaderIdentifier(L"inoHitGroup");
		hitGroupShaderIdentifier[1] = stateObjectProperties->GetShaderIdentifier(L"inoHitGroup2");
		shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	}

	// Ray gen shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable rayGenShaderTable(dxrDevice.Get(), numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
		rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize));
		rayGenShaderTableStride = rayGenShaderTable.GetShaderRecordSize();
		resRayGenShaderTable = rayGenShaderTable.GetResource();
	}

	// Miss shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable missShaderTable(dxrDevice.Get(), numShaderRecords, shaderRecordSize, L"MissShaderTable");
		missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
		missShaderTableStride = missShaderTable.GetShaderRecordSize();
		resMissShaderTable = missShaderTable.GetResource();
	}

	// Hit group shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize + sizeof(HitArgs);
		ShaderTable hitGroupShaderTable(dxrDevice.Get(), numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");

		instanceConstants.clear();
		for (int i = 0; i < as.GetBlasCount(); ++i)
		{
			auto& instCB = as.GetConstant(i);
			instanceConstants.push_back(CreateUploadResource(sizeof(InstanceConstant), &instCB));
			auto cbAddr = instanceConstants.back()->GetGPUVirtualAddress();
			HitArgs cb = {};
			memcpy(&cb.indexBufferGPUHandle, &as.GetBlas(i)->IndexBuffer(), sizeof(as.GetBlas(i)->IndexBuffer()));
			memcpy(&cb.vertexBufferGPUHandle, &as.GetBlas(i)->VertexBuffer().StartAddress, sizeof(as.GetBlas(i)->VertexBuffer().StartAddress));
			memcpy(&cb.constantBuffer, &cbAddr, sizeof(instanceConstants.back()->GetGPUVirtualAddress()));
			if (as.GetAlbedo(i) != nullptr)
			{
				instanceConstants.push_back(as.GetAlbedo(i)->GetHandle());
	//			auto texAddr = as.GetAlbedo(i)->GetHeap()->GetGPUDescriptorHandleForHeapStart();
				memcpy(&cb.albedoTexture, &cbAddr, sizeof(instanceConstants.back()->GetGPUVirtualAddress()) );
				hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier[1], shaderIdentifierSize, &cb, sizeof(cb)));
			}
			else
			{
				hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier[0], shaderIdentifierSize, &cb, sizeof(cb)));
			}
		}
		hitGroupShaderTableStride = hitGroupShaderTable.GetShaderRecordSize();
		resHitGroupShaderTable = hitGroupShaderTable.GetResource();
	}
}

void DxrPipeline::Dispatch(Tlas&as)
{
	dxrCommandList->SetComputeRootSignature(globalRootSignature.Get());

	for (int i = 0; i < 3; ++i)
	{
		dxrCommandList->SetDescriptorHeaps(1, dxrHeap[i].GetAddressOf());
		dxrCommandList->SetComputeRootDescriptorTable(i, dxrHeap[i]->GetGPUDescriptorHandleForHeapStart());
	}
	dxrCommandList->SetComputeRootShaderResourceView(3, as.Get()->GetGPUVirtualAddress());
	sceneConstantBuffer.SetToCompute(dxrCommandList.Get(), sceneCB, 4);

	std::vector<ID3D12DescriptorHeap*> texHeaps;
	for (int i = 0; i < as.GetBlasCount(); ++i)
	{
		if (as.GetAlbedo(i) == nullptr)
			continue;
		texHeaps.push_back( as.GetAlbedo(i)->GetHeap().Get() );
	}

	if (texHeaps.size())
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = (uint32_t)texHeaps.size(),
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		};
		device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&albedoHeap));

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(albedoHeap->GetCPUDescriptorHandleForHeapStart());
		int offsetSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		for (int i = 0; i < as.GetBlasCount(); ++i)
		{
			if (as.GetAlbedo(i) == nullptr)
				continue;
			D3D12_SHADER_RESOURCE_VIEW_DESC resourct_view_desc{};
			resourct_view_desc.Format = as.GetAlbedo(i)->GetHandle()->GetDesc().Format;
			resourct_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			resourct_view_desc.Texture2D.MipLevels = 1;
			resourct_view_desc.Texture2D.MostDetailedMip = 0;
			resourct_view_desc.Texture2D.PlaneSlice = 0;
			resourct_view_desc.Texture2D.ResourceMinLODClamp = 0.0F;
			resourct_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			device->CreateShaderResourceView(as.GetAlbedo(i)->GetHandle().Get(), &resourct_view_desc, handle);
			handle.ptr += offsetSize;
		}
		dxrCommandList->SetDescriptorHeaps(1, albedoHeap.GetAddressOf());
		dxrCommandList->SetComputeRootDescriptorTable(5, albedoHeap->GetGPUDescriptorHandleForHeapStart());
	}

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	dispatchDesc.RayGenerationShaderRecord.StartAddress = resRayGenShaderTable->GetGPUVirtualAddress();
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = resRayGenShaderTable->GetDesc().Width;
	dispatchDesc.MissShaderTable.SizeInBytes = resMissShaderTable->GetDesc().Width;
	dispatchDesc.MissShaderTable.StrideInBytes = missShaderTableStride;
	dispatchDesc.HitGroupTable.StartAddress = resHitGroupShaderTable->GetGPUVirtualAddress();
	dispatchDesc.HitGroupTable.SizeInBytes = resHitGroupShaderTable->GetDesc().Width;
	dispatchDesc.HitGroupTable.StrideInBytes = hitGroupShaderTableStride;
	dispatchDesc.MissShaderTable.StartAddress = resMissShaderTable->GetGPUVirtualAddress();
	dispatchDesc.Width = screen_width;
	dispatchDesc.Height = screen_height;
	dispatchDesc.Depth = 1;
	dxrCommandList->SetPipelineState1(rtpso.Get());
	dxrCommandList->DispatchRays(&dispatchDesc);
}

void DxrPipeline::CopyToScreen(renderTexture & target,uint32_t i)
{
	D3D12_RESOURCE_BARRIER preCopyBarriers[2] = {};
	preCopyBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	preCopyBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	preCopyBarriers[0].Transition.pResource = target.GetHandle().Get();
	preCopyBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	preCopyBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	preCopyBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	preCopyBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	preCopyBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	preCopyBarriers[1].Transition.pResource = dxrRenderTarget[i].Get();
	preCopyBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	preCopyBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	preCopyBarriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	dxrCommandList->ResourceBarrier(2, preCopyBarriers);

	dxrCommandList->CopyResource(target.GetHandle().Get(), dxrRenderTarget[i].Get());

	D3D12_RESOURCE_BARRIER postCopyBarriers[2];
	postCopyBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	postCopyBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	postCopyBarriers[0].Transition.pResource = target.GetHandle().Get();
	postCopyBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	postCopyBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	postCopyBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	postCopyBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	postCopyBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	postCopyBarriers[1].Transition.pResource = dxrRenderTarget[i].Get();
	postCopyBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
	postCopyBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	postCopyBarriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	dxrCommandList->ResourceBarrier(2, postCopyBarriers);
}

::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DxrPipeline::GetRtvHeap()
{
	return nullptr;// dxrAfterHeap;
}

}