#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include <d3dUtil.h>

class ShadowMap
{
public:
	ShadowMap(ID3D12Device* device,
		UINT width, UINT height);
		
	ShadowMap(const ShadowMap& rhs)=delete;
	ShadowMap& operator=(const ShadowMap& rhs)=delete;
	~ShadowMap()=default;

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

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv_Debug,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv_Debug,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv_Debug
	);

	void OnResize(UINT newWidth, UINT newHeight);

private:
	void BuildDescriptors();
	void BuildResource();

private:

	ID3D12Device* m_pd3dDevice = nullptr;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	UINT m_Width = 0;
	UINT m_Height = 0;
	DXGI_FORMAT m_Format = DXGI_FORMAT_R24G8_TYPELESS;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuDsv;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuRtv_Debug;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv_Debug;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv_Debug;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_ShadowMap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_DebugShadowMap = nullptr;
};

#endif // SHADOWMAP_H

 