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

DLL_EXPORT void SetCurrentCamera(point p,point t,point up);
DLL_EXPORT point GetCurrentCameraPos(UINT id);
DLL_EXPORT point GetCurrentCameraTar(UINT id);
DLL_EXPORT point GetCurrentCameraUp(UINT id);
DLL_EXPORT UINT GetCurrentCameraTime(UINT id);

DLL_EXPORT void AddPlotCamera(UINT id, point pos, point tar, point up, UINT t);
DLL_EXPORT void DelPlotCamera(UINT id);

DLL_EXPORT void SetCurrentObject(UINT shape, point pos, point rot,point sc);
DLL_EXPORT void AddPlotObject(UINT shape,UINT objectID,
	point pos, point rot, point sc,UINT ti);
DLL_EXPORT void SetPlotObject(
	UINT objectID, UINT isBegin,
	point pos, point rot, point sc, UINT ti);
DLL_EXPORT void DelPlotObject(UINT objectID);
DLL_EXPORT UINT GetPlotShape(UINT id);
DLL_EXPORT point GetPlotPos(UINT id, UINT isBegin);
DLL_EXPORT point GetPlotRot(UINT id, UINT isBegin);
DLL_EXPORT point GetPlotScale(UINT id, UINT isBegin);
DLL_EXPORT UINT GetPlotTime(UINT id, UINT isBegin);

//DLL_EXPORT BOOL PushCameraPlot(point3d pos, point3d target);
//DLL_EXPORT UINT GetLastCameraID();
