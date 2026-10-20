#include "internal.hpp"
#include <fstream>
#include <thread>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace soundgrind::internal {
#ifdef _WIN32
static std::wstring widen(const std::string& s) {
  if(s.empty())return {}; int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,nullptr,0); std::wstring w(n,L'\0'); MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,w.data(),n); w.resize(n-1); return w;
}
static std::wstring quote(const std::wstring& s) {
  std::wstring o=L"\""; unsigned sl=0;
  for(auto ch:s) { if(ch==L'"') { o.append(sl*2+1,L'\\'); o+=ch; sl=0; } else if(ch==L'\\') { ++sl; o+=ch; } else { sl=0; o+=ch; } }
  o.append(sl,L'\\'); o+=L'"'; return o;
}
ChildResult run_child(const std::filesystem::path& exe,const std::vector<std::string>& args,
                      std::uint32_t timeout,const std::filesystem::path& stderr_path) {
  std::filesystem::create_directories(stderr_path.parent_path());
  HANDLE err=CreateFileW(stderr_path.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(err==INVALID_HANDLE_VALUE) throw std::runtime_error("cannot create worker stderr file");
  SetHandleInformation(err,HANDLE_FLAG_INHERIT,HANDLE_FLAG_INHERIT);
  STARTUPINFOW si{}; si.cb=sizeof(si); si.dwFlags=STARTF_USESTDHANDLES; si.hStdError=err; si.hStdOutput=err; si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
  PROCESS_INFORMATION pi{}; std::wstring cmd=quote(exe.wstring()); for(const auto& a:args)cmd+=L" "+quote(widen(a));
  std::vector<wchar_t> writable(cmd.begin(),cmd.end()); writable.push_back(0);
  const BOOL ok=CreateProcessW(exe.c_str(),writable.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi);
  CloseHandle(err); if(!ok)throw std::runtime_error("CreateProcessW failed for worker");
  ChildResult r; const DWORD wait=WaitForSingleObject(pi.hProcess,timeout);
  if(wait==WAIT_TIMEOUT) { r.timed_out=true; TerminateProcess(pi.hProcess,124); WaitForSingleObject(pi.hProcess,INFINITE); }
  DWORD code=1; GetExitCodeProcess(pi.hProcess,&code); r.exit_code=static_cast<int>(code); CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
  std::ifstream in(stderr_path); r.stderr_text.assign(std::istreambuf_iterator<char>(in),{}); return r;
}
#else
ChildResult run_child(const std::filesystem::path& exe,const std::vector<std::string>& args,
                      std::uint32_t timeout,const std::filesystem::path& stderr_path) {
  std::filesystem::create_directories(stderr_path.parent_path());
  pid_t pid=fork(); if(pid<0)throw std::runtime_error("fork failed");
  if(pid==0) {
    int fd=open(stderr_path.c_str(),O_CREAT|O_WRONLY|O_TRUNC,0600); if(fd>=0){dup2(fd,STDERR_FILENO);dup2(fd,STDOUT_FILENO);close(fd);}
    std::vector<std::string> all; all.push_back(exe.string()); all.insert(all.end(),args.begin(),args.end());
    std::vector<char*> av; for(auto& s:all)av.push_back(s.data()); av.push_back(nullptr);
    execv(exe.c_str(),av.data()); _exit(127);
  }
  ChildResult r; const auto end=std::chrono::steady_clock::now()+std::chrono::milliseconds(timeout); int status=0;
  for(;;) { const auto w=waitpid(pid,&status,WNOHANG); if(w==pid)break; if(w<0&&errno!=EINTR){r.exit_code=127;break;} if(std::chrono::steady_clock::now()>=end){r.timed_out=true;kill(pid,SIGKILL);while(waitpid(pid,&status,0)<0&&errno==EINTR){};break;} std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
  if(!r.timed_out)r.exit_code=WIFEXITED(status)?WEXITSTATUS(status):128+(WIFSIGNALED(status)?WTERMSIG(status):0);
  std::ifstream in(stderr_path); r.stderr_text.assign(std::istreambuf_iterator<char>(in),{}); return r;
}
#endif
}
