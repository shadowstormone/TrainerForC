#pragma once
#include <iostream>
#include <Windows.h>
#include <vector>
#include <string>
#include <TlHelp32.h>
#include <sstream>
#include <iterator>
#include <Psapi.h>
#include <map>

//Custom includes
#include "Cheat.h"
#include "CheatOption.h"
#include "Memory_Functions.h"
#include "Patch.h"
#include "NopPatch.h"
#include "main.h"
#include "CavePatch.h"

#define NMD_ASSEMBLY_IMPLEMENTATION
#include "nmd_assembly.h"
#include "BaseRenderer.h"
#include "SimpleRenderer.h"