/**********************************************************************
 @FILE		4_D3DBoxDemo.cpp
 @BRIEF		Box demo application class.
 @AUTHOR	Ray1024
 @DATE		2016.11.24
 *********************************************************************/
#include "../Common/D3D11App.h"
#include "../Common/MathHelper.h"
#include "d3dx11Effect.h"

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

class D3D11BoxDemoApp : public D3D11App
{
public:
	D3D11BoxDemoApp(HINSTANCE hInstance);
	~D3D11BoxDemoApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene(); 

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void BuildGeometryBuffers();
	void BuildFX();
	void BuildVertexLayout();

private:
	ID3D11Buffer* m_pBoxVB;	// 顶点缓冲区
	ID3D11Buffer* m_pBoxIB; // 索引缓冲区

	ID3DX11Effect* m_pFX;	// effect
	ID3DX11EffectTechnique* m_pTech;	// technique
	ID3DX11EffectMatrixVariable* m_pFXWorldViewProj;	// 存储effect中的变量

	ID3D11InputLayout* m_pInputLayout;	// 顶点输入布局

	ID3D11RasterizerState* m_pWireframeRS;	// 线框模式
	ID3D11RasterizerState* m_pSolidRS;	// 实心模式

	XMFLOAT4X4 m_world;
	XMFLOAT4X4 m_view;	
	XMFLOAT4X4 m_proj;	

	float m_theta;	//
	float m_phi;	//
	float m_radius;	//半径

	POINT m_lastMousePos;
};

// 程序入口
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	D3D11BoxDemoApp theApp(hInstance);

	if( !theApp.Init() )
		return 0;

	return theApp.Run();
}

//
// D3D11BoxDemoApp Implement
//

D3D11BoxDemoApp::D3D11BoxDemoApp(HINSTANCE hInstance)
	: D3D11App(hInstance)
	, m_pBoxVB(0)
	, m_pBoxIB(0)
	, m_pFX(0)
	, m_pTech(0)
	, m_pFXWorldViewProj(0)
	, m_pInputLayout(0)
	, m_theta(1.5f*MathHelper::Pi)
	, m_phi(0.25f*MathHelper::Pi)
	, m_radius(5.0f)
	, m_pWireframeRS(NULL)
	, m_pSolidRS(NULL)
{
	m_mainWndCaption = L"4_D3DBoxDemo";

	m_lastMousePos.x = 0;
	m_lastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&m_world, I);
	XMStoreFloat4x4(&m_view, I);
	XMStoreFloat4x4(&m_proj, I);
}

D3D11BoxDemoApp::~D3D11BoxDemoApp()
{
	ReleaseCOM(m_pBoxVB);
	ReleaseCOM(m_pBoxIB);
	ReleaseCOM(m_pFX);
	ReleaseCOM(m_pInputLayout);
	ReleaseCOM(m_pWireframeRS);
	ReleaseCOM(m_pSolidRS);
}

bool D3D11BoxDemoApp::Init()
{
	if(!D3D11App::Init())
		return false;

	BuildGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

	// 在初始化时创建所需的渲染状态
	D3D11_RASTERIZER_DESC rsDesc; 
	ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC)); 
	rsDesc.FillMode = D3D11_FILL_WIREFRAME; 
	rsDesc.CullMode = D3D11_CULL_NONE; 
	rsDesc.FrontCounterClockwise = false; 
	rsDesc.DepthClipEnable = true;

	HR(m_pD3DDevice->CreateRasterizerState(&rsDesc,&m_pWireframeRS));

	return true;
}

void D3D11BoxDemoApp::OnResize()
{
	D3D11App::OnResize();

	// 当窗口大小改变时，需要更新横纵比，并重新计算投影矩阵
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_proj, P);
}

void D3D11BoxDemoApp::UpdateScene(float dt)
{
	/************************************************************************/
	/* 6.更新每帧的矩阵变换                                                 */
	/************************************************************************/

	// 视角变换矩阵

	// 将球面坐标转换为笛卡尔坐标
	float x = m_radius*sinf(m_phi)*cosf(m_theta);
	float z = m_radius*sinf(m_phi)*sinf(m_theta);
	float y = m_radius*cosf(m_phi);

	XMVECTOR pos    = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up     = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_view, V);

	// 把三个变换相乘，合成一个
	XMMATRIX world = XMLoadFloat4x4(&m_world);
	XMMATRIX view  = XMLoadFloat4x4(&m_view);
	XMMATRIX proj  = XMLoadFloat4x4(&m_proj);
	XMMATRIX worldViewProj = world*view*proj;

	// 通过C++程序更新Shader相应的变量
	m_pFXWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
}

void D3D11BoxDemoApp::DrawScene()
{
	/************************************************************************/
	/* 7.场景绘制                                                           */
	/************************************************************************/

	// 清屏
	m_pD3DImmediateContext->ClearRenderTargetView(m_pRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	m_pD3DImmediateContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 指定输入布局、图元拓扑类型、顶点缓冲、索引缓冲、渲染状态
	m_pD3DImmediateContext->IASetInputLayout(m_pInputLayout);
	m_pD3DImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	m_pD3DImmediateContext->IASetVertexBuffers(0, 1, &m_pBoxVB, &stride, &offset);
	m_pD3DImmediateContext->IASetIndexBuffer(m_pBoxIB, DXGI_FORMAT_R32_UINT, 0);
	// 是否使用线框模式
	//m_pD3DImmediateContext->RSSetState(m_pWireframeRS);

	// 从technique获取pass并逐个渲染
	D3DX11_TECHNIQUE_DESC techDesc;
	m_pTech->GetDesc( &techDesc );
	for(UINT p = 0; p < techDesc.Passes; ++p)
	{
		m_pTech->GetPassByIndex(p)->Apply(0, m_pD3DImmediateContext);
		m_pD3DImmediateContext->DrawIndexed(36, 0, 0);// 立方体有36个索引
	}

	// 显示
	HR(m_pSwapChain->Present(0, 0));
}

void D3D11BoxDemoApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;

	// 一旦窗口捕获了鼠标，所有鼠标输入都针对该窗口，无论光标是否在窗口的边界内
	SetCapture(m_hMainWnd);
}

void D3D11BoxDemoApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	// 不需要继续获得鼠标消息就释放掉
	ReleaseCapture();
}

void D3D11BoxDemoApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	// 鼠标左键调整视角角度
	if( (btnState & MK_LBUTTON) != 0 )
	{
		// 根据鼠标Pos和lastPos在x/y方向上的变化量dx/dy得出角度，每个像素的距离相当于1度，再将角度转为弧度
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - m_lastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - m_lastMousePos.y));

		// 根据dx和dy更新角度
		m_theta += dx;
		m_phi   += dy;

		// 限制角度m_phi
		m_phi = MathHelper::Clamp(m_phi, 0.1f, MathHelper::Pi-0.1f);
	}
	// 鼠标左键调整视角距离
	else if( (btnState & MK_RBUTTON) != 0 )
	{
		// 使每个像素对应于场景中的0.005个单元
		float dx = 0.005f*static_cast<float>(x - m_lastMousePos.x);
		float dy = 0.005f*static_cast<float>(y - m_lastMousePos.y);

		// 根据鼠标输入更新摄像机半径
		m_radius += dx - dy;

		// 限制m_radius
		m_radius = MathHelper::Clamp(m_radius, 3.0f, 15.0f);
	}

	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}

void D3D11BoxDemoApp::BuildGeometryBuffers()
{
	/************************************************************************/
	/* 1.创建顶点缓冲                                                       */
	/************************************************************************/
	Vertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), (const float*)&Colors::White   },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), (const float*)&Colors::Black   },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), (const float*)&Colors::Red     },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), (const float*)&Colors::Green   },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), (const float*)&Colors::Blue    },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), (const float*)&Colors::Yellow  },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), (const float*)&Colors::Cyan    },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), (const float*)&Colors::Magenta }
	};
	// 准备结构体，描述缓冲区
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * 8;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	// 准备结构体，为缓冲区指定初始化数据
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = vertices;
	// 创建缓冲区
	HR(m_pD3DDevice->CreateBuffer(&vbd, &vinitData, &m_pBoxVB));

	/************************************************************************/
	/* 2.创建索引缓冲                                                       */
	/************************************************************************/
	UINT indices[] = {
		// 前表面
		0, 1, 2,
		0, 2, 3,

		// 后表面
		4, 6, 5,
		4, 7, 6,

		// 左表面
		4, 5, 1,
		4, 1, 0,

		// 右表面
		3, 2, 6,
		3, 6, 7,

		// 上表面
		1, 5, 6,
		1, 6, 2,

		// 下表面
		4, 0, 3, 
		4, 3, 7
	};
	// 准备结构体，描述缓冲区
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * 36;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
	// 准备结构体，为缓冲区指定初始化数据
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = indices;
	// 创建缓冲区
	HR(m_pD3DDevice->CreateBuffer(&ibd, &iinitData, &m_pBoxIB));
}

void D3D11BoxDemoApp::BuildFX()
{
	/************************************************************************/
	/* 4.编译着色器，创建Effect                                             */
	/************************************************************************/
	// 编译着色器程序
	DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
	shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

	ID3D10Blob* compiledShader = 0;
	ID3D10Blob* compilationMsgs = 0;
	HRESULT hr = D3DX11CompileFromFile(L"FX/color.fx", 0, 0, 0, "fx_5_0", shaderFlags, 
		0, 0, &compiledShader, &compilationMsgs, 0);

	// compilationMsgs中包含错误或警告信息
	if( compilationMsgs != 0 )
	{
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(compilationMsgs);
	}

	// 就算没有compilationMsgs，也需要确保没有其他错误
	if(FAILED(hr))
	{
		DXTrace(__FILE__, (DWORD)__LINE__, hr, L"D3DX11CompileFromFile", true);
	}

	// 创建Effect
	HR(D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 
		0, m_pD3DDevice, &m_pFX));

	// 编译完成释放资源
	ReleaseCOM(compiledShader);

	// 从Effect中获取technique对象
	m_pTech    = m_pFX->GetTechniqueByName("ColorTech");
	// 从Effect中获取常量缓冲
	m_pFXWorldViewProj = m_pFX->GetVariableByName("gWorldViewProj")->AsMatrix();
}

void D3D11BoxDemoApp::BuildVertexLayout()
{
/************************************************************************/
/* 5.创建输入布局                                                       */
/************************************************************************/
// 顶点输入布局描述
D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
};

// 从technique对象中获取pass信息
D3DX11_PASS_DESC passDesc;
m_pTech->GetPassByIndex(0)->GetDesc(&passDesc);

// 创建顶点输入布局
HR(m_pD3DDevice->CreateInputLayout(vertexDesc, 2, passDesc.pIAInputSignature, 
	passDesc.IAInputSignatureSize, &m_pInputLayout));
}