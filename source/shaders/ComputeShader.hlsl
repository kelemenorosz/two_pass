#define BLOCK_SIZE (8)

struct ComputeShaderInput {
	uint3 DispatchTreadID : SV_DispatchThreadID;
	uint3 GroupID : SV_GroupID;
	uint GroupIndex : SV_GroupIndex;
	uint3 GroupThreadID : SV_GroupThreadID;
};

struct GenerateMips {
	uint SrcMipLevel;
	float2 TexelSize;
};

ConstantBuffer<GenerateMips> GenerateMipsCB : register(b0);

Texture2D<float4> SrcMip : register (t0);

RWTexture2D<float4> OutMip1 : register(u0);
RWTexture2D<float4> OutMip2 : register(u1);
RWTexture2D<float4> OutMip3 : register(u2);
RWTexture2D<float4> OutMip4 : register(u3);

SamplerState LinearClampSampler : register (s0);

#define	CS_RootSignature	"RootFlags(0), " \
							"RootConstants(num32BitConstants = 3, b0), " \
							"DescriptorTable(SRV(t0)), " \
							"DescriptorTable(UAV(u0, numDescriptors = 4)), " \
							"StaticSampler(s0, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, " \
							"filter = FILTER_MIN_MAG_MIP_LINEAR)"

groupshared float groupshared_R[64];
groupshared float groupshared_G[64];
groupshared float groupshared_B[64];
groupshared float groupshared_A[64];

void StoreColor(uint Index, float4 color) {

	groupshared_R[Index] = color.r;
	groupshared_G[Index] = color.g;
	groupshared_B[Index] = color.b;
	groupshared_A[Index] = color.a;

}

[RootSignature(CS_RootSignature)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(ComputeShaderInput IN) {

	float4 SampleSrc = (float4)0;

	float2 TextureCoordinates = GenerateMipsCB.TexelSize * (IN.GroupThreadID.xy + 0.5f);
	SampleSrc = SrcMip.SampleLevel(LinearClampSampler, TextureCoordinates, GenerateMipsCB.SrcMipLevel);

	OutMip1[IN.GroupThreadID.xy] = SampleSrc;
	StoreColor(IN.GroupIndex, SampleSrc);

	return;

}