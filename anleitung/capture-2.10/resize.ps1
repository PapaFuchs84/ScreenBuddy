param([int]$ProcId,[int]$Wd=1520,[int]$Ht=940)
Add-Type -MemberDefinition '[DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr v);' -Name D -Namespace N; [void][N.D]::SetProcessDpiAwarenessContext([IntPtr](-4))
Add-Type -MemberDefinition '[DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h,IntPtr a,int x,int y,int cx,int cy,uint f);' -Name R -Namespace N
$h=(Get-Process -Id $ProcId).MainWindowHandle
[void][N.R]::SetWindowPos($h,[IntPtr]::Zero,0,0,$Wd,$Ht,0x16)  # NOMOVE|NOZORDER|NOACTIVATE

