module CppImports

open System.Runtime.InteropServices;;
    
[<DllImport("ino_dll.Dll", EntryPoint="InitDxContext",CallingConvention = CallingConvention.Cdecl)>]
extern bool InitDxContext(nativeint hwnd, int width,int height)
[<DllImport("ino_dll.Dll", EntryPoint="DxContextFlush",CallingConvention = CallingConvention.Cdecl)>]
extern bool DxContextFlush()
[<DllImport("ino_dll.Dll", EntryPoint="ReleaseDxContext",CallingConvention = CallingConvention.Cdecl)>]
extern bool ReleaseDxContext()
[<DllImport("ino_dll.Dll", EntryPoint="MeshCount",CallingConvention = CallingConvention.Cdecl)>]
extern int MeshCount()
[<DllImport("ino_dll.Dll", EntryPoint="SetTime",CallingConvention = CallingConvention.Cdecl)>]
extern void SetTime(int)
[<DllImport("ino_dll.Dll", EntryPoint="SetPlotMode",CallingConvention = CallingConvention.Cdecl)>]
extern void SetPlotMode(int)

type vec = 
    struct
        val mutable x : float32
        val mutable y : float32
        val mutable z : float32
        val mutable w : float32
        static member (+)(l :vec,r:vec) = vec(x=l.x+r.x,y=l.y+r.y,z=l.z+r.z,w=0.f)
        static member (-)(l :vec,r:vec) = vec(x=l.x-r.x,y=l.y-r.y,z=l.z-r.z,w=0.f)
        static member (*)(l :vec,r:vec) = vec(x=l.x*r.x,y=l.y*r.y,z=l.z*r.z,w=0.f)
        static member (*)(l :vec,r:float32) = vec(x=l.x*r,y=l.y*r,z=l.z*r,w=0.f)
    end

let length(v : vec) = System.Math.Sqrt(float(v.x*v.x))+System.Math.Sqrt(float(v.y*v.y))+System.Math.Sqrt(float(v.z*v.z))

let norm(v : vec) = v*(1.f/float32(length(v)))
let cross(u :vec,v :vec) = vec(x = u.y*v.z-u.z*v.y,y = u.z*v.x-u.x*v.z,z = u.x*v.y-u.y*v.x,w = 1.f)
let rotX (v : vec,th : float) = vec(x=v.x,y=v.y*float32( System.Math.Cos(th) ) + v.z* float32( System.Math.Sin(th) ),z=v.z* float32( System.Math.Cos(th) ) - v.y*float32( System.Math.Sin(th) ))
let rotY (v : vec,th : float) = vec(x=v.x*float32( System.Math.Cos(th) ) - v.z* float32( System.Math.Sin(th) ),y=v.y,z=v.x*float32( System.Math.Sin(th) ) + v.z* float32( System.Math.Cos(th) ))
let rotZ (v : vec,th : float) = vec(x=v.x*float32( System.Math.Cos(th) ) + v.y*float32( System.Math.Sin(th) ),y=v.y*float32( System.Math.Cos(th) ) - v.x*float32( System.Math.Sin(th) ),z=v.z)

[<DllImport("ino_dll.Dll", EntryPoint="SetCurrentCamera",CallingConvention = CallingConvention.Cdecl)>]
extern void SetCurrentCamera(vec,vec,vec)

[<DllImport("ino_dll.Dll", EntryPoint="AddPlotCamera",CallingConvention = CallingConvention.Cdecl)>]
extern void AddPlotCamera(int,vec, vec,vec,int)

[<DllImport("ino_dll.Dll", EntryPoint="DelPlotCamera",CallingConvention = CallingConvention.Cdecl)>]
extern void DelPlotCamera(int)

[<DllImport("ino_dll.Dll", EntryPoint="GetCurrentCameraPos",CallingConvention = CallingConvention.Cdecl)>]
extern vec GetCurrentCameraPos(int id)

[<DllImport("ino_dll.Dll", EntryPoint="GetCurrentCameraTar",CallingConvention = CallingConvention.Cdecl)>]
extern vec GetCurrentCameraTar(int id)

[<DllImport("ino_dll.Dll", EntryPoint="GetCurrentCameraUp",CallingConvention = CallingConvention.Cdecl)>]
extern vec GetCurrentCameraUp(int id)

[<DllImport("ino_dll.Dll", EntryPoint="GetCurrentCameraTime",CallingConvention = CallingConvention.Cdecl)>]
extern int GetCurrentCameraTime(int id)

[<DllImport("ino_dll.Dll", EntryPoint="SetCurrentObject",CallingConvention = CallingConvention.Cdecl)>]
extern void SetCurrentObject(int, vec, vec, vec)

[<DllImport("ino_dll.Dll", EntryPoint="ExistObject",CallingConvention = CallingConvention.Cdecl)>]
extern bool ExistObject(int)

[<DllImport("ino_dll.Dll", EntryPoint="AddObject",CallingConvention = CallingConvention.Cdecl)>]
extern void AddObject(int)

[<DllImport("ino_dll.Dll", EntryPoint="SetObject",CallingConvention = CallingConvention.Cdecl)>]
extern void SetObject(int,int,vec,vec,vec,int);

[<DllImport("ino_dll.Dll", EntryPoint="DelObject",CallingConvention = CallingConvention.Cdecl)>]
extern void DelObject(int)

[<DllImport("ino_dll.Dll", EntryPoint="DelPlot",CallingConvention = CallingConvention.Cdecl)>]
extern void DelPlot(int,int)

[<DllImport("ino_dll.Dll", EntryPoint="PlotCount",CallingConvention = CallingConvention.Cdecl)>]
extern int PlotCount(int)

[<DllImport("ino_dll.Dll", EntryPoint="SetPlotShape",CallingConvention = CallingConvention.Cdecl)>]
extern void SetPlotShape(int,int);

[<DllImport("ino_dll.Dll", EntryPoint="GetPlotShape",CallingConvention = CallingConvention.Cdecl)>]
extern int GetPlotShape(int);

[<DllImport("ino_dll.Dll", EntryPoint="GetPlotPos",CallingConvention = CallingConvention.Cdecl)>]
extern vec GetPlotPos(int,int);

[<DllImport("ino_dll.Dll", EntryPoint="GetPlotRot",CallingConvention = CallingConvention.Cdecl)>]
extern vec GetPlotRot(int,int);

[<DllImport("ino_dll.Dll", EntryPoint="GetPlotScale",CallingConvention = CallingConvention.Cdecl)>]
extern vec GetPlotScale(int,int);

[<DllImport("ino_dll.Dll", EntryPoint="GetPlotTime",CallingConvention = CallingConvention.Cdecl)>]
extern int GetPlotTime(int,int);