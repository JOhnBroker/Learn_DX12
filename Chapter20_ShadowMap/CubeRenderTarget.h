#ifndef CUBERENDERTARGET_H
#define CUBERENDERTARGET_H

#include <d3dUtil.h>

enum class CubeMapFace : int
{
	PositiveX = 0,
	NegativeX = 1,
	PositiveY = 2,
	NegativeY = 3,
	PositiveZ = 4,
	NegativeZ = 5
};

class CubeRenderTarget
{
public:
	CubeRenderTarget(ID3D12Device* device,
		UINT width, UINT height, DXGI_FORMAT format);
	CubeRenderTarget(const CubeRenderTarget& lhs) = delete;
	CubeRenderTarget& operator=(const CubeRenderTarget& lhs) = delete;
	~CubeRenderTarget() = default;

	ID3D12Resource* Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrv();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(int faceIndex);
	D3D12_VIEWPORT Viewport() const;
	D3D12_RECT ScissorRect() const;

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv[6]);
	void Resize(UINT newWidth, UINT newHeight);

private:
	void BuildDescriptors();
	void BuildResource();

private:
	ID3D12Device* m_pd3dDevice = nullptr;
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	UINT m_Width = 0;
	UINT m_Height = 0;
	DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuRtv[6];

	Microsoft::WRL::ComPtr<ID3D12Resource> m_CubeMap = nullptr;
};

#endif

