char tex_gen_shader1[] = "\
#define rootSig \"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),CBV(b0)\"\n \
float time : register(b0);\n\
struct PSInput {\n\
	float4	position : SV_POSITION;\n\
	float4 texcoord : TEXCOORD;\n\
};\n\
float4 drawLine(float2 coord, float2 be, float2 en, float wid, float4 col, float4 bg)\n\
{\n\
    float2 l = en - be;\n\
    if (abs(l.x) < abs(l.y))\n\
    {\n\
        float a = (l.x / l.y);\n\
        float b = be.x - a * be.y;\n\
        if (\n\
            coord.y > be.y && coord.y < en.y &&\n\
            coord.y * a + b > coord.x - wid &&\n\
            coord.y * a + b < coord.x + wid)\n\
        {\n\
            return col;\n\
        }\n\
    }\n\
\n\
    float a = (l.y / l.x);\n\
    float b = be.y - a * be.x;\n\
    if (\n\
        coord.x > be.x && coord.x < en.x &&\n\
        coord.x * a + b > coord.y - wid &&\n\
        coord.x * a + b < coord.y + wid)\n\
    {\n\
        return col;\n\
    }\n\
    return bg;\n\
}\n\
\n\
float4 drawQuad(float2 coord, float2 be, float2 en, float4 col, float4 bg)\n\
{\n\
    if (coord.x > be.x && coord.x < en.x &&\n\
        coord.y > be.y && coord.y < en.y)\n\
        return col;\n\
    return bg;\n\
}\n\
[RootSignature(rootSig)]\n\
PSInput VSMain(float4 position : POSITION,float4 color : COLOR,float4 texcoord : TEXCOORD)\n\
{\n\
	PSInput	result;\n\
	result.position = float4(texcoord.x*2.0-1.0,texcoord.y*2.0-1.0,0,1);\n\
	result.texcoord = texcoord;\n\
	return result;\n\
}\n\
[RootSignature(rootSig)]\n\
float4 PSMain(PSInput input) : SV_TARGET\n\
{\n\
    float2 uv = input.texcoord.xy;\n\
    uv.y = 1.0 - uv.y;\n\
    float4 col = float4(0.8, 0, 0.6, 1.0);\n\
\
    float top = min(0.0 + time * 0.1, 0.8);\n\
    float bottom = 1.0 - top;\n\
\
    float4 gcol = float4(0.6, 1.1, 0, 5);\n\
    float i=0.0;\
    for (i = 0.0; i < 7.0; i += 1.0) {\n\
        float t = fmod(i + time * 2.0, 7.0);\n\
        col = drawLine(uv, float2(0.0, i / 7.0 - top), float2(1.0, i / 7.0 - top), 0.005, gcol, col);\n\
    }\n\
    for (i = 0.0; i < 7.0; i += 1.0) {\n\
        float t = fmod(i + time * 2.0, 7.0);\n\
        float of = (t - 5.0) * top * 0.1;\n\
        col = drawLine(uv, float2(t / 7.0 + of, 0.0), float2(t / 7.0, 1.0 - top), 0.005, gcol, col);\n\
    }\n\
\
    if (uv.y > bottom)\n\
    {\n\
        float b = uv.y - bottom;\n\
        col = float4(1.0 - b * 0.1, 0, 0.6 + b * 0.2, 1.0);\n\
        float i = 0.0;\
        for (i = 0.0; i < 6.0; i += 1.0) {\n\
            float t = fmod(i + time * abs(sin((i + 0.1) * 190.0)), 7.0);\n\
            float h = (i * 0.1);\n\
            col = drawQuad(uv, float2(t / 6.0 - 0.2, bottom), float2(t / 6.0, 0.9 - h), float4(0, 0.3, 0.7, 1), col);\n\
            col = drawQuad(uv, float2(t / 6.0 - 0.2 + 0.1, bottom), float2(t / 6.0, 0.9 - h), float4(0.7, 0.7, 0, 1), col);\n\
            for (float j = 0.0; j < 8.0; j += 1.0)\n\
            {\n\
                col = drawQuad(uv, float2(t / 6.0 - 0.2 + 0.1 + 0.02, 0.9 - h - 0.06 - 0.08 * j), float2(t / 6.0 - 0.05, 0.9 - h - 0.03 - 0.08 * j), float4(0, 0.3, 0.7, 1), col);\n\
                col = drawQuad(uv, float2(t / 6.0 - 0.2 + 0.1 + 0.065, 0.9 - h - 0.06 - 0.08 * j), float2(t / 6.0 - 0.01, 0.9 - h - 0.03 - 0.08 * j), float4(0, 0.3, 0.7, 1), col);\n\
            }\n\
        }\n\
    }\n\
    return col;\n\
}\n\
";

char loading_shader[] ="\
#define rootSig \"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),CBV(b0)\" \n\
float time : register(b0);\n\
struct PSInput {\n\
    float4	position : SV_POSITION;\n\
    float4	color : COLOR;\n\
    float2	uv : TEXCOORD0;\n\
};\n\
float4 drawRect(float2 uv, float2 b, float2 e, float4 c, float4 bg)\n\
{\n\
    if (b.x < uv.x && e.x > uv.x && b.y < uv.y && e.y > uv.y)\n\
        return c;\n\
    return bg;\n\
}\n\
[RootSignature(rootSig)]\n\
PSInput VSMain(float3 position : POSITION, float4 color : COLOR, float2 uv : TEXCOORD0) {\n\
\n\
    PSInput	result;\n\
    result.position = float4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, 0, 1);\n\
    result.uv = uv;\n\
    return result;\n\
}\n\
[RootSignature(rootSig)]\n\
float4 PSMain(PSInput input) : SV_TARGET\n\
{\n\
    float2 uv = input.uv;\n\
\n\
    float4 col = float4(0.0,0.0,0.0,1.0);\n\
    col = drawRect(uv, float2(0.1, 0.45), float2(0.9, 0.55), float4(1.0,1.0,1.0,1.0), col); \n\
    float a = min(time, 1.0);\n\
    float ep = 0.11 * (1.0 - a) + 0.89 * a;\n\
    col = drawRect(uv, float2(0.11, 0.46), float2(ep, 0.54), float4(0.0, 1.0, 0.0,1.0), col);\n\
\n\
    return col;\n\
}\n\
";

char post_shader[] = "\
#define rootSig \"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),CBV(b0),DescriptorTable(SRV(t0)),DescriptorTable(SRV(t1)),StaticSampler(s0) \"\n \
float time : register(b0);\n\
Texture2D<float4> tex_ : register(t0);\
Texture2D<float4> tex2_ : register(t1);\
SamplerState samp_ : register(s0);\
struct PSInput {\n\
    float4	position : SV_POSITION;\n\
    float4	uv : TEXCOORD0;\n\
};\n\
[RootSignature(rootSig)]\n\
PSInput VSMain(float4 position : POSITION, float4 color : COLOR, float4 uv : TEXCOORD0) {\n\
\n\
    PSInput	result;\n\
    result.position = float4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, 0, 1);\n\
    result.uv = float4(uv.x,1.0 - uv.y,0,0);\n\
    return result;\n\
}\n\
[RootSignature(rootSig)]\n\
float4 PSMain(PSInput input) : SV_TARGET\n\
{\n\
    float2 uv = input.uv;\n\
    float4 col = (tex_.Sample(samp_, input.uv) + tex2_.Sample(samp_, input.uv))*0.5;\n\
\n\
    return col;\n\
}\n\
";

HRESULT create_pipeline_tex_gen1(ino::d3d::pipeline& pipe)
{
    pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, tex_gen_shader1, sizeof(tex_gen_shader1), L"VSMain");
    pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, tex_gen_shader1, sizeof(tex_gen_shader1), L"PSMain");

    D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,sizeof(float) * 3 + sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    pipe.Create(elementDescs, 3);

    pipe.view = {
        .Width = static_cast<FLOAT>(ino::d3d::screen_width),
        .Height = static_cast<FLOAT>(ino::d3d::screen_height),
    };
    pipe.scissor = {
        .right = ino::d3d::screen_width,
        .bottom = ino::d3d::screen_height
    };

    return S_OK;
}

HRESULT create_pipeline_loading(ino::d3d::pipeline& pipe)
{
    pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, loading_shader, sizeof(loading_shader), L"VSMain");
    pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, loading_shader, sizeof(loading_shader), L"PSMain");

    D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,sizeof(float) * 3 + sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    pipe.Create(elementDescs, 3);

    pipe.view = {
        .Width = static_cast<FLOAT>(ino::d3d::screen_width),
        .Height = static_cast<FLOAT>(ino::d3d::screen_height),
    };
    pipe.scissor = {
        .right = ino::d3d::screen_width,
        .bottom = ino::d3d::screen_height
    };

    return S_OK;
}

HRESULT create_pipeline_postProcess(ino::d3d::pipeline& pipe)
{
    pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, post_shader, sizeof(post_shader), "VSMain");
    pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, post_shader, sizeof(post_shader), "PSMain");

    D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,sizeof(float) * 3 + sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_FILTER filter[] = { D3D12_FILTER::D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR };
    D3D12_TEXTURE_ADDRESS_MODE warp[] = { D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP };
    pipe.CreateSampler(1, filter, warp);

    pipe.Create(elementDescs, 3);

    pipe.view = {
        .Width = static_cast<FLOAT>(ino::d3d::screen_width),
        .Height = static_cast<FLOAT>(ino::d3d::screen_height),
    };
    pipe.scissor = {
        .right = ino::d3d::screen_width,
        .bottom = ino::d3d::screen_height
    };

    return S_OK;
}