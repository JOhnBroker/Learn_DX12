#include "GameApp.h"

GameApp::GameApp(HINSTANCE hInstance) :D3DApp(hInstance) 
{
}

GameApp::GameApp(HINSTANCE hInstance, int width, int height)
	: D3DApp(hInstance, width, height) 
{
}

GameApp::~GameApp()
{
	if (m_pd3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool GameApp::Initialize()
{
	if (!D3DApp::Initialize()) 
	{
		return false;
	}

	HR(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	if (!InitResource()) 
	{
		return false;
	}

	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildCarGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	// execute initialization commands
	HR(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	return true;
}

bool GameApp::InitResource()
{
	bool bResult = false;

	if (m_CameraMode == CameraMode::FirstPerson)
	{
		auto camera = std::make_shared<FirstPersonCamera>();
		m_pCamera = camera;
		camera->SetFrustum(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		camera->LookAt(XMFLOAT3(0.0f, 2.0f, -15.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
		BoundingFrustum::CreateFromMatrix(m_CamFrustum, m_pCamera->GetProjXM());
	}
	else if (m_CameraMode == CameraMode::ThirdPerson)
	{
		auto camera = std::make_shared<ThirdPersonCamera>();
		m_pCamera = camera;
		camera->SetFrustum(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		camera->SetTarget(XMFLOAT3(0.0f, 0.0f, 1.0f));
		camera->SetDistance(10.0f);
		camera->SetDistanceMinMax(3.0f, 20.0f);
		BoundingFrustum::CreateFromMatrix(m_CamFrustum, m_pCamera->GetProjXM());
	}

	LoadTexture("bricksTex", L"..\\Textures\\bricks.dds");
	LoadTexture("stoneTex", L"..\\Textures\\stone.dds");
	LoadTexture("tileTex", L"..\\Textures\\tile.dds");
	LoadTexture("crateTex", L"..\\Textures\\WoodCrate01.dds");
	LoadTexture("whiteTex", L"..\\Textures\\white1x1.dds");

	bResult = true;

	return bResult;
}

void GameApp::OnResize()
{
	D3DApp::OnResize();

	if (m_pCamera != nullptr) 
	{
		m_pCamera->SetFrustum(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		BoundingFrustum::CreateFromMatrix(m_CamFrustum, m_pCamera->GetProjXM());
	}
}

void GameApp::Update(const GameTimer& timer)
{
	auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);
	UpdateCamera(timer);

	// ImGui
	ImGuiIO& io = ImGui::GetIO();
	
	const float dt = timer.DeltaTime();
	if (ImGui::Begin("CameraDemo"))
	{
		ImGui::Checkbox("Wireframe", &m_WireframeEnable);
		ImGui::Checkbox("FrustumCulling", &m_FrustumCullingEnable);

		static int curr_cameramode = static_cast<int>(m_CameraMode);
		static const char* cameraMode[] = {
				"First Person",
				"Third Person",
		};
		if (ImGui::Combo("Camera Mode", &curr_cameramode, cameraMode, ARRAYSIZE(cameraMode)))
		{
			if (curr_cameramode == 0 && m_CameraMode != CameraMode::FirstPerson)
			{
				if (!cam1st)
				{
					cam1st = std::make_shared<FirstPersonCamera>();
					cam1st->SetFrustum(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
					m_pCamera = cam1st;
				}

				cam1st->LookAt(XMFLOAT3(0.0f, 2.0f, -15.0f),
					XMFLOAT3(0.0f, 0.0f, -1.0f),
					XMFLOAT3(0.0f, 1.0f, 0.0f));
				cam1st->UpdateViewMatrix();

				m_CameraMode = CameraMode::FirstPerson;
			}
			else if (curr_cameramode == 1 && m_CameraMode != CameraMode::ThirdPerson)
			{
				if (!cam3rd)
				{
					cam3rd = std::make_shared<ThirdPersonCamera>();
					cam3rd->SetFrustum(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
					m_pCamera = cam3rd;
				}
				cam3rd->SetTarget(XMFLOAT3(0.0f, 0.0f, 1.0f));
				cam3rd->SetDistance(10.0f);
				cam3rd->SetDistanceMinMax(3.0f, 20.0f);
				cam3rd->UpdateViewMatrix();
				m_CameraMode = CameraMode::ThirdPerson;
			}
		}
		
		static int curr_showmode = static_cast<int>(m_ShowMode);
		static const char* showMode[] = {
				"Sphere",
				"Box",
		};
		if (ImGui::Combo("Show Mode", &curr_showmode, showMode, ARRAYSIZE(showMode)))
		{
			if (curr_showmode == 0 && m_ShowMode != ShowMode::Sphere)
			{
				m_ShowMode = ShowMode::Sphere;
				// TODO
			}
			else if (curr_showmode == 1 && m_ShowMode != ShowMode::Box) 
			{
				m_ShowMode = ShowMode::Box;
				// TODO
			}
		}
		ImGui::Text("SumObjectCount %d\n VisibleObjectCount %d", m_AllRitems[0]->Instances.size(), m_CurrVisibleInstanceCount);
	}
	
	if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))
		m_SunTheta -= 1.0f * dt;
	if (ImGui::IsKeyDown(ImGuiKey_RightArrow))
		m_SunTheta += 1.0f * dt;
	if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
		m_SunPhi -= 1.0f * dt;
	if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
		m_SunPhi += 1.0f * dt;

	ImGui::End();
	ImGui::Render();

	m_CurrFrameResourceIndex = (m_CurrFrameResourceIndex + 1) % g_numFrameResources;
	m_CurrFrameResource = m_FrameResources[m_CurrFrameResourceIndex].get();

	// 等候GPU完成当前帧资源的命令
	if (m_CurrFrameResource->Fence != 0 && m_Fence->GetCompletedValue() < m_CurrFrameResource->Fence) 
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		HR(m_Fence->SetEventOnCompletion(m_CurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateInstanceData(timer);
	UpdateMaterialBuffer(timer);
	UpdateMainPassCB(timer);
}

void GameApp::Draw(const GameTimer& timer)
{
	auto cmdListAlloc = m_CurrFrameResource->CmdListAlloc;

	HR(cmdListAlloc->Reset());

	if (m_WireframeEnable)
	{
		HR(m_CommandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque_wireframe"].Get()));
	}
	else 
	{
		HR(m_CommandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque"].Get()));
	}

	m_CommandList->RSSetViewports(1, &m_ScreenViewPort);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_CommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	m_CommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_CommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeap[] = { m_SrvDescriptorHeap.Get() };
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeap), descriptorHeap);

	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	auto matBuffer = m_CurrFrameResource->MaterialBuffer->Resource();
	m_CommandList->SetGraphicsRootShaderResourceView(1, matBuffer->GetGPUVirtualAddress());
	auto passCB = m_CurrFrameResource->PassCB->Resource();
	m_CommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
	m_CommandList->SetGraphicsRootDescriptorTable(3, m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	DrawRenderItems(m_CommandList.Get(), m_OpaqueRitems);

	m_CommandList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());

	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	HR(m_CommandList->Close());
	
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	HR(m_pSwapChain->Present(0, 0));
	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % SwapChainBufferCount;
	m_CurrFrameResource->Fence = ++m_CurrentFence;

	m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence);

}

void GameApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_RBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_LastMousePos.y));

		//m_Camera.Pitch(dx);
		//m_Camera.RotateY(dy);
	}
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
}

void GameApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void GameApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
	SetCapture(m_hMainWnd);
}

void GameApp::UpdateCamera(const GameTimer& gt)
{
	auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);

	ImGuiIO& io = ImGui::GetIO();
	if (m_CameraMode == CameraMode::FirstPerson) 
	{
		float d1 = 0.0f, d2 = 0.0f;
		if (ImGui::IsKeyDown(ImGuiKey_W))
			d1 += gt.DeltaTime();
		if (ImGui::IsKeyDown(ImGuiKey_S))
			d1 -= gt.DeltaTime();
		if (ImGui::IsKeyDown(ImGuiKey_A))
			d2 -= gt.DeltaTime();
		if (ImGui::IsKeyDown(ImGuiKey_D))
			d2 += gt.DeltaTime();

		cam1st->Walk(d1 * 6.0f);
		cam1st->Strafe(d2 * 6.0f);

		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
		{
			cam1st->Pitch(io.MouseDelta.y * 0.01f);
			cam1st->RotateY(io.MouseDelta.x * 0.01f);
		}
	}
	else if (m_CameraMode == CameraMode::ThirdPerson) 
	{
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
		{
			cam3rd->RotateX(io.MouseDelta.y * 0.01f);
			cam3rd->RotateY(io.MouseDelta.x * 0.01f);
		}
		cam3rd->Approach(-io.MouseWheel * 1.0f);
	}
	m_pCamera->UpdateViewMatrix();
}

void GameApp::UpdateInstanceData(const GameTimer& gt)
{
	XMMATRIX view = m_pCamera->GetViewXM();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

	m_CurrVisibleInstanceCount = 0;
	
	auto currInstanceBuffer = m_CurrFrameResource->InstanceBuffer.get();
	for (auto& obj : m_OpaqueRitems) 
	{
		const auto& instanceData = obj->Instances;
		float visibleInstanceCount = 0;

		for (UINT i = 0; i < (UINT)instanceData.size(); ++i)
		{
			// 在局部空间，进行视锥剔除
			XMMATRIX world = XMLoadFloat4x4(&instanceData[i].World);
			XMMATRIX texTransform = XMLoadFloat4x4(&instanceData[i].TexTransform);

			XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);
			XMMATRIX viewToLocal = XMMatrixMultiply(invView, invWorld);

			BoundingFrustum localSpaceFrustum;
			m_CamFrustum.Transform(localSpaceFrustum, viewToLocal);
			if ((localSpaceFrustum.Contains(obj->Bounds) != DirectX::DISJOINT) || (false == m_FrustumCullingEnable)) 
			{
				InstanceData data; 
				XMStoreFloat4x4(&data.World, XMMatrixTranspose(world));
				XMStoreFloat4x4(&data.TexTransform, XMMatrixTranspose(texTransform));
				data.MaterialIndex = instanceData[i].MaterialIndex;

				currInstanceBuffer->CopyData(visibleInstanceCount++, data);
			}
		}
		obj->InstanceCount = visibleInstanceCount;
		m_CurrVisibleInstanceCount += visibleInstanceCount;
	}
}

void GameApp::UpdateMaterialBuffer(const GameTimer& gt)
{
	auto currMaterialCB = m_CurrFrameResource->MaterialBuffer.get();
	for (auto& pair : m_Materials) 
	{
		Material* mat = pair.second.get();
		if (mat->m_NumFramesDirty > 0) 
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->m_MatTransform);

			MaterialData matData;
			matData.DiffuseAlbedo = mat->m_DiffuseAlbedo;
			matData.FresnelR0 = mat->m_FresnelR0;
			matData.Roughness = mat->m_Roughness;
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
			matData.DiffuseMapIndex = mat->m_DiffuseSrvHeapIndex;

			currMaterialCB->CopyData(mat->m_MatCBIndex, matData);

			mat->m_NumFramesDirty--;
		}
	}
}

void GameApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = m_pCamera->GetViewXM();
	XMMATRIX proj = m_pCamera->GetProjXM();

	XMMATRIX viewproj = XMMatrixMultiply(view, proj);
	XMVECTOR dView = XMMatrixDeterminant(view);
	XMVECTOR dProj = XMMatrixDeterminant(proj);
	XMVECTOR dViewProj = XMMatrixDeterminant(viewproj);
	XMMATRIX invView = XMMatrixInverse(&dView, view);
	XMMATRIX invProj = XMMatrixInverse(&dProj, proj);
	XMMATRIX invViewproj = XMMatrixInverse(&dViewProj, viewproj);

	XMStoreFloat4x4(&m_MainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_MainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&m_MainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&m_MainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&m_MainPassCB.ViewProj, XMMatrixTranspose(viewproj));
	XMStoreFloat4x4(&m_MainPassCB.InvViewProj, XMMatrixTranspose(invViewproj));
	m_MainPassCB.EyePosW = m_pCamera->GetPosition();
	m_MainPassCB.RenderTargetSize = XMFLOAT2((float)m_ClientWidth, (float)m_ClientHeight);
	m_MainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / m_ClientWidth, 1.0f / m_ClientHeight);
	m_MainPassCB.NearZ = 1.0f;
	m_MainPassCB.FarZ = 1000.0f;
	m_MainPassCB.TotalTime = gt.TotalTime();
	m_MainPassCB.DeltaTime = gt.DeltaTime();
	m_MainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	m_MainPassCB.Lights[0].m_Direction = { 0.57735f, -0.57735f, 0.57735f };
	m_MainPassCB.Lights[0].m_Strength = { 0.8f, 0.8f, 0.8f };
	m_MainPassCB.Lights[1].m_Direction = { -0.57735f, -0.57735f, 0.57735f };
	m_MainPassCB.Lights[1].m_Strength = { 0.4f, 0.4f, 0.4f };
	m_MainPassCB.Lights[2].m_Direction = { 0.0f, -0.707f, -0.707f };
	m_MainPassCB.Lights[2].m_Strength = { 0.2f, 0.2f, 0.2f };
	
	auto currPassCB = m_CurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_MainPassCB);
}

void GameApp::BuildRootSignature()
{
	HRESULT hReturn = E_FAIL;
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0, 0);
	
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// 创建根描述符表
	slotRootParameter[0].InitAsShaderResourceView(0, 1);
	slotRootParameter[1].InitAsShaderResourceView(1, 1);
	slotRootParameter[2].InitAsConstantBufferView(0);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = GetStaticSamplers();

	// 一个根签名由一组根参数组成
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(slotRootParameter), slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// 序列化处理
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	hReturn = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr) 
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	HR(hReturn);

	HR(m_pd3dDevice->CreateRootSignature(
		0, 
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(), 
		IID_PPV_ARGS(m_RootSignature.GetAddressOf())));
}

void GameApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 5;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HR(m_pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto bricksTex = m_Textures["bricksTex"]->m_Resource;
	auto stoneTex = m_Textures["stoneTex"]->m_Resource;
	auto tileTex = m_Textures["tileTex"]->m_Resource;
	auto crateTex = m_Textures["crateTex"]->m_Resource;
	auto whiteTex = m_Textures["whiteTex"]->m_Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = bricksTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = bricksTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0;
	m_pd3dDevice->CreateShaderResourceView(bricksTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	srvDesc.Format = stoneTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = stoneTex->GetDesc().MipLevels;
	m_pd3dDevice->CreateShaderResourceView(stoneTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	srvDesc.Format = tileTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = tileTex->GetDesc().MipLevels;
	m_pd3dDevice->CreateShaderResourceView(tileTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	srvDesc.Format = crateTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = crateTex->GetDesc().MipLevels;
	m_pd3dDevice->CreateShaderResourceView(crateTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	srvDesc.Format = whiteTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = whiteTex->GetDesc().MipLevels;
	m_pd3dDevice->CreateShaderResourceView(whiteTex.Get(), &srvDesc, hDescriptor);
}

void GameApp::BuildShadersAndInputLayout()
{
	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter16\\Default.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter16\\Default.hlsl", nullptr, "PS", "ps_5_1");

	m_InputLayout =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL"  ,0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};
}

void GameApp::BuildCarGeometry()
{
	std::vector<Vertex> vertices(1);
	std::vector<std::uint16_t> indices;
	BoundingBox bounds;

	// read from file 
	ReadDataFromFile(vertices, indices, bounds);

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	submesh.Bounds = bounds;

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "carGeo";

	HR(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(),
		vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(),
		indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["car"] = submesh;

	m_Geometries[geo->Name] = std::move(geo);
}

void GameApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { m_InputLayout.data(),(UINT)m_InputLayout.size() };
	opaquePsoDesc.pRootSignature = m_RootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["standardVS"]->GetBufferPointer()),
		m_Shaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["opaquePS"]->GetBufferPointer()),
		m_Shaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = m_BackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = m_DepthStencilFormat;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_PSOs["opaque"])));

	// wireframe object
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&m_PSOs["opaque_wireframe"])));

}

void GameApp::BuildFrameResources()
{
	for(int i = 0;i<g_numFrameResources;++i)
	{
		m_FrameResources.push_back(std::make_unique<FrameResource>(m_pd3dDevice.Get(),
			1, (UINT)m_AllRitems.size(), (UINT)m_Materials.size()));
	}
}

void GameApp::BuildMaterials()
{
	auto bricks = std::make_unique<Material>();
	bricks->m_Name = "bricks";
	bricks->m_MatCBIndex = 0;
	bricks->m_DiffuseSrvHeapIndex = 0;
	bricks->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks->m_FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	bricks->m_Roughness = 0.1f;

	auto stone = std::make_unique<Material>();
	stone->m_Name = "stone";
	stone->m_MatCBIndex = 1;
	stone->m_DiffuseSrvHeapIndex = 1;
	stone->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	stone->m_FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	stone->m_Roughness = 0.3f;

	auto tile = std::make_unique<Material>();
	tile->m_Name = "tile";
	tile->m_MatCBIndex = 2;
	tile->m_DiffuseSrvHeapIndex = 2;
	tile->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	tile->m_FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile->m_Roughness = 0.3f;

	auto crateMat = std::make_unique<Material>();
	crateMat->m_Name = "crate";
	crateMat->m_MatCBIndex = 3;
	crateMat->m_DiffuseSrvHeapIndex = 3;
	crateMat->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	crateMat->m_FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	crateMat->m_Roughness = 0.2f;

	auto skullMat = std::make_unique<Material>();
	skullMat->m_Name = "skull";
	skullMat->m_MatCBIndex = 4;
	skullMat->m_DiffuseSrvHeapIndex = 4;
	skullMat->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skullMat->m_FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	skullMat->m_Roughness = 0.3f;

	m_Materials["bricks"] = std::move(bricks);
	m_Materials["stone"] = std::move(stone);
	m_Materials["tile"] = std::move(tile);
	m_Materials["crate"] = std::move(crateMat);
	m_Materials["skull"] = std::move(skullMat);
}

void GameApp::BuildRenderItems()
{
	float scale = 1.0f;
	auto carRitem = std::make_unique<RenderItem>();
	carRitem->World = MathHelper::Identity4x4();
	carRitem->TexTransform = MathHelper::Identity4x4();
	carRitem->ObjCBIndex = 0;
	carRitem->Mat = m_Materials["tile"].get();
	carRitem->Geo = m_Geometries["carGeo"].get();
	carRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	carRitem->IndexCount = carRitem->Geo->DrawArgs["car"].IndexCount;
	carRitem->StartIndexLocation = carRitem->Geo->DrawArgs["car"].StartIndexLocation;
	carRitem->BaseVertexLocation = carRitem->Geo->DrawArgs["car"].BaseVertexLocation;
	carRitem->Bounds = carRitem->Geo->DrawArgs["car"].Bounds;
	
	m_AllRitems.push_back(std::move(carRitem));
	m_OpaqueRitems.push_back(carRitem.get());
}

void GameApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	for (size_t i = 0; i < ritems.size(); ++i) 
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->GetVertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->GetIndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		// 使用描述符
		auto instanceBuffer = m_CurrFrameResource->InstanceBuffer->Resource();
		cmdList->SetGraphicsRootShaderResourceView(0, instanceBuffer->GetGPUVirtualAddress());

		cmdList->DrawIndexedInstanced(ri->IndexCount, ri->InstanceCount, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void GameApp::Pick(int sx, int sy)
{
}

void GameApp::ReadDataFromFile(std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices, BoundingBox& bounds)
{
	std::ifstream fin(m_VertexFileName);

	if (!fin)
	{
		OutputDebugStringA(m_VertexFileName.c_str());
		OutputDebugStringA(" not found,\n");
		return;
	}

	UINT vcount = 0, tcount = 0;
	float normal = 0.0f;
	std::string ignore;
	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	vertices.resize(vcount);
	indices.resize(3 * tcount);

	XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

		XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

		XMFLOAT3 spherePos;
		XMStoreFloat3(&spherePos, XMVector3Normalize(P));

		float theta = atan2f(spherePos.z, spherePos.x);

		if (theta < 0.0f) 
		{
			theta += XM_2PI;
		}
		float phi = acosf(spherePos.y);

		float u = theta / XM_PI * 0.5f;
		float v = phi / XM_PI;

		vertices[i].TexC = { u,v };

		vMin = XMVectorMin(vMin, P);
		vMax = XMVectorMax(vMax, P);
	}

	XMStoreFloat3(&bounds.Center, 0.5 * (vMin + vMax));
	XMStoreFloat3(&bounds.Extents, 0.5 * (vMin - vMax));

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, STATICSAMPLERCOUNT> GameApp::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,
		8);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f,
		8);

	return { pointWrap, pointClamp,
			linearWrap, linearClamp,
			anisotropicWrap ,anisotropicClamp };
}

void GameApp::LoadTexture(std::string name, std::wstring filename)
{
	auto texture = std::make_unique<Texture>();

	texture->m_Name = name;
	texture->m_Filename = filename;
	HR(DirectX::CreateDDSTextureFromFile12(m_pd3dDevice.Get(), m_CommandList.Get(),
		texture->m_Filename.c_str(), texture->m_Resource, texture->m_UploadHeap));
	m_Textures[name] = std::move(texture);
}
