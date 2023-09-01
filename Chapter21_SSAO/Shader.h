#ifndef SHADER_H
#define SHADER_H

#include "d3dUtil.h"
#include "UploadBuffer.h"
#include "Property.h"
#include <d3d12shader.h>
#include <filesystem>

struct ConstantBufferData;
struct ConstantBufferVariable;
struct VertexShaderInfo;
struct DomainShaderInfo;
struct HullShaderInfo;
struct GeometryShaderInfo;
struct PixelShaderInfo;
struct ComputeShaderInfo;

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

	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Shader(ID3D12Device* device, std::string filename);
	~Shader() = default;

	// 编译着色器 或 读取着色器字节码，按下述顺序：
   // 1. 如果开启着色器字节码文件缓存路径 且 关闭强制覆盖，则优先尝试读取${cacheDir}/${shaderName}.cso并添加
   // 2. 否则读取filename。若为着色器字节码，直接添加
   // 3. 若filename为hlsl源码，则进行编译和添加。开启着色器字节码文件缓存会保存着色器字节码到${cacheDir}/${shaderName}.cso
	HRESULT CreateShaderFromFile(const ShaderInfo& shaderInfo, std::filesystem::path cacheDir, bool forceWrite = true);
	HRESULT CreateShadersFromFile(const std::vector<ShaderInfo>& shadersInfo);
	// 仅编译着色器
	HRESULT CompileShaderFromFile(const ShaderInfo& shaderInfo);
	HRESULT CompileShaderFromFile(const std::vector<ShaderInfo>& shadersInfo);

	void SetConstantBuffer(std::string name);
	void SetShaderResource(std::string name, CD3DX12_GPU_DESCRIPTOR_HANDLE SRV);
	void SetShaderResources(std::string name, std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE>& SRVList);
	void SetUnorderedAccessResource(std::string name, CD3DX12_GPU_DESCRIPTOR_HANDLE UAV);
	void SetUnorderedAccessResources(std::string name, std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE>& UAVList);

	void BindParameters();

private:
	// 添加编译好的着色器二进制信息并为其设置标识名
	// 该函数不会保存着色器二进制编码到文件
	HRESULT AddShader(std::string name, ComPtr<ID3DBlob> pShaderByteCode);

	D3D12_SHADER_VISIBILITY GetShaderVisibility(SHADER_TYPE type);
	std::vector<CD3DX12_STATIC_SAMPLER_DESC> CreateStaticSamplers();
	void CreateRootSignature();

	// 更新收集着色器反射信息
	HRESULT UpdateShaderReflection(std::string name, ID3D12ShaderReflection* pShaderReflection, SHADER_TYPE shaderFlags);
	// 清空所有资源与反射信息
	void Clear();

public:
	std::unordered_map<size_t, std::shared_ptr<VertexShaderInfo>> m_VertexShaders;		
	std::unordered_map<size_t, std::shared_ptr<HullShaderInfo>> m_HullShaders;
	std::unordered_map<size_t, std::shared_ptr<DomainShaderInfo>> m_DomainShaders;
	std::unordered_map<size_t, std::shared_ptr<GeometryShaderInfo>> m_GeometryShaders;
	std::unordered_map<size_t, std::shared_ptr<PixelShaderInfo>> m_PixelShaders;
	std::unordered_map<size_t, std::shared_ptr<ComputeShaderInfo>> m_ComputeShaders;

	std::unordered_map<size_t, std::shared_ptr<ConstantBufferVariable>> m_ConstantBufferVariables;	// 常量缓冲区的变量		// name转hash,作为ID
	std::unordered_map<size_t, std::shared_ptr<ConstantBufferData>> m_CBVParams;					// 绑定槽位作为ID
	std::unordered_map<size_t, std::shared_ptr<ShaderSRVParameter>> m_SRVParams;					// 绑定槽位作为ID
	std::unordered_map<size_t, std::shared_ptr<ShaderUAVParameter>> m_UAVParams;					// 绑定槽位作为ID
	std::unordered_map<size_t, std::shared_ptr<ShaderSamplerParameter>> m_SamplerParams;			// 绑定槽位作为ID	

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


class ShaderManager 
{
public:
	ShaderManager();
	~ShaderManager();

	ShaderManager(const ShaderManager&) = delete;
	ShaderManager& operator=(const ShaderManager&) = delete;
	ShaderManager(ShaderManager&&) = default;
	ShaderManager& operator=(ShaderManager&&) = default;

	HRESULT AddShader(std::string fileName);

	void Clear();


private:
	std::filesystem::path m_CacheDir;
	bool m_ForceWrite = false;				// 强制编译后缓存Shader
};


#endif // !SHADER_H
