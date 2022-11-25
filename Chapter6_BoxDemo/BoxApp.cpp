#include "BoxApp.h"

BoxApp::BoxApp(HINSTANCE hInstance)
	:D3DApp(hInstance)
{
}

BoxApp::BoxApp(HINSTANCE hInstance, int witdh, int height)
	: D3DApp(hInstance, witdh, height)
{
}

BoxApp::~BoxApp()
{
}

bool BoxApp::Initialize()
{
	return false;
}

void BoxApp::OnResize()
{
}

void BoxApp::Update(const GameTimer& timer)
{
}

void BoxApp::Draw(const GameTimer& timer)
{
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
}

void BoxApp::BuildDescriptorHeaps()
{
}

void BoxApp::BuildConstantBuffers()
{
}

void BoxApp::BuildRootSignature()
{
}

void BoxApp::BuildShaderAndInputLayout()
{
}

void BoxApp::BuildBoxGeometry()
{
}

void BoxApp::BuildPSO()
{
}
