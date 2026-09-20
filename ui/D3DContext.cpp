#include "ui/D3DContext.h"

namespace
{
    constexpr UINT BUFFER_COUNT = 2;
    constexpr UINT REFRESH_RATE = 60;
}

D3DContext::~D3DContext()
{
    Destroy();
}

bool D3DContext::Create(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = BUFFER_COUNT;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = REFRESH_RATE;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL featureLevel;

    const HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        featureLevelArray, ARRAYSIZE(featureLevelArray), D3D11_SDK_VERSION,
        &sd, &_swapChain, &_device, &featureLevel, &_context);

    if (FAILED(hr))
    {
        Destroy();
        return false;
    }

    CreateRenderTarget();
    return true;
}

void D3DContext::CreateRenderTarget()
{
    if (!_swapChain || !_device) return;

    ID3D11Texture2D* backBuffer = nullptr;
    if (SUCCEEDED(_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))) && backBuffer)
    {
        _device->CreateRenderTargetView(backBuffer, nullptr, &_renderTarget);
        backBuffer->Release();
    }
}

void D3DContext::ReleaseRenderTarget()
{
    if (_renderTarget)
    {
        _renderTarget->Release();
        _renderTarget = nullptr;
    }
}

void D3DContext::Resize(UINT width, UINT height)
{
    if (!_device || !_swapChain) return;

    ReleaseRenderTarget();
    _swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTarget();
}

void D3DContext::BeginFrame(const float clearColor[4])
{
    if (!_context || !_renderTarget) return;

    _context->OMSetRenderTargets(1, &_renderTarget, nullptr);
    _context->ClearRenderTargetView(_renderTarget, clearColor);
}

void D3DContext::Present(UINT syncInterval)
{
    if (_swapChain) _swapChain->Present(syncInterval, 0);
}

void D3DContext::Destroy()
{
    ReleaseRenderTarget();

    if (_swapChain)
    {
        _swapChain->Release();
        _swapChain = nullptr;
    }
    if (_context)
    {
        _context->Release();
        _context = nullptr;
    }
    if (_device)
    {
        _device->Release();
        _device = nullptr;
    }
}
