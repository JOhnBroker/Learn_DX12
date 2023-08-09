#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include "d3dUtil.h"
#include "Texture.h"
#include <string>

class TextureManager
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	TextureManager();
	~TextureManager();
	TextureManager(TextureManager&) = delete;
	TextureManager& operator= (const TextureManager&) = delete;
	TextureManager(TextureManager&&) = default;
	TextureManager& operator=(TextureManager&&) = default;

	static TextureManager& Get();
	void Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void UpdateDescriptorCount();
	ITexture* CreateFromeFile(std::string filename, std::string name, bool isCube = false, bool forceSRGB = false);
	ITexture* CreateFromeMemory(std::string name, void* data, size_t byteWidth, bool isCube = false, bool forceSRGB = false);
	bool AddTexture(std::string name, std::shared_ptr<ITexture> texture);
	void RemoveTexture(std::string name);
	ITexture* GetTexture(std::string name);
	int GetTextureSrvIndex(std::string name);
	int GetTextureDsvIndex(std::string name);
	int GetTextureRtvIndex(std::string name);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetNullTexture() { return m_NullTex; }
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetNullCubeTexture() { return m_NullCubeTex; }
	UINT GetTextureCount() { return m_Textures.size(); }

	void BuildDescriptor(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize);
	void ReBuildDescriptor(std::string name, std::shared_ptr<ITexture> texture);
	UINT GetSRVDescriptorCount() { return m_nSRVCount; }
	UINT GetDSVDescriptorCount() { return m_nDSVCount; }
	UINT GetRTVDescriptorCount() { return m_nRTVCount; }

private:
	bool AddTextureIndex(std::string name, std::array<int, 3> index);
	bool AddTextureIndex(XID nameID, std::array<int, 3> index);
	void RemoveTextureIndex(std::string name);

private:
	ComPtr<ID3D12Device> m_pDevice;
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;

#ifdef _DEBUG
	std::unordered_map<std::string, std::shared_ptr<ITexture>> m_Textures;
	std::unordered_map<std::string, std::array<int, 3>> m_TexturesIndex;
#else
	std::unordered_map<XID, std::shared_ptr<ITexture>> m_Textures;
	std::unordered_map<XID, std::array<int, 3>> m_TexturesIndex;
#endif // _DEBUG


	UINT m_uiSrvDescriptorSize = 0;
	UINT m_uiDsvDescriptorSize = 0;
	UINT m_uiRtvDescriptorSize = 0;

	UINT m_nSRVCount = 0;
	UINT m_nDSVCount = 0;
	UINT m_nRTVCount = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuDsv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuRtv;

	CD3DX12_GPU_DESCRIPTOR_HANDLE m_NullTex;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_NullCubeTex;
};

#endif // !TEXTUREMANAGER_H
