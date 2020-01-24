#include "systemclass.h"

SystemClass::SystemClass()
{
	m_Input = 0;
	m_Graphics = 0;
}

SystemClass::SystemClass(const SystemClass& other)
{
}

SystemClass::~SystemClass()
{
}

bool SystemClass::Initialize()
{
	int screenWidth, screenHeight;
	//bool result;

	// initialize the width and height of the screen to zero before sending the variables into the function;
	screenWidth = 0;
	screenHeight = 0;

	// initialized the windows api
	InitializeWindows(screenWidth, screenHeight);

	// create input object
	m_Input = new InputClass;
	if(!m_Input)
	{
		// generate failed
		return false;
	}

	// initialized the input obj
	m_Input->Initialize();

	// create the graphics object
	m_Graphics = new GraphicsClass;
	if(!m_Graphics)
	{
		// generate failed
		return false;
	}

	return m_Graphics->Initialize(screenWidth, screenHeight, m_hwnd);
}

void SystemClass::Shutdown()
{
	// Release the graphics obj
	if(m_Graphics)
	{
		m_Graphics->Shutdown();
		delete m_Graphics;
		m_Graphics = 0;
	}

	// release the input obj
	if(m_Input)
	{
		delete m_Input;
		m_Input = 0;
	}

	// shutdown window
	ShutdownWindows();

	return;
}

void SystemClass::Run()
{
	MSG msg;
	bool done, result;

	// init the msg structures;
	ZeroMemory(&msg, sizeof(MSG));

	// loop until there is a quit message from the window or the user
	done = false;
	while(!done)
	{
		// handle the windows messages
		if(PeekMessage(&msg, NULL, 0,0,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// if windows signals to end the application then exit out
		if(msg.message == WM_QUIT)
		{
			done = true;
		} else
		{
			// otherwise do the frame processing
			result = Frame();
			if(!result)
			{
				done = true;
			}
		}
	}

	return;
}

LRESULT SystemClass::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch(umsg)
	{
		// check if a key has been pressed on the keyboard
	case WM_KEYDOWN:
		{
			// if a key is pressed send it to the input object so it can record that state;
		m_Input->KeyDown((unsigned int)wparam);
		return 0;
		}

	case WM_KEYUP:
		{
			// if a key is release then send it to the input obj so it can unset the state for that key.
		m_Input->KeyUp((unsigned int)wparam);
		return 0;
		}

		// any other messages send to the default message handler as our application won't make use of them
	default:
		{
		return DefWindowProc(hwnd, umsg, wparam, lparam);
		}
	}
}

bool SystemClass::Frame()
{
	bool result;

	// check if the user pressed escape and wants to exit the applicaion
	if(m_Input->IsKeyDown(VK_ESCAPE))
	{
		return false;
	}

	// do ther frame processing for the graphics obj
	result = m_Graphics->Frame();
	if(!result)
	{
		return false;
	}

	return true;
}

void SystemClass::InitializeWindows(int& screenWidth, int& screenHeight)
{
	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;
	int posX, posY;

	// get an external pointer to this obj
	ApplicationHandle = this;

	// get the instance of this application
	m_hinstance = GetModuleHandle(NULL);

	// give the application a name;
	m_applicationName = L"Engine";

	// setup the windwos class with default settings.
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hinstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = m_applicationName;
	wc.cbSize = sizeof(WNDCLASSEX);

	// register the window class.
	RegisterClassEx(&wc);

	// determine the resolution of the clients desktop screen.
	screenWidth = GetSystemMetrics(SM_CXSCREEN);
	screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// setup the screen settings depending on whether it is running in full screen of in windowed mode.
	if(FULL_SCREEN)
	{
		// if full screen set the screen to maximum size of the users desktop and 32bit;
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = (unsigned long)screenWidth;
		dmScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// change the display settings to full screen.
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		// set the pos on the window to the top left corner;
		posX = posY = 0;
	}else
	{
		// if windowed then set it to 800x600 resolution
		screenWidth = 800;
		screenHeight = 600;

		// place the window in the middle of the screen
		posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;
	}

	// create the window with the screen settings and get the handle to it.
	m_hwnd = CreateWindowEx(WS_EX_APPWINDOW, m_applicationName, m_applicationName,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
		posX, posY, screenWidth, screenHeight, NULL, NULL, m_hinstance, NULL);

	// bring the window up on the screen and set it as main focus
	ShowWindow(m_hwnd, SW_SHOW);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	// hide the mouse cursor
	ShowCursor(false);

	return;
}

void SystemClass::ShutdownWindows()
{
	// show the mouse cursor
	ShowCursor(true);

	// fix the display settings if leaving full screen mode.
	if(FULL_SCREEN)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	// remove the window
	DestroyWindow(m_hwnd);
	m_hwnd = NULL;

	// remove the application instance
	UnregisterClass(m_applicationName, m_hinstance);
	m_hinstance = NULL;

	// release the pointer to this class
	ApplicationHandle = NULL;

	return;
}

LRESULT WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch (umsg)
	{
		// check if the window is being destroyed.
	case WM_DESTROY:
		{
		PostQuitMessage(0);
		return 0;
		}

		// check if the window is being closed.
	case WM_CLOSE:
		{
		PostQuitMessage(0);
		return 0;
		}

		// all other messages pass to the message handler in the system class.
	default:
		{
		return ApplicationHandle->MessageHandler(hwnd, umsg, wparam, lparam);
		}
	}
}
