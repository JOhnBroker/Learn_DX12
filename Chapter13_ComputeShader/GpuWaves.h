#ifndef GPUWAVES_H
#define GPUWAVES_H

#include "d3dUtil.h"
#include "GameTimer.h"

class GpuWaves
{
public:
	GpuWaves(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
		int m, int n, float dx, float dt, float speed, float damping);
	GpuWaves(GpuWaves& lhs) = delete;
	GpuWaves& operator=(GpuWaves& lhs) = delete;
	~GpuWaves() = default;

	UINT RowCount()const;
	UINT ColumnCount()const;
	UINT VertexCount()const;
	UINT TriangleCount()const;
	float Width()const;
	float Depth()const;
	float SpatialStep()const;

	CD3DX12_GPU_DESCRIPTOR_HANDLE DisplacementMap()const;
	UINT DescriptorCount()const;
	
	void BuildResource(ID3D12GraphicsCommandList* cmdList);
	void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
		UINT uiDescriptorSize);
	void Update(const GameTimer& gt,
		ID3D12GraphicsCommandList* cmdList,
		ID3D12RootSignature* rootSig,
		ID3D12PipelineState* pso);
	void Disturb(ID3D12GraphicsCommandList* cmdList,
		ID3D12RootSignature* rootSig,
		ID3D12PipelineState* pso,
		UINT i, UINT j,
		float magnitude);
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const UINT m_NumDescriptorCount = 6;
	UINT m_NumRows;
	UINT m_NumCols;
	UINT m_VertexCount;
	UINT m_TriangleCount;

	// Simulation constants we can precompute.
	float m_K[3];

	float m_fTimeStep = 0.0f;
	float m_fSpatialStep = 0.0f;

	ID3D12Device* m_d3dDevice = nullptr;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_PrevSolSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_CurrSolSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_NextSolSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_PrevSolUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_CurrSolUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_NextSolUav;

	ComPtr<ID3D12Resource> m_PrevSol = nullptr;
	ComPtr<ID3D12Resource> m_CurrSol = nullptr;
	ComPtr<ID3D12Resource> m_NextSol = nullptr;

	ComPtr<ID3D12Resource> m_PrevUploadBuffer = nullptr;
	ComPtr<ID3D12Resource> m_CurrUploadBuffer = nullptr;
};

#endif