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

#include "BlurFilter.h"
#include "SobelFilter.h"
#include "GpuWaves.h"
#include "RenderTarget.h"

#define STATICSAMPLERCOUNT 6

using namespace DirectX;
using namespace DirectX::PackedVector;

struct RenderItem
{
	RenderItem() = default;

	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	DirectX::XMFLOAT2 DisplacementMapTexelSize = { 1.0f,1.0f };
	float GridSpatialStep = 1.0f;

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
	GpuWaves,
	Count
};

class CSApp : public D3DApp
{
public:
	enum class ShowMode { Wireframe, Opaque, Blur, WavesCS, SobelFilter};
public:
	CSApp(HINSTANCE hInstance);
	CSApp(HINSTANCE hInstance, int width, int height);
	CSApp(const CSApp& rhs) = delete;
	CSApp& operator=(const CSApp& rhs) = delete;
	~CSApp();

	virtual bool Initialize()override;
	bool InitResource();

public:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& timer) override;
	virtual void Draw(const GameTimer& timer) override;

	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;

	void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt);
	void UpdateWavesGpu(const GameTimer& gt);

	void BuildRootSignature();
	void BuildCSRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildPSOs();
	void BuildLandGeometry();
	void BuildWavesGeometry();
	void BuildBoxGeometry();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void DrawFullScreenQuad(ID3D12GraphicsCommandList* cmdList);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, STATICSAMPLERCOUNT>GetStaticSamplers();

	float GetHillsHeight(float x, float z)const;
	XMFLOAT3 GetHillsNormal(float x, float z)const;

	void LoadTexture(std::string name, std::wstring filename);

private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
	FrameResource* m_CurrFrameResource = nullptr;
	int m_CurrFrameResourceIndex = 0;

	ComPtr<ID3D12DescriptorHeap> m_SrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> m_Textures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_Shaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>>m_PSOs;
	std::unordered_map<std::string, ComPtr<ID3D12RootSignature>>m_RootSignatures;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

	RenderItem* m_WavesRitem = nullptr;
	std::vector<std::unique_ptr<RenderItem>> m_AllRitems;
	std::vector<RenderItem*> m_RitemLayer[(int)RenderLayer::Count];

	std::unique_ptr<Waves> m_Waves;
	std::unique_ptr<GpuWaves> m_GpuWaves;
	std::unique_ptr<BlurFilter> m_BlurFilter;
	std::unique_ptr<SobelFilter> m_SobelFilter;

	std::unique_ptr<RenderTarget> m_OffScreenRT = nullptr;

	PassConstants m_MainPassCB;

	XMFLOAT3 m_EyePos = { 0.0f,0.0f,0.0f };
	XMFLOAT4X4 m_View = MathHelper::Identity4x4();
	XMFLOAT4X4 m_Proj = MathHelper::Identity4x4();

	float m_Theta = 1.5f * XM_PI;
	float m_Phi = 0.2f * XM_PI;
	float m_Radius = 15.0f;

	float m_SunTheta = 1.25f * XM_PI;
	float m_SunPhi = XM_PIDIV4;

	POINT m_LastMousePos;
	
	bool m_bIsCompleteCS = false;

	// ImGui operable item
	ShowMode m_CurrMode = ShowMode::Opaque;
	XMFLOAT3 m_BoxTexScale = { 1.0f,1.0f,1.0f };
	int m_iBlurCount = 4;

};


#endif // LIGHTWAVEAPP_H