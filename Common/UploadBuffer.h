#ifndef UPLOADBUFFER_H
#define UPLOADBUFFER_H

#include "d3dUtil.h"

template<class T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device,UINT elementCount,bool isConstantBuffer)
		:m_bIsConstantBuffer(isConstantBuffer)
	{
		m_ElementByteSize = sizeof(T);

		// Constant buffer elements need to be multiples of 256 bytes.
		// This is because the hardware can only view constant data 
		// at m*256 byte offsets and of n*256 byte lengths. 
		// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
		// UINT64 OffsetInBytes; // multiple of 256
		// UINT   SizeInBytes;   // multiple of 256
		// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
		if (isConstantBuffer)
			m_ElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));
	
		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC uploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_ElementByteSize * elementCount);
		HR(device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&uploadResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_UploadBuffer)));

		HR(m_UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData)));
	}

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator= (const UploadBuffer& rhs) = delete;
	~UploadBuffer() 
	{
		if (m_UploadBuffer != nullptr) 
		{
			m_UploadBuffer->Unmap(0, nullptr);
		}

		m_MappedData = nullptr;
	}

	ID3D12Resource* Resource()const
	{
		return m_UploadBuffer.Get();
	}

	void CopyData(int elementIndex, const T& data) 
	{
		memcpy(&m_MappedData[elementIndex * m_ElementByteSize], &data, sizeof(T));
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_UploadBuffer;
	BYTE* m_MappedData = nullptr;

	UINT m_ElementByteSize = 0;
	bool m_bIsConstantBuffer = false;
};

class UploadConstantBuffer
{
public:
	UploadConstantBuffer(ID3D12Device* device, UINT elementByteSize, UINT elementCount)
		:m_ElementByteSize(elementByteSize) 
	{
		// Constant buffer elements need to be multiples of 256 bytes.
		// This is because the hardware can only view constant data 
		// at m*256 byte offsets and of n*256 byte lengths. 
		// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
		// UINT64 OffsetInBytes; // multiple of 256
		// UINT   SizeInBytes;   // multiple of 256
		// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
		m_ElementByteSize = d3dUtil::CalcConstantBufferByteSize(m_ElementByteSize);

		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC uploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_ElementByteSize * elementCount);
		HR(device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&uploadResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_UploadBuffer)));

		HR(m_UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData)));
	}

	UploadConstantBuffer(const UploadConstantBuffer& rhs) = delete;
	UploadConstantBuffer& operator= (const UploadConstantBuffer& rhs) = delete;
	~UploadConstantBuffer()
	{
		if (m_UploadBuffer != nullptr)
		{
			m_UploadBuffer->Unmap(0, nullptr);
		}

		m_MappedData = nullptr;
	}

	HRESULT CreateBuffer(ID3D12Device* device, UINT elementByteSize, UINT elementCount)
	{
		HRESULT hResult = S_OK;
		if (m_UploadBuffer != nullptr) return hResult;
		m_ElementByteSize = elementByteSize;

		// Constant buffer elements need to be multiples of 256 bytes.
		// This is because the hardware can only view constant data 
		// at m*256 byte offsets and of n*256 byte lengths. 
		// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
		// UINT64 OffsetInBytes; // multiple of 256
		// UINT   SizeInBytes;   // multiple of 256
		// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
		m_ElementByteSize = d3dUtil::CalcConstantBufferByteSize(m_ElementByteSize);

		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC uploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_ElementByteSize * elementCount);
		HR(hResult = device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&uploadResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_UploadBuffer)));

		HR(hResult = m_UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData)));

		return hResult;
	}

	ID3D12Resource* Resource()const
	{
		return m_UploadBuffer.Get();
	}

	void CopyData(int elementIndex, void* data)
	{
		memcpy_s(&m_MappedData[elementIndex * m_ElementByteSize], m_ElementByteSize, &data, m_ElementByteSize);
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_UploadBuffer;
	BYTE* m_MappedData = nullptr;

	UINT m_ElementByteSize = 0;
};




#endif