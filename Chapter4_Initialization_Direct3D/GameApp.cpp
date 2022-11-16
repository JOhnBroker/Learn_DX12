#include "GameApp.h"

GameApp::GameApp(HINSTANCE hInstance)
	:D3DApp(hInstance)
{
}

GameApp::GameApp(HINSTANCE hInstance, int width, int height)
	: D3DApp(hInstance, width, height)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;
	return true;
}

void GameApp::OnResize()
{
	D3DApp::OnResize();
}

void GameApp::Update(const GameTimer& timer)
{
}

void GameApp::Draw(const GameTimer& timer)
{

	HR(m_DirectCmdListAlloc->Reset());

	HR(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	CD3DX12_RESOURCE_BARRIER resourceBarr1 = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_CommandList->ResourceBarrier(1, &resourceBarr1);

	m_CommandList->RSSetViewports(1, &m_ScreenViewPort);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	m_CommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::Blue, 0, nullptr);
	m_CommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE backBufferView = CurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE dsBufferView = DepthStencilView();
	m_CommandList->OMSetRenderTargets(1, &backBufferView, true, &dsBufferView);

	CD3DX12_RESOURCE_BARRIER resourceBarr2 = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_CommandList->ResourceBarrier(1, &resourceBarr2);

	HR(m_CommandList->Close());

	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	HR(m_pSwapChain->Present(0, 0));
	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}
