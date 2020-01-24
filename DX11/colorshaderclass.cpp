#include "colorshaderclass.h"

ColorShaderClass::ColorShaderClass()
{
	m_vertexShader = nullptr;
	m_pixelShader = nullptr;
	m_layout = nullptr;
	m_matrixBuffer = nullptr;
}

ColorShaderClass::ColorShaderClass(const ColorShaderClass&)
{
}

ColorShaderClass::~ColorShaderClass()
{
}

bool ColorShaderClass::Initialize(ID3D11Device* device, HWND hwnd)
{
	bool result;
	WCHAR* vs = const_cast<WCHAR*>(L"../DX11/Color.vs");
	WCHAR* ps = const_cast<WCHAR*>(L"../DX11/Color.ps");
	result = InitializeShader(device, hwnd, vs, ps);
	if(!result)
	{
		return false;
	}

	return true;
}

void ColorShaderClass::Shutdown()
{
	ShutdownShader();
	return;
}

bool ColorShaderClass::Render(ID3D11DeviceContext* deviceContext, int indexCount, 
	XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix)
{
	bool result;

	result = SetShaderParameters(deviceContext, worldMatrix, viewMatrix, projectionMatrix);
	if(!result)
	{
		return false;
	}

	RenderShader(deviceContext, indexCount);

	return true;
}

bool ColorShaderClass::InitializeShader(ID3D11Device* device, HWND hwnd, WCHAR* vsFileName, WCHAR* psFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	ID3D10Blob* vertexShaderBuffer;
	ID3D10Blob* pixelShaderBuffer;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
	unsigned int numElements;
	D3D11_BUFFER_DESC matrixBufferDesc;

	errorMessage = nullptr;
	vertexShaderBuffer = nullptr;
	pixelShaderBuffer = nullptr;

	result = D3DCompileFromFile(vsFileName, NULL, NULL, "ColorVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0
		, &vertexShaderBuffer, &errorMessage);

	if(FAILED(result))
	{
		if(errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, vsFileName);
		}else
		{
			MessageBox(hwnd, vsFileName , L"Missing Shader File", MB_OK);
		}

		return false;
	}

	result = D3DCompileFromFile(psFilename, NULL, NULL, "ColorPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&pixelShaderBuffer, &errorMessage);
	if(FAILED(result))
	{
		if(errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, psFilename);
		} else
		{
			MessageBox(hwnd, psFilename, L"Missing Shader File", MB_OK);
		}
		
		return false;
	}

	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_vertexShader);
	if(FAILED(result))
	{
		return false;
	}
	
	return true;
}

void ColorShaderClass::ShutdownShader()
{
}

void ColorShaderClass::OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*)
{
}

bool ColorShaderClass::SetShaderParameters(ID3D11DeviceContext*, XMMATRIX, XMMATRIX, XMMATRIX)
{
	return true;
}

void ColorShaderClass::RenderShader(ID3D11DeviceContext*, int)
{
}
