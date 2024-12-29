#pragma once

#include <helpers.h>
#include <objects.h>
#include <render.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <wrl.h>

class TwoPassRender : public Render {

public:

	TwoPassRender();
	~TwoPassRender();
	void LoadContent() override;
	void OnRender() override;
	void OnUpdate(double elapsedTime) override;
	void OnMouseWheel(float mouseWheelDelta) override;


private:

	//VerticesWrapper, IndiciesWrapper and InstancesWrapper helper structs for assembling vertex, index and instance buffer data
	struct VerticesWrapper {

		VertexInfo* p_vertices;
		UINT64 c_vertices;

	};

	struct IndiciesWrapper {

		WORD* p_indicies;
		UINT64 c_indicies;

	};

	struct InstancesWrapper {

		DirectX::XMFLOAT3* p_instances;
		UINT64 c_instances;

	};

	//VerticesWrapper, IndiciesWrapper and InstancesWrapper instances for assebling vertex, index and instance buffer data
	VerticesWrapper m_vertices;
	IndiciesWrapper m_indicies;
	InstancesWrapper m_instances;

	//ID3D12 Device
	Microsoft::WRL::ComPtr<ID3D12Device2> m_device;

	//ID3D12Heap for vertex, index and instance buffer resources
	Microsoft::WRL::ComPtr<ID3D12Heap> m_IAHeap;

	//Vertex, Index and Instance buffer resources
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceBuffer;

	//Vertex, Index and Instance buffer views
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_VERTEX_BUFFER_VIEW m_instanceBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	//Constant buffer for view-projection matrix, cbv descriptor heap, pointer to the resource
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_CBVDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_VPConstantBuffer;
	BYTE* mp_VPConstantBuffer = nullptr;

	//Depth-stencil buffer and descriptor heap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;

	//Root signature and pipeline state objects
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_firstPassPipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_secondPassPipelineState;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_firstPassRootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_secondPassRootSignature;

	//Shader blobs
	Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShaderBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> m_firstPassPixelShaderBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> m_secondPassPixelShaderBlob;

	//Viewport and scissor rectangle
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	//Fence values associated with the backbuffers
	uint64_t m_fenceValues[FRAME_COUNT] = {};

	//Content loaded boolean
	bool m_contentLoaded = false;

	//Temporary render target, descriptor heap for SRV
	Microsoft::WRL::ComPtr<ID3D12Resource> m_tempRenderTarget;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SRVDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;

	//Sampler descriptor heap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_samplerDescriptorHeap;

	//Load Content functions
	void LoadIAStage();
	void LoadShaders();
	void ReleaseShaders();
	void CreateConstantBuffer();
	void CreateRootSignature();
	void CreatePipelineStateObject();
	void CreateDepthStencilBuffer(UINT64 width, UINT height);
	void CreateTemporaryRenderTarget();
	void CreateSampler();

	//Render functions
	void TransitionResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
		D3D12_RESOURCE_STATES firstState, D3D12_RESOURCE_STATES secondState);
	void ClearRTV(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, FLOAT* color);
	void ClearDSV(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

};