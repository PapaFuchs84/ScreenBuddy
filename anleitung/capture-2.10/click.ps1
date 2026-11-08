param([int]$ProcId,[int]$X,[int]$Y,[string]$Out="",[int]$Wait=1500)
Add-Type -MemberDefinition '[DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr v);' -Name D -Namespace N; [void][N.D]::SetProcessDpiAwarenessContext([IntPtr](-4))
Add-Type @"
using System;using System.Runtime.InteropServices;
public class C{[DllImport("user32.dll")]public static extern bool SetWindowPos(IntPtr h,IntPtr a,int x,int y,int cx,int cy,uint f);
[DllImport("user32.dll")]public static extern bool GetWindowRect(IntPtr h,out R r);
[DllImport("user32.dll")]public static extern bool SetCursorPos(int x,int y);
[DllImport("user32.dll")]public static extern IntPtr WindowFromPoint(P p);
[DllImport("user32.dll")]public static extern IntPtr GetAncestor(IntPtr h,uint f);
[DllImport("user32.dll")]public static extern void mouse_event(uint f,uint x,uint y,uint d,UIntPtr e);
public struct P{public int X,Y;} public struct R{public int L,T,Rt,B;}}
"@
$h=(Get-Process -Id $ProcId).MainWindowHandle
[void][C]::SetWindowPos($h,[IntPtr](-1),0,0,0,0,0x13)   # TOPMOST, no move/size/activate
Start-Sleep -Milliseconds 400
$r=New-Object C+R;[void][C]::GetWindowRect($h,[ref]$r)
$p=New-Object C+P;$p.X=$r.L+$X;$p.Y=$r.T+$Y
$p0=New-Object C+P;$p0.X=$r.L+700;$p0.Y=$r.T+16;[void][C]::SetCursorPos($p0.X,$p0.Y);Start-Sleep -Milliseconds 150;[C]::mouse_event(2,0,0,0,[UIntPtr]::Zero);[C]::mouse_event(4,0,0,0,[UIntPtr]::Zero);Start-Sleep -Milliseconds 400
$top=[C]::GetAncestor([C]::WindowFromPoint($p),2)
if($top -ne $h){ throw "Window not on top at click point ($top vs $h) - no click sent" }
[void][C]::SetCursorPos($p.X-15,$p.Y-6);Start-Sleep -Milliseconds 200;[void][C]::SetCursorPos($p.X-4,$p.Y-1);Start-Sleep -Milliseconds 150;[void][C]::SetCursorPos($p.X,$p.Y);Start-Sleep -Milliseconds 300
[C]::mouse_event(2,0,0,0,[UIntPtr]::Zero);[C]::mouse_event(4,0,0,0,[UIntPtr]::Zero)
Start-Sleep -Milliseconds $Wait
if($Out){ & pwsh -NoProfile -File "$PSScriptRoot\cap.ps1" -ProcId $ProcId -Out $Out }



