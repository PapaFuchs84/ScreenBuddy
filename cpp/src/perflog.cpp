// © 2026 FuchsLabs
#include "perflog.h"
#include "util.h"
#include <windows.h>
#include <wmistr.h>
#include <evntrace.h>
#include <lm.h>
#include <sddl.h>
#include <shellapi.h>
#include <memory>
#include <optional>
#include <vector>
namespace fl::perflog {
namespace {
struct LocalFree_ {void operator()(void* p)const{if(p)LocalFree(p);}};
std::unique_ptr<void,LocalFree_> well_known_group(){
    PSID sid=nullptr;if(!ConvertStringSidToSidW(L"S-1-5-32-559",&sid))return {};return std::unique_ptr<void,LocalFree_>(sid);
}
std::wstring group_name(){
    auto sid=well_known_group();if(!sid)return L"Performance Log Users";
    wchar_t name[256],domain[256];DWORD n=256,d=256;SID_NAME_USE use;
    return LookupAccountSidW(nullptr,sid.get(),name,&n,domain,&d,&use)?std::wstring(name):std::wstring(L"Performance Log Users");
}
std::wstring current_user_sid(){
    HANDLE token=nullptr;if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))return {};
    DWORD size=0;GetTokenInformation(token,TokenUser,nullptr,0,&size);std::vector<unsigned char> buffer(size?size:1);std::wstring out;
    if(GetTokenInformation(token,TokenUser,buffer.data(),size,&size)){wchar_t* text=nullptr;if(ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid,&text)){out=text;LocalFree(text);}}
    CloseHandle(token);return out;
}
bool elevated(){
    HANDLE token=nullptr;if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))return false;
    TOKEN_ELEVATION e{};DWORD size=0;bool result=GetTokenInformation(token,TokenElevation,&e,sizeof e,&size) && e.TokenIsElevated;CloseHandle(token);return result;
}
bool is_member_in_directory(const std::wstring& user_sid){
    // Directory membership (independent of the token): enumerate the group's members and compare SIDs.
    auto group=group_name();DWORD_PTR resume=0;DWORD read=0,total=0;LOCALGROUP_MEMBERS_INFO_0* info=nullptr;bool found=false;
    PSID wanted=nullptr;if(!ConvertStringSidToSidW(user_sid.c_str(),&wanted))return false;
    auto status=NetLocalGroupGetMembers(nullptr,group.c_str(),0,reinterpret_cast<LPBYTE*>(&info),MAX_PREFERRED_LENGTH,&read,&total,&resume);
    if(status==NERR_Success || status==ERROR_MORE_DATA){for(DWORD i=0;i<read;++i)if(EqualSid(info[i].lgrmi0_sid,wanted)){found=true;break;}}
    if(info)NetApiBufferFree(info);LocalFree(wanted);return found;
}
}
// Real test instead of trusting the token: can this process create a real-time ETW session right now?
bool can_start_trace(){
    std::wstring name=L"ScreenBuddy.Probe."+std::to_wstring(GetCurrentProcessId());
    std::vector<unsigned char> buffer(sizeof(EVENT_TRACE_PROPERTIES)+(name.size()+1)*sizeof(wchar_t),0);
    auto p=reinterpret_cast<EVENT_TRACE_PROPERTIES*>(buffer.data());p->Wnode.BufferSize=ULONG(buffer.size());p->Wnode.Flags=WNODE_FLAG_TRACED_GUID;p->Wnode.ClientContext=1;
    p->LogFileMode=EVENT_TRACE_REAL_TIME_MODE;p->LoggerNameOffset=sizeof(EVENT_TRACE_PROPERTIES);p->BufferSize=64;p->MinimumBuffers=2;p->MaximumBuffers=4;
    TRACEHANDLE session=0;auto error=StartTraceW(&session,name.c_str(),p);
    if(error==ERROR_SUCCESS){ControlTraceW(session,name.c_str(),p,EVENT_TRACE_CONTROL_STOP);return true;}
    return false;
}
std::optional<Access> test_override;
Access access(){
    if(test_override)return *test_override;
    if(can_start_trace())return Access::Granted;
    BOOL member=FALSE;auto sid=well_known_group();
    const bool in_token=elevated() || (sid && CheckTokenMembership(nullptr,sid.get(),&member) && member);
    auto user=current_user_sid();
    const bool in_group=!user.empty() && is_member_in_directory(user);
    if(in_token && in_group)return Access::Blocked;   // group present, yet Windows still refuses sessions (policy/ACL): needs an elevated start
    return in_group?Access::NeedsRelogin:Access::NotMember;
}
bool request_grant(){
    auto user=current_user_sid();if(user.empty())return false;
    std::wstring parameters=L"--grant-perflog "+user;
    auto exe=exe_path().wstring();
    SHELLEXECUTEINFOW info{sizeof info};info.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC;info.lpVerb=L"runas";info.lpFile=exe.c_str();info.lpParameters=parameters.c_str();info.nShow=SW_HIDE;
    if(!ShellExecuteExW(&info) || !info.hProcess){LOGW("perflog","Rechte-Dialog abgebrochen oder nicht möglich (%lu)",GetLastError());return false;}
    WaitForSingleObject(info.hProcess,60000);DWORD code=1;GetExitCodeProcess(info.hProcess,&code);CloseHandle(info.hProcess);
    return code==0;
}
void set_test_override(Access value){test_override=value;}
int run_helper(const std::wstring& user_sid){
    // Only a syntactically valid user SID is accepted (S-1-5-21-... domain/local account or Microsoft/Azure account), never well-known groups.
    PSID sid=nullptr;if(user_sid.rfind(L"S-1-5-21-",0)!=0 && user_sid.rfind(L"S-1-12-1-",0)!=0)return 2;
    if(!ConvertStringSidToSidW(user_sid.c_str(),&sid))return 2;
    LOCALGROUP_MEMBERS_INFO_0 member{sid};auto group=group_name();
    auto status=NetLocalGroupAddMembers(nullptr,group.c_str(),0,reinterpret_cast<LPBYTE>(&member),1);
    LocalFree(sid);
    if(status==NERR_Success || status==ERROR_MEMBER_IN_ALIAS)return 0;
    LOGE("perflog","Gruppe '%s' konnte nicht erweitert werden (%lu)",u8(group).c_str(),status);return 1;
}
}
