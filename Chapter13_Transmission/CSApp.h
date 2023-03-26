#ifndef CSAPP_H
#define CSAPP_H

#include <d3dApp.h>
#include <array>
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"
#include "Waves.h"
#include "Light.h"
#include "Material.h"
#include "Texture.h"

#define STATICSAMPLERCOUNT 6

using namespace DirectX;
using namespace DirectX::PackedVector;

struct RenderItem
{
	RenderItem() = default;

	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	int NumFramesDirty = g_numFrameResources;

	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

	// Primitive topology
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

enum class RenderLayer :int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Count
};

class CSApp : public D3DApp
{
public:
	enum class ShowMode { Exercise1, Exercise2 };
	struct Data
	{
		XMFLOAT3 vec;
	};
public:
	CSApp(HINSTANCE hInstance);
	CSApp(HINSTANCE hInstance, int width, int height);
	CSApp(const CSApp& rhs) = delete;
	CSApp& operator=(const CSApp& rhs) = delete;
	~CSApp();

	virtual bool Initialize()override;

public:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& timer) override;
	virtual void Draw(const GameTimer& timer) override;

	void DoComputeWork(ComPtr<ID3D12Resource> output, int type);

	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;

	void BuildCSResource();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildPSOs();
	void BuildFrameResources();
	
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, STATICSAMPLERCOUNT>GetStaticSamplers();

private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
	FrameResource* m_CurrFrameResource = nullptr;
	int m_CurrFrameResourceIndex = 0;

	ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> m_SrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_Shaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>>m_PSOs;

	const int m_numDataElement = 64;

	// CS Resource
	ComPtr<ID3D12Resource> m_CSExercise1Input = nullptr;			// SRV输入资源
	ComPtr<ID3D12Resource> m_CSExerceise1UploadBuffer = nullptr;	// 上传Buffer
	ComPtr<ID3D12Resource> m_CSExerceise1OutputBuffer = nullptr;	// UAV输出资源
	ComPtr<ID3D12Resource> m_CSExerceise1ReadbackBuffer = nullptr;	
	ComPtr<ID3D12Resource> m_CSExerceise2ConsumeBuffer = nullptr;	// SRV输入资源
	ComPtr<ID3D12Resource> m_CSExerceise2AppendBuffer = nullptr;	// UAV输出资源

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

	std::vector<Data> m_VecData;
	PassConstants m_MainPassCB;

	POINT m_LastMousePos;
	
	// ImGui operable item
	ShowMode m_CurrMode = ShowMode::Exercise1;
	bool m_bIsReset = false;
};


#endif // LIGHTWAVEAPP_H