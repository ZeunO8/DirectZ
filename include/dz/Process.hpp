#pragma once
#include <string>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <iostream>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#endif

namespace dz
{
    struct Process
    {
        struct Stream
        {
#if defined(_WIN32)
            HANDLE h;
#else
            int fd;
#endif
            ~Stream()
            {
#if defined(_WIN32)
                if (h)
                    CloseHandle(h);
#else
                if (fd >= 0)
                    close(fd);
#endif
            }
        };
        struct ReadStream : Stream
        {
            ReadStream &operator>>(std::string &output)
            {
                char buffer[4096];
#if defined(_WIN32)
                DWORD read;
                if (ReadFile(h, buffer, sizeof(buffer) - 1, &read, NULL) && read > 0)
                {
                    buffer[read] = '\0';
                    output = buffer;
                }
#else
                ssize_t n = ::read(fd, buffer, sizeof(buffer) - 1);
                if (n > 0)
                {
                    buffer[n] = '\0';
                    output = buffer;
                }
#endif
                return *this;
            }
        };
        struct WriteStream : Stream
        {
            WriteStream &operator<<(const std::string &input)
            {
#if defined(_WIN32)
                DWORD written;
                WriteFile(h, input.c_str(), (DWORD)input.size(), &written, NULL);
#else
                ::write(fd, input.c_str(), input.size());
#endif
                return *this;
            }
        };
#if defined(_WIN32)
        PROCESS_INFORMATION pi;
#else
        pid_t pid;
#endif
        WriteStream in;
        ReadStream out;
        ReadStream err;

        Process(const std::string &command)
        {
#if defined(_WIN32)
            SECURITY_ATTRIBUTES saAttr;
            saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
            saAttr.bInheritHandle = TRUE;
            saAttr.lpSecurityDescriptor = NULL;

            HANDLE hStdinRead, hStdoutWrite, hStderrWrite;

            if (!CreatePipe(&out.h, &hStdoutWrite, &saAttr, 0))
                throw std::runtime_error("Stdout pipe creation failed");
            if (!SetHandleInformation(out.h, HANDLE_FLAG_INHERIT, 0))
                throw std::runtime_error("Stdout SetHandleInformation failed");

            if (!CreatePipe(&err.h, &hStderrWrite, &saAttr, 0))
                throw std::runtime_error("Stderr pipe creation failed");
            if (!SetHandleInformation(err.h, HANDLE_FLAG_INHERIT, 0))
                throw std::runtime_error("Stderr SetHandleInformation failed");

            if (!CreatePipe(&hStdinRead, &in.h, &saAttr, 0))
                throw std::runtime_error("Stdin pipe creation failed");
            if (!SetHandleInformation(in.h, HANDLE_FLAG_INHERIT, 0))
                throw std::runtime_error("Stdin SetHandleInformation failed");

            STARTUPINFOA si;
            ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
            ZeroMemory(&si, sizeof(STARTUPINFOA));
            si.cb = sizeof(STARTUPINFOA);
            si.hStdError = hStderrWrite;
            si.hStdOutput = hStdoutWrite;
            si.hStdInput = hStdinRead;
            si.dwFlags |= STARTF_USESTDHANDLES;

            if (!CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
                throw std::runtime_error("Process creation failed");

            CloseHandle(hStdoutWrite);
            CloseHandle(hStderrWrite);
            CloseHandle(hStdinRead);
#else
            int inpipe[2];
            int outpipe[2];
            int errpipe[2];
            if (pipe(inpipe) == -1 || pipe(outpipe) == -1 || pipe(errpipe) == -1)
                throw std::runtime_error("Pipe creation failed");

            pid = fork();
            if (pid == -1)
                throw std::runtime_error("Fork failed");

            if (pid == 0)
            {
                dup2(inpipe[0], STDIN_FILENO);
                dup2(outpipe[1], STDOUT_FILENO);
                dup2(errpipe[1], STDERR_FILENO);

                close(inpipe[1]);
                close(outpipe[0]);
                close(errpipe[0]);

                execl("/bin/sh", "sh", "-c", command.c_str(), (char *)NULL);
                _exit(1);
            }
            else
            {
                close(inpipe[0]);
                close(outpipe[1]);
                close(errpipe[1]);

                in.fd = inpipe[1];
                out.fd = outpipe[0];
                err.fd = errpipe[0];
            }
#endif
        }

        ~Process()
        {
#if defined(_WIN32)
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
#else
            int status;
            waitpid(pid, &status, 0);
#endif
        }

        unsigned long long GetPID() const
        {
#if defined(_WIN32)
            return static_cast<unsigned long long>(pi.dwProcessId);
#else
            return static_cast<unsigned long long>(pid);
#endif
        }

        inline static unsigned long long GetCurrentPID()
        {
#if defined(_WIN32)
            return static_cast<unsigned long long>(::GetCurrentProcessId());
#elif defined(__unix__) || defined(__APPLE__)
            return static_cast<unsigned long long>(::getpid());
#else
#error "Unsupported platform"
#endif
        }
    };
}