// © 2026 FuchsLabs – rights for the ETW FPS measurement (local group "Performance Log Users", S-1-5-32-559).
#pragma once
#include <string>
namespace fl::perflog {
enum class Access {
    Granted,         // this process may start ETW sessions (group member in the token, or elevated)
    NeedsRelogin,    // user is already a group member, but the current sign-in token does not contain it yet
    NotMember,       // the one-time grant is still needed
    Blocked,         // group is set and in the token, but Windows still refuses ETW sessions (needs administrator)
};
Access access();
void set_test_override(Access value);  // tests only: makes the status independent of the machine
// Starts this EXE elevated (one UAC prompt, ShellExecute "runas") with "--grant-perflog <user SID>" and waits for it.
// Returns true when the helper ended successfully. Never keeps administrator rights.
bool request_grant();
// Entry point of the elevated helper: adds the SID to the group. Returns a process exit code (0 = success).
int run_helper(const std::wstring& user_sid);
}
