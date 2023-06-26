#ifndef GameApp_H
#define GameApp_H

#include <d3dApp.h>
#include "UploadBuffer.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"
#include "TextureManager.h"
#include "Material.h"
#include "Light.h"

#include "Camera.h"
#include "Octree.h"

#include "CubeRenderTarget.h"
#include "ShadowMap.h"
#include "SSAO.h"

#define STATICSAMPLERCOUNT 7
#define LIGHTCOUNT 3
#define CUBEMAP_SIZE 512

using namespace DirectX;
using namespace DirectX::PackedVector;

enum class RenderLayer :int
{
	Opaque = 0,
	Sky,
	Count
};

class GameApp : public D3DApp
{
public:
	enum class CameraMode { FirstPerson, ThirdPerson };
	enum class ShowMode { CPU_AO, SSAO, SSAO_SelfInter, SSAO_Gaussian };
	enum class LightMode { Orthogonal, Perspective };
public:
	GameApp(HINSTANCE hInstance);
	GameApp(HINSTANCE hInstance, int width, int height);
	GameApp(const GameApp& rhs) = delete;
	GameApp& operator=(const GameApp& rhs) = delete;
	~GameApp();

	virtual bool Initialize()override;
	bool InitResource();

public:
	virtual void CreateRTVAndDSVDescriptorHeaps() override;
	virtual void OnResize() override;
	virtual void Update(const GameTimer& timer) override;
	virtual void Draw(const GameTimer& timer) override;

	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;

	void AnimateMaterials(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialBuffer(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateShadowTransform(const GameTimer& gt);
	void UpdateShadowPassCB(const GameTimer& gt);
	void UpdateSSAOCB(const GameTimer& gt);

	void BuildRootSignature();
	void BuildSSAORootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildSkullGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	// CPU AO
	void BuildVertexAmbientOcclusion(std::vector<CPU_SSAO_Vertex>& vertices, const std::vector<std::uint32_t>& indices);

	void ReadDataFromFile(std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices, BoundingBox& bounds);
	void ReadDataFromFile(std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices, BoundingSphere& bounds);
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, STATICSAMPLERCOUNT>GetStaticSamplers();

private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
	FrameResource* m_CurrFrameResource = nullptr;
	int m_CurrFrameResourceIndex = 0;

	std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> m_RootSignatures;
	ComPtr<ID3D12Resource> m_CubeDepthStencilBuffer = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;
	TextureManager m_TextureManager;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_Shaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>>m_PSOs;

	std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> m_InputLayouts;

	std::vector<std::unique_ptr<RenderItem>> m_AllRitems;
	std::vector<RenderItem*> m_RitemLayer[(int)RenderLayer::Count];
	PassConstants m_MainPassCB;
	PassConstants m_ShadowPassCB;

	// other
	DirectX::BoundingSphere m_SceneBounds;
	std::shared_ptr<Camera> m_pCamera = nullptr;
	RenderItem* m_SkullRitem = nullptr;
	
	// static SkyBox
	UINT m_SkyTexHeapIndex = 0;
	DirectX::XMFLOAT3 m_SkyBoxScale = { 5000.0f, 5000.0f, 5000.0f };

	// light & shadow
	float m_SunTheta = 1.25f * XM_PI;
	float m_SunPhi = XM_PIDIV4;
	std::array<std::shared_ptr<Light>, LIGHTCOUNT> m_Lights;
	int m_ShadowMapSize = 0;
	std::unique_ptr<ShadowMap> m_ShadowMap;
	std::unique_ptr<SSAO> m_SSAO;

	//Imgui
	bool m_ShadowMapDebugEnable = false;
	bool m_AODebugEnable = false;
	bool m_SSAODebugEnable = false;
	CameraMode m_CameraMode = CameraMode::FirstPerson;
	ShowMode m_ShowMode = ShowMode::SSAO;
	LightMode m_LightMode = LightMode::Orthogonal;
	POINT m_LastMousePos;
	std::string m_VertexFileName = "..\\Models\\skull.txt";
};


#endif // GameApp_H