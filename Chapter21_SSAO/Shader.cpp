#include "Shader.h"
#include <locale>
#include <codecvt>

using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;

// 代码宏
#define _CREATE_SHADER(FullShaderType,ShaderType)\
{\
	m_##FullShaderType##s[nameID] = std::make_shared<FullShaderType##Info>();\
	m_##FullShaderType##s[nameID]->name = name;\
	m_##FullShaderType##s[nameID]->p##ShaderType = pShaderByteCode;\
	break;\
}

// TODO: set cbuffer
#define _SET_SHADER_AND_PARRAM(FullShaderType,ShaderType)\
{\
	auto it = pShader->m_##FullShaderType##s.find(StringToID(name));\
	if (it != pShader->m_##FullShaderType##s.end())\
}

#define _SET_SHADER(FullShaderType, ShaderType)\
{\
	auto it  = pShader->m_##FullShaderType##s.find(StringToID(name));\
	if (it != pShader->m_##FullShaderType##s.end())\
	{\
		auto& pBlob = it->second->p##ShaderType;\
		if(pBlob)\
		{\
			psoDesc.ShaderType =\
			{\
				reinterpret_cast<BYTE*>(pBlob->GetBufferPointer()),\
				pBlob->GetBufferSize()\
			};\
		}\
	}\
}

// 将对应的常量缓冲区与RootDescriptorTable的特定值绑定
#define _SET_COMSTANTBUFFER(ShaderType)\
{\
}

// 1. 创建采样器描述符堆
// 2. CreateSampler创建动态采样器
// 3. 修改RootDescriptorTable中的绑定信息
// 4. 应用到渲染队列中SetGraphicsRootDescriptorTable
#define _SET_PARAM(ShaderType)\
{\
}

// 将对应的贴图与RootDescriptorTable的特定值绑定
#define _SET_SHADERRESOURCE(ShaderType)\
{\
}



// 常量缓冲区
struct ConstantBufferData
{
	bool isDirty = false;
	std::string m_CBName;								// 常量缓冲区的名称
	uint32_t m_StartSlot = 0;							// 绑定寄存器编号
	std::unique_ptr<UploadConstantBuffer> m_CBuffer;	// 常量缓冲区
	std::vector<uint8_t> m_CBufferData;					// 参数的位置

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

// 常量缓冲区中每个参数
struct ConstantBufferVariable
{
	std::string m_ValueName;						// 参数名
	uint32_t m_StartByteOffset = 0;					// 相对常量缓冲区的偏移值
	uint32_t m_ByteWidth = 0;						// 参数的大小
	ConstantBufferData* m_pCBufferData = nullptr;	// 所属常量缓冲的指针

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
		void operator()(const DirectX::XMFLOAT4X4& val) { cbv.SetFloatMatrix(4, 4, reinterpret_cast<const float*>(&val)); }
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

struct VertexShaderInfo
{
	std::string name;
	ComPtr<ID3D11VertexShader> pVS;
	uint32_t cbUseMask = 0;
	uint32_t ssUseMask = 0;
	uint32_t unused = 0;
	uint32_t srUseMasks[4] = {};
	std::unique_ptr<ConstantBufferData> pParamData = nullptr;
	std::unordered_map<size_t, std::shared_ptr<ConstantBufferVariable>> params;
};

struct DomainShaderInfo
{
	std::string name;
	ComPtr<ID3D11DomainShader> pDS;
	uint32_t cbUseMask = 0;
	uint32_t ssUseMask = 0;
	uint32_t unused = 0;
	uint32_t srUseMasks[4] = {};
	std::unique_ptr<ConstantBufferData> pParamData = nullptr;
	std::unordered_map<size_t, std::shared_ptr<ConstantBufferVariable>> params;
};

struct HullShaderInfo
{
	std::string name;
	ComPtr<ID3D11HullShader> pHS;
	uint32_t cbUseMask = 0;
	uint32_t ssUseMask = 0;
	uint32_t unused = 0;
	uint32_t srUseMasks[4] = {};
	std::unique_ptr<ConstantBufferData> pParamData = nullptr;
	std::unordered_map<size_t, std::shared_ptr<ConstantBufferVariable>> params;
};

struct GeometryShaderInfo
{
	std::string name;
	ComPtr<ID3DBlob> pGS;
	uint32_t cbUseMask = 0;
	uint32_t ssUseMask = 0;
	uint32_t unused = 0;
	uint32_t srUseMasks[4] = {};
	std::unique_ptr<ConstantBufferData> pParamData = nullptr;
	std::unordered_map<size_t, std::shared_ptr<ConstantBufferVariable>> params;
};

struct PixelShaderInfo
{
	std::string name;
	ComPtr<ID3DBlob> pPS;
	uint32_t cbUseMask = 0;
	uint32_t ssUseMask = 0;
	uint32_t rwUseMask = 0;
	uint32_t srUseMasks[4] = {};
	std::unique_ptr<ConstantBufferData> pParamData = nullptr;
	std::unordered_map<size_t, std::shared_ptr<ConstantBufferVariable>> params;
};

struct ComputeShaderInfo
{
	std::string name;
	ComPtr<ID3DBlob> pCS;
	uint32_t cbUseMask = 0;
	uint32_t ssUseMask = 0;
	uint32_t rwUseMask = 0;
	uint32_t srUseMasks[4] = {};
	uint32_t threadGroupSizeX = 0;
	uint32_t threadGroupSizeY = 0;
	uint32_t threadGroupSizeZ = 0;
	std::unique_ptr<ConstantBufferData> pParamData = nullptr;
	std::unordered_map<size_t, std::shared_ptr<ConstantBufferVariable>> params;
};


bool ShaderDefines::operator==(const ShaderDefines& other) const
{
    if(definesMap.size() !=  other.definesMap.size())
        return false;

    for (const auto& pair : definesMap) 
    {
        auto iter = other.definesMap.find(pair.first);
        if (iter == other.definesMap.end() || iter->second != pair.second) 
        {
            return false;
        }
    }

    return true;
}

void ShaderDefines::GetD3DShaderMacro(std::vector<D3D_SHADER_MACRO>& retMacros) const
{
    for (const auto& pair : definesMap) 
    {
        D3D_SHADER_MACRO macro;
        macro.Name = pair.first.c_str();
        macro.Definition = pair.second.c_str();
        retMacros.push_back(macro);
    }
	D3D_SHADER_MACRO macro;
	macro.Name = NULL;
	macro.Definition = NULL;
	retMacros.push_back(macro);
}

void ShaderDefines::SetDefine(const std::string& name, const std::string& definition)
{
    definesMap.insert_or_assign(name, definition);
}

Shader::Shader(ID3D12Device* device, std::string filename)
    : m_pDevice(device), m_FileName(filename)
{
}

HRESULT Shader::CreateShaderFromFile(const ShaderInfo& shaderInfo, std::filesystem::path cacheDir, bool forceWrite /* = true*/)
{
    static char dxbc_header[] = { 'D', 'X', 'B', 'C' };
    ComPtr<ID3DBlob> pBlobIn = nullptr, pBlobOut = nullptr;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    HRESULT hr = E_FAIL;

    // 读缓存
    if (!cacheDir.empty() && !forceWrite) 
    {
        std::filesystem::path cacheFileName = cacheDir / (converter.from_bytes(shaderInfo.strShaderName) + L".cso");
        std::wstring wstr = cacheFileName.generic_wstring();
        hr = D3DReadFileToBlob(wstr.c_str(), &pBlobOut);
        if (SUCCEEDED(hr))
        {
            hr = AddShader(shaderInfo.strShaderName, pBlobOut);

            
        }
    }

    hr = D3DReadFileToBlob(converter.from_bytes(m_FileName).c_str(), &pBlobIn);
    if (FAILED(hr))
        return hr;
    if (memcmp(pBlobIn->GetBufferPointer(), dxbc_header, sizeof(dxbc_header))) 
    {
        // 若文件为HLSL源码，则进行编译
        uint32_t dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
        dwShaderFlags |= D3DCOMPILE_DEBUG;
        dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        ID3DBlob* errorBlob = nullptr;
        std::vector<D3D_SHADER_MACRO> defines;
        shaderInfo.shaderDefines.GetD3DShaderMacro(defines);
        hr = D3DCompile(pBlobIn->GetBufferPointer(), pBlobIn->GetBufferSize(), m_FileName.c_str(),
            defines.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, shaderInfo.strEntryPoint.c_str(),
            shaderInfo.strShaderModle.c_str(), dwShaderFlags, 0, &pBlobOut, &errorBlob);
        pBlobIn->Release();

        if (FAILED(hr)) 
        {
            if (errorBlob != nullptr) 
            {
                OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
                errorBlob->Release();
            }
            return hr;
        }
        if (!cacheDir.empty())
        {
            std::filesystem::path cacheFileName = cacheDir / (converter.from_bytes(shaderInfo.strShaderName) + L".cso");
            std::wstring wstr = cacheFileName.generic_wstring();
            D3DWriteBlobToFile(pBlobOut.Get(), wstr.c_str(), forceWrite);
        }
    }
    else 
    {
        std::swap(pBlobIn, pBlobOut);
    }

    hr = AddShader(shaderInfo.strShaderName, pBlobOut);

    

    return hr;
}

HRESULT Shader::CreateShadersFromFile(const std::vector<ShaderInfo>& shadersInfo)
{
    HRESULT hr = E_FAIL;
    return hr;
}

HRESULT Shader::CompileShaderFromFile(const ShaderInfo& shaderInfo)
{
    HRESULT hr = E_FAIL;
    return hr;
}

HRESULT Shader::CompileShaderFromFile(const std::vector<ShaderInfo>& shadersInfo)
{
    HRESULT hr = E_FAIL;
    return hr;
}

HRESULT Shader::AddShader(std::string name, ComPtr<ID3DBlob> pShaderByteCode)
{
    if (name.empty() || pShaderByteCode == nullptr)
        return E_INVALIDARG;

    HRESULT hr = E_FAIL;

    ComPtr<ID3D12ShaderReflection> pShaderReflection;
    hr = D3DReflect(pShaderByteCode->GetBufferPointer(), pShaderByteCode->GetBufferSize(),
        __uuidof(ID3D12ShaderReflection), reinterpret_cast<void**>(pShaderReflection.GetAddressOf()));
    if (FAILED(hr))
        return hr;

    D3D12_SHADER_DESC sd;
    pShaderReflection->GetDesc(&sd);
    SHADER_TYPE shaderFlag = static_cast<SHADER_TYPE>(1 << D3D12_SHVER_GET_TYPE(sd.Version));

    XID nameID = StringToID(name);
    switch (shaderFlag)
    {
    case SHADER_TYPE::PixelShader:		_CREATE_SHADER(PixelShader, PS);
    case SHADER_TYPE::VertexShader:		_CREATE_SHADER(VertexShader, VS);
    case SHADER_TYPE::GeometryShader:	_CREATE_SHADER(GeometryShader, GS);
    case SHADER_TYPE::HullShader:		_CREATE_SHADER(HullShader, HS);
    case SHADER_TYPE::DomainShader:		_CREATE_SHADER(DomainShader, DS);
    case SHADER_TYPE::ComputeShader:	_CREATE_SHADER(ComputeShader, CS);
    }

    return UpdateShaderReflection(name, pShaderReflection.Get(), shaderFlag);
}

HRESULT Shader::UpdateShaderReflection(std::string name, ID3D12ShaderReflection* pShaderReflection, SHADER_TYPE shaderFlags)
{
    HRESULT hr = E_FAIL;

    D3D12_SHADER_DESC sd;
    hr = pShaderReflection->GetDesc(&sd);
    if (FAILED(hr))
        return hr;

    size_t nameID = StringToID(name);

    if (shaderFlags == SHADER_TYPE::ComputeShader) 
    {
		pShaderReflection->GetThreadGroupSize(
			&m_ComputeShaders[nameID]->threadGroupSizeX,
			&m_ComputeShaders[nameID]->threadGroupSizeY,
			&m_ComputeShaders[nameID]->threadGroupSizeZ);
    }

	for (uint32_t i = 0;; ++i) 
	{
		D3D12_SHADER_INPUT_BIND_DESC sibDesc;
		hr = pShaderReflection->GetResourceBindingDesc(i, &sibDesc);
		if (FAILED(hr))
			break;

		if (sibDesc.Type == D3D_SIT_CBUFFER)
		{
			ID3D12ShaderReflectionConstantBuffer* pSRCbuffer = pShaderReflection->GetConstantBufferByName(sibDesc.Name);
			// 
			D3D12_SHADER_BUFFER_DESC cbDesc = {};
			hr = pSRCbuffer->GetDesc(&cbDesc);
			if (FAILED(hr))
				return hr;

			bool isParam = !strcmp(sibDesc.Name, "$Parmas");
			if (!isParam) 
			{
				auto it = m_ConstantBufferVariables.find(sibDesc.BindPoint);
			}


		}
	}



    return hr;
}
