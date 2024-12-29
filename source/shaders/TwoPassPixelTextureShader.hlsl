struct PixelShaderInput {

	float4 Tex_Position : TEX_POSITION;
	float4 Color		: COLOR;

};

Texture2D		textureMap		: register(t0);
SamplerState	textureSampler	: register(s0);

float4 main(PixelShaderInput IN) : SV_Target{

	return textureMap.Sample(textureSampler, IN.Tex_Position.xy);

}