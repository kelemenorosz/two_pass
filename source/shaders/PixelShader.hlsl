struct PixelShaderInput {

	float4 TexPosition : TEXPOSITION;

};

Texture2D textureMap : register(t0);

SamplerState anisotropicWrap : register(s0);

float4 main(PixelShaderInput IN) : SV_Target{

	float2 texCoords = IN.TexPosition.xy;

	return textureMap.Sample(anisotropicWrap, texCoords);

}