//***************************************************************************************
// d3dApp.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#ifndef D3DAPP_H
#define D3DAPP_H

#if defined(DEBUG)||defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "GameTimer.h"
#include "ImguiManager.h"

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"D3D12.lib")
#pragma comment(lib,"dxgi.lib")

class D3DApp
{
protected:
	D3DApp(HINSTANCE hInstance);
	D3DApp(HINSTANCE hInstance, int width, int height);
	D3DApp(const D3DApp& rhs) = delete;
	D3DApp& operator=(const D3DApp& rhs) = delete;
	virtual ~D3DApp();

public:
	static  D3DApp* GetApp();
	HINSTANCE		AppInst()const;						// 获取应用实例的句柄
	HWND			MainWnd()const;						// 获取主窗口句柄
	float			AspectRatio()const;					// 获取屏幕宽高比

	bool			Get4xMsaaState()const;
	void			Set4xMsaaState(bool value);

	int				Run();								// 运行程序，进行游戏主循环

	virtual bool	Initialize();						// 该父类方法需要初始化窗口和Direct3D部分
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);	// 窗口的消息回调函数

protected:
	virtual void CreateRTVAndDSVDescriptorHeaps();
	virtual void OnResize();							// 该父类方法需要在窗口大小变动的时候调用
	virtual void Update(const GameTimer& timer) = 0;	// 子类需要实现该方法，完成每一帧的更新
	virtual void Draw(const GameTimer& timer) = 0;		// 子类需要实现该方法，完成每一帧的绘制

	//
	virtual void OnMouseDown(WPARAM btnState, int x, int y) {};
	virtual void OnMouseUp(WPARAM btnState, int x, int y) {};
	virtual void OnMouseMove(WPARAM btnState, int x, int y) {};

protected:
	bool InitMainWindow();
	bool InitDirect3D();
	bool InitImGui();
	void CreateCommandObjects();										// 创建命令队列、命令列表分配器和命令列表
	void CreateSwapChain();												// 创建交换链
	void FlushCommandQueue();											// 强制CPU等待GPU，直到GPU处理完队列中的所有命令
	ID3D12Resource* CurrentBackBuffer()const;							// 返回交换链中当前的后台缓冲区 ID3D12Resource
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;			// 返回当前后台缓冲区的RTV
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;				// 返回主深度/模板缓冲区的DSV

	void CalculateFrameStats();											// 计算每秒的平均帧耗时以及每帧平均的毫秒时长

	void LogAdapters();													// 枚举系统中所有的适配器
	void LogAdapterOutputs(IDXGIAdapter* adapter);						// 枚举指定适配器的全部显示输出
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);// 枚举某个显示输出对特定格式支持的所有显示模式

protected:

	static D3DApp* m_App;

	HINSTANCE	m_hAppInst = nullptr;						// 应用实例句柄
	HWND		m_hMainWnd = nullptr;						// 主窗口句柄
	bool		m_AppPaused;								// 应用是否暂停
	bool		m_Minimized;								// 应用是否最小化
	bool		m_Maximized;								// 应用是否最大化
	bool		m_Resizing;									// 窗口大小是否变化
	bool		m_FullscreenState;							// 应用是否全屏化

	bool m_4xMsaaState;										// 是否开启4倍多重采样
	UINT m_4xMsaaQuality;									// MSAA支持的质量等级

	GameTimer m_Timer;										// 计时器
	ImguiManager m_ImGuiManager;

	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;				//简化类型名

	ComPtr<IDXGIFactory4>	m_pDxgiFactory;					//
	ComPtr<IDXGISwapChain>	m_pSwapChain;					// D3D12交换链
	ComPtr<ID3D12Device>	m_pd3dDevice;					// D3D12设备
	ComPtr<ID3D12Fence>		m_Fence;						// 用于同步
	UINT64 m_CurrentFence;

	ComPtr<ID3D12CommandQueue>m_CommandQueue;				// D3D12命令队列
	ComPtr<ID3D12CommandAllocator> m_DirectCmdListAlloc;	// D3D12命令分配器
	ComPtr<ID3D12GraphicsCommandList>m_CommandList;			// D3D12命令列表

	static const int SwapChainBufferCount = 2;				// 交换链中缓冲区的数量
	int m_CurrentBackBuffer = 0;							// 当前后台缓冲区的索引
	ComPtr<ID3D12Resource>m_SwapChainBuffer[SwapChainBufferCount];		// 前后台缓冲区
	ComPtr<ID3D12Resource>m_DepthStencilBuffer;				// 深度/模板缓冲区
	ComPtr<ID3D12DescriptorHeap>m_RTVHeap;					// RTV的堆描述符
	ComPtr<ID3D12DescriptorHeap>m_DSVHeap;					// DSV的堆描述符
	ComPtr<ID3D12DescriptorHeap>m_SRVHeap;					// 着色器资源的堆描述符

	D3D12_VIEWPORT m_ScreenViewPort;						// 视口
	D3D12_RECT m_ScissorRect;								// 裁剪矩阵

	UINT m_RTVDescriptorSize = 0;
	UINT m_DSVDescriptorSize = 0;
	UINT m_CBVSRVDescriptorSize = 0;

	D3D_DRIVER_TYPE m_d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;			// Direct3D驱动类型
	DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;		// 前后台缓冲区的格式
	DXGI_FORMAT m_DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;	// 深度/模板缓冲区的格式

	// 派生类应该在构造函数设置好这些自定义的初始参数
	std::wstring m_MainWndCaption = L"LightWave demo";			// 主窗口标题
	int m_ClientWidth;											// 视口宽度
	int m_ClientHeight;											// 视口高度
};


#endif