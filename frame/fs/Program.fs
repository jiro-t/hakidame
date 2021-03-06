﻿module App

open System.Windows;;
open System.Windows.Forms;;

open System.IO;;

open CppImports;;

type Edit3DPosition (title : string,x : int,y : int,control : System.Windows.Forms.Control.ControlCollection) =
    class
        let titleText = new Label(Left = x,Top = y,Text = title,Height = 15)
        let xEdit = new TextBox(Left = x + 30,Top = titleText.Bottom,Height = 15,Width = 60)
        let yEdit = new TextBox(Left = x + 110,Top = titleText.Bottom,Height = 15,Width = 60)
        let zEdit = new TextBox(Left = x + 190,Top = titleText.Bottom,Height = 15,Width = 60)
        let xText = new Label(Text = "x",Left = x+10,Top=titleText.Bottom,Height = 15,Width = 20)
        let yText = new Label(Text = "y",Left = x+110-20,Top=titleText.Bottom,Height = 15,Width = 20)
        let zText = new Label(Text = "z",Left = x+190-20,Top=titleText.Bottom,Height = 15,Width = 20)
        let label = [xText;yText;zText]
        let text = [xEdit;yEdit;zEdit]
        do
            control.Add(titleText)
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
        member this.Enable b = for v in text do v.Enabled <- b
        member this.Edit f =  for v in text do v.TextChanged.Add(f)
    end

type PlotManager (text : string,x : int,y : int,control : System.Windows.Forms.Control.ControlCollection) =
    class
        let cmb = new ComboBox(Left = x,Top = y,Width = 60,Height = 120)
        let addBtn = new Button(Text = "ADD",Left = x+60,Top = y,Width = 40,Height = 20)
        let setBtn = new Button(Text = "SET",Left = x+100,Top = y,Width = 40,Height = 20)
        let delBtn = new Button(Text = "DEL",Left = x+140,Top = y,Width = 40,Height = 20)
        let label = new Label(Text = text,Left = x - 60,Top = y+5,Width = 60,Height = 20)
        do
            for c in [cmb :> Control;addBtn :> Control;setBtn :> Control;delBtn :> Control;label :> Control] do
                control.Add(c)
        member this.Add f = do 
            addBtn.Click.Add(f)
        member this.Del f = do 
            delBtn.Click.Add(f)
        member this.Set f = do 
            setBtn.Click.Add(f)
        member this.Select = cmb.SelectedIndex
        member this.SelectedIndexChanged f = cmb.SelectedIndexChanged.Add(f)
        member this.AddItem = do
            cmb.Items.Add( cmb.Items.Count ) |> ignore
        member this.DelItem index = do
            if index >= 0 then cmb.Items.RemoveAt( index ) |> ignore
            for idx = 1 to cmb.Items.Count do cmb.Items.Item(idx-1) <- idx-1
        member this.Count() = cmb.Items.Count
    end

let export owner = do
    ()

type MFrame (title : string,timeLength : int) as this = 
    class 
        inherit Form(Text = title,Width = 1000,Height = 600)
        let renderTarget = new PictureBox(Left = 10,Top = 10,Width = 700,Height = 500)
        let captions = [new TextBox();new TextBox();new TextBox()]
        let timeScroll = new HScrollBar(Left = renderTarget.Left , Top = renderTarget.Bottom , Width = renderTarget.Width)
        let play = new CheckBox(Text = "▶",Left = timeScroll.Width/2+35,Top = timeScroll.Bottom + 10,Width = 25,Height = 20,Appearance = Appearance.Button)
        let play2x = new CheckBox(Text = "▶▶",Left = timeScroll.Width/2+60,Top = timeScroll.Bottom + 10,Width = 25,Height = 20,Appearance = Appearance.Button)
        let back = new CheckBox(Text = "◀",Left = timeScroll.Width/2-15,Top = timeScroll.Bottom + 10,Width = 25,Height = 20,Appearance = Appearance.Button)
        let back2x = new CheckBox(Text = "◀◀",Left = timeScroll.Width/2-40,Top = timeScroll.Bottom + 10,Width = 25,Height = 20,Appearance = Appearance.Button)
        let timeEvent = new Timer()
        let saveBtn = new Button(Text = "save",Left = this.Width - 280,Top = 500,Width = 40,Height = 20)
        let loadBtn = new Button(Text = "load",Left = this.Width - 240,Top = 500,Width = 40,Height = 20)
        let exportBtn = new Button(Text = "export",Left = this.Width - 190,Top = 500,Width = 45,Height = 20)
        let mutable time = 0
        let mutable timeBefore = System.DateTime.Now
        let timeCaption = new Label(Top = back.Top+5,Left = timeScroll.Left ,Width = 50)
        let camPosUI = new Edit3DPosition("camera_position",this.Width - 280,10,this.Controls)
        let camTarUI = new Edit3DPosition("camera_target",this.Width - 280,45,this.Controls)
        let camPlot = new PlotManager("cam_plot",this.Width - 200,85,this.Controls)
        let objPosUI = new Edit3DPosition("object_position",this.Width - 280,150,this.Controls)
        let objRotUI = new Edit3DPosition("object_rotation",this.Width - 280,185,this.Controls)
        let objScaleUI = new Edit3DPosition("object_scale",this.Width - 280,220,this.Controls)
        let objPlot = new PlotManager("obj_plot",this.Width - 200,260,this.Controls)
        let modelID = new PlotManager("model_id",this.Width - 200,310,this.Controls)
        let mutable camPos = vec(x = 0.f,y = 0.f,z = -1.f,w = 0.f)
        let mutable camTar = vec(x = 0.f,y = 0.f,z = 1.f,w = 0.f)
        let mutable camUp = vec(x = 0.f,y = 1.f,z = 0.f,w = 0.f)
        let mutable camRot = vec(x = 0.f,y = 0.f,z = 0.f,w = 0.f)
        //WriteFile
        let writeFile() = do
            let dlg = new System.Windows.Forms.SaveFileDialog()
            dlg.Filter <- "txt files (*.txt)|*.txt"
            dlg.FilterIndex <- 0
            if dlg.ShowDialog() = DialogResult.OK then do
                if File.Exists(dlg.FileName) then File.Delete(dlg.FileName)
                else File.Create(dlg.FileName) |> ignore
                //camera
                File.AppendAllText(dlg.FileName,camPlot.Count().ToString()+"\n")
                for i = 0 to camPlot.Count()-1 do 
                    let p = GetCurrentCameraPos(i)
                    let t = GetCurrentCameraTar(i)
                    let u = GetCurrentCameraUp(i)
                    let ti = GetCurrentCameraTime(i)
                    File.AppendAllText(dlg.FileName,
                        p.x.ToString() + "," + p.y.ToString() + "," + p.z.ToString() + "," +
                        t.x.ToString() + "," + t.y.ToString() + "," + t.z.ToString() + "," +
                        u.x.ToString() + "," + u.y.ToString() + "," + u.z.ToString() + "," +
                        ti.ToString() + "\n"
                    )
                //object
                File.AppendAllText(dlg.FileName,(objPlot.Count()/2).ToString()+"\n")
                for i = 0 to objPlot.Count()/2 - 1 do 
                    for j = 0 to 1 do
                        let isBegin = System.Convert.ToInt32( j < 1 )
                        let p = GetPlotPos(i,isBegin)
                        let r = GetPlotRot(i,isBegin)
                        let s = GetPlotScale(i,isBegin)
                        let ti = GetPlotTime(i,isBegin)
                        File.AppendAllText(dlg.FileName,
                            p.x.ToString() + "," + p.y.ToString() + "," + p.z.ToString() + "," +
                            r.x.ToString() + "," + r.y.ToString() + "," + r.z.ToString() + "," +
                            s.x.ToString() + "," + s.y.ToString() + "," + s.z.ToString() + "," +
                            GetPlotShape(i).ToString() + "," +
                            ti.ToString() + "\n"
                        )
        //--WriteFile

        //LoadFile
        let loadFile() = do 
            let dlg = new System.Windows.Forms.OpenFileDialog()
            dlg.Filter <- "txt files (*.txt)|*.txt"
            dlg.FilterIndex <- 0
            if dlg.ShowDialog() = DialogResult.OK then do
                //cleanup
                for i = 0 to camPlot.Count()-1 do 
                    DelPlotCamera(i)
                    camPlot.DelItem(i)
                for i = 0 to objPlot.Select/2 - 1 do 
                    DelPlotObject( objPlot.Select/2 ) 
                    objPlot.DelItem(i+1)
                    objPlot.DelItem(i)
                //create
                let toFloat(x : string)= float32( System.Convert.ToDouble(x) )
                let line = File.ReadAllLines(dlg.FileName)
                let camCnt = System.Convert.ToInt32((line.[0]))
                for i = 0 to camCnt - 1 do
                    let value = (line.[i+1]).Split(',')
                    let p = vec(x=toFloat(value.[0]),y=toFloat(value.[1]),z=toFloat(value.[2]))
                    let t = vec(x=toFloat(value.[3]),y=toFloat(value.[4]),z=toFloat(value.[5]))
                    let u = vec(x=toFloat(value.[6]),y=toFloat(value.[7]),z=toFloat(value.[8]))
                    let ti = System.Convert.ToInt32(value.[9])
                    AddPlotCamera(-1,p,t,u,ti)
                    camPlot.AddItem
                let plotCnt = System.Convert.ToInt32((line.[camCnt+1]))
                for i = 0 to plotCnt*2 - 1 do
                    let value = (line.[i+camCnt+2]).Split(',')
                    let p = vec(x=toFloat(value.[0]),y=toFloat(value.[1]),z=toFloat(value.[2]))
                    let r = vec(x=toFloat(value.[3]),y=toFloat(value.[4]),z=toFloat(value.[5]))
                    let s = vec(x=toFloat(value.[6]),y=toFloat(value.[7]),z=toFloat(value.[8]))
                    let shape = System.Convert.ToInt32(value.[9])
                    let ti = System.Convert.ToInt32(value.[10])
                    if i % 2 = 1 then
                        SetPlotObject(i/2,0,p,r,s,ti)
                    else
                        AddPlotObject(shape,-1,p,r,s,ti)
                    objPlot.AddItem
        //--LoadFile

        //Construct
        do
            camPosUI.setValue(0.f,0.f,0.f)
            camPosUI.Edit(fun _ -> do
                camPos <- camPosUI.xyz
                SetCurrentCamera( camPos,camPos+camTar,camUp )
            )
            camTarUI.setValue(0.f,0.f,0.f)
            camTarUI.Enable(false)
            for ui in [objPosUI;objRotUI;objScaleUI] do
                if ui = objScaleUI then ui.setValue(1.f,1.f,1.f) else ui.setValue(0.f,0.f,0.f)
                ui.Edit(fun _ -> SetCurrentObject( 0,objPosUI.xyz,objRotUI.xyz,objScaleUI.xyz ) )
            //Controls
            renderTarget.Click.Add(fun _ -> renderTarget.Focus() |> ignore )
            renderTarget.KeyDown.Add(fun e -> do
                match e.KeyCode with
                | Keys.Left ->  if e.Control then camRot <- camRot + vec(z = 0.1f)
                | Keys.Right -> if e.Control then camRot <- camRot + vec(z = -0.1f)
                | Keys.Up -> if e.Control then camPos <- camPos + camUp*0.11f
                | Keys.Down -> if e.Control then camPos <- camPos - camUp*0.11f
                | Keys.A -> if e.Control then camRot <- camRot + vec(y = 0.1f)
                | Keys.D -> if e.Control then camRot <- camRot + vec(y = -0.1f)
                | Keys.W -> if e.Control then do camRot <- camRot + vec(x = -0.1f)
                | Keys.S -> if e.Control then do camRot <- camRot + vec(x = 0.1f)
                | _ -> ()

                camTar <- rotY( rotX( rotZ ( vec(z = -1.f),float camRot.z ),float camRot.x ),float camRot.y )
                camUp <- rotY( rotX( rotZ ( vec(y = 1.f),float camRot.z ),float camRot.x ),float camRot.y )

                match e.KeyCode with
                | Keys.A -> if not e.Control then camPos <- camPos + cross(camTar,camUp)*0.11f
                | Keys.D -> if not e.Control then camPos <- camPos - cross(camTar,camUp)*0.11f
                | Keys.W -> if not e.Control then camPos <- camPos + camTar*0.11f
                | Keys.S -> if not e.Control then camPos <- camPos - camTar*0.11f
                | _ -> ()


                camPosUI.setValue(camPos.x,camPos.y,camPos.z)
                camTarUI.setValue(camTar.x,camTar.y,camTar.z)
                SetCurrentCamera( camPos,camPos+camTar,camUp )
            )
            renderTarget.Paint.Add(fun e -> e.Dispose() )
            this.Controls.Add( renderTarget )
            timeScroll.Value <- 0
            timeScroll.LargeChange <- 1
            timeScroll.Minimum <- 0
            timeScroll.Maximum <- timeLength
            timeScroll.Scroll.Add( fun e -> do 
                time <- e.NewValue
                SetPlotMode(1)
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
            //SaveLoad
            for b in [saveBtn;loadBtn;exportBtn] do this.Controls.Add(b)
            //Key Event
            this.KeyPreview <- true
            this.KeyDown.Add( fun e -> do
                if Keys.Escape = e.KeyCode then this.Close()
            )
            //PlotManage
            camPlot.Add( fun _ -> do
                AddPlotCamera(camPlot.Select,camPos,camTar,camUp,time)
                camPlot.AddItem
            )
            camPlot.Del( fun _ -> do
                DelPlotCamera(camPlot.Select)
                camPlot.DelItem(camPlot.Select)
            )
            camPlot.Set( fun _ -> do
                DelPlotCamera(camPlot.Select)
                AddPlotCamera(camPlot.Select-1,camPos,camTar,camUp,time)
            )
            camPlot.SelectedIndexChanged( fun _ -> do
                camPos <- GetCurrentCameraPos(camPlot.Select)
                camTar <- GetCurrentCameraTar(camPlot.Select)
                camUp <- GetCurrentCameraUp(camPlot.Select)
                time <- GetCurrentCameraTime(camPlot.Select)
                camPosUI.setValue(camPos.x,camPos.y,camPos.z)
                camTarUI.setValue(camTar.x,camTar.y,camTar.z)
                SetCurrentCamera( camPos,camPos+camTar,camUp )
                timeScroll.Value <- time;
                timeCaption.Text <- time.ToString()
            )
            objPlot.Add(fun _ -> 
                objPlot.AddItem
                objPlot.AddItem
                AddPlotObject(
                    0, (if objPlot.Select > 0 then objPlot.Select/2 else -1) ,
                    objPosUI.xyz,objRotUI.xyz,objScaleUI.xyz,time) 
                )
            objPlot.Del(fun _ -> do
                DelPlotObject( objPlot.Select/2 ) 
                let index = objPlot.Select - (objPlot.Select%2)
                objPlot.DelItem(index+1)
                objPlot.DelItem(index)
            )
            objPlot.Set(fun _-> do
                let isBegin = (objPlot.Select+1)%2
                SetPlotObject(objPlot.Select/2,isBegin,objPosUI.xyz,objRotUI.xyz,objScaleUI.xyz,time)
            )
            objPlot.SelectedIndexChanged(fun _ -> do
                let pos = GetPlotPos(objPlot.Select/2,(objPlot.Select+1)%2)
                let rot = GetPlotRot(objPlot.Select/2,(objPlot.Select+1)%2)
                let scale = GetPlotScale(objPlot.Select/2,(objPlot.Select+1)%2)
                time <- GetPlotTime(objPlot.Select/2,(objPlot.Select+1)%2)
                objPosUI.setValue(pos.x,pos.y,pos.z)
                objRotUI.setValue(rot.x,rot.y,rot.z)
                objScaleUI.setValue(scale.x,scale.y,scale.z)
                timeScroll.Value <- time;
                timeCaption.Text <- time.ToString()
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

                SetPlotMode( if bias <> 0 then 1 else 0 )

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
            renderTarget.Select()
            saveBtn.Click.Add ( fun _ -> do writeFile() )
            loadBtn.Click.Add ( fun _ -> do loadFile() )
        //--end Construct
        override this.Finalize() = ()
    end

[<System.STAThreadAttribute;EntryPoint>]
let main argv =
    let title = "t  o    o   l"
    System.Console.WriteLine("t  o    o   l") 
    // Create Window
    let MainForm = new MFrame(title,360000)
    let mutable return_code = 0
    try
        System.Windows.Forms.Application.Run(MainForm)
    with | :? System.SystemException -> return_code <- 1
    if ReleaseDxContext() = false then return_code <- 1
    return_code
