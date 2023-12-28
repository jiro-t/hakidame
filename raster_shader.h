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
    float4 col = float4(0.8, 0, 0.6, 1.0);\n\
\
    float top = min(0.0 + max(0.0,time-9.0) * 0.1, 0.8);\n\
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

char tex_gen_shader2[] = "\
#define rootSig \"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),CBV(b0)\"\n \
#define N 12.0\n\
float time : register(b0);\n\
struct PSInput {\n\
    float4	position : SV_POSITION;\n\
    float4 texcoord : TEXCOORD;\n\
};\n\
float3 cir(float2 uv, float2 c, float r, float3 col, float3 bg)\n\
{\n\
    if (length(c - uv) < r)\n\
        return col;\n\
    return bg;\n\
}\n\
\n\
float3 metaball(float2 uv, float3 bg)\n\
{\n\
    float2 p1 = float2(0.6 + sin(time * 2.0) * 0.7, 0.5 + cos(time * 2.0) * 0.4);\n\
    float2 p2 = float2(1.3 - sin(time * 1.2) * 0.7, 0.2 - cos(time * 1.2) * 0.6);\n\
    float2 p3 = float2(1.0, 0.5 + cos(time * 2.0) * 0.1);\n\
\n\
    float3 col = bg;\n\
\n\
    float pot1 = pow(0.3 / length(p1 - uv), 1.0);\n\
    float pot2 = pow(0.3 / length(p2 - uv), 1.0);\n\
    float pot3 = pow(0.3 / length(p3 - uv), 1.0);\n\
\n\
    float pot4 = pow(1.0 / (pot1 + pot2 + pot3), 2.0);\n\
    float val = 0.02 / (pot4 * pot4) + 0.2;\n\
    if (pot4 < 0.3)\n\
        col = float3(val, val, val);\n\
\n\
    return col;\n\
}\n\
\n\
[RootSignature(rootSig)]\n\
PSInput VSMain(float4 position : POSITION, float4 color : COLOR, float4 texcoord : TEXCOORD)\n\
{\n\
    PSInput	result;\n\
    result.position = float4(texcoord.x * 2.0 - 1.0, texcoord.y * 2.0 - 1.0, 0, 1);\n\
    result.texcoord = texcoord;\n\
    return result;\n\
}\n\
\n\
[RootSignature(rootSig)]\n\
float4 PSMain(PSInput input) : SV_TARGET\n\
{\n\
    float2 uv = input.texcoord.xy;\n\
    float3 col = float3(0,0,0);\n\
\n\
    float z = 100.0;\n\
    for (float i = 0.0; i < N; i += 1.0)\n\
    {\n\
        float t = fmod(time * 0.5 - i / N,1.0) * 2.0;\n\
        float2 p = float2(0.87,0.5) + float2(cos(2.0 * 3.14 * i / N),sin(2.0 * 3.14 * i / N)) * 0.2;\n\
        if (z > t && length(p - uv) < t)\n\
        {\n\
            float3 c = float3(fmod(i,N) / N,fmod(N - i,N) / N,fmod(N + N / 2.0 - i,N) / N);\n\
            col = cir(uv,p,t,c * t,col);\n\
            z = t;\n\
        }\n\
    }\n\
\n\
    col = metaball(uv,col);\n\
    return float4(col,1.0);\n\
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

/*Edge-Avoiding A-Trous Wavelet Transform for fast Global Illumination Filtering*/
char denoise_hader[] = "\
#define rootSig \"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),CBV(b0),DescriptorTable(SRV(t0)),DescriptorTable(SRV(t1)),DescriptorTable(SRV(t2)),StaticSampler(s0) \"\n\
cbuffer cb : register(b0){\n\
    float width;\n\
    float height;\n\
    float c_phi;\n\
    float n_phi;\n\
    float p_phi;\n\
    int stepwidth;\n\
};\n\
Texture2D<float4> render_ : register(t0);\n\
Texture2D<float4> normal_ : register(t1);\n\
Texture2D<float4> world_ : register(t2);\n\
SamplerState samp_ : register(s0);\n\
struct PSInput {\n\
    float4	position : SV_POSITION;\n\
    float4	uv : TEXCOORD0;\n\
};\n\
[RootSignature(rootSig)]\n\
PSInput VSMain(float4 position : POSITION, float4 color : COLOR, float4 uv : TEXCOORD0) {\n\
    PSInput	result;\n\
    result.position = float4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, 0, 1);\n\
    result.uv = float4(uv.x,1.0 - uv.y,0,0);\n\
    return result;\n\
}\n\
const static float kernel[25] = {\n\
    1.0 / 256.0, 1.0 / 64.0, 3.0 / 128.0, 1.0 / 64.0, 1.0 / 256.0,\n\
    1.0 / 64.0, 1.0 / 16.0, 3.0 / 32.0, 1.0 / 16.0, 1.0 / 64.0,\n\
    3.0 / 128.0, 3.0 / 32.0, 9.0 / 64.0, 3.0 / 32.0, 3.0 / 128.0,\n\
    1.0 / 64.0, 1.0 / 16.0, 3.0 / 32.0, 1.0 / 16.0, 1.0 / 64.0,\n\
    1.0 / 256.0, 1.0 / 64.0, 3.0 / 128.0, 1.0 / 64.0, 1.0 / 256.0 };\n\
\n\
const static int2 offset[25] = {\n\
    int2(-2, -2), int2(-1, -2), int2(0, -2), int2(1, -2), int2(2, -2),\n\
    int2(-2, -1), int2(-1, -1), int2(0, -2), int2(1, -1), int2(2, -1),\n\
    int2(-2, 0), int2(-1, 0), int2(0, 0), int2(1, 0), int2(2, 0),\n\
    int2(-2, 1), int2(-1, 1), int2(0, 1), int2(1, 1), int2(2, 1),\n\
    int2(-2, 2), int2(-1, 2), int2(0, 2), int2(1, 2), int2(2, 2) };\n\
\n\
float4 PSMain(PSInput input) : SV_TARGET\n\
{\n\
    float3 sum = float3(0,0,0);\n\
    float2 step = float2(1. / width, 1. / height);\n\
\n\
    float3 cval = render_.Sample(samp_, input.uv.xy).xyz;\n\
    float3 nval = normal_.Sample(samp_, input.uv.xy).xyz;\n\
    float3 pval = world_.Sample(samp_, input.uv.xy).xyz;\n\
\n\
    float cum_w = 0.0;\n\
\n\
    for (int i = 0; i < 25; i++) {\n\
        float2 uv = input.uv.xy + offset[i] * step * stepwidth;\n\
\n\
        float3 ctmp = render_.Sample(samp_,uv).xyz;\n\
        float3 t = cval - ctmp;\n\
        float dist2 = dot(t,t);\n\
        float c_w = min(exp(-(dist2) / c_phi),1.0);\n\
\n\
        float3 ntmp = normal_.Sample(samp_,uv).xyz;\n\
        t = nval - ntmp;\n\
        dist2 = max(dot(t,t) / (stepwidth * stepwidth),0.0);\n\
        float n_w = min(exp(-(dist2) / n_phi), 1.0);\n\
\n\
        float3 ptmp = world_.Sample(samp_,uv).xyz;\n\
        t = pval - ptmp;\n\
        dist2 = dot(t,t);\n\
        float p_w = min(exp(-(dist2) / p_phi),1.0);\n\
\n\
        float weight = c_w * n_w * p_w;\n\
        sum += ctmp * weight * kernel[i];\n\
        cum_w += weight * kernel[i];\n\
    }\n\
\n\
    return float4(sum / cum_w,1);\n\
}\n\
";

/*
char denoise_hader[] = "\
#define rootSig \"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),CBV(b0),DescriptorTable(SRV(t0)),DescriptorTable(SRV(t1)),DescriptorTable(SRV(t2)),StaticSampler(s0) \"\n\
cbuffer cb : register(b0){\n\
    float width;\n\
    float height;\n\
    float c_phi;\n\
    float n_phi;\n\
    float p_phi;\n\
    int stepwidth;\n\
};\n\
Texture2D<float4> render_ : register(t0);\n\
Texture2D<float4> normal_ : register(t1);\n\
Texture2D<float4> world_ : register(t2);\n\
SamplerState samp_ : register(s0);\n\
struct PSInput {\n\
    float4	position : SV_POSITION;\n\
    float4	uv : TEXCOORD0;\n\
};\n\
[RootSignature(rootSig)]\n\
PSInput VSMain(float4 position : POSITION, float4 color : COLOR, float4 uv : TEXCOORD0) {\n\
    PSInput	result;\n\
    result.position = float4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, 0, 1);\n\
    result.uv = float4(uv.x,1.0 - uv.y,0,0);\n\
    return result;\n\
}\n\
const static float kernel[25] = {\n\
    1.0 / 256.0, 1.0 / 64.0, 3.0 / 128.0, 1.0 / 64.0, 1.0 / 256.0,\n\
    1.0 / 64.0, 1.0 / 16.0, 3.0 / 32.0, 1.0 / 16.0, 1.0 / 64.0,\n\
    3.0 / 128.0, 3.0 / 32.0, 9.0 / 64.0, 3.0 / 32.0, 3.0 / 128.0,\n\
    1.0 / 64.0, 1.0 / 16.0, 3.0 / 32.0, 1.0 / 16.0, 1.0 / 64.0,\n\
    1.0 / 256.0, 1.0 / 64.0, 3.0 / 128.0, 1.0 / 64.0, 1.0 / 256.0 };\n\
\n\
const static int2 offset[25] = {\n\
    int2(-2, -2), int2(-1, -2), int2(0, -2), int2(1, -2), int2(2, -2),\n\
    int2(-2, -1), int2(-1, -1), int2(0, -2), int2(1, -1), int2(2, -1),\n\
    int2(-2, 0), int2(-1, 0), int2(0, 0), int2(1, 0), int2(2, 0),\n\
    int2(-2, 1), int2(-1, 1), int2(0, 1), int2(1, 1), int2(2, 1),\n\
    int2(-2, 2), int2(-1, 2), int2(0, 2), int2(1, 2), int2(2, 2) };\n\
\n\
float4 PSMain(PSInput input) : SV_TARGET\n\
{\n\
    float3 sum = float4(0,0,0,0);\n\
    float2 step = float2(1. / width, 1. / height);\n\
\n\
    float3 cval = render_.Sample(samp_, input.uv.xy).xyz;\n\
    float3 nval = normal_.Sample(samp_, input.uv.xy).xyz;\n\
    float3 pval = world_.Sample(samp_, input.uv.xy).xyz;\n\
\n\
    float cum_w = 0.0;\n\
\n\
    for (int i = 0; i < 25; i++) {\n\
        float2 uv = input.uv.xy + offset[i] * step * stepwidth;\n\
\n\
        float4 ctmp = render_.Sample(samp_,uv);\n\
        float4 t = cval - ctmp;\n\
        float dist2 = dot(t,t);\n\
        float c_w = min(exp(-(dist2) / c_phi),1.0);\n\
\n\
        float4 ntmp = normal_.Sample(samp_,uv);\n\
        t = nval - ntmp;\n\
        dist2 = max(dot(t,t) / (stepwidth * stepwidth),0.0);\n\
        float n_w = min(exp(-(dist2) / n_phi), 1.0);\n\
\n\
        float4 ptmp = world_.Sample(samp_,uv);\n\
        t = pval - ptmp;\n\
        dist2 = dot(t,t);\n\
        float p_w = min(exp(-(dist2) / p_phi),1.0);\n\
\n\
        float weight = c_w * n_w * p_w;\n\
        sum += ctmp * weight * kernel[i];\n\
        cum_w += weight * kernel[i];\n\
    }\n\
\n\
    return float4(sum / cum_w,1);\n\
}\n\
";
*/
char post_shader[] = "\
#define rootSig \"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),CBV(b0),DescriptorTable(SRV(t0)),StaticSampler(s0) \"\n \
float time :  register(b0);\n\
Texture2D<float4> render_ : register(t0);\
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
    float fade = max(0.0,min(1.0,102.5f - time));\n\
    fade = min(fade,min(1.0,max(0.0,time*0.25 - 1)));\n\
    float4 col = float4(render_.Sample(samp_, input.uv).xyz*fade,1);\n\
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

HRESULT create_pipeline_tex_gen2(ino::d3d::pipeline& pipe)
{
    pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, tex_gen_shader2, sizeof(tex_gen_shader2), L"VSMain");
    pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, tex_gen_shader2, sizeof(tex_gen_shader2), L"PSMain");

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

struct DenoiseCBO {
    float width;
    float height;
    float c_phi;
    float n_phi;
    float p_phi;
    uint32_t stepwidth;
};

HRESULT create_pipeline_denoise(ino::d3d::pipeline& pipe)
{
    pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, denoise_hader, sizeof(denoise_hader), L"VSMain");
    pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, denoise_hader, sizeof(denoise_hader), L"PSMain");

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

HRESULT create_pipeline_postProcess(ino::d3d::pipeline& pipe)
{
    pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, post_shader, sizeof(post_shader), L"VSMain");
    pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, post_shader, sizeof(post_shader), L"PSMain");

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