#pragma once
#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "Mesh.h"

using namespace DirectX;

struct Vertex 
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

class BoxApp :public D3DApp
{
public:
	enum class ShowMode { Box, Pyramid};
public:
	BoxApp(HINSTANCE hInstance);
	BoxApp(HINSTANCE hInstance, int witdh, int height);
	BoxApp(const BoxApp& rhs) = delete;
	BoxApp& operator=(const BoxApp& rhs) = delete;
	~BoxApp();
	virtual bool Initialize() override;
	bool InitResource();

public:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& timer) override;
	virtual void Draw(const GameTimer& timer) override;

	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShaderAndInputLayout();
	void BuildBoxGeometry();
	void BuildPSO();

private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	std::unique_ptr<UploadBuffer<ObjectConstants>> m_ObjectCB = nullptr;
	std::unique_ptr<MeshGeometry> m_BoxGeo = nullptr;
	std::unique_ptr<MeshGeometry> m_PyramidGeo = nullptr;

	ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> m_CBVHeap = nullptr;

	ComPtr<ID3DBlob> m_VSByteCode = nullptr;
	ComPtr<ID3DBlob> m_PSByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_PSOs;

	XMFLOAT4X4 m_World = MathHelper::Identity4x4();
	XMFLOAT4X4 m_View = MathHelper::Identity4x4();
	XMFLOAT4X4 m_Proj = MathHelper::Identity4x4();

	float m_Theta = 1.5f * XM_PI;
	float m_Phi = XM_PIDIV4;
	float m_Radius = 5.0f;

	ShowMode m_CurrMode = ShowMode::Box;
	bool m_LineMode = false;
	bool m_FrontMode = false;
	POINT m_LastMousePos;
};

