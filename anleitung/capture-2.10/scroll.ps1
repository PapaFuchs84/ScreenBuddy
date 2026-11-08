param([int]$ProcId,[int]$X,[int]$Y,[int]$Delta,[string]$Out="")
Add-Type -MemberDefinition '[DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr v);' -Name D -Namespace N; [void][N.D]::SetProcessDpiAwarenessContext([IntPtr](-4))
Add-Type @"
using System;using System.Runtime.InteropServices;
public class S{[DllImport("user32.dll")]public static extern bool GetWindowRect(IntPtr h,out R r);
[DllImport("user32.dll")]public static extern bool SetCursorPos(int x,int y);
[DllImport("user32.dll")]public static extern IntPtr WindowFromPoint(P p);
[DllImport("user32.dll")]public static extern IntPtr GetAncestor(IntPtr h,uint f);
[DllImport("user32.dll")]public static extern void mouse_event(uint f,uint x,uint y,int d,UIntPtr e);
public struct P{public int X,Y;} public struct R{public int L,T,Rt,B;}}
"@
$h=(Get-Process -Id $ProcId).MainWindowHandle
$r=New-Object S+R;[void][S]::GetWindowRect($h,[ref]$r)
$p=New-Object S+P;$p.X=$r.L+$X;$p.Y=$r.T+$Y
if([S]::GetAncestor([S]::WindowFromPoint($p),2) -ne $h){ throw "window not on top" }
[void][S]::SetCursorPos($p.X,$p.Y);Start-Sleep -Milliseconds 200
[S]::mouse_event(0x0800,0,0,$Delta,[UIntPtr]::Zero);Start-Sleep -Milliseconds 1200
if($Out){ & pwsh -NoProfile -File "$PSScriptRoot\cap.ps1" -ProcId $ProcId -Out $Out }
