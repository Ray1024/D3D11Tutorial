#include "D3D11App.h"
#include <WindowsX.h>
#include <sstream>

namespace
{
	// 因为无法直接将D3D11App::MsgProc赋值给WNDCLASS::lpfnWndProc
	// 可以将全局函数MainWndProc赋值给WNDCLASS::lpfnWndProc
	// 在MainWndProc函数内用一个全局的D3D11App变量转发消息
	D3D11App* g_D3D11App = 0;
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// 在这之前，D3D11App的构造函数会给g_D3D11App赋值
	return g_D3D11App->MsgProc(hwnd, msg, wParam, lParam);
}


//
// D3D11App Implement
//

D3D11App::D3D11App(HINSTANCE hInstance)
:	m_hAppInst(hInstance),
	m_mainWndCaption(L"D3D11 Application"),
	m_D3DDriverType(D3D_DRIVER_TYPE_HARDWARE),
	m_clientWidth(800),
	m_clientHeight(600),
	m_enable4xMsaa(false),
	m_hMainWnd(0),
	m_appPaused(false),
	m_minimized(false),
	m_maximized(false),
	m_Resizing(false),
	m_4xMsaaQuality(0),
 
	m_pD3DDevice(0),
	m_pD3DImmediateContext(0),
	m_pSwapChain(0),
	m_pDepthStencilBuffer(0),
	m_pRenderTargetView(0),
	m_pDepthStencilView(0)
{
	ZeroMemory(&m_screenViewport, sizeof(D3D11_VIEWPORT));

	g_D3D11App = this;
}

D3D11App::~D3D11App()
{
	ReleaseCOM(m_pRenderTargetView);
	ReleaseCOM(m_pDepthStencilView);
	ReleaseCOM(m_pSwapChain);
	ReleaseCOM(m_pDepthStencilBuffer);

	// Restore all default settings.
	if( m_pD3DImmediateContext )
		m_pD3DImmediateContext->ClearState();

	ReleaseCOM(m_pD3DImmediateContext);
	ReleaseCOM(m_pD3DDevice);
}

HINSTANCE D3D11App::AppInst()const
{
	return m_hAppInst;
}

HWND D3D11App::MainWnd()const
{
	return m_hMainWnd;
}

float D3D11App::AspectRatio()const
{
	return static_cast<float>(m_clientWidth) / m_clientHeight;
}

int D3D11App::Run()
{
	MSG msg = {0};
 
	m_timer.Reset();

	while(msg.message != WM_QUIT)
	{
		// 如果接收到Window消息，则处理这些消息
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}
		// 否则，则运行动画/游戏
		else
        {	
			m_timer.Tick();

			if( !m_appPaused )
			{
				CalculateFrameStats();
				UpdateScene(m_timer.DeltaTime());	
				DrawScene();
			}
			else
			{
				Sleep(100);
			}
        }
    }

	return (int)msg.wParam;
}

bool D3D11App::Init()
{
	if(!InitMainWindow())
		return false;

	if(!InitDirect3D())
		return false;

	return true;
}
 
void D3D11App::OnResize()
{
	assert(m_pD3DImmediateContext);
	assert(m_pD3DDevice);
	assert(m_pSwapChain);

	// 释放旧视图，因为它们持有对我们将要销毁的缓冲区的引用
	// 同样释放旧的深度/模板缓冲区
	ReleaseCOM(m_pRenderTargetView);
	ReleaseCOM(m_pDepthStencilView);
	ReleaseCOM(m_pDepthStencilBuffer);


	// 重置交换链大小 重新创建渲染目标视图

	HR(m_pSwapChain->ResizeBuffers(1, m_clientWidth, m_clientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	ID3D11Texture2D* backBuffer;
	HR(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)));
	HR(m_pD3DDevice->CreateRenderTargetView(backBuffer, 0, &m_pRenderTargetView));
	ReleaseCOM(backBuffer);

	// 创建深度/模板缓冲区和视图

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	
	depthStencilDesc.Width     = m_clientWidth;
	depthStencilDesc.Height    = m_clientHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	if( m_enable4xMsaa )
	{
		depthStencilDesc.SampleDesc.Count   = 4;
		depthStencilDesc.SampleDesc.Quality = m_4xMsaaQuality-1;
	}
	// No MSAA
	else
	{
		depthStencilDesc.SampleDesc.Count   = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
	}

	depthStencilDesc.Usage          = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0; 
	depthStencilDesc.MiscFlags      = 0;

	HR(m_pD3DDevice->CreateTexture2D(&depthStencilDesc, 0, &m_pDepthStencilBuffer));
	HR(m_pD3DDevice->CreateDepthStencilView(m_pDepthStencilBuffer, 0, &m_pDepthStencilView));


	// 将视图绑定到输出合并器阶段

	m_pD3DImmediateContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);
	

	// 设置视口

	m_screenViewport.TopLeftX = 0;
	m_screenViewport.TopLeftY = 0;
	m_screenViewport.Width    = static_cast<float>(m_clientWidth);
	m_screenViewport.Height   = static_cast<float>(m_clientHeight);
	m_screenViewport.MinDepth = 0.0f;
	m_screenViewport.MaxDepth = 1.0f;

	m_pD3DImmediateContext->RSSetViewports(1, &m_screenViewport);
}
 
LRESULT D3D11App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	// 当窗口被激活或非激活时会发送WM_ACTIVATE消息。  
	// 当非激活时我们会暂停游戏，当激活时则重新开启游戏。 
	case WM_ACTIVATE:
		if( LOWORD(wParam) == WA_INACTIVE )
		{
			m_appPaused = true;
			m_timer.Stop();
		}
		else
		{
			m_appPaused = false;
			m_timer.Start();
		}
		return 0;

	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		m_clientWidth  = LOWORD(lParam);
		m_clientHeight = HIWORD(lParam);
		if( m_pD3DDevice )
		{
			if( wParam == SIZE_MINIMIZED )
			{
				m_appPaused = true;
				m_minimized = true;
				m_maximized = false;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				m_appPaused = false;
				m_minimized = false;
				m_maximized = true;
				OnResize();
			}
			else if( wParam == SIZE_RESTORED )
			{
				
				// Restoring from minimized state?
				if( m_minimized )
				{
					m_appPaused = false;
					m_minimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if( m_maximized )
				{
					m_appPaused = false;
					m_maximized = false;
					OnResize();
				}
				else if( m_Resizing )
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or m_pSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

	// 当用户拖动窗口边框时会发送WM_EXITSIZEMOVE消息
	case WM_ENTERSIZEMOVE:
		m_appPaused = true;
		m_Resizing  = true;
		m_timer.Stop();
		return 0;

	// 当用户是否窗口边框时会发送WM_EXITSIZEMOVE消息
	// 然后我们会基于新的窗口大小重置所有图形变量
	case WM_EXITSIZEMOVE:
		m_appPaused = false;
		m_Resizing  = false;
		m_timer.Start();
		OnResize();
		return 0;
 
	// 窗口被销毁时发送WM_DESTROY消息
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	// 如果使用者按下Alt和一个与菜单项不匹配的字符时，或者在显示弹出式菜单而
	// 使用者按下一个与弹出式菜单里的项目不匹配的字符键时
	case WM_MENUCHAR:
        // 按下alt-enter切换全屏时不发出声响
        return MAKELRESULT(0, MNC_CLOSE);

	// 防止窗口变得过小
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}


bool D3D11App::InitMainWindow()
{
	WNDCLASS wc;
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = MainWndProc; 
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = m_hAppInst;
	wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = L"D3DWndClassName";

	if( !RegisterClass(&wc) )
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, m_clientWidth, m_clientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width  = R.right - R.left;
	int height = R.bottom - R.top;

	m_hMainWnd = CreateWindow(L"D3DWndClassName", m_mainWndCaption.c_str(), 
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_hAppInst, 0); 
	if( !m_hMainWnd )
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(m_hMainWnd, SW_SHOW);
	UpdateWindow(m_hMainWnd);

	return true;
}

bool D3D11App::InitDirect3D()
{
	// 创建设备和上下文

	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(
			0,                 // default adapter
			m_D3DDriverType,
			0,                 // no software device
			createDeviceFlags, 
			0, 0,              // default feature level array
			D3D11_SDK_VERSION,
			&m_pD3DDevice,
			&featureLevel,
			&m_pD3DImmediateContext);

	if( FAILED(hr) )
	{
		MessageBox(0, L"D3D11CreateDevice Failed.", 0, 0);
		return false;
	}

	if( featureLevel != D3D_FEATURE_LEVEL_11_0 )
	{
		MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
		return false;
	}

	// 检测4X多重采样质量支持

	HR(m_pD3DDevice->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m_4xMsaaQuality));
	assert( m_4xMsaaQuality > 0 );

	// 描述交换链

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width  = m_clientWidth;
	sd.BufferDesc.Height = m_clientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Use 4X MSAA? 
	if( m_enable4xMsaa )
	{
		sd.SampleDesc.Count   = 4;
		sd.SampleDesc.Quality = m_4xMsaaQuality-1;
	}
	// No MSAA
	else
	{
		sd.SampleDesc.Count   = 1;
		sd.SampleDesc.Quality = 0;
	}

	sd.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount  = 1;
	sd.OutputWindow = m_hMainWnd;
	sd.Windowed     = true;
	sd.SwapEffect   = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags        = 0;

	// 创建交换链

	IDXGIDevice* dxgiDevice = 0;
	HR(m_pD3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));
	      
	IDXGIAdapter* dxgiAdapter = 0;
	HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));

	IDXGIFactory* dxgiFactory = 0;
	HR(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory));

	HR(dxgiFactory->CreateSwapChain(m_pD3DDevice, &sd, &m_pSwapChain));
	
	ReleaseCOM(dxgiDevice);
	ReleaseCOM(dxgiAdapter);
	ReleaseCOM(dxgiFactory);

	// D3D初始化的剩余步骤需要在每次调整窗口大小时执行
	// 所以只需在这里调用OnResize方法，以避免代码重复
	
	OnResize();

	return true;
}

void D3D11App::CalculateFrameStats()
{
	// 计算每秒平均帧数的代码，还计算了绘制一帧的平均时间
	// 这些统计信息会显示在窗口标题栏中
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// 计算一秒时间内的平均值
	if( (m_timer.TotalTime() - timeElapsed) >= 1.0f )
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wostringstream outs;   
		outs.precision(6);
		outs << m_mainWndCaption << L"    "
			 << L"FPS: " << fps << L"    " 
			 << L"Frame Time: " << mspf << L" (ms)";
		SetWindowText(m_hMainWnd, outs.str().c_str());
		
		// 为了计算下一个平均值重置一些值
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}