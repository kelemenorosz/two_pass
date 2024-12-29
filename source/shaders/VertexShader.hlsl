struct ModelViewProjection {

	matrix MVP;

};

struct RotationAngleStruct {

	float RotationAngle;

};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);
ConstantBuffer<RotationAngleStruct> RotationAngleCB : register(b1);

struct VertexPosColor {

	float3 Position : POSITION;
	float2 TexPosition : TEXPOSITION;
	float3 InstanceShift : INSTANCE_SHIFT;

};

struct VertexShaderOutput {

	float4 TexPosition : TEXPOSITION;
	float4 Position : SV_Position;

};


VertexShaderOutput main(VertexPosColor IN) {

	VertexShaderOutput OUT;

	static matrix<float, 4, 4> Translation = {
	1.0f, 0.0f, 0.0f, IN.InstanceShift.x,
	0.0f, 1.0f, 0.0f, IN.InstanceShift.y,
	0.0f, 0.0f, 1.0f, IN.InstanceShift.z,
	0.0f, 0.0f, 0.0f, 1.0f
	};

	static matrix<float, 4, 4> TranslationBack = {
	1.0f, 0.0f, 0.0f, -IN.InstanceShift.x,
	0.0f, 1.0f, 0.0f, -IN.InstanceShift.y,
	0.0f, 0.0f, 1.0f, -IN.InstanceShift.z,
	0.0f, 0.0f, 0.0f, 1.0f
	};

	static matrix<float, 4, 4> RotationX = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, cos(RotationAngleCB.RotationAngle), -sin(RotationAngleCB.RotationAngle), 0.0f,
	0.0f, sin(RotationAngleCB.RotationAngle), cos(RotationAngleCB.RotationAngle), 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
	};

	static matrix<float, 4, 4> RotationY = {
	cos(RotationAngleCB.RotationAngle), 0.0f, -sin(RotationAngleCB.RotationAngle), 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	sin(RotationAngleCB.RotationAngle), 0.0f, cos(RotationAngleCB.RotationAngle), 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
	};

	static matrix<float, 4, 4> RotationZ = {
	cos(RotationAngleCB.RotationAngle), -sin(RotationAngleCB.RotationAngle), 0.0f, 0.0f,
	sin(RotationAngleCB.RotationAngle), cos(RotationAngleCB.RotationAngle), 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
	};

	
	//ModelMatrix = mul(RotationX, RotationY);
	//ModelMatrix = mul(ModelMatrix, Translation);

	static matrix<float, 4, 4> IdentityMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	matrix<float, 4, 4> ModelMatrix = mul(RotationX, IdentityMatrix);
	ModelMatrix = mul(RotationZ, ModelMatrix);
	ModelMatrix = mul(Translation, ModelMatrix);

	//matrix ModelMatrix = mul(Translation, IdentityMatrix);

	ModelMatrix = mul(ModelViewProjectionCB.MVP, ModelMatrix);

	OUT.Position = mul(ModelMatrix, float4(IN.Position, 1.0f));
	OUT.TexPosition = float4(IN.TexPosition, 1.0f, 1.0f);

	return OUT;

}