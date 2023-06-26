#ifndef SSAO_H
#define SSAO_H

#include "d3dUtil.h"
#include "FrameResource.h"
#include "RenderItem.h"
#include "TextureManager.h"

#define RANDOMVECTORMAP_NAME "RandomVector"
#define NORMALDEPTHMAP_NAME "NormalDepth"
#define SSAOXMAP_NAME "SSAO_X"
#define SSAOYMAP_NAME "SSAO_Y"
#define SSAODEBUGMAP_NAME "SSAO_Debug"
#define RANDOMVECTORCOUNT 14

class SSAO
{
public:
	SSAO(ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList, 
		UINT width, UINT height);
	~SSAO() = default;
	SSAO(const SSAO&) = delete;
	SSAO& operator=(const SSAO&) = delete;

	UINT GetWidth() const { return m_RenderTargetWidth; }
	UINT GetHeight()const { return m_RenderTargetHeight; }

	void GetOffsetVectors(DirectX::XMFLOAT4 offsets[14]);
	std::vector<float> CalcGaussWeights(float sigma);

	ID3D12Resource* GetNormalDepthMap() { return m_NormalDepthMap->GetTexture(); }
	ID3D12Resource* GetAOMap() { return m_SSAOMap0->GetTexture(); }

	CD3DX12_GPU_DESCRIPTOR_HANDLE GetNormalDepthSrv()const { return m_NormalDepthMap->GetShaderResource(); }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetNormalDepthRtv()const { return m_NormalDepthMap->GetRenderTarget(); }
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetAOMapSrv()const { return m_SSAOMap0->GetShaderResource(); }
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetAOMapDebugSrv()const { return m_SSAODebugMap->GetShaderResource(); }

	void OnResize(UINT newWidth, UINT newHeight);
	// Pass1：绘制观察空间法向量和深度贴图
	void RenderNormalDepthMap(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pPso, 
		FrameResource* currFrame, const std::vector<RenderItem*>& items);
	// Pass2：绘制SSAO
	void RenderToSSAOTexture(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pPso,
		FrameResource* currFrame);
	// Pass3：对SSAO进行滤波
	void BlurAOMap(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pPso, 
		FrameResource* currFrame, int blurCount);
	// Render Debug AO
	void RenderAOToTexture(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pPso);

private:
	void BuildResource();
	void BuildRandomVectorTexture(ID3D12GraphicsCommandList* cmdList);
	void BuildOffsetVectors();
	void BlurAOMap(ID3D12GraphicsCommandList* cmdList, bool horzBlur);

private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ID3D12Device* m_pDevice;
	
	ID3D12PipelineState* m_SSAOPso = nullptr;
	ID3D12PipelineState* m_BlurPso = nullptr;

	const DXGI_FORMAT m_AOMapFormat = DXGI_FORMAT_R16_UNORM;
	const DXGI_FORMAT m_NormalDepthMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	bool bIsResize = false;
	std::shared_ptr<Texture2D> m_RandomVectorMap;
	ComPtr<ID3D12Resource> m_RandomVectorResource;
	ComPtr<ID3D12Resource> m_RandomVectorUploadBuffer;
	std::shared_ptr<Texture2D> m_NormalDepthMap;
	std::shared_ptr<Texture2D> m_SSAOMap0;
	std::shared_ptr<Texture2D> m_SSAOMap1;
	std::shared_ptr<Texture2D> m_SSAODebugMap;

	const int m_MaxBlurRadius = 5;

	UINT m_RenderTargetWidth;
	UINT m_RenderTargetHeight;

	D3D12_VIEWPORT m_ViewPort;
	D3D12_RECT m_ScissotRect;

	DirectX::XMFLOAT4 m_Offsets[RANDOMVECTORCOUNT];
};


#endif // SSAO_H



