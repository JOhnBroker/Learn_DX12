#ifndef BLURFILTER_H
#define BLURFILTER_H

#include "d3dUtil.h"

class BlurFilter
{
public:
	BlurFilter(ID3D12Device* device, UINT uiWidth, UINT uiHeight, DXGI_FORMAT format);
	BlurFilter(const BlurFilter& lhs) = delete;
	BlurFilter& operator=(const BlurFilter& lhs) = delete;
	~BlurFilter() = default;

	void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
		UINT uiDescriptorSize);
	void OnResize(UINT uiWidth, UINT uiHeight);
	void OnProcess(ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSig,
		ID3D12PipelineState* horzBlurPSO, ID3D12PipelineState* vertBlurPSO,
		ID3D12Resource* input, int blurCount);

	ID3D12Resource* Output();

private:
	std::vector<float> CalcGaussWeights(float sigma);
	void BuildDescriptors();
	void BuildResource();
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const int m_MaxBlurRadius = 5;
	ID3D12Device* m_d3dDevice = nullptr;
	UINT m_uiWidth = 0;
	UINT m_uiHeight = 0;
	DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_Blur0CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_Blur0CpuUav;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_Blur1CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_Blur1CpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE m_Blur0GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_Blur0GpuUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_Blur1GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_Blur1GpuUav;

	ComPtr<ID3D12Resource> m_BlurMap0 = nullptr;
	ComPtr<ID3D12Resource> m_BlurMap1 = nullptr;
};

#endif
