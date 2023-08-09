#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include <d3dUtil.h>
#include "FrameResource.h"
#include "RenderItem.h"
#include "TextureManager.h"

#define SHADOWMAP_NAME "shadow"
#define SHADOWMAP_DEBUG_NAME "shadow_debug"


class ShadowMap
{
public:
	ShadowMap(ID3D12Device* device,
		UINT width, UINT height);
		
	ShadowMap(const ShadowMap& rhs)=delete;
	ShadowMap& operator=(const ShadowMap& rhs)=delete;
	~ShadowMap()=default;

	void SetBuildDescriptorState(bool option);
    UINT GetWidth()const;
    UINT GetHeight()const;
	ID3D12Resource* GetResource();
	ID3D12Resource* GetDebugResource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrv()const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv()const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetDebugSrv()const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDebugRtv()const;

	D3D12_VIEWPORT GetViewport()const;
	D3D12_RECT GetScissorRect()const;

	void OnResize(UINT newWidth, UINT newHeight);
	// Render scene to shadow map
	void DrawShadowMap(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pPso, 
		FrameResource* currFrame, const std::vector<RenderItem*>& items);
	// Render debug shadow map
	void RenderShadowMapToTexture(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pPso);

private:
	void BuildResource();

private:

	ID3D12Device* m_pd3dDevice = nullptr;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
	UINT m_Width = 0;
	UINT m_Height = 0;

	bool bIsResize = false;
	bool bIsBuildDescriptor = false;
	std::shared_ptr<Depth2D> m_ShadowMap;
	std::shared_ptr<Texture2D> m_ShadowMap_Debug;
};

#endif // SHADOWMAP_H

 