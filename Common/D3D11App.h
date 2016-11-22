/**********************************************************************
 @FILE		D3D11App.h
 @BRIEF		Simple Direct3D demo application class.
 @AUTHOR	Ray1024
 @DATE		2016.11.16
 *********************************************************************/

#ifndef D3D11App_H
#define D3D11App_H

#include "D3D11Util.h"
#include "GameTimer.h"
#include <string>

class D3D11App
{
public:
	D3D11App(HINSTANCE hInstance);
	virtual ~D3D11App();
	
	// 获取应用程序实例句柄
	HINSTANCE AppInst()const;
	// 获取主窗口句柄
	HWND MainWnd()const;
	// 后台缓存区的长宽比
	float AspectRatio()const;
	// 应用程序消息循环
	int Run();
 
	// 框架方法
	// 派生类需要重载这些方法实现所需的功能

	virtual bool Init();
	virtual void OnResize(); 
	virtual void UpdateScene(float dt)=0;
	virtual void DrawScene()=0; 
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// 处理鼠标输入事件的便捷重载函数
	virtual void OnMouseDown(WPARAM btnState, int x, int y){ }
	virtual void OnMouseUp(WPARAM btnState, int x, int y)  { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y){ }

protected:

	// 创建窗口
	bool InitMainWindow();

	// 初始化D3D
	bool InitDirect3D();

	// 计算帧率
	void CalculateFrameStats();

protected:

	HINSTANCE m_hAppInst;		// 应用程序实例句柄
	HWND      m_hMainWnd;		// 主窗口句柄
	bool      m_appPaused;		// 程序是否处在暂停状态
	bool      m_minimized;		// 程序是否最小化
	bool      m_maximized;		// 程序是否最大化
	bool      m_Resizing;		// 程序是否处在改变大小的状态
	UINT      m_4xMsaaQuality;	// 4X MSAA质量等级

	GameTimer m_timer;			// 用于记录deltatime和游戏时间

	ID3D11Device*			m_pD3DDevice;			// D3D11设备
	ID3D11DeviceContext*	m_pD3DImmediateContext;	// 上下文
	IDXGISwapChain*			m_pSwapChain;			// 交换链
	ID3D11Texture2D*		m_pDepthStencilBuffer;	// 深度缓冲区
	ID3D11RenderTargetView* m_pRenderTargetView;	// 渲染目标视图
	ID3D11DepthStencilView* m_pDepthStencilView;	// 深度缓冲视图	
	D3D11_VIEWPORT			m_screenViewport;		// 视口


	std::wstring	m_mainWndCaption;		// 窗口标题
	D3D_DRIVER_TYPE m_D3DDriverType;		// 是否使用硬件加速
	int				m_clientWidth;			// 窗口大小
	int				m_clientHeight;			// 窗口大小
	bool			m_enable4xMsaa;			// 是否使用4XMSAA
};

#endif // D3D11App_H