//***************************************************************************************
// GameApp.h by XYC (C) 2022-2023 All Rights Reserved.
// Licensed under the Apache License 2.0.
//***************************************************************************************

#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <d3dApp.h>
#include <DirectXColors.h>

class GameApp :public D3DApp
{
public:
	GameApp(HINSTANCE hInstance);
	GameApp(HINSTANCE hInstance, int width, int height);
	~GameApp();

	virtual bool Initialize() override;
private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& timer) override;
	virtual void Draw(const GameTimer& timer)override;
};

#endif
