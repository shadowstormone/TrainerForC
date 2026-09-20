#pragma once
#include <Windows.h>
#include <d3d11.h>

// Владеет устройством DirectX 11, цепочкой обмена и целью рендеринга.
// Ресурсы освобождаются в деструкторе, поэтому ручной Cleanup больше не нужен.
class D3DContext
{
    ID3D11Device* _device = nullptr;
    ID3D11DeviceContext* _context = nullptr;
    IDXGISwapChain* _swapChain = nullptr;
    ID3D11RenderTargetView* _renderTarget = nullptr;

    void CreateRenderTarget();
    void ReleaseRenderTarget();

public:
    D3DContext() = default;
    ~D3DContext();

    D3DContext(const D3DContext&) = delete;
    D3DContext& operator=(const D3DContext&) = delete;

    // Создаёт устройство и цепочку обмена для указанного окна.
    bool Create(HWND hWnd);
    void Destroy();

    // Пересоздаёт цель рендеринга под новый размер клиентской области (WM_SIZE).
    void Resize(UINT width, UINT height);

    // Делает цель текущей и заливает её цветом.
    void BeginFrame(const float clearColor[4]);
    void Present(UINT syncInterval = 1);

    bool IsValid() const { return _device != nullptr; }
    ID3D11Device* Device() const { return _device; }
    ID3D11DeviceContext* Context() const { return _context; }
};
