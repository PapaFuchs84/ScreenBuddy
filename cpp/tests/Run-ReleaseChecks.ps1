[CmdletBinding()]
param(
    [string]$BuildDirectory = "$PSScriptRoot/../build/screenbuddy-release",
    [string]$OutputDirectory = "$PSScriptRoot/../build/release-checks-private-$(Get-Date -Format yyyyMMdd-HHmmss)",
    [switch]$LayoutOnly
)
$ErrorActionPreference='Stop'
# Use real HWNDs and the original test executables without sharing input/focus
# with the user's desktop. This does not switch desktops, install trust, or
# change machine policy. Only this invocation's temporary desktop is closed.
$build=(Resolve-Path -LiteralPath $BuildDirectory).Path
if (Test-Path -LiteralPath $OutputDirectory) { throw 'Select a fresh test output directory.' }
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
$output=(Resolve-Path -LiteralPath $OutputDirectory).Path
$working=(Resolve-Path -LiteralPath "$PSScriptRoot/..").Path
if ($LayoutOnly) {
    $program=(Resolve-Path -LiteralPath (Join-Path $build 'fl-layout-ui-tests.exe')).Path
    $arguments=@($program)
} else {
    $program=(Get-Command python -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
    $runner=(Resolve-Path -LiteralPath "$PSScriptRoot/run_release_checks.py").Path
    $arguments=@($program,$runner,'--build',$build,'--output',$output)
}
# Windows filesystem paths cannot contain quotes. Escape trailing backslashes
# for CreateProcess/CRT argument parsing; no shell interprets this command line.
$quoted=foreach($argument in $arguments) {
    if ($argument.Contains('"')) { throw 'Unexpected quote in test path.' }
    '"'+[regex]::Replace($argument,'(\\+)$','$1$1')+'"'
}
$command=$quoted -join ' '
$log=Join-Path $output 'console.txt'
Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;
using System.ComponentModel;
public static class PrivateDesktopTest {
 [StructLayout(LayoutKind.Sequential)] struct SecurityAttributes { public int length; public IntPtr descriptor; [MarshalAs(UnmanagedType.Bool)] public bool inherit; }
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] struct StartupInfo {
  public int cb; public string reserved; public string desktop; public string title;
  public uint x,y,xSize,ySize,xCount,yCount,fill,flags; public ushort show,reservedSize;
  public IntPtr reservedBytes,input,output,error;
 }
 [StructLayout(LayoutKind.Sequential)] struct ProcessInfo { public IntPtr process,thread; public uint id,threadId; }
 [DllImport("user32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr CreateDesktop(string name,string device,IntPtr devmode,uint flags,uint access,IntPtr security);
 [DllImport("user32.dll",SetLastError=true)] static extern bool CloseDesktop(IntPtr desktop);
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr CreateFile(string path,uint access,uint share,ref SecurityAttributes security,uint creation,uint flags,IntPtr template);
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern bool CreateProcess(string program,StringBuilder command,IntPtr processSecurity,IntPtr threadSecurity,bool inherit,uint flags,IntPtr environment,string directory,ref StartupInfo startup,out ProcessInfo process);
 [DllImport("kernel32.dll",SetLastError=true)] static extern uint WaitForSingleObject(IntPtr handle,uint milliseconds);
 [DllImport("kernel32.dll",SetLastError=true)] static extern bool GetExitCodeProcess(IntPtr process,out uint code);
 [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr handle);
 public static int Run(string program,string command,string directory,string log) {
  string name="ScreenBuddyTests-"+Guid.NewGuid().ToString("N");
  IntPtr desktop=CreateDesktop(name,null,IntPtr.Zero,0,0x10000000,IntPtr.Zero);
  if(desktop==IntPtr.Zero) throw new Win32Exception(Marshal.GetLastWin32Error(),"Create private test desktop");
  IntPtr output=IntPtr.Zero,input=IntPtr.Zero; ProcessInfo process=new ProcessInfo();
  try {
   SecurityAttributes security=new SecurityAttributes{length=Marshal.SizeOf(typeof(SecurityAttributes)),inherit=true};
   output=CreateFile(log,0x40000000,3,ref security,2,0x80,IntPtr.Zero);
   if(output==new IntPtr(-1)) throw new Win32Exception(Marshal.GetLastWin32Error(),"Create test log");
   input=CreateFile("NUL",0x80000000,3,ref security,3,0x80,IntPtr.Zero);
   if(input==new IntPtr(-1)) throw new Win32Exception(Marshal.GetLastWin32Error(),"Open test input");
   StartupInfo startup=new StartupInfo{cb=Marshal.SizeOf(typeof(StartupInfo)),desktop="winsta0\\"+name,flags=0x100,input=input,output=output,error=output};
   if(!CreateProcess(program,new StringBuilder(command),IntPtr.Zero,IntPtr.Zero,true,0x08000000,IntPtr.Zero,directory,ref startup,out process)) throw new Win32Exception(Marshal.GetLastWin32Error(),"Start private-desktop test");
   Console.WriteLine("Private desktop test started; desktop is never switched.");
   if(WaitForSingleObject(process.process,0xffffffff)!=0) throw new Win32Exception(Marshal.GetLastWin32Error(),"Wait for private-desktop test");
   uint code; if(!GetExitCodeProcess(process.process,out code)) throw new Win32Exception(Marshal.GetLastWin32Error());
   return unchecked((int)code);
  } finally {
   if(process.thread!=IntPtr.Zero)CloseHandle(process.thread);
   if(process.process!=IntPtr.Zero)CloseHandle(process.process);
   if(input!=IntPtr.Zero && input!=new IntPtr(-1))CloseHandle(input);
   if(output!=IntPtr.Zero && output!=new IntPtr(-1))CloseHandle(output);
   if(!CloseDesktop(desktop))Console.Error.WriteLine("Private desktop handle did not close: "+Marshal.GetLastWin32Error());
  }
 }
}
'@
$code=[PrivateDesktopTest]::Run($program,$command,$working,$log)
Get-Content -LiteralPath $log -Tail 30
Write-Output "Private desktop test exit: $code"
exit $code