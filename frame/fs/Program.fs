open System.Windows;;
open System.Windows.Forms;;
open System.Runtime.InteropServices;;

[<DllImport("ino_dll.Dll", EntryPoint="InitDxContext",CallingConvention = CallingConvention.Cdecl)>]
extern bool InitDxContext(nativeint hwnd, int width,int height);
[<DllImport("ino_dll.Dll", EntryPoint="DxContextFlush",CallingConvention = CallingConvention.Cdecl)>]
extern bool DxContextFlush();
[<DllImport("ino_dll.Dll", EntryPoint="ReleaseDxContext",CallingConvention = CallingConvention.Cdecl)>]
extern bool ReleaseDxContext();
[<DllImport("ino_dll.Dll", EntryPoint="SetTime",CallingConvention = CallingConvention.Cdecl)>]
extern void SetTime(int);

//MathNet.Numerics.LinearAlgebra.Vector;

type vec = 
    struct
        val mutable x : float32
        val mutable y : float32
        val mutable z : float32
        val mutable w : float32
        static member (+)(l :vec,r:vec) = vec(x=l.x+r.x,y=l.y+r.y,z=l.z+r.z,w=1.f)
        static member (-)(l :vec,r:vec) = vec(x=l.x-r.x,y=l.y-r.y,z=l.z-r.z,w=1.f)
        static member (*)(l :vec,r:vec) = vec(x=l.x*r.x,y=l.y*r.y,z=l.z*r.z,w=1.f)
        static member (*)(l :vec,r:float32) = vec(x=l.x*r,y=l.y*r,z=l.z*r,w=1.f)
    end

let length(v : vec) = System.Math.Sqrt(float(v.x*v.x))+System.Math.Sqrt(float(v.y*v.y))+System.Math.Sqrt(float(v.z*v.z))

let norm(v : vec) = v*(1.f/float32(length(v)))
let cross(u :vec,v :vec) = vec(x = u.y*v.z-u.z*v.y,y = u.z*v.x-u.x*v.z,z = u.x*v.y-u.y*v.x,w = 1.f)
let rotX (v : vec,th : float) = vec(x=v.x,y=v.y*float32( System.Math.Cos(th) ) + v.z* float32( System.Math.Sin(th) ),z=v.z* float32( System.Math.Cos(th) ) - v.y*float32( System.Math.Sin(th) ))
let rotY (v : vec,th : float) = vec(x=v.x*float32( System.Math.Cos(th) ) - v.z* float32( System.Math.Sin(th) ),y=v.y,z=v.x*float32( System.Math.Sin(th) ) + v.z* float32( System.Math.Cos(th) ))
let rotZ (v : vec,th : float) = vec(x=v.x*float32( System.Math.Cos(th) ) + v.y*float32( System.Math.Sin(th) ),y=v.y*float32( System.Math.Cos(th) ) - v.x*float32( System.Math.Sin(th) ),z=v.z)

[<DllImport("ino_dll.Dll", EntryPoint="SetCurrentCamera",CallingConvention = CallingConvention.Cdecl)>]
extern void SetCurrentCamera(vec,vec,vec);

type Edit3DPosition (title : string,x : int,y : int,control : System.Windows.Forms.Control.ControlCollection) as this =
    class
        let titleText = new Label(Left = x,Top = y,Text = title,Height = 15)
        let xEdit = new TextBox(Left = x + 30,Top = titleText.Bottom,Height = 15,Width = 60)
        let yEdit = new TextBox(Left = x + 110,Top = titleText.Bottom,Height = 15,Width = 60)
        let zEdit = new TextBox(Left = x + 190,Top = titleText.Bottom,Height = 15,Width = 60)
        let xText = new Label(Text = "x",Left = x+10,Top=titleText.Bottom,Height = 15,Width = 20)
        let yText = new Label(Text = "y",Left = x+110-20,Top=titleText.Bottom,Height = 15,Width = 20)
        let zText = new Label(Text = "z",Left = x+190-20,Top=titleText.Bottom,Height = 15,Width = 20)
        do
            control.Add(titleText)
            let label = [xText;yText;zText]
            let text = [xEdit;yEdit;zEdit]
            for v in label do 
                control.Add(v)
                v.TextAlign <- System.Drawing.ContentAlignment.MiddleRight
            for v in text do control.Add(v)
        member this.x = try System.Convert.ToDouble(xEdit.Text) with | :? System.SystemException -> 0.0
        member this.y = try System.Convert.ToDouble(yEdit.Text) with | :? System.SystemException -> 0.0
        member this.z = try System.Convert.ToDouble(zEdit.Text) with | :? System.SystemException -> 0.0
        member this.xyz = vec(x=float32 this.x,y=float32 this.y,z=float32 this.z,w=0.f)
        member this.setValue(x : float32,y : float32,z : float32) = do
            xEdit.Text <- x.ToString()
            yEdit.Text <- y.ToString()
            zEdit.Text <- z.ToString()
    end

type PlotManager (text : string,x : int,y : int,control : System.Windows.Forms.Control.ControlCollection) as this =
    class
        let cmb = new ComboBox(Left = x,Top = y,Width = 60,Height = 120)
        let addBtn = new Button(Text = "ADD",Left = x+60,Top = y,Width = 40,Height = 20)
        let delBtn = new Button(Text = "DEL",Left = x+100,Top = y,Width = 40,Height = 20)
        let label = new Label(Text = text,Left = x - 60,Top = y+5,Width = 60,Height = 20)
        do
            addBtn.Click.Add( fun _ -> 
                do this.add |> ignore 
            )
            delBtn.Click.Add( fun _ -> 
                do this.del |> ignore 
                for idx = 1 to cmb.Items.Count do cmb.Items.Item(idx-1) <- idx-1
            )
            control.Add(cmb)
            control.Add(addBtn)
            control.Add(delBtn)
            control.Add(label)
        member this.add = cmb.Items.Add( cmb.Items.Count )
        member this.del = cmb.Items.RemoveAt( this.Select )
        member this.Add f = do 
            this.add |> ignore
            addBtn.Click.Add(f)
        member this.Del f = do 
            this.del |> ignore
            delBtn.Click.Add(f)
        member this.Select = cmb.SelectedIndex
    end


type MFrame (timeLength : int) as this = 
    class 
        inherit Form(Text = "taal tool",Width = 1000,Height = 600)
        let renderTarget = new PictureBox(Left = 10,Top = 10,Width = 700,Height = 500)
        let captions = [new TextBox();new TextBox();new TextBox()]
        let timeScroll = new HScrollBar(Left = renderTarget.Left , Top = renderTarget.Bottom , Width = renderTarget.Width)
        let play = new CheckBox(Text = "▶",Left = timeScroll.Width/2+35,Top = timeScroll.Bottom + 10,Width = 25,Height = 20,Appearance = Appearance.Button)
        let play2x = new CheckBox(Text = "▶▶",Left = timeScroll.Width/2+60,Top = timeScroll.Bottom + 10,Width = 25,Height = 20,Appearance = Appearance.Button)
        let back = new CheckBox(Text = "◀",Left = timeScroll.Width/2-15,Top = timeScroll.Bottom + 10,Width = 25,Height = 20,Appearance = Appearance.Button)
        let back2x = new CheckBox(Text = "◀◀",Left = timeScroll.Width/2-40,Top = timeScroll.Bottom + 10,Width = 25,Height = 20,Appearance = Appearance.Button)
        let timeEvent = new Timer()
        let mutable time = 0
        let mutable timeBefore = System.DateTime.Now
        let timeCaption = new Label(Top = back.Top+5,Left = timeScroll.Left ,Width = 50)
        let camPosUI = new Edit3DPosition("camera_position",this.Width - 280,10,this.Controls)
        let camTarUI = new Edit3DPosition("camera_target",this.Width - 280,45,this.Controls)
        let camPlot = new PlotManager("cam_plot",this.Width - 160,85,this.Controls)
        let objPosUI = new Edit3DPosition("object_position",this.Width - 280,150,this.Controls)
        let objRotUI = new Edit3DPosition("object_rotation",this.Width - 280,185,this.Controls)
        let objIDs = new PlotManager("obj_id",this.Width - 160,225,this.Controls)
        let objPlot = new PlotManager("obj_plot",this.Width - 160,245,this.Controls)
        let modelID = new PlotManager("model_id",this.Width - 160,275,this.Controls)
        let saveBtn = new Button()
        let loadBtn = new Button()
        let mutable camPos = vec(x = 0.f,y = 0.f,z = -1.f,w = 0.f)
        let mutable camTar = vec(x = 0.f,y = 0.f,z = 1.f,w = 0.f)
        let mutable camUp = vec(x = 0.f,y = 1.f,z = 0.f,w = 0.f)
        do
            camPosUI.setValue(0.f,0.f,0.f)
            camTarUI.setValue(0.f,0.f,0.f)
            objPosUI.setValue(0.f,0.f,0.f)
            objRotUI.setValue(0.f,0.f,0.f)
            //Controls
            renderTarget.Paint.Add(fun e -> e.Dispose() )
            this.Controls.Add( renderTarget )
            timeScroll.Value <- 0
            timeScroll.LargeChange <- 1
            timeScroll.Minimum <- 0
            timeScroll.Maximum <- timeLength
            timeScroll.Scroll.Add( fun e -> do 
                time <- e.NewValue
                SetTime( time )
            )
            this.Controls.Add( timeScroll )
            this.Controls.Add( timeCaption )
            // ----- ButtonDriven -------
            let timeControls = [play;play2x;back;back2x]
            for b in timeControls do
                b.Click.Add( fun _ -> do 
                    for b2 in timeControls do
                        if not (b.Equals b2) then b2.Checked <- false
                    )
                this.Controls.Add( b )
            //Key Event
            this.KeyPreview <- true
            this.KeyDown.Add( fun e -> do
                if Keys.Escape = e.KeyCode then this.Close()
                //System.Math.Sin(0.f)
                
                let mutable camRot = vec(x = 0.f,y = 0.f,z = 0.f,w = 0.f)
                if Keys.Left = e.KeyCode then do
                    if e.Control then do
                        camTar <- rotZ(camTar,0.03)
                        camUp <- rotZ(camUp,0.03)
                    else do
                        camTar <- rotY(camTar,0.03)
                        camUp <- rotY(camUp,0.03)
                if Keys.Right = e.KeyCode then do
                    if e.Control then do
                        camTar <- rotZ(camTar,-0.03)
                        camUp <- rotZ(camUp,-0.03)
                    else do
                        camTar <- rotY(camTar,-0.03)
                        camUp <- rotY(camUp,-0.03)
                if Keys.Up = e.KeyCode then do
                    if e.Control then do
                        camPos <- camPos + camUp*0.11f
                    else do
                        camTar <- rotX(camTar,-0.05)
                        camUp <- rotX(camUp,-0.05)
                if Keys.Down = e.KeyCode then do
                    if e.Control then do
                        camPos <- camPos - camUp*0.11f
                    else do
                        camTar <- rotX(camTar,0.05)
                        camUp <- rotX(camUp,0.05)
                if Keys.A = e.KeyCode then camPos <- camPos + cross(camTar,camUp)*0.11f
                if Keys.D = e.KeyCode then camPos <- camPos - cross(camTar,camUp)*0.11f
                if Keys.W = e.KeyCode then camPos <- camPos + camTar*0.11f
                if Keys.S = e.KeyCode then camPos <- camPos - camTar*0.11f
                camTar <- norm(camTar)
                camUp <- norm(camUp)
                camPosUI.setValue(camPos.x,camPos.y,camPos.z)
                camTarUI.setValue(camTar.x,camTar.y,camTar.z)
                SetCurrentCamera( camPos,camPos+camTar,camUp )
            )
            // ----- TimerDriven --------
            if InitDxContext( renderTarget.Handle,renderTarget.Width,renderTarget.Height ) = false then MessageBox.Show("Error:InitDx12") |> ignore
            let mutable return_code = 0
            timeEvent.Tick.Add( fun _ -> do
                let mutable bias = 0;
                if play.Checked then bias <- 1
                if play2x.Checked then bias <- 2
                if back.Checked then bias <- -1
                if back2x.Checked then bias <- -2

                time <- time + (System.DateTime.Now - timeBefore).Milliseconds * bias;
                timeBefore <- System.DateTime.Now
                time <-(max (min time timeScroll.Maximum) timeScroll.Minimum)
                timeScroll.Value <- time;
                timeCaption.Text <- time.ToString()
                SetTime( time )
                try DxContextFlush() |> ignore
                finally ()
            )
            timeEvent.Interval <- 20;
            timeEvent.Start()
        override this.Finalize() = ()
    end

[<EntryPoint>]
let main argv =
    System.Console.WriteLine("taal tool") 
    // Create Window
    let MainForm = new MFrame(6000)
    let mutable return_code = 0
    try
        System.Windows.Forms.Application.Run(MainForm)
    with | :? System.SystemException -> return_code <- 1
    if ReleaseDxContext() = false then return_code <- 1
    return_code
