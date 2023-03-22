#ifndef SOBELFILTER_H
#define SOBELFILTER_H

#include "d3dUtil.h"

class SobelFilter
{
public:
	SobelFilter(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format);
	SobelFilter(SobelFilter& lhs) = delete;
	SobelFilter& operator= (const SobelFilter& lhs) = delete;
	~SobelFilter() = default;

	CD3DX12_GPU_DESCRIPTOR_HANDLE OutputSrv();
	UINT DescriptorCount()const;
	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
		UINT descriptorSize);
	void OnResize(UINT newWidth, UINT newHeight);
	void OnProcess(ID3D12GraphicsCommandList* cmdList,
		ID3D12RootSignature* rootSig,
		ID3D12PipelineState* pso,
		CD3DX12_GPU_DESCRIPTOR_HANDLE input);

private:
	void BuildDescriptors();
	void BuildResource();

private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	
	ID3D12Device* m_d3dDevice = nullptr;
	UINT m_uiWidth = 0;
	UINT m_uiHeight = 0;
	UINT m_uiDescriptorCount = 2;
	DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuUav;

	ComPtr<ID3D12Resource> m_Output = nullptr;
};

#endif