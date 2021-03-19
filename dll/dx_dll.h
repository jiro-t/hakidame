#pragma once

#define NOMINMAX
#include <Windows.h>

#include <DirectXMath.h>

#ifdef DLL_EXPORT
#undef DLL_EXPORT
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DLL_EXPORT extern "C" __declspec(dllimport)
#endif

struct point {
	float x, y, z, w;
};

DLL_EXPORT BOOL InitDxContext(HWND hwnd,UINT width,UINT height);
DLL_EXPORT BOOL ReleaseDxContext();

DLL_EXPORT BOOL DxContextFlush();

DLL_EXPORT void SetTime(UINT t_);
DLL_EXPORT void SetPlotMode(BOOL isPlot);

DLL_EXPORT void SetCurrentCamera(point p,point t,point up);
//DLL_EXPORT BOOL PushObjectPlot(UINT shape,UINT objectID,point3d point, point3d rot);
//DLL_EXPORT UINT GetLastObjectID();

//DLL_EXPORT BOOL PushCameraPlot(point3d pos, point3d target);
//DLL_EXPORT UINT GetLastCameraID();
