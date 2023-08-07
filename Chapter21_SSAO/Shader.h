#ifndef SHADER_H
#define SHADER_H

#include "d3dUtil.h"
#include <UploadBuffer.h>
#include <d3d12shader.h>

struct ShaderDefines
{
public:
	bool operator==(const ShaderDefines& other)const;

	void GetD3DShaderMacro(std::vector<D3D_SHADER_MACRO>& retMacros) const;
	void SetDefine(const std::string& name,const std::string& definition);

public:
	std::unordered_map<std::string, std::string> definesMap;
};

enum class SHADER_TYPE
{
	PixelShader = 0x1,
	VertexShader = 0x2,
	GeometryShader = 0x4,
	HullShader = 0x8,
	DomainShader = 0x10,
	ComputeShader = 0x20
};

struct ShaderInfo
{
	std::string strShaderName;
	ShaderDefines shaderDefines;
	std::string strEntryPoint;
	std::string strShaderModle;
	SHADER_TYPE eType;
};

struct ConstantBufferData 
{
	bool isDirty = false;
	std::string m_CBName;
	uint32_t m_StartSlot = 0;
	std::unique_ptr<UploadConstantBuffer> m_CBuffer;
	std::vector<uint8_t> m_CBufferData;
	
	ConstantBufferData() = default;
	ConstantBufferData(const std::string& name, uint32_t startSlot, uint32_t byteWidth, BYTE* initData = nullptr)
		: m_CBName(name), m_StartSlot(startSlot), m_CBufferData(byteWidth) 
	{
		if (m_CBuffer && initData) 
		{
			m_CBuffer->CopyData(0, initData);
		}
	}

	HRESULT CreateBuffer(ID3D12Device* device, UINT elementCount)
	{
		if (m_CBuffer != nullptr)return S_OK;
		return m_CBuffer->CreateBuffer(device, m_CBufferData.size(), elementCount);
	}

	void UpdateBuffer() 
	{
		if (isDirty) 
		{
			isDirty = false;
		}
	}
};


struct ConstantBufferVariable 
{
	ConstantBufferVariable() = default;

};


class Shader
{
public:

	Shader(ID3D12Device* device);
	~Shader() = default;

	void SetConstantBuffer(std::string name);
	void SetShaderResource(std::string name, CD3DX12_GPU_DESCRIPTOR_HANDLE SRV);
	void SetShaderResources(std::string name, std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE>& SRVList);
	void SetUnorderedAccessResource(std::string name, CD3DX12_GPU_DESCRIPTOR_HANDLE UAV);
	void SetUnorderedAccessResources(std::string name, std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE>& UAVList);

	void BindParameters();


private:
	HRESULT CreateShaderFromFile(std::string fileName, const std::vector<ShaderInfo>& shaderInfo);
	HRESULT CompileShaderFromFile();

	D3D12_SHADER_VISIBILITY GetShaderVisibility(SHADER_TYPE type);
	std::vector<CD3DX12_STATIC_SAMPLER_DESC> CreateStaticSamplers();
	void CreateRootSignature();

	// 更新收集着色器反射信息
	HRESULT UpdateShaderReflection(std::string name, ID3D12ShaderReflection* pShaderReflection, uint32_t shaderFlags);
	void GetShaderParameters();
	// 清空所有资源与反射信息
	void Clear();

public:

	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	std::unordered_map<size_t, ComPtr<ID3DBlob>> m_VertexShader;
	std::unordered_map<size_t, ComPtr<ID3DBlob>> m_HullShader;
	std::unordered_map<size_t, ComPtr<ID3DBlob>> m_DomainShader;
	std::unordered_map<size_t, ComPtr<ID3DBlob>> m_GeometryShader;
	std::unordered_map<size_t, ComPtr<ID3DBlob>> m_PixelShader;
	std::unordered_map<size_t, ComPtr<ID3DBlob>> m_ComputerShader;

	std::vector<int> m_CBVParams;
	std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> m_SRVParams;
	std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> m_UAVParams;
	std::vector<int> m_SamplerParams;

	int m_CBVSignatureBaseBindSlot = -1;
	int m_SRVSignatureBindSlot = -1;
	UINT m_SRVCount = 0;
	int m_UAVSignatureBindSlot = -1;
	UINT m_UAVCount = 0;
	int m_SamplerSignatureBindSlot = -1;
	ComPtr<ID3D12RootSignature> m_RootSignature;

private:
	ID3D12Device* m_pDevice;

};

#endif // !SHADER_H
