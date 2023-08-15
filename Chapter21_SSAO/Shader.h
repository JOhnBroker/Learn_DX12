#ifndef SHADER_H
#define SHADER_H

#include "d3dUtil.h"
#include "UploadBuffer.h"
#include "Property.h"
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

// ����������
struct ConstantBufferData 
{
	bool isDirty = false;
	std::string m_CBName;								// ����������������
	uint32_t m_StartSlot = 0;							// �󶨼Ĵ������
	std::unique_ptr<UploadConstantBuffer> m_CBuffer;	// ����������
	std::vector<uint8_t> m_CBufferData;					// ������λ��
	
	ConstantBufferData() = default;
	ConstantBufferData(const std::string& name, uint32_t startSlot, uint32_t byteWidth, BYTE* initData = nullptr)
		: m_CBName(name), m_StartSlot(startSlot), m_CBufferData(byteWidth) 
	{
		if (m_CBuffer && initData) 
		{
			m_CBuffer->CopyData(initData);
		}
	}

	HRESULT CreateBuffer(ID3D12Device* device)
	{
		if (m_CBuffer != nullptr) return S_OK;
		return m_CBuffer->CreateBuffer(device, m_CBufferData.size());
	}

	void UpdateBuffer() 
	{
		if (isDirty) 
		{
			isDirty = false;
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferView() 
	{
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = 0;
		if (m_CBuffer != nullptr) 
		{
			cbAddress = m_CBuffer->Resource()->GetGPUVirtualAddress();
		}
		return cbAddress;
	}

};

// ������������ÿ������
struct ConstantBufferVariable 
{
	std::string m_ValueName;						// ������
	uint32_t m_StartByteOffset = 0;					// ��Գ�����������ƫ��ֵ
	uint32_t m_ByteWidth = 0;						// �����Ĵ�С
	ConstantBufferData* m_pCBufferData = nullptr;	// �������������ָ��

	ConstantBufferVariable() = default;
	~ConstantBufferVariable() = default;

	ConstantBufferVariable(const std::string& name, uint32_t offset, uint32_t size, ConstantBufferData* pData)
		: m_ValueName(name), m_StartByteOffset(offset), m_ByteWidth(size), m_pCBufferData(pData)
	{};

	void SetRaw(const void* data, uint32_t byteOffset = 0, uint32_t byteCount = 0xFFFFFFFF)
	{
		if (!data || byteOffset > m_ByteWidth)
			return;
		if (byteCount + byteOffset > m_ByteWidth)
			byteCount = m_ByteWidth - byteOffset;

		if (memcmp(m_pCBufferData->m_CBufferData.data() + m_StartByteOffset + byteOffset, data, byteCount))
		{
			memcpy_s(m_pCBufferData->m_CBufferData.data() + m_StartByteOffset + byteOffset, byteCount, data, byteCount);
		}
	}

	void SetMatrixInBytes(uint32_t rows, uint32_t col, const BYTE* noPadData) 
	{
		if (rows == 0 || rows > 4 || col == 0 || col > 4)
			return;
		uint32_t remainBytes = m_ByteWidth < 64 ? m_ByteWidth : 64;
		BYTE* pData = m_pCBufferData->m_CBufferData.data() + m_StartByteOffset;
		while (remainBytes > 0 && rows > 0) 
		{
			uint32_t rowPitch = sizeof(uint32_t) * col < remainBytes ? sizeof(uint32_t) * col : remainBytes;
			if (memcmp(pData, noPadData, rowPitch)) 
			{
				memcpy_s(pData, rowPitch, noPadData, rowPitch);
			}
			noPadData += col * sizeof(uint32_t);
			pData += 16;
			remainBytes = remainBytes < 16 ? 0 : remainBytes - 16;
		}
	}

	void SetUInt(uint32_t val) 
	{
		SetRaw(&val, 0, 4);
	}

	void SetSInt(int val) 
	{
		SetRaw(&val, 0, 4);
	}

	void SetFloat(float val)
	{
		SetRaw(&val, 0, 4);
	}

	void SetUIntVector(uint32_t numComponents, const uint32_t data[4]) 
	{
		if (numComponents > 4)
			numComponents = 4;
		uint32_t byteCount = numComponents * sizeof(uint32_t);
		if (byteCount > m_ByteWidth)
			byteCount = m_ByteWidth;
		SetRaw(data, 0, byteCount);
	}

	void SetSIntVector(uint32_t numComponents, const int data[4])
	{
		if (numComponents > 4)
			numComponents = 4;
		uint32_t byteCount = numComponents * sizeof(int);
		if (byteCount > m_ByteWidth)
			byteCount = m_ByteWidth;
		SetRaw(data, 0, byteCount);
	}

	void SetFloatVector(uint32_t numComponents, const float data[4])
	{
		if (numComponents > 4)
			numComponents = 4;
		uint32_t byteCount = numComponents * sizeof(float);
		if (byteCount > m_ByteWidth)
			byteCount = m_ByteWidth;
		SetRaw(data, 0, byteCount);
	}

	void SetUIntMatrix(uint32_t rows, uint32_t cols, const uint32_t* noPadData)
	{
		SetMatrixInBytes(rows, cols, reinterpret_cast<const BYTE*>(noPadData));
	}

	void SetSIntMatrix(uint32_t rows, uint32_t cols, const int* noPadData)
	{
		SetMatrixInBytes(rows, cols, reinterpret_cast<const BYTE*>(noPadData));
	}

	void SetFloatMatrix(uint32_t rows, uint32_t cols, const float* noPadData)
	{
		SetMatrixInBytes(rows, cols, reinterpret_cast<const BYTE*>(noPadData));
	}

	struct PropertyFunctor
	{
		PropertyFunctor(ConstantBufferVariable& _cbv) :cbv(_cbv) {}
		void operator()(int val) { cbv.SetSInt(val); }
		void operator()(uint32_t val) { cbv.SetUInt(val); }
		void operator()(float val) { cbv.SetFloat(val); }
		void operator()(const DirectX::XMFLOAT2& val) { cbv.SetFloatVector(2, reinterpret_cast<const float*>(&val)); }
		void operator()(const DirectX::XMFLOAT3& val) { cbv.SetFloatVector(3, reinterpret_cast<const float*>(&val)); }
		void operator()(const DirectX::XMFLOAT4& val) { cbv.SetFloatVector(4, reinterpret_cast<const float*>(&val)); }
		void operator()(const DirectX::XMFLOAT4X4& val) { cbv.SetFloatMatrix(4,4, reinterpret_cast<const float*>(&val)); }
		void operator()(const std::vector<float>& val) { cbv.SetRaw(val.data()); }
		void operator()(const std::vector<DirectX::XMFLOAT4>& val) { cbv.SetRaw(val.data()); }
		void operator()(const std::vector<DirectX::XMFLOAT4X4>& val) { cbv.SetRaw(val.data()); }
		void operator()(const std::string& val) { }
		ConstantBufferVariable& cbv;
	};

	void Set(const Property& prop) 
	{
		std::visit(PropertyFunctor(*this), prop);
	}

	HRESULT GetRaw(void* pOutput, uint32_t byteOffset = 0, uint32_t byteCount = 0xFFFFFFFF) 
	{
		if (byteOffset > m_ByteWidth || byteCount > m_ByteWidth - byteOffset)
			return E_BOUNDS;
		if (!pOutput)
			return E_INVALIDARG;
		memcpy_s(pOutput, byteCount, m_pCBufferData->m_CBufferData.data() + m_StartByteOffset + byteOffset, byteCount);
		return S_OK;
	}
};

struct ShaderParameter
{
	std::string name;
	SHADER_TYPE shaderType;
	UINT bindPoint;
	UINT registerSpace;
};

struct ShaderSRVParameter : ShaderParameter
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE srv;
};

struct ShaderUAVParameter :ShaderParameter 
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE uav;
};

struct ShaderSamplerParameter :ShaderParameter {};

class Shader
{
public:

	Shader(ID3D12Device* device, std::string filename);
	~Shader() = default;

	// ������ɫ�� �� ��ȡ��ɫ���ֽ��룬������˳��
   // 1. ���������ɫ���ֽ����ļ�����·�� �� �ر�ǿ�Ƹ��ǣ������ȳ��Զ�ȡ${cacheDir}/${shaderName}.cso�����
   // 2. �����ȡfilename����Ϊ��ɫ���ֽ��룬ֱ�����
   // 3. ��filenameΪhlslԴ�룬����б������ӡ�������ɫ���ֽ����ļ�����ᱣ����ɫ���ֽ��뵽${cacheDir}/${shaderName}.cso
	HRESULT CreateShaderFromFile(std::string fileName, const std::vector<ShaderInfo>& shaderInfo);
	// ��������ɫ��
	HRESULT CompileShaderFromFile(std::string fileName, const std::vector<ShaderInfo>& shaderInfo);

	void SetConstantBuffer(std::string name);
	void SetShaderResource(std::string name, CD3DX12_GPU_DESCRIPTOR_HANDLE SRV);
	void SetShaderResources(std::string name, std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE>& SRVList);
	void SetUnorderedAccessResource(std::string name, CD3DX12_GPU_DESCRIPTOR_HANDLE UAV);
	void SetUnorderedAccessResources(std::string name, std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE>& UAVList);

	void BindParameters();

private:
	// ��ӱ���õ���ɫ����������Ϣ��Ϊ�����ñ�ʶ��
	// �ú������ᱣ����ɫ�������Ʊ��뵽�ļ�
	HRESULT AddShader(std::string name, ID3DBlob** ppShaderByteCode);

	D3D12_SHADER_VISIBILITY GetShaderVisibility(SHADER_TYPE type);
	std::vector<CD3DX12_STATIC_SAMPLER_DESC> CreateStaticSamplers();
	void CreateRootSignature();

	// �����ռ���ɫ��������Ϣ
	HRESULT UpdateShaderReflection(std::string name, ID3D12ShaderReflection* pShaderReflection, uint32_t shaderFlags);
	// ���������Դ�뷴����Ϣ
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

	std::unordered_map<size_t, std::shared_ptr<ConstantBufferVariable>> m_ConstantBufferVariables;	// �����������ı���		// nameתhash,��ΪID
	std::unordered_map<size_t, std::shared_ptr<ConstantBufferData>> m_CBVParams;					// �󶨲�λ��ΪID
	std::unordered_map<size_t, std::shared_ptr<ShaderSRVParameter>> m_SRVParams;					// �󶨲�λ��ΪID
	std::unordered_map<size_t, std::shared_ptr<ShaderUAVParameter>> m_UAVParams;					// �󶨲�λ��ΪID
	std::unordered_map<size_t, std::shared_ptr<ShaderSamplerParameter>> m_SamplerParams;			// �󶨲�λ��ΪID	

	int m_CBVSignatureBaseBindSlot = -1;
	int m_SRVSignatureBindSlot = -1;
	UINT m_SRVCount = 0;
	int m_UAVSignatureBindSlot = -1;
	UINT m_UAVCount = 0;
	int m_SamplerSignatureBindSlot = -1;
	ComPtr<ID3D12RootSignature> m_RootSignature;

private:
	std::string m_FileName;
	ID3D12Device* m_pDevice;

};

#endif // !SHADER_H
