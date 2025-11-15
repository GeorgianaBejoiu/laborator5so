#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

void ErrorExit(const char* msg) {
    std::cerr << msg << " Error: " << GetLastError() << "\n";
    exit(1);
}

int main() {
    const int numProcesses = 10;
    const int intervalSize = 1000;

    for (int i = 0; i < numProcesses; ++i) {
        HANDLE hChildStd_IN_Rd = NULL;
        HANDLE hChildStd_IN_Wr = NULL;
        HANDLE hChildStd_OUT_Rd = NULL;
        HANDLE hChildStd_OUT_Wr = NULL;

        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        // Create pipe for child's STDOUT
        if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
            ErrorExit("Stdout pipe creation failed");

        // Ensure the read handle to the pipe for STDOUT is not inherited.
        if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
            ErrorExit("Stdout SetHandleInformation failed");

        // Create pipe for child's STDIN
        if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0))
            ErrorExit("Stdin pipe creation failed");

        // Ensure the write handle to the pipe for STDIN is not inherited.
        if (!SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
            ErrorExit("Stdin SetHandleInformation failed");

        PROCESS_INFORMATION piProcInfo;
        STARTUPINFO siStartInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        siStartInfo.hStdOutput = hChildStd_OUT_Wr;
        siStartInfo.hStdInput = hChildStd_IN_Rd;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        // Command line: prime_worker.exe
        std::string cmdLine = "windows_primes.exe";

        // Create the child process.
        BOOL success = CreateProcess(NULL,
            (LPSTR)cmdLine.c_str(),
            NULL,
            NULL,
            TRUE,
            0,
            NULL,
            NULL,
            &siStartInfo,
            &piProcInfo);

        if (!success) {
            ErrorExit("CreateProcess failed");
        }

        // Close handles that are not needed by the parent.
        CloseHandle(hChildStd_OUT_Wr);
        CloseHandle(hChildStd_IN_Rd);

        // Write interval to child's STDIN pipe
        int start = i * intervalSize + 1;
        int end = (i + 1) * intervalSize;
        std::string interval = std::to_string(start) + " " + std::to_string(end) + "\n";
        DWORD written;
        if (!WriteFile(hChildStd_IN_Wr, interval.c_str(), (DWORD)interval.size(), &written, NULL))
            ErrorExit("WriteFile to child stdin failed");
        CloseHandle(hChildStd_IN_Wr);

        // Read child's output
        CHAR buffer[4096];
        DWORD read;
        std::string primesStr;
        while (ReadFile(hChildStd_OUT_Rd, buffer, sizeof(buffer) - 1, &read, NULL) && read > 0) {
            buffer[read] = '\0';
            primesStr += buffer;
        }

        std::cout << "Primes in interval [" << start << "," << end << "]: " << primesStr << "\n";

        // Clean up handles and wait for process to exit
        CloseHandle(hChildStd_OUT_Rd);
        WaitForSingleObject(piProcInfo.hProcess, INFINITE);
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
    }

    return 0;
}
