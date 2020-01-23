#include "d3dclass.h"

D3DClass::D3DClass()
{
	m_swapChain = 0;
	m_device = 0;
	m_deviceContext = 0;
	m_renderTargetView = 0;
	m_depthStencilBuffer = 0;
	m_depthStencilState = 0;
	m_depthStencilView = 0;
	m_rasterState = 0;
}

D3DClass::D3DClass(const D3DClass&)
{
}

D3DClass::~D3DClass()
{
}

bool D3DClass::Initialize(int screenWidth, int screenHeight, bool vsync, HWND hwnd, bool fullScreen, float screenDepth,
	float screenNear)
{
	HRESULT result;
	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGIOutput* adapterOutput;
	unsigned int numModes, i, numerator, denominator;
	unsigned long long stringLength;
	DXGI_MODE_DESC* displayModeList;
	DXGI_ADAPTER_DESC adapterDesc;
	int error;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D_FEATURE_LEVEL featureLevel;
	ID3D11Texture2D* backBufferPtr;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	D3D11_RASTERIZER_DESC rasterDesc;
	D3D11_VIEWPORT viewport;
	float fieldOfView, screenAspect;

	// store the vsync setting
	m_vsync_enabled = vsync;

	/*
	 * before we can initialize direct3d we have to get the refresh rate from the video card/monitor.
	 * each computer may be slightly different so we will need to query for that information.
	 * we query for the numerator and denominator values and then apss them to directx during the setup and it will calculate the proper refresh rate.
	 * if we don't do this and just set the refresh rate to a default value which may not exist on all computers then direct x will respond by performing a blit instaed of a uffer flip which will degrade performance and give us annoying erros in the debug output.
	 */
	
	// create a direct x graphics interface factory
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	if(FAILED(result))
	{
		return false;
	}

	// use the factory to create an adapter for the primary graphics interface (video card).
	result = factory->EnumAdapters(0, &adapter);
	if(FAILED(result))
	{
		return false;
	}

	// enumerate the primary adpater output( monitor)
	result = adapter->EnumOutputs(0, &adapterOutput);
	if(FAILED(result))
	{
		return false;
	}

	// get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor)
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if(FAILED(result))
	{
		return false;
	}

	// create a list to hold all the possible display modes for this monitor/video ccard combination;
	displayModeList = new DXGI_MODE_DESC[numModes];
	if(!displayModeList)
	{
		return false;
	}

	// not fill the display mode list structures.
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	if(FAILED(result))
	{
		return false;
	}

	// now go throu all the display modes and find the one that matches the screen width and height.
	// when a match is found store the numerator and denominator of the refresh rate for that monitor.
	for(i=0;i<numModes;i++)
	{
		if(displayModeList[i].Width == (unsigned int)screenWidth)
		{
			if(displayModeList[i].Height == (unsigned int) screenHeight)
			{
				numerator = displayModeList[i].RefreshRate.Numerator;
				denominator = displayModeList[i].RefreshRate.Denominator;
			}
		}
	}

	/*
	 * We now have the numerator and denominator for the refresh rate.
	 * The last thing we will retrieve using the adapter is the name of the video card and the amount of video memory.
	 */

	// get the adapter (video card) description.
	result = adapter->GetDesc(&adapterDesc);
	if(FAILED(result))
	{
		return false;
	}

	// store the dedicated video card memory in megabytes.
	m_videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// convert the name of the video card to a character array and store it.
	error = wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128);
	if(error != 0)
	{
		return false;
	}

	/*
	 * Now that we have stored the numeraotr and denominator for the refresh rate and the video card information we can release the structures and interfaces used to get that information.
	 */

	// release the display mode list
	delete[] displayModeList;
	displayModeList = 0;

	// release the adapter output
	adapterOutput->Release();
	adapterOutput = 0;

	// release the adapter
	adapter->Release();
	adapter = 0;

	// release the factory
	factory->Release();
	factory = 0;

	/*
	 * Now that we have the refresh rate from the system we can start the direct x initialization.
	 * the first thing we'll do is fill out the description of the swap chain.
	 * the swap chain is the front and back buffer to which the graphics wil be drawn.
	 * generally you use a single back buffer, do all your drawing to it, and the nswap it to the fron buffer which then displays on the user's screen.
	 * thar is why it is called a swap chain.
	 */

	// initialize the swap chain description
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	//set to a single back buffer
	swapChainDesc.BufferCount = 1;

	// set the width and height of the back buffer
	swapChainDesc.BufferDesc.Width = screenWidth;
	swapChainDesc.BufferDesc.Height = screenHeight;

	// set regular 32-bit surface for the back buffer
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	/*
	 * The next part of the description of the swap chain is the refresh rate.
	 * the refresh rate is how many tiems a second it draws the back buffer to the front buffer.
	 * if vsync is set to true in our graphicsclass.h header then this will lock the refresh rate to the system settings (for example 60hz).
	 * that means it will only draw the screen 60 times a second (or higher if the system refresh rate is more than 60).
	 * however if we set vsync to false then it wil draw the screen as many tiems a second as it can, however this can cause some visual artifacts.
	 */

	// set the refresh rate of the back buffer.
	if(m_vsync_enabled)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	} else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// set the handle for the window to render to.
	swapChainDesc.OutputWindow = hwnd;

	// turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// set to full screen or windowed mode.
	if(fullScreen)
	{
		swapChainDesc.Windowed = false;
	} else
	{
		swapChainDesc.Windowed = true;
	}

	// set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// discard the back buffer contents after presenting.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// do not set the advanced flags.
	swapChainDesc.Flags = 0;


	/*
	 * after setting up the swap chain description we also need to setup one more vairalbe called the feature level.
	 * this variables tells directx what version we plan to use.
	 * here we set the feature levelt o11.0 which is direct x 11.
	 * you can set this to 10 or 9 to use a lower level version of direct x if you plan on supporting multiple versions or running on lower end hardware.
	 */

	// set the feature level to directx 11.
	featureLevel = D3D_FEATURE_LEVEL_11_0;

	/*
	 * now that the swap chain desc and feature level have been filled out we can create the swap chain, the direct 3d device, and the direct 3d device context.
	 * the direct3d device and direct 3d device context are very important, they are the interface to all of the direct3d functions.
	 * we will use the device and device context for almost everything from this point forward.
	 *
	 * Those of you reading this who are familiar with the rpevious versions fo direct x will recognize the direct 3d device but will be unfamiliar with the new direct3d device context.
	 * basically they took the functionality of the direct 3d device and split it up into two different devices so you need to use both now.
	 *
	 * Note thatif the user does not have a direct x 11 video card this fucntion call will fail to create the device and device context.
	 * also if you are testing direct x 11 functionality yourself and do not have a direct x 11 video card then you can replace D3D_DRIVER_TYPE_HARDWARE with D3D_DRIVER_TYPE_REFERENCE and direct x will use your CPU to draw instaed ot the vide card hardware.
	 * note that this runs 1/1000 the speed but it is good for people who do not have direct x 11 video cards yet on all their machines.
	 */

	// create the swap chain, direct 3d device ,and direct 3d device context
	result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &featureLevel, 1,
		D3D11_SDK_VERSION, &swapChainDesc, &m_swapChain, &m_device, NULL, &m_deviceContext);

	if(FAILED(result))
	{
		return false;
	}

	/*
	 * sometimes this call to create the device will fail fi the primary video card is not compatible with direct x 11.
	 * some machines .amy have the primary card a a direct x 10 video card and the secondary card as a direct 11 video card.
	 * also some hybrid graphics cards work that way with the primary being the low power intel cal and the secondary being the high power Nvidia card.
	 * to get around this you will need to not use the default device and instead enumerate all the video cards in the machine and have the user choose which one to use and then specify that car when creating the device.
	 *
	 * now that we have a swap chai we need to get a pointer to the back buffer and then attach it to the swa pchain.
	 * we'll use the CreateRenderTargetView function to attach the back buffer to our swap chain.
	 */

	// get the pointer to the back buffer.
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	if(FAILED(result))
	{
		return false;
	}

	// create the render target view with the back buffer pointer.
	result = m_device->CreateRenderTargetView(backBufferPtr, NULL, &m_renderTargetView);
	if(FAILED(result))
	{
		return false;
	}

	// release pointer to the back buffer as we no longer need it.
	backBufferPtr->Release();
	backBufferPtr = 0;

	/*
	 * We will also need to set up a depth buffer description.
	 * we'll use this to create a depth buffer so that our polygons can be rendered properly in 3d space.
	 * at the same time we will attach a stencil buffer to our depth buffer.
	 * the stencil buffer can be used to achieve effects such as motion blur, volumetric shadows, and other things.
	 */

	// initialize the description of the depth buffer.
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// set up the description of the depth buffer.
	depthBufferDesc.Width = screenWidth;
	depthBufferDesc.Height = screenHeight;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	/* not we craete depth/stencil buffer using that description.
	 * you will notice we use the CreateTexture2D function to make the buffer, hence the buffer is just a 2d texture.
	 * The reason for this is that once your polygons are sorted and then rasterized they just end up being colored pixels in this 2D buffer.
	 * then this 2d buffer is drawn to the screen.
	 */

	// create the texture for the depth buffer using the filled out description.
	result = m_device->CreateTexture2D(&depthBufferDesc, NULL, &m_depthStencilBuffer);
	if(FAILED(result))
	{
		return false;
	}

	/*
	 * now we need to setup the depth stencil description .
	 * this allows us to control what type of depth test direct 3d will do for each pixel.
	 */

	// initialize the description of the stencil state.
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// stencil operation if pixel is front-facing
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// stencil operations if picel is back-facing
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	/*
	 * With the description filled out we can now create a depth stencil state.
	 */

	// create the depth stencil state.
	result = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
	if(FAILED(result))
	{
		return false;
	}

	/*
	 * With the created depth stencil state we can now set it so that it takes effect.
	 * notice we use the device context to set it.
	 */

	// set the depth stencil state.
	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);

	/*
	 * the next thing we need to create is the description of the vie of the depth stencil buffer.
	 * we do this so that direct 3d knows to use the depth buffer as a depth stencil texture.
	 * after filling out the description we then call the function CreateDepthStencilView to create it.
	 */

	// initialize the depth stencil view.
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// set up the depth stencil view description
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// create the depth stencil view.
	result = m_device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilViewDesc, &m_depthStencilView);
	if(FAILED(result))
	{
		return false;
	}

	/*
	 * With that created we can now call OMSetRenderTargets.
	 * this will bind the render target view and the depth stencil buffer to the output render pipeline.
	 * this way the graphics that the pipeline renders will get drawn to our back buffer that we privously created.
	 * with the graphics written to the back buffer we can then swap it to the front and display our graphics on the user's screen.
	 */

	// bind the render target view and dpeth stencil buffer to the output render pipeline.
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);


	/*
	 * Now that the render targets are setup we can continue on to some extra fuctions that will give us more control over our scenes for future tutorials.
	 * first thing is we'll create is a rasterizer state.
	 * this will give us control over how polygons are rendered.
	 * we can do things like make our scenes render in wireframe mode or have DirectX draw both the front and back faces of polygons.
	 * by default direct x already has a rasterizer state set up and working the exact same as the one below but you have no control to change it unless you set up one yourself.
	 */

	 // Setup the raster description which will determine how and what polygons will be drawn.

	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// create the rasterizer state from the description we just filled out.
	result = m_device->CreateRasterizerState(&rasterDesc, &m_rasterState);
	if(FAILED(result))
	{
		return false;
	}

	// now set the rasterizer state.
	m_deviceContext->RSSetState(m_rasterState);

	/*
	 * the viewport also needs to be setup so that direct3d can map clip space coordinates to the render target space.
	 * set this to be the entire size of the window.
	 */

	 // Setup the viewport for rendering.
	viewport.Width = (float)screenWidth;
	viewport.Height = (float)screenHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	// Create the viewport.
	m_deviceContext->RSSetViewports(1, &viewport);

	/*
	 * now we will create theprojection matrix.
	 * thr projectio nmatrix is used to translate the 3d scene into the 2d viewport space that we previously created.
	 * we will need to keep a copy of this matrix so that we can pass it to our shaders that will be used to render our scenes.
	 */

	// setup the projection matrix
	fieldOfView = 3.141592654f / 4.0f;
	screenAspect = (float)screenWidth / (float)screenHeight;

	// create the project matrix for 3d rendering.
	m_projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, screenNear, screenDepth);

	/*
	 * we will also create another matrix called the world matrix.
	 * this matrix is used to convert the vertices of our objects into vertices in the 3d scene.
	 * this matrix will also be use to rotate, translate, and scale our objects in 3d space.
	 * from the start we will just initialize the matrix to the identity matrix and keep a copy of it in this object.
	 * the copy will be needed to be passed to the shaders for rendering also.
	 */

	// initialize the world matrix to the identity matrix.
	m_worldMatrix = XMMatrixIdentity();

	/*
	 * this is where you would generally create a view matrix.
	 * the view matrix is used to calculate the position of where we are looking at the scene from.
	 * you can think of it as a camera and you only view the scene through this camera.
	 * because of its purpose i am going to create it in a camera classin later tutorials since logically it fits better there and just skip it for now.
	 *
	 * and the final thing we will setup in the initialize function is an orthographic projection matrix.
	 * this matrix is used for rendering 2d elements like use interfaces on the screen allowing use to skip the 3d rendering.
	 * you will see this used in later tutorials when we look at rendering 2d graphics and fonts to the screen.
	 */

	// create an orthographic project matrix for 2d rendering.
	m_orthoMatrix = XMMatrixOrthographicLH((float)screenWidth, (float)screenHeight, screenNear, screenDepth);

	return true;
}

void D3DClass::Shutdown()
{
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (m_swapChain)
	{
		m_swapChain->SetFullscreenState(false, NULL);
	}

	if (m_rasterState)
	{
		m_rasterState->Release();
		m_rasterState = 0;
	}

	if (m_depthStencilView)
	{
		m_depthStencilView->Release();
		m_depthStencilView = 0;
	}

	if (m_depthStencilState)
	{
		m_depthStencilState->Release();
		m_depthStencilState = 0;
	}

	if (m_depthStencilBuffer)
	{
		m_depthStencilBuffer->Release();
		m_depthStencilBuffer = 0;
	}

	if (m_renderTargetView)
	{
		m_renderTargetView->Release();
		m_renderTargetView = 0;
	}

	if (m_deviceContext)
	{
		m_deviceContext->Release();
		m_deviceContext = 0;
	}

	if (m_device)
	{
		m_device->Release();
		m_device = 0;
	}

	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = 0;
	}

}

void D3DClass::BeginScene(float red, float green, float blue, float alpha)
{
	float color[4];

	// setup the color to clear the buffer to.
	color[0] = red;
	color[1] = green;
	color[2] = blue;
	color[3] = alpha;

	//clear the back buffer
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, color);

	// clear the depth buffer
	m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void D3DClass::EndScene()
{
	// Present the back buffer to the screen since rendering is complete.
	if (m_vsync_enabled)
	{
		// Lock to screen refresh rate.
		m_swapChain->Present(1, 0);
	}
	else
	{
		// Present as fast as possible.
		m_swapChain->Present(0, 0);
	}
}

ID3D11Device* D3DClass::GetDevice()
{
	return m_device;
}

ID3D11DeviceContext* D3DClass::GetDeviceContext()
{
	return m_deviceContext;
}


void D3DClass::GetProjectionMatrix(XMMATRIX& projectionMatrix)
{
	projectionMatrix = m_projectionMatrix;
	return;
}

void D3DClass::GetWorldMatrix(XMMATRIX& worldMatrix)
{
	worldMatrix = m_worldMatrix;
	return;
}

void D3DClass::GetOrthoMatrix(XMMATRIX& orthoMatrix)
{
	orthoMatrix = m_orthoMatrix;
	return;
}

void D3DClass::GetVideoCardInfo(char* cardName, int& memory)
{
	strcpy_s(cardName, 128, m_videoCardDescription);
	memory = m_videoCardMemory;
	return;
}
