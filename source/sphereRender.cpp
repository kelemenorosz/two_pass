#define WIN32_LEAN_AND_MEAN

#include <include/directx/d3dx12.h>

#pragma comment(lib, "d3d12.lib")

#include <commandqueue.h>
#include <sphereRender.h>
#include <application.h>
#include <helpers.h>
#include <utils/utils.h>
#include <objects.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <wrl.h>

SphereRender::SphereRender() {

	//Get D3D12 device
	m_device = Application::Get()->GetDevice();

	//Tetrahedron info
	TetrahedronInfo tetrahedronInfo;

	//Prepare bundle of 1
	{
		float xShift[] = { 0.0f };
		float yShift[] = { 0.0f };
		float zShift[] = { 0.0f };

		Objects::PrepareBundleDraw(tetrahedronInfo.tetrahedronVertex, VERTEX_COUNT_TETRAHEDRON, tetrahedronInfo.tetrahedronIndex, INDEX_COUNT_TETRAHEDRON,
			&m_tetrahedronVertices.p_vertices, m_tetrahedronVertices.c_vertices, &m_tetrahedronIndicies.p_indicies, m_tetrahedronIndicies.c_incidies, 1, xShift, yShift, zShift);
	}

	//Prepare instance of 1
	{
		float xShift[] = { 0.0f };
		float yShift[] = { 0.0f };
		float zShift[] = { 0.0f };

		Objects::PrepareInstancedDraw(&m_tetrahedronInstances.p_instances, m_tetrahedronInstances.c_instances, 1, xShift, yShift, zShift);
	}

}

SphereRender::~SphereRender() {

	//Free allocated memory
	free(m_tetrahedronVertices.p_vertices);
	free(m_tetrahedronIndicies.p_indicies);
	free(m_tetrahedronInstances.p_instances);

}

void SphereRender::LoadContent() {

	LoadIAStage();
	CreateRootSignature();

}

void SphereRender::LoadIAStage() {

	//Get command queue and command list
	auto commandQueue = Application::Get()->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	//Intermediate resources
	Microsoft::WRL::ComPtr<ID3D12Resource> m_intermediaryVertex;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_intermediaryIndex;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_intermediaryInstance;

	//Upload resources to the GPU
	Utils::UpdateBufferResource(m_device, commandList, &m_vertexBuffer, &m_intermediaryVertex, m_tetrahedronVertices.c_vertices, sizeof(VertexInfo), m_tetrahedronVertices.p_vertices);
	Utils::UpdateBufferResource(m_device, commandList, &m_indexBuffer, &m_intermediaryIndex, m_tetrahedronIndicies.c_incidies, sizeof(WORD), m_tetrahedronIndicies.p_indicies);
	Utils::UpdateBufferResource(m_device, commandList, &m_instanceBuffer, &m_intermediaryInstance, m_tetrahedronInstances.c_instances, sizeof(DirectX::XMFLOAT3), m_tetrahedronInstances.p_instances);

	//Create buffer views
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = sizeof(VertexInfo) * m_tetrahedronVertices.c_vertices;
	m_vertexBufferView.StrideInBytes = sizeof(VertexInfo);

	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_indexBufferView.SizeInBytes = sizeof(WORD) * m_tetrahedronIndicies.c_incidies;

	m_instanceBufferView.BufferLocation = m_instanceBuffer->GetGPUVirtualAddress();
	m_instanceBufferView.SizeInBytes = sizeof(DirectX::XMFLOAT3) * m_tetrahedronInstances.c_instances;
	m_instanceBufferView.StrideInBytes = sizeof(DirectX::XMFLOAT3);

	//Execute command list
	commandQueue->WaitForFenceValue(commandQueue->ExecuteCommandList(commandList));

}

void SphereRender::CreateRootSignature() {

	//Check root signature version
	D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSignatureVersion = {};
	rootSignatureVersion.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootSignatureVersion, sizeof(rootSignatureVersion)))) {
		rootSignatureVersion.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	//Root parameters
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

	//Create root signature
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	Microsoft::WRL::ComPtr<ID3DBlob> serializedRSBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, rootSignatureVersion.HighestVersion, &serializedRSBlob, &errorBlob));
	ThrowIfFailed(m_device->CreateRootSignature(0, serializedRSBlob->GetBufferPointer(), serializedRSBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

}

void SphereRender::OnRender() {



}

void SphereRender::OnUpdate(double elapsedTime) {



}

void SphereRender::OnMouseWheel(float mouseWheelDelta) {



}

