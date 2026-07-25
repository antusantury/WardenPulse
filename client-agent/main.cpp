#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <UIAutomation.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <gdiplus.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")

// Placeholder for client agent implementation
// This is a basic Windows service structure for the DLP agent

class ActivityCapture {
private:
    HHOOK keyboardHook;
    IUIAutomation* uiAutomation;
    SOCKET serverSocket;
    std::mutex mtx;
    bool running;
    std::thread monitorThread;
    static ActivityCapture* instance;
    ULONG_PTR gdiplusToken;

    // Server details
    const char* serverIP = "127.0.0.1"; // Change to actual server IP
    const int serverPort = 8080;

    // Keywords
    std::vector<std::string> keywords = {"секретно", "пароль"};

    int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
    std::string ws2s(const std::wstring& wstr);

    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    LRESULT KeyboardProcImpl(int nCode, WPARAM wParam, LPARAM lParam);

    void MonitorWindows();
    bool IsMessengerWindow(HWND hwnd);
    std::string GetWindowText(HWND hwnd);
    bool ContainsKeywords(const std::string& text);
    void CaptureScreenshot(HWND hwnd);
    void SendScreenshot(const std::vector<char>& data);
    void ConnectToServer();

public:
    ActivityCapture();
    ~ActivityCapture();
    void Start();
    void Stop();
};

ActivityCapture* ActivityCapture::instance = nullptr;

int ActivityCapture::GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)malloc(size);
    if (!pImageCodecInfo) return -1;
    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}

std::string ActivityCapture::ws2s(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

ActivityCapture::ActivityCapture() : keyboardHook(NULL), uiAutomation(NULL), serverSocket(INVALID_SOCKET), running(false), gdiplusToken(0) {
    instance = this;
    CoInitialize(NULL);
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation), (void**)&uiAutomation);
    ConnectToServer();
}

ActivityCapture::~ActivityCapture() {
    Stop();
    if (serverSocket != INVALID_SOCKET) closesocket(serverSocket);
    if (uiAutomation) uiAutomation->Release();
    CoUninitialize();
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

void ActivityCapture::Start() {
    if (running) return;
    running = true;
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    monitorThread = std::thread(&ActivityCapture::MonitorWindows, this);
}

void ActivityCapture::Stop() {
    if (!running) return;
    running = false;
    if (monitorThread.joinable()) monitorThread.join();
    if (keyboardHook) UnhookWindowsHookEx(keyboardHook);
}

LRESULT CALLBACK ActivityCapture::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (instance) {
        return instance->KeyboardProcImpl(nCode, wParam, lParam);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT ActivityCapture::KeyboardProcImpl(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        // Example: suppress certain keys, but for now pass
        // KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        // if (p->vkCode == VK_RETURN) return 1; // suppress enter
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

void ActivityCapture::MonitorWindows() {
    while (running) {
        HWND hwnd = GetForegroundWindow();
        if (hwnd && IsMessengerWindow(hwnd)) {
            std::string text = GetWindowText(hwnd);
            if (ContainsKeywords(text)) {
                CaptureScreenshot(hwnd);
            }
        }
        Sleep(1000);
    }
}

bool ActivityCapture::IsMessengerWindow(HWND hwnd) {
    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    std::string t = title;
    return t.find("Slack") != std::string::npos || t.find("Telegram") != std::string::npos;
}

std::string ActivityCapture::GetWindowText(HWND hwnd) {
    IUIAutomationElement* root = NULL;
    uiAutomation->ElementFromHandle(hwnd, &root);
    if (!root) return "";

    IUIAutomationCondition* condition = NULL;
    VARIANT vt;
    vt.vt = VT_I4;
    vt.lVal = UIA_TextControlTypeId;
    uiAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, vt, &condition);

    IUIAutomationElementArray* elements = NULL;
    root->FindAll(TreeScope_Descendants, condition, &elements);

    int count = 0;
    if (elements) elements->get_Length(&count);

    std::string text;
    for (int i = 0; i < count; ++i) {
        IUIAutomationElement* el = NULL;
        elements->GetElement(i, &el);
        if (el) {
            BSTR bstr = NULL;
            el->get_CurrentName(&bstr);
            if (bstr) {
                text += ws2s(std::wstring(bstr, SysStringLen(bstr))) + " ";
                SysFreeString(bstr);
            }
            el->Release();
        }
    }

    if (elements) elements->Release();
    if (condition) condition->Release();
    if (root) root->Release();

    return text;
}

bool ActivityCapture::ContainsKeywords(const std::string& text) {
    for (const auto& kw : keywords) {
        if (text.find(kw) != std::string::npos) return true;
    }
    return false;
}

void ActivityCapture::CaptureScreenshot(HWND hwnd) {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    HBITMAP hbmScreen = CreateCompatibleBitmap(hdcScreen, width, height);
    HBITMAP oldBmp = (HBITMAP)SelectObject(hdcMem, hbmScreen);
    PrintWindow(hwnd, hdcMem, 0);

    Gdiplus::Bitmap bitmap(hbmScreen, NULL);
    CLSID clsid;
    if (GetEncoderClsid(L"image/png", &clsid) != -1) {
        IStream* stream = NULL;
        CreateStreamOnHGlobal(NULL, TRUE, &stream);
        if (stream) {
            bitmap.Save(stream, &clsid);
            HGLOBAL hGlobal = NULL;
            GetHGlobalFromStream(stream, &hGlobal);
            if (hGlobal) {
                SIZE_T size = GlobalSize(hGlobal);
                std::vector<char> data(size);
                LPVOID pData = GlobalLock(hGlobal);
                if (pData) {
                    memcpy(data.data(), pData, size);
                    SendScreenshot(data);
                }
                GlobalUnlock(hGlobal);
            }
            stream->Release();
        }
    }

    SelectObject(hdcMem, oldBmp);
    DeleteObject(hbmScreen);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void ActivityCapture::SendScreenshot(const std::vector<char>& data) {
    if (serverSocket != INVALID_SOCKET) {
        send(serverSocket, data.data(), data.size(), 0);
    }
}

void ActivityCapture::ConnectToServer() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIP, &server.sin_addr);
    connect(serverSocket, (sockaddr*)&server, sizeof(server));
}

SERVICE_STATUS g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

#define SERVICE_NAME  "SentinelDLPAgent"

int main(int argc, char* argv[]) {
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE) {
        return GetLastError();
    }

    return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv) {
    DWORD Status = E_FAIL;

    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

    if (g_StatusHandle == NULL) {
        goto EXIT;
    }

    // Tell the service controller we are starting
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
        OutputDebugString(TEXT("SentinelDLPAgent: ServiceMain: SetServiceStatus returned error"));
    }

    // Create stop event to wait on later
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL) {
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
            OutputDebugString(TEXT("SentinelDLPAgent: ServiceMain: SetServiceStatus returned error"));
        }
        goto EXIT;
    }

    // Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
        OutputDebugString(TEXT("SentinelDLPAgent: ServiceMain: SetServiceStatus returned error"));
    }

    // Start the thread that will perform the main task of the service
    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    // Wait until our worker thread exits effectively signaling that the service needs to stop
    WaitForSingleObject(hThread, INFINITE);

    // Perform any cleanup tasks
    CloseHandle(g_ServiceStopEvent);

    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 1;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
        OutputDebugString(TEXT("SentinelDLPAgent: ServiceMain: SetServiceStatus returned error"));
    }

EXIT:
    return;
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode) {
    switch (CtrlCode) {
    case SERVICE_CONTROL_STOP:
        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
            OutputDebugString(TEXT("SentinelDLPAgent: ServiceCtrlHandler: SetServiceStatus returned error"));
        }

        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);

        break;

    default:
        break;
    }
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam) {
    ActivityCapture ac;
    ac.Start();

    // Periodically check if the service has been requested to stop
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0) {
        Sleep(1000);
    }

    ac.Stop();
    return ERROR_SUCCESS;
}