#include "d3dUtil.h"

template<class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer)
{
	return ComPtr<ID3D12Resource>();
}

ComPtr<ID3DBlob> d3dUtil::LoadBinary(const std::wstring& filename)
{
	ComPtr<ID3DBlob>blob;
	std::ifstream fin(filename, std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	std::ifstream::pos_type size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);

	HR(D3DCreateBlob(size, blob.GetAddressOf()));

	fin.read((char*)blob->GetBufferPointer(), size);
	fin.close();

	return blob;
}

ComPtr<ID3DBlob> d3dUtil::CompileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target)
{
	UINT compileFlags = 0;
#if defined(DEBUG)|| defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;

	HR(D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors));

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	return byteCode;
}
