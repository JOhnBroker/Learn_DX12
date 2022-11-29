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
			m_ElementByteSize = d3dUtil;
	
		//TODO
	}


private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_UploadBuffer;
	BYTE* m_MappedData = nullptr;

	UINT m_ElementByteSize = 0;
	bool m_bIsConstantBuffer = false;
};

#endif