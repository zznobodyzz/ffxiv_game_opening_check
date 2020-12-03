//Main.cpp:
//
#include "stdafx.h"
#include <string>
#include <windows.h>
#include <Psapi.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include "resource.h"
#include <commctrl.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

#define MAINWINDOWWIDTH 770
#define MAINWINDOWHEIGHT 450

#define BUTTON_GET_IP_AND_PORT 3301
#define BUTTON_START_CHECK     3302
#define BUTTON_SET_EXE_TYPE_DX11 3303
#define BUTTON_SET_EXE_TYPE_UNDX11 3304
#define BUTTON_LOCAL 3305
#define MAX_LOADSTRING 100

WCHAR szTitle[MAX_LOADSTRING] = _T("开服检测器");                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING] = _T("win32app");            // 主窗口类名

HWND hListProcess;
HWND hStartButton;
HWND hCheckButton;
HWND hLocalButton;
HWND hwnd_server_ip;
HWND hwnd_server_port;
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
HFONT hfont;
HINSTANCE hInst;
HANDLE hThread;
UINT server_ip, server_port;
bool is_initialized = false;
bool is_started = false;
bool is_loaded = false;
wchar_t cProcessName[128] = {0};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX			wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    //对已创建的窗口类进行注册。 使用 RegisterClassEx 函数，并将窗口类结构作为参数传递。  
    if (!RegisterClassEx(&wcex)) {
        MessageBox(NULL, _T("Call to RegisterClassEx failed!"), _T("Win32 Guided Tour"), NULL);
        return 1;
    }

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    hInst = hInstance;

    HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, 
        CW_USEDEFAULT, CW_USEDEFAULT, MAINWINDOWWIDTH, MAINWINDOWHEIGHT, NULL, NULL, hInstance, NULL);
    if (!hWnd) {
        MessageBox(NULL, _T("Call to CreateWindow failed!"), _T("Win32 Guided Tour"), NULL);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);

    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); //翻译消息  
        DispatchMessage(&msg);  //派遣消息  
    }
    return (int)msg.wParam;
}

void AddInfo(const wchar_t *action, const wchar_t *result)
{
    LV_ITEM vitem;
    static INT iItem = 0;
    static INT iDitem = 0;
    static WCHAR szText[256];
    wchar_t action_tmp[256];
    wchar_t result_tmp[256];

    //初始化                
    memset(&vitem, 0, sizeof(LV_ITEM));
    vitem.mask = LVIF_TEXT;
    SYSTEMTIME time;
    GetLocalTime(&time);
    _stprintf(szText, L"%02d:%02d:%02d", time.wHour, time.wMinute, time.wSecond);//值                
    vitem.pszText = szText;
    vitem.iItem = iItem;                //行                
    vitem.iSubItem = 0;                //列
    //ListView_InsertItem(hListProcess, &vitem);    //一个宏和SendMessage作用一样                
    SendMessage(hListProcess, LVM_INSERTITEM, 0, (DWORD)&vitem);    //如果用SendMessage,给第一列赋值用LVM_INSERTITEM，其它列用LVM_SETITEM，用ListView同理                

    wcscpy(action_tmp, action);
    vitem.pszText = action_tmp;
    vitem.iSubItem = 1;
    ListView_SetItem(hListProcess, &vitem);

    wcscpy(result_tmp, result);
    vitem.pszText = result_tmp;
    vitem.iSubItem = 2;
    ListView_SetItem(hListProcess, &vitem);

    iItem++;

    SendMessage(hListProcess, LVM_ENSUREVISIBLE, (iItem-1), 0);

    if (iItem > 1000)
    {
        SendMessage(hListProcess, LVM_DELETEALLITEMS, 0, 0);
        iItem = 0;
    }

    UpdateWindow(hListProcess);
}

//初始化进程窗口，就是给ListView控件添加列
void initListView(HWND hDlg) {
    LV_COLUMN lv;

    //初始化，局部变量堆栈中分配，不知道是什么数据所以先清零                                
    memset(&lv, 0, sizeof(LV_COLUMN));
    //获取IDC_LIST_PROCESS句柄                                
    hListProcess = CreateWindow(WC_LISTVIEW, L"日志", WS_VISIBLE | WS_CHILD | WS_VSCROLL | LVS_REPORT, 
        380, 30, 350, 350, hDlg, NULL, hInst, NULL);
    SendMessage(hListProcess, WM_SETFONT, WPARAM(hfont), true);
    //设置整行选中，窗口是windows来管理的无法直接操作，程序能做的只能发送一个消息来让windows直到该怎么做                                
    SendMessage(hListProcess, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
    
    //第一列                                
    lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    wchar_t time[] = L"时间";
    lv.pszText = time;                //列标题                
    lv.cx = 100;                       //列宽
    lv.iSubItem = 0;
    //ListView_InsertColumn(hListProcess, 0, &lv);                                
    SendMessage(hListProcess, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
    //第二列
    wchar_t action[] = L"动作";
    lv.pszText = action;
    lv.cx = 170;
    lv.iSubItem = 1;
    //ListView_InsertColumn(hListProcess, 1, &lv);                                
    SendMessage(hListProcess, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
    //第三列      
    wchar_t result[] = L"结果";
    lv.pszText = result;
    lv.cx = 80;
    lv.iSubItem = 2;
    SendMessage(hListProcess, LVM_INSERTCOLUMN, 2, (DWORD)&lv);
}

UINT Check_Process(void)
{
    HANDLE hProcess = NULL;
    PROCESSENTRY32 prry;
    prry.dwSize = sizeof(PROCESSENTRY32);
    BOOL bRet;
    wchar_t* pmbbuf = new wchar_t[260];
    if (NULL == pmbbuf)
    {
        return 0;
    }
    hProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    bRet = Process32First(hProcess, &prry);
    while (bRet)
    {
        memset(pmbbuf, 0, 260);
        swprintf_s(pmbbuf, 260, L"%s", prry.szExeFile);
        if (0 == wcscmp(pmbbuf, cProcessName))
        {
            //找到后返回进程ID
            delete pmbbuf;
            pmbbuf = NULL;
            return prry.th32ProcessID;
        }
        else
        {
            //查找下一个进程
            bRet = Process32Next(hProcess, &prry);
        }
    };
    delete pmbbuf;
    pmbbuf = NULL;
    return 0;
}
bool Get_Server_Ip_Port(UINT process_id)
{
    PMIB_TCPTABLE2 pTcpTable;
    ULONG ulSize = 0;
    DWORD dwRetVal = 0;
    char szLocalAddr[128];
    char szRemoteAddr[128];
    struct in_addr IpAddr;
    int i;
    pTcpTable = (MIB_TCPTABLE2 *)MALLOC(sizeof(MIB_TCPTABLE2));
    if (pTcpTable == NULL) {
        return false;
    }
    ulSize = sizeof(MIB_TCPTABLE);
    // Make an initial call to GetTcpTable2 to
    // get the necessary size into the ulSize variable
    if ((dwRetVal = GetTcpTable2(pTcpTable, &ulSize, TRUE)) ==
        ERROR_INSUFFICIENT_BUFFER) {
        FREE(pTcpTable);
        pTcpTable = (MIB_TCPTABLE2 *)MALLOC(ulSize);
        if (pTcpTable == NULL) {
            return false;
        }
    }
    // Make a second call to GetTcpTable2 to get
    // the actual data we require
    if ((dwRetVal = GetTcpTable2(pTcpTable, &ulSize, TRUE)) == NO_ERROR) 
    {
        for (i = 0; i < (int)pTcpTable->dwNumEntries; i++) 
        {
            if (process_id == (unsigned int)pTcpTable->table[i].dwOwningPid && ntohs(pTcpTable->table[i].dwRemotePort) > 1024)
            {
                server_ip = pTcpTable->table[i].dwRemoteAddr;
                server_port = pTcpTable->table[i].dwRemotePort;
                FREE(pTcpTable);
                return true;
            }
        }
    }
    else 
    {
        FREE(pTcpTable);
        return false;
    }
    if (pTcpTable != NULL) 
    {
        FREE(pTcpTable);
        pTcpTable = NULL;
    }
    return false;
}

bool Check_Online(void)
{
    bool ret = false;
    int error = -1;
    int len = sizeof(int);
    //----------------------
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) 
    {
        return false;
    }
    //----------------------
    // Create a SOCKET for connecting to server
    SOCKET ConnectSocket;
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET)
    {
        WSACleanup();
        return false;
    }
    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = server_ip;
    clientService.sin_port = server_port;

    //----------------------
    // Connect to server.
    struct timeval timeout;
    fd_set r;
    unsigned long ul = 1;
    ioctlsocket(ConnectSocket, FIONBIO, &ul);
    iResult = connect(ConnectSocket, (SOCKADDR *)& clientService, sizeof(clientService));
    if (iResult == SOCKET_ERROR) 
    {
        FD_ZERO(&r);
        FD_SET(ConnectSocket, &r);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if (select(0, NULL, &r, NULL, &timeout) > 0)
        {
            getsockopt(ConnectSocket, SOL_SOCKET, SO_ERROR, (char *)&error, &len);
            if (error == 0)
            {
                ret = true;
            }
        }
        if (ret == false)
        {
            iResult = closesocket(ConnectSocket);
            WSACleanup();
            return false;
        }
    }

    iResult = closesocket(ConnectSocket);
    if (iResult == SOCKET_ERROR) {
        WSACleanup();
        return false;
    }

    WSACleanup();
    return true;
}

DWORD WINAPI Try_Connect(void * param)
{
    UINT max_tries = 10000;
    Sleep(200);
    while (max_tries)
    {
        if (is_started == false)
        {
            return 0;
        }
        if (Check_Online())
        {
            AddInfo(L"尝试连接服务器", L"成功");
            MessageBox(NULL, _T("服务器已开放"), _T("Success"), NULL);
            is_started = false;
            SetWindowText(hStartButton, L"开始检测");
            return 0;
        }

        AddInfo(L"尝试连接服务器", L"失败");

        max_tries--;
        Sleep(1000);
    }
    MessageBox(NULL, _T("达到尝试最大次数，请稍后再试"), _T("Attetion"), NULL);
    EnableWindow(hStartButton, true);
    return 0;
}

bool Get_Server_Ip_Port_From_Edit()
{
    TCHAR server_port_tmp[256];
    int len = GetWindowTextLength(hwnd_server_port);
    memset(server_port_tmp, 0, 256);
    GetWindowText(hwnd_server_port, (LPWSTR)server_port_tmp, len + 1);
    server_port_tmp[len+1] = '\0';
    server_port = _wtoi(server_port_tmp);
    if (server_port > 65535 || server_port <= 1024)
    {
        server_port_tmp[0] = '\0';
        SetWindowText(hwnd_server_port, server_port_tmp);
        AddInfo(L"获取用户输入端口", L"失败(不合规)");
        return false;
    }

    server_port = htons(server_port);

    SendMessage(hwnd_server_ip, IPM_GETADDRESS, 0, int(&server_ip));

    server_ip = htonl(server_ip);
    if (server_ip == 0)
    {
        SendMessage(hwnd_server_ip, IPM_CLEARADDRESS, 0, 0);
        AddInfo(L"获取用户输入IP", L"失败(不合规)");
        return false;
    }

    return true;
}

void display_server_ip()
{
    SendMessage(hwnd_server_ip, IPM_SETADDRESS, 0, ntohl(server_ip));
}

void display_server_port()
{
    wchar_t buff[256] = { 0 };
    _itow(ntohs(server_port), buff, 10);
    SetWindowText(hwnd_server_port, buff);
}
void Load_Server_Ip_Port()
{
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(L"./开服检测器save.txt", &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        AddInfo(L"检测已保存的服务器IP端口", L"失败");
        MessageBox(NULL, _T("没有找到文件名为:[开服检测器save.txt]的保存文件，请尝试自动获取"), _T("Error"), NULL);
        return;
    }
    FILE *fpdst;
    fpdst = fopen("./开服检测器save.txt", "r");
    if (fpdst == NULL)
    {
        AddInfo(L"读取已保存的服务器IP端口", L"失败");
        return;
    }
    char line[256] = { 0 };
    char buf[256] = { 0 };
    if (fgets(line, 256, fpdst) == NULL)
    {
        AddInfo(L"读取已保存的服务器IP端口", L"失败");
        return;
    }
    for (int i = 0; i < 255; i++)
    {
        if (line[i] == ':')
        {
            strcpy(buf, &line[i + 1]);
            break;
        }
    }
    server_ip = inet_addr(buf);
    display_server_ip();
    AddInfo(L"读取已保存的服务器IP", L"成功");

    if (fgets(line, 256, fpdst) == NULL)
    {
        AddInfo(L"读取已保存的服务器端口", L"失败");
        return;
    }
    for (int i = 0; i < 255; i++)
    {
        if (line[i] == ':')
        {
            strcpy(buf, &line[i + 1]);
            break;
        }
    }
    server_port = htons(atoi(buf));
    display_server_port();
    AddInfo(L"读取已保存的服务器端口", L"成功");
    fclose(fpdst);

    is_loaded = true;

    SetWindowText(hLocalButton, L"保存至本地");
}

void Save_Server_Ip_Port(void)
{
    FILE *fpdst;
    fpdst = fopen("./开服检测器save.txt", "w");
    if (fpdst == NULL)
    {
        AddInfo(L"保存服务器IP端口", L"失败");
        return;
    }
    struct in_addr ipaddr;
    ipaddr.s_addr = server_ip;
    fprintf(fpdst, "server-ip:%s\nserver-port:%d\n", inet_ntoa(ipaddr), ntohs(server_port));
    fclose(fpdst);
    AddInfo(L"保存服务器IP端口", L"成功");
    MessageBox(NULL, _T("保存服务器IP端口成功，文件名为:[开服检测器save.txt]"), _T("Success"), NULL);
}

HFONT Set_Font()
{
    LOGFONT lf;
    lf.lfHeight = 14;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = 0;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = GB2312_CHARSET;
    lf.lfOutPrecision = 0;
    lf.lfClipPrecision = 0;
    lf.lfQuality = 0;
    lf.lfPitchAndFamily = 0;
    lstrcpy(lf.lfFaceName, _T("楷体"));
    return CreateFontIndirect(&lf);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc;
    static HWND hChooseButton1;
    static HWND hChooseButton2;
    static int          cxClient, cyClient;
    static bool FileChoosen = false;
    static bool ConfigChoosen = false;
    static UINT max_tries = 1000;
    static UINT process_id;
    bool ret;
    hfont = Set_Font();
    switch (message) {
    case WM_CREATE:
        int scrWidth, scrHeight;
        RECT rect;
        //获得屏幕尺寸
        scrWidth = GetSystemMetrics(SM_CXSCREEN);
        scrHeight = GetSystemMetrics(SM_CYSCREEN);
        //取得窗口尺寸
        GetWindowRect(hWnd, &rect);
        //重新设置rect里的值
        rect.right = MAINWINDOWWIDTH;
        rect.bottom = MAINWINDOWHEIGHT;
        rect.left = (scrWidth - rect.right) / 2;
        rect.top = (scrHeight - rect.bottom) / 2;
        //移动窗口到指定的位置
        SetWindowPos(hWnd, HWND_TOP, rect.left, rect.top, rect.right, rect.bottom, SWP_SHOWWINDOW);

        hChooseButton1 = CreateWindow(L"Button", L"dx11", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
            100, 110, 100, 25, hWnd, (HMENU)BUTTON_SET_EXE_TYPE_DX11, hInst, NULL);
        hChooseButton2 = CreateWindow(L"Button", L"not dx11", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            220, 110, 100, 25, hWnd, (HMENU)BUTTON_SET_EXE_TYPE_UNDX11, hInst, NULL);
        hCheckButton = CreateWindow(L"Button", L"获取服务器信息", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            115, 140, 150, 25, hWnd, (HMENU)BUTTON_GET_IP_AND_PORT, hInst, NULL);
        hwnd_server_ip = CreateWindow(WC_IPADDRESS, NULL, WS_CHILD | WS_VISIBLE,
            170, 210, 140, 25, hWnd, 0, hInst, NULL);
        hwnd_server_port = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_RIGHT | WS_BORDER,
            170, 240, 140, 25, hWnd, 0, hInst, NULL);
        hLocalButton = CreateWindow(L"Button", L"从本地读取", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            120, 270, 140, 25, hWnd, (HMENU)BUTTON_LOCAL, hInst, NULL);
        hStartButton = CreateWindow(L"Button", L"开始检测", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            150, 350, 80, 25, hWnd, (HMENU)BUTTON_START_CHECK, hInst, NULL);
        SendMessage(hChooseButton1, WM_SETFONT, WPARAM(hfont), true);
        SendMessage(hChooseButton2, WM_SETFONT, WPARAM(hfont), true);
        SendMessage(hCheckButton, WM_SETFONT, WPARAM(hfont), true);
        SendMessage(hwnd_server_ip, WM_SETFONT, WPARAM(hfont), true);
        SendMessage(hwnd_server_port, WM_SETFONT, WPARAM(hfont), true);
        SendMessage(hLocalButton, WM_SETFONT, WPARAM(hfont), true);
        SendMessage(hStartButton, WM_SETFONT, WPARAM(hfont), true);
        initListView(hWnd);

        break;
    case WM_SIZE:
        cxClient = LOWORD(lParam);
        cyClient = HIWORD(lParam);
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        hfont = Set_Font();
        SelectObject(hdc, hfont);
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc, 20, 10, _T("第一步，获取服务器信息:"), _tcslen(_T("第一步，获取服务器信息:")));
        TextOut(hdc, 380, 10, _T("程序日志:"), _tcslen(_T("程序日志:")));
        TextOut(hdc, 40, 50, _T("自动检测:"), _tcslen(_T("自动检测:")));
        TextOut(hdc, 80, 80, _T("选择客户端类型:"), _tcslen(_T("选择客户端类型:")));
        TextOut(hdc, 40, 180, _T("或 手动输入:"), _tcslen(_T("或 手动输入:")));
        TextOut(hdc, 80, 210, _T("服务器IP:"), _tcslen(_T("服务器IP:")));
        TextOut(hdc, 80, 240, _T("服务器端口:"), _tcslen(_T("服务器端口:")));
        TextOut(hdc, 20, 320, _T("第二步，开始/停止检测服务器状态:"), _tcslen(_T("第二步，开始/停止检测服务器状态:")));
        EndPaint(hWnd, &ps);
        break;
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
            case BUTTON_SET_EXE_TYPE_DX11:
                wcscpy(cProcessName, L"ffxiv_dx11.exe");
                break;
            case BUTTON_SET_EXE_TYPE_UNDX11:
                wcscpy(cProcessName, L"ffxiv.exe");
                break;
            case BUTTON_LOCAL:
                if (is_loaded)
                {
                    Save_Server_Ip_Port();
                }
                else
                {
                    Load_Server_Ip_Port();
                }
                break;
            case BUTTON_GET_IP_AND_PORT:
                if (cProcessName[0] == 0)
                {
                    MessageBox(NULL, _T("请先选择游戏客户端类型(dx11或者not dx11)"), _T("Error"), NULL);
                    break;
                }
                process_id = Check_Process();
                if (process_id == 0)
                {
                    AddInfo(L"获取客户端进程号", L"失败");
                    MessageBox(NULL, _T("没有检测到客户端，需要进入游戏世界以获得服务器地址"), _T("Error"), NULL);
                    break;
                }
                AddInfo(L"获取客户端进程号", L"成功");
                ret = Get_Server_Ip_Port(process_id);
                if (!ret)
                {
                    AddInfo(L"获取服务器IP和端口", L"失败");
                    MessageBox(NULL, _T("获取服务器IP失败，请尝试重新进入游戏"), _T("Error"), NULL);
                    break;
                }
                display_server_ip();
                display_server_port();
                AddInfo(L"获取服务器IP和端口", L"成功");
                
                is_loaded = true;

                SetWindowText(hLocalButton, L"保存至本地");

                break;
            case BUTTON_START_CHECK:
                if (is_started)
                {
                    is_started = false;
                    Sleep(1000);
                    SetWindowText(hStartButton, L"开始检测");
                }
                else
                {
                    is_initialized = Get_Server_Ip_Port_From_Edit();
                    if (is_initialized)
                    {
                        hThread = CreateThread(NULL, NULL, Try_Connect, NULL, 0, 0);
                        is_started = true;
                        SetWindowText(hStartButton, L"停止检测");
                    }
                    else
                    {
                        MessageBox(NULL, _T("输入的服务器IP或端口不合规范，请先输入服务器IP和端口，或者尝试自动获取"), _T("Error"), NULL);
                    }
                }
                break;
            default:
                break;
        }
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }
    return 0;
}
