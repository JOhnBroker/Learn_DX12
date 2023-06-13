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
	ID3D12Resource* GetTexture(std::string name);
	ID3D12Resource* GetNullTexture();

	void BuildSRVDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv);
	void BuildRTVDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);
	void BuildDSVDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv);

private:
	void BuildDescriptors();
	void BuildResource();

private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D12Device> m_pDevice;
	std::unordered_map<XID, ComPtr<ID3D12Resource>> m_Textures;
	// 是存放Resource 还是存放Texture2D 好？
};

#endif // !TEXTUREMANAGER_H
