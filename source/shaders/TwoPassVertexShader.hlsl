cbuffer WorldProjectionMatrix : register(b0) {

	float4x4 worldProjectionMatrix;

};

struct VertexShaderInput {

	float3 Position			: POSITION;
	float2 Tex_Position		: TEX_POSITION;
	float3 Color			: COLOR;
	float3 Inst_Position	: INST_POSITION;

};

struct VertexShaderOutput {

	float4 Tex_Position	: TEX_POSITION;
	float4 Color		: COLOR;
	float4 Position		: SV_Position;

};

static matrix<float, 4, 4> IdentityMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
};

VertexShaderOutput main(VertexShaderInput IN) {

	VertexShaderOutput OUT;

	matrix<float, 4, 4> Translation = {
	1.0f, 0.0f, 0.0f, IN.Inst_Position.x,
	0.0f, 1.0f, 0.0f, IN.Inst_Position.y,
	0.0f, 0.0f, 1.0f, IN.Inst_Position.z,
	0.0f, 0.0f, 0.0f, 1.0f
	};

	float RotationAngle = radians(45.0f);

	matrix<float, 4, 4> RotationX = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, cos(RotationAngle), -sin(RotationAngle), 0.0f,
	0.0f, sin(RotationAngle), cos(RotationAngle), 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
	};

	matrix<float, 4, 4> RotationY = {
	cos(RotationAngle), 0.0f, -sin(RotationAngle), 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	sin(RotationAngle), 0.0f, cos(RotationAngle), 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
	};

	matrix<float, 4, 4> ModelMatrix = mul(RotationY, IdentityMatrix);
	ModelMatrix = mul(RotationX, ModelMatrix);
	ModelMatrix = mul(Translation, ModelMatrix);
	ModelMatrix = mul(worldProjectionMatrix, ModelMatrix);

	OUT.Position = mul(ModelMatrix, float4(IN.Position, 1.0f));
	OUT.Tex_Position = float4(IN.Tex_Position, 1.0f, 1.0f);
	OUT.Color = float4(IN.Color, 1.0f);

	return OUT;

}