#ifndef LIGHTWAVEAPP_H
#define LIGHTWAVEAPP_H

#include <d3dApp.h>
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"
#include "Waves.h"
#include "Material.h"
#include "Light.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

struct RenderItem
{
	RenderItem() = default;

	XMFLOAT4X4 World = MathHelper::Identity4x4();

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
	Count
};

class TexWaveApp : public D3DApp
{
public:
	TexWaveApp(HINSTANCE hInstance);
	TexWaveApp(HINSTANCE hInstance, int width, int height);
	TexWaveApp(const TexWaveApp& rhs) = delete;
	TexWaveApp& operator=(const TexWaveApp& rhs) = delete;
	~TexWaveApp();

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
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt);

	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildLandGeometry();
	void BuildWavesGeometryBuffers();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	float GetHillsHeight(float x, float z)const;
	XMFLOAT3 GetHillsNormal(float x, float z)const;

private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
	FrameResource* m_CurrFrameResource = nullptr;
	int m_CurrFrameResourceIndex = 0;

	ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_Shaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>>m_PSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

	RenderItem* m_WavesRitem = nullptr;

	std::vector<std::unique_ptr<RenderItem>> m_AllRitems;

	std::vector<RenderItem*> m_RitemLayer[(int)RenderLayer::Count];

	std::unique_ptr<Waves> m_Waves;

	PassConstants m_MainPassCB;

	UINT m_PassCbvOffset = 0;

	XMFLOAT3 m_EyePos = { 0.0f,0.0f,0.0f };
	XMFLOAT4X4 m_View = MathHelper::Identity4x4();
	XMFLOAT4X4 m_Proj = MathHelper::Identity4x4();

	float m_Theta = 1.5f * XM_PI;
	float m_Phi = 0.2f * XM_PI;
	float m_Radius = 15.0f;

	float m_SunTheta = 1.25f * XM_PI;
	float m_SunPhi = XM_PIDIV4;

	bool m_IsWireframe = true;
	POINT m_LastMousePos;
};


#endif // LIGHTWAVEAPP_H