//***************************************************************************************
// d3dUtil.h by XYC (C) 2022-2023 All Rights Reserved.
// Licensed under the Apache License 2.0.
//
// D3D实用工具集
// Direct3D utility tools.
//***************************************************************************************

#ifndef D3DUTIL_H
#define D3DUTIL_H

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include <d3dx12.h>
#include <DXTrace.h>

// 是否开启图形调试对象名称
#if (defined(DEBUG) || defined(_DEBUG)) && !defined(GRAPHICS_DEBUGGER_OBJECT_NAME)
#define GRAPHICS_DEBUGGER_OBJECT_NAME 1
#endif


// 设置调试对象名称
// ------------------------------
// DXGISetDebugObjectName函数
// ------------------------------
// 为DXGI对象在图形调试器中设置对象名
// [In]obj					DXGI对象
// [In]name					对象名
inline void DXGISetDebugObjectName(IDXGIObject* obj, const char* name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	if (obj)
	{
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
#else
	UNREFERENCED_PARAMETER(obj);
	UNREFERENCED_PARAMETER(name);
#endif
}

// ------------------------------
// D3D12SetDebugObjectName函数
// ------------------------------
// 为D3D设备创建出来的对象在图形调试器中设置对象名
// [In]obj					D3D12设备创建出的对象
// [In]name					对象名
inline void D3D12SetDebugObjectName(ID3D12Device* obj, const char* name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	if (obj)
	{
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
#else
	UNREFERENCED_PARAMETER(obj);
	UNREFERENCED_PARAMETER(name);
#endif
}

// ------------------------------
// D3D12SetDebugObjectName函数
// ------------------------------
// 为D3D设备创建出来的对象在图形调试器中设置对象名
// [In]obj					D3D12设备创建出的对象
// [In]name					对象名
inline void D3D12SetDebugObjectName(ID3D12DeviceChild* obj, const char* name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	if (obj)
	{
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
#else
	UNREFERENCED_PARAMETER(obj);
	UNREFERENCED_PARAMETER(name);
#endif
}


class d3dUtil
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	static ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);
	static ComPtr<ID3DBlob> CompileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint, const std::string& taget);
};

#ifndef ReleaseCom
#define ReleaseCom(x){if(x){x->Release();x =0;}}
#endif


#endif
