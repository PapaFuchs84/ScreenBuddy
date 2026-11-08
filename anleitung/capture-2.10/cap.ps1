param([int]$ProcId,[string]$Out)
Add-Type -MemberDefinition '[DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr v);' -Name D -Namespace N; [void][N.D]::SetProcessDpiAwarenessContext([IntPtr](-4))
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;using System.Runtime.InteropServices;
public class P{[DllImport("user32.dll")]public static extern bool PrintWindow(IntPtr h,IntPtr dc,uint f);
[DllImport("user32.dll")]public static extern bool GetWindowRect(IntPtr h,out R r);
public struct R{public int L,T,Rt,B;}}
"@
$h=(Get-Process -Id $ProcId).MainWindowHandle
$r=New-Object P+R;[void][P]::GetWindowRect($h,[ref]$r)
$w=$r.Rt-$r.L;$ht=$r.B-$r.T
$bmp=New-Object Drawing.Bitmap $w,$ht
$g=[Drawing.Graphics]::FromImage($bmp);$dc=$g.GetHdc()
[void][P]::PrintWindow($h,$dc,2);$g.ReleaseHdc($dc);$g.Dispose()
$bmp.Save($Out,[Drawing.Imaging.ImageFormat]::Png);"$w x $ht"

