#include "../Common/D3D11App.h"

class TestApp : public D3D11App
{
public:
	TestApp(HINSTANCE hInstance);

	void UpdateScene(float dt);
	void DrawScene(); 
};

// ³ÌÐòÈë¿Ú
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	TestApp theApp(hInstance);

	if( !theApp.Init() )
		return 0;

	return theApp.Run();
}

//
// TestApp Implement
//

TestApp::TestApp(HINSTANCE hInstance)
	: D3D11App(hInstance)
{
	m_mainWndCaption = L"2_D3DTimingAndAnimation";
}

void TestApp::UpdateScene(float dt)
{

}

void TestApp::DrawScene()
{
	m_pD3DImmediateContext->ClearRenderTargetView(m_pRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	m_pD3DImmediateContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	HR(m_pSwapChain->Present(0, 0));
}