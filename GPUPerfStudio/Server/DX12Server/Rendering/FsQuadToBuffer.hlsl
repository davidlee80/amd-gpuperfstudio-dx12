R"(

//*************************************************************************************************
// FsQuadToBuffer.hlsl
//*************************************************************************************************
struct PS_INPUT
{
    float4 pos      : SV_POSITION;
    float2 tex      : TEXCOORD0;
};

cbuffer ConstantBuffer : register(b0)
{
    uint rtWidth;
    uint flipX;
    uint flipY;
};

Texture2D inTex : register(t0);
SamplerState samLinear : register(s0);
RWStructuredBuffer<uint> outBuf : register(u1);

//*************************************************************************************************
// UpscaleRange()
//
// Convert from 0..1 range to 0..255 range.
//*************************************************************************************************
uint UpscaleRange(float val)
{
    return lerp(0, 255, clamp(val, 0, 1));
}

//*************************************************************************************************
// Vertex Shader
//
// Ouputs a full screen quad with tex coords.
//*************************************************************************************************
PS_INPUT VsMain(uint VertexID: SV_VertexID)
{
    PS_INPUT output;

    output.tex = float2((VertexID << 1) & 2, VertexID & 2);
    output.pos = float4(output.tex * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    if (flipX)
    {
        output.tex.x -= 1.0f;
        output.tex.x *= -1.0f;
    }

    if (flipY)
    {
        output.tex.y -= 1.0f;
        output.tex.y *= -1.0f;
    }

    return output;
}

//*************************************************************************************************
// Pixel Shader
//
// Writes full screen quad to a linear output buffer where each channel fits in 1 byte.
//*************************************************************************************************
float4 PsMain(PS_INPUT input) : SV_Target
{
    float4 outColor = inTex.Sample(samLinear, input.tex);

    uint flatPixelIdx = ((input.pos.y - 0.5f) * rtWidth) + (input.pos.x - 0.5f);

    uint compressedColor = 0;

    compressedColor |= UpscaleRange(1);
    compressedColor <<= 8;

    compressedColor |= UpscaleRange(outColor.z);
    compressedColor <<= 8;

    compressedColor |= UpscaleRange(outColor.y);
    compressedColor <<= 8;

    compressedColor |= UpscaleRange(outColor.x);

    outBuf[flatPixelIdx] = compressedColor;

    return outColor;
}

)"