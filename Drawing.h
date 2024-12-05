#pragma once
#include <Windows.h>
#include <tchar.h>
#include "d3d9.h"
#include "d3d11.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "Cheat.h"

constexpr auto WIDTH = 400;
constexpr auto HEIGHT = 444;

class Drawing
{
private:
    static LPCSTR lpWindowName;
    static ImVec2 vWindowSize;
    static ImGuiWindowFlags WindowFlags;
    static bool bDraw;
    static Cheat* _cheat;

    static std::string WStringToUtf8(const std::wstring& wstr);

public:
    static void Initialize(Cheat* cheat);
    static void Active();
    static bool isActive();
    static void Draw();
};