#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include "d3dUtil.h"
#include <string>

class TextureManager
{
public:
	TextureManager();
	~TextureManager();
	TextureManager(TextureManager&) = delete;
	TextureManager& operator= (const TextureManager&) = delete;
	TextureManager(TextureManager&&) = default;
	TextureManager& operator=(TextureManager&&) = default;

	static TextureManager& Get();
	void Init(ID3D12Device* device);
	ID3D12Resource* CreateFromeFile(std::string filename, bool enableMips = false, bool forceSRGB = false);
	ID3D12Resource* CreateFromeMemory(std::string name, void* data, size_t byteWidth, bool enableMips = false, bool forceSRGB = false);
	bool AddTexture(std::string name, ID3D12Resource* texture);
	void RemoveTexture(std::string name);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetTexture(std::string name);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetNullTexture();

	void BuildDescriptor();
	UINT GetSRVDescriptorCount();
	UINT GetDSVDescriptorCount();
	UINT GetRTVDescriptorCount();

private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D12Device> m_pDevice;
	using TextureData = std::pair<INT, ComPtr<ID3D12Resource>>;
	std::unordered_map<XID, TextureData> m_Textures;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;

};

#endif // !TEXTUREMANAGER_H
