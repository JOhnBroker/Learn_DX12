#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include "d3dUtil.h"

class RenderTarget
{
public:
	RenderTarget(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format);
	RenderTarget(RenderTarget& lhs) = delete;
	RenderTarget& operator=(RenderTarget& lhs) = delete;
	~RenderTarget() = default;

	ID3D12Resource* Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrv();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv();
	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuUav,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);
	void OnResize(UINT newWidth, UINT newHeight);

private:
	void BuildDescriptors();
	void BuildResource();
	
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	
	ID3D12Device* m_d3dDevice = nullptr;

	UINT m_uiWidth = 0;
	UINT m_uiHeight = 0;
	DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuRtv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;

	ComPtr<ID3D12Resource> m_OffscreenTex = nullptr;
};

#endif
