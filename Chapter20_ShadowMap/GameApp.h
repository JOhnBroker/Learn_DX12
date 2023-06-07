#ifndef GameApp_H
#define GameApp_H

#include <d3dApp.h>
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"
#include "Waves.h"
#include "Material.h"
#include "Texture.h"
#include "Light.h"
#include "Camera.h"
#include "CubeRenderTarget.h"
#include "ShadowMap.h"

#define STATICSAMPLERCOUNT 7
#define LIGHTCOUNT 3
#define CUBEMAP_SIZE 512

using namespace DirectX;
using namespace DirectX::PackedVector;

struct RenderItem
{
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = delete;

	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	int NumFramesDirty = g_numFrameResources;

	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

	// Primitive topology
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	BoundingBox Bounds;

	// DrawIndexedInstanced parameters
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

enum class RenderLayer :int
{
	Opaque = 0,
	Debug,
	Sky,
	Count
};

class GameApp : public D3DApp
{
public:
	enum class CameraMode { FirstPerson, ThirdPerson };
	enum class ShowMode { SoftShadow, SoftShadow_AplhaTest, HardShadow, PeterPanning, ShadowAcne };
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

	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildSkullGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void DrawSceneToShadowMap();
	void RenderShadowMapToTexture();

	void ReadDataFromFile(std::vector<Vertex>& vertices, std::vector<std::int32_t>& indices, BoundingBox& bounds);
	void ReadDataFromFile(std::vector<Vertex>& vertices, std::vector<std::int32_t>& indices, BoundingSphere& bounds);
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, STATICSAMPLERCOUNT>GetStaticSamplers();
	void LoadTexture(std::string name, std::wstring filename);

private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
	FrameResource* m_CurrFrameResource = nullptr;
	int m_CurrFrameResourceIndex = 0;

	ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	ComPtr<ID3D12Resource> m_CubeDepthStencilBuffer = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> m_Textures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_Shaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>>m_PSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

	std::vector<std::unique_ptr<RenderItem>> m_AllRitems;
	std::vector<RenderItem*> m_RitemLayer[(int)RenderLayer::Count];
	PassConstants m_MainPassCB;
	PassConstants m_ShadowPassCB;

	DirectX::BoundingSphere m_SceneBounds;
	std::shared_ptr<Camera> m_pCamera = nullptr;

	// other
	RenderItem* m_SkullRitem = nullptr;
	
	// static SkyBox
	UINT m_SkyTexHeapIndex = 0;
	DirectX::XMFLOAT3 m_SkyBoxScale = { 5000.0f, 5000.0f, 5000.0f };

	// light
	float m_SunTheta = 1.25f * XM_PI;
	float m_SunPhi = XM_PIDIV4;
	std::array<std::shared_ptr<Light>, LIGHTCOUNT> m_Lights;

	//shadow
	int m_ShadowMapSize = 0;
	UINT m_ShadowMapHeapIndex = 0;
	UINT m_NullCubeSrvIndex = 0;
	UINT m_NullTexSrvIndex = 0;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_NullCubeSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_NullTexSrv;
	std::unique_ptr<ShadowMap> m_ShadowMap;

	//Imgui
	bool m_WireframeEnable = false;
	bool m_ShadowMapDebugEnable = false;
	CameraMode m_CameraMode = CameraMode::FirstPerson;
	ShowMode m_ShowMode = ShowMode::SoftShadow;
	LightMode m_LightMode = LightMode::Orthogonal;
	POINT m_LastMousePos;
	std::string m_VertexFileName = "..\\Models\\skull.txt";
};


#endif // GameApp_H