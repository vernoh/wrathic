#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <wininet.h>
#include <dwmapi.h>
#include <psapi.h>
#include <mmsystem.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <intrin.h>
#include <iphlpapi.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <mutex>
#include <algorithm>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")
#include "logo_icon.h"

#define APP_VERSION      L"v2.2.4"
#define CHANGELOG_VERSION L"v2.2.4"
#define UPDATE_URL        "https://api.github.com/repos/vernoh/pulsekps/releases/latest"
#define TRIAL_DAYS        2
#define TRIAL_REG_KEY     L"Software\\MacroApp\\Trial"
#define INITIAL_KPS   20
#define MIN_KPS       20
#define MAX_KPS       2000
#define SUPABASE_URL  L"autxjoshycjhnvdpbtmn.supabase.co"
#define SUPABASE_ANON "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImF1dHhqb3NoeWNqaG52ZHBidG1uIiwicm9sZSI6ImFub24iLCJpYXQiOjE3Nzg4NTEwMTEsImV4cCI6MjA5NDQyNzAxMX0.6R_G0eLKa2p55nlT8-mRgVRBzpQFzCOYTWuAIsW3bwQ"
#define SERVICE_KEY   "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImF1dHhqb3NoeWNqaG52ZHBidG1uIiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImlhdCI6MTc3ODg1MTAxMSwiZXhwIjoyMDk0NDI3MDExfQ.jZUlYNFIPInicnl8EWnrqL_XBo-uggQIlohg9GxAUeg"
#define LICENSE_FILE  L"license.dat"
#define SETTINGS_FILE L"settings.dat"
#define LOG_FILE      L"logs.txt"
#define APP_W         420
#define APP_H         600
#define DOCK_H        52  // bottom tab dock height

// ===================== LOGGING =====================
std::mutex logMutex;
std::vector<std::wstring> logEntries;

void addLog(const wchar_t* type, const wchar_t* msg) {
    SYSTEMTIME st; GetLocalTime(&st);

    // Build wide string for in-memory log display
    wchar_t wbuf[512];
    swprintf(wbuf, 512, L"[%02d:%02d:%02d] [%s] %s",
        st.wHour, st.wMinute, st.wSecond, type, msg);
    {
        std::lock_guard<std::mutex> lk(logMutex);
        logEntries.push_back(std::wstring(wbuf));
    }

    // Build narrow ASCII string for file - avoid any wide->narrow conversion issues
    // by manually building it char by char from safe ASCII-range wchars
    char nbuf[512];
    int ni = 0;
    // Timestamp and type prefix - all ASCII safe
    nbuf[ni++] = '[';
    nbuf[ni++] = '0' + (st.wHour/10);   nbuf[ni++] = '0' + (st.wHour%10);
    nbuf[ni++] = ':';
    nbuf[ni++] = '0' + (st.wMinute/10); nbuf[ni++] = '0' + (st.wMinute%10);
    nbuf[ni++] = ':';
    nbuf[ni++] = '0' + (st.wSecond/10); nbuf[ni++] = '0' + (st.wSecond%10);
    nbuf[ni++] = ']'; nbuf[ni++] = ' '; nbuf[ni++] = '[';
    // type (OK/ERR/INFO)
    for (const wchar_t* p = type; *p && ni < 500; p++)
        nbuf[ni++] = (char)((*p < 128) ? *p : '?');
    nbuf[ni++] = ']'; nbuf[ni++] = ' ';
    // message
    for (const wchar_t* p = msg; *p && ni < 508; p++)
        nbuf[ni++] = (char)((*p < 128) ? *p : '?');
    nbuf[ni++] = '\n';
    nbuf[ni] = 0;

    // Append to log file
    FILE* f = fopen("logs.txt", "a");
    if (f) {
        fputs(nbuf, f);
        fclose(f);
    }
}
#define LOG_OK(m)   addLog(L"OK",   m)
#define LOG_ERR(m)  addLog(L"ERR",  m)
#define LOG_INFO(m) addLog(L"INFO", m)

// ===================== THEMES =====================
struct Theme {
    COLORREF bg, surface, card, border, text, subtext, accent, btn, btnHov, green, red;
    const wchar_t* name;
};
Theme THEMES[] = {
    // Dark - deep charcoal, indigo accent
    {RGB(10,10,14),RGB(16,16,22),RGB(20,20,28),RGB(32,32,46),
     RGB(235,235,250),RGB(100,100,130),
     RGB(99,102,241),RGB(26,26,36),RGB(38,38,54),
     RGB(52,211,153),RGB(248,113,113),L"Dark"},
    // Midnight - near-black, violet
    {RGB(6,6,10),RGB(12,12,18),RGB(16,16,24),RGB(28,28,42),
     RGB(210,210,240),RGB(90,90,120),
     RGB(139,92,246),RGB(20,20,32),RGB(32,32,48),
     RGB(52,211,153),RGB(248,113,113),L"Midnight"},
    // Light - clean white, strong contrast
    {RGB(240,240,248),RGB(252,252,255),RGB(228,228,240),RGB(180,180,210),
     RGB(10,10,24),RGB(60,60,100),
     RGB(79,82,221),RGB(210,210,230),RGB(190,190,218),
     RGB(16,150,100),RGB(210,40,40),L"Light"},
    // Red - dark with crimson
    {RGB(10,6,6),RGB(18,10,10),RGB(22,12,12),RGB(48,20,20),
     RGB(255,240,240),RGB(150,100,100),
     RGB(239,68,68),RGB(36,16,16),RGB(52,24,24),
     RGB(52,211,153),RGB(239,68,68),L"Red"},
    // Ocean - dark teal
    {RGB(6,14,20),RGB(10,20,30),RGB(12,24,36),RGB(20,48,70),
     RGB(210,235,255),RGB(90,140,175),
     RGB(14,165,233),RGB(12,32,50),RGB(18,44,66),
     RGB(52,211,153),RGB(248,113,113),L"Ocean"},
    // Void - pure black
    {RGB(0,0,0),RGB(4,4,4),RGB(8,8,8),RGB(16,16,16),
     RGB(255,255,255),RGB(120,120,120),
     RGB(160,160,160),RGB(10,10,10),RGB(18,18,18),
     RGB(52,211,153),RGB(248,113,113),L"Void"},
};
int themeIdx = 5;
#define T THEMES[themeIdx]

// Fonts - default Consolas (index 2)
const wchar_t* FONT_NAMES[] = {L"Calibri", L"Segoe UI", L"Consolas", L"Arial", L"Bahnschrift"};
int fontIdx = 2; // Consolas default

// KPS presets - 20, 100, 200, 500, 750, 1000, 2000
const int KPS_PRESETS[]   = {20, 100, 200, 500, 750, 1000, 2000};
const int KPS_PRESET_COUNT = 7;

// ===================== IDs =====================
#define ID_SET_HOTKEY    101
#define ID_ADD_KEY       103
#define ID_REMOVE_KEY    104
#define ID_MODE_TOGGLE   108
#define ID_MODE_HOLD     109
#define ID_LICENSE_INPUT 200
#define ID_LICENSE_BTN   201
#define ID_SETTINGS_BTN  202
#define ID_KPS_OVERLAY   204
#define ID_RES_OVERLAY   205
#define ID_KPS_INPUT     206
#define ID_OPEN_LOGS     207
#define ID_MEM_CLEAN     208
#define ID_EXPORT_CFG    209
#define ID_IMPORT_CFG    210
#define TIMER_ANIM       300
#define TIMER_SETT       301  // slower timer for settings refresh
#define ID_TRAY_ICON     400
#define ID_TRAY_SHOW     401
#define ID_TRAY_EXIT     402
#define WM_TRAYICON      (WM_USER+1)
#define ID_AUTOLAUNCH    403

// ===================== STATE =====================
std::atomic<bool> macroRunning(false);
std::atomic<bool> appRunning(true);
std::atomic<int>  kps(INITIAL_KPS);
std::atomic<int>  hotkeyVK(VK_F8);
std::atomic<bool> holdMode(false);
std::atomic<bool> capturingHotkey(false);
std::atomic<bool> capturingKey(false);
std::atomic<bool> robloxFocused(false);

std::vector<int> keysToSend;
CRITICAL_SECTION keyListCS;

std::wstring savedLicenseKey;
std::wstring machineHWID;

HWND hwndMain=NULL, hwndSettings=NULL;
HWND hwndOverlay=NULL; // Single unified overlay
bool keyVisible=false;
bool kpsOverlayEnabled=false;
bool resOverlayEnabled=false;
bool autoLaunchEnabled=false;
bool minimiseToTray=false;

bool trialMode=false;
int  trialDaysLeft=TRIAL_DAYS;
bool changelogShown=false;
bool showChangelogOnStartup=true; // user can disable in settings
POINT overlayDrag={};
bool overlayDragging=false;
int overlayX=0, overlayY=80; // saved overlay position
int settScrollPos=0;
int settTotalHeight=0;
int activeTab=0;         // 0=Macro, 1=Settings
bool settingsDirty=true;  // force repaint of settings when true
float tabAnim=0.0f;      // 0.0=Macro, 1.0=Settings (animated)
int mainScrollPos=0;
int mainTotalHeight=0;
bool fontDropOpen=false;
int  fontDropHov=-1;

HFONT hFontBig, hFontMed, hFontSmall, hFontMono;
HWND hwndHovered=NULL;

// Animation globals
float animMacroStatus=0.0f;
float animModeHold=0.0f;
float animRunning=0.0f;
float fadeAlpha=0.0f;      // Main window fade-in (0=invisible, 1=fully visible)
float settFadeAlpha=0.0f;  // Settings window fade-in
float fadeOffsetY=0.0f;    // Vertical float offset for main window elements

// ===================== EASING =====================
// Cubic ease-in-out: slow start, fast middle, slow end
float easeInOut(float t){
    t=std::max(0.0f,std::min(1.0f,t));
    return t<0.5f?4*t*t*t:1-(-2*t+2)*(-2*t+2)*(-2*t+2)/2;
}
// Ease-out quart: fast start, slow end (good for hover)
float easeOut(float t){
    t=std::max(0.0f,std::min(1.0f,t));
    float u=1-t;
    return 1-u*u*u*u;
}
// Spring: overshoots slightly then settles (good for toggles)
float easeSpring(float t){
    t=std::max(0.0f,std::min(1.0f,t));
    return 1-(float)cos(t*3.14159f)*exp(-t*4.0f);
}
// Smooth step (sigmoid-like)
float smoothStep(float t){
    t=std::max(0.0f,std::min(1.0f,t));
    return t*t*(3-2*t);
}

// Animate a float toward a target using spring easing
// speed: higher = faster (0.1=slow, 0.3=normal, 0.6=fast)
void animateTo(float& val, float target, float speed){
    float diff=target-val;
    if(fabsf(diff)<0.001f){val=target;return;}
    val+=easeInOut(fabsf(diff))*diff*speed;
}

// Per-button hover animation values (smooth 0->1)
float hoverBtn[16]={};
int   hoverBtnId[16]={500,501,502,503,504,505,506,
                      ID_MODE_TOGGLE,ID_MODE_HOLD,
                      ID_SET_HOTKEY,ID_ADD_KEY,ID_REMOVE_KEY,
                      0,0,0,0};

// Get hover alpha for a given hit ID
float getHoverAlpha(int id){
    for(int i=0;i<16;i++) if(hoverBtnId[i]==id) return hoverBtn[i];
    return 0.0f;
}

// Lerp two colours
COLORREF lerpCol(COLORREF a,COLORREF b,float t){
    t=easeOut(t);
    return RGB(
        (int)(GetRValue(a)+(GetRValue(b)-GetRValue(a))*t),
        (int)(GetGValue(a)+(GetGValue(b)-GetGValue(a))*t),
        (int)(GetBValue(a)+(GetBValue(b)-GetBValue(a))*t)
    );
}

// CPU via GetProcessTimes
float cpuUsage=0.0f;
SIZE_T memUsageKB=0;
std::atomic<int> actualKPS(0);
std::atomic<int> keypressCount(0);
DWORD lastKpsTick=0;

// Overlay drag

int recommendedKPS=200;
HINSTANCE gInst;

// ===================== SETTINGS =====================
void saveSettings() {
    std::wofstream f(SETTINGS_FILE);
    if (!f.is_open()) { LOG_ERR(L"Failed to open settings file for writing"); return; }
    f << themeIdx << L"\n"
      << kps.load() << L"\n"
      << hotkeyVK.load() << L"\n"
      << (int)holdMode.load() << L"\n"
      << (int)kpsOverlayEnabled << L"\n"
      << (int)resOverlayEnabled << L"\n"
      << fontIdx << L"\n"
      << (int)autoLaunchEnabled << L"\n"
      << (int)minimiseToTray << L"\n"
      << CHANGELOG_VERSION << L"\n";
    EnterCriticalSection(&keyListCS);
    f << keysToSend.size() << L"\n";
    for (int v : keysToSend) f << v << L"\n";
    LeaveCriticalSection(&keyListCS);
    f << overlayX << L"\n" << overlayY << L"\n" << showChangelogOnStartup << L"\n";
    LOG_OK(L"Settings saved");
}

void loadSettings() {
    std::wifstream f(SETTINGS_FILE);
    if (!f.is_open()) {
        LOG_INFO(L"First launch - using default settings");
        EnterCriticalSection(&keyListCS);
        keysToSend.push_back('F');
        LeaveCriticalSection(&keyListCS);
        return;
    }
    int ti, k, hk, hm, kov, rov, fi, al, ksz;
    wchar_t savedVer[32]={};
    if (!(f >> ti >> k >> hk >> hm >> kov >> rov >> fi >> al)) {
        LOG_ERR(L"Settings file is corrupt - reverting to defaults"); return;
    }
    if (ti>=0&&ti<6) themeIdx=ti;
    kps = std::max(MIN_KPS, std::min(MAX_KPS, k));
    hotkeyVK = hk ? hk : VK_F8;
    holdMode = hm != 0;
    kpsOverlayEnabled = kov != 0;
    resOverlayEnabled = rov != 0;
    if (fi>=0&&fi<5) fontIdx=fi;
    autoLaunchEnabled = al!=0;
    int mtt=1; if(f>>mtt) minimiseToTray=(mtt!=0);
    // Read optional changelog version then keys size
    std::wstring savedChangelogVer;
    if (f >> savedChangelogVer) {
        changelogShown = (savedChangelogVer == std::wstring(CHANGELOG_VERSION));
        f >> ksz;
    } else {
        changelogShown = false;
        ksz = 0;
    }
    EnterCriticalSection(&keyListCS);
    keysToSend.clear();
    for (int i=0; i<ksz; i++) {
        int v; if (f>>v && v>0 && v<256) keysToSend.push_back(v);
    }
    LeaveCriticalSection(&keyListCS);
    { int ox=0,oy=80,scl=1; if(f>>ox>>oy){overlayX=ox;overlayY=oy;} if(f>>scl)showChangelogOnStartup=(scl!=0); }
    LOG_OK(L"Settings loaded");
    // If no keys saved, add F as default
    EnterCriticalSection(&keyListCS);
    if(keysToSend.empty()) keysToSend.push_back('F');
    LeaveCriticalSection(&keyListCS);
}

void exportConfig() {
    wchar_t path[MAX_PATH]={};
    OPENFILENAME ofn={};
    ofn.lStructSize=sizeof(ofn);
    ofn.hwndOwner=hwndMain;
    ofn.lpstrFilter=L"Macro Config (*.mcfg)\0*.mcfg\0";
    ofn.lpstrFile=path;
    ofn.nMaxFile=MAX_PATH;
    ofn.lpstrDefExt=L"mcfg";
    ofn.Flags=OFN_OVERWRITEPROMPT;
    if (!GetSaveFileName(&ofn)) return;
    std::wofstream f(path);
    if (!f.is_open()) { LOG_ERR(L"Export failed - could not write file"); return; }
    f << themeIdx << L"\n" << kps.load() << L"\n"
      << hotkeyVK.load() << L"\n" << (int)holdMode.load() << L"\n"
      << fontIdx << L"\n";
    EnterCriticalSection(&keyListCS);
    f << keysToSend.size() << L"\n";
    for (int v : keysToSend) f << v << L"\n";
    LeaveCriticalSection(&keyListCS);
    LOG_OK(L"Config exported successfully");
}

void importConfig() {
    wchar_t path[MAX_PATH]={};
    OPENFILENAME ofn={};
    ofn.lStructSize=sizeof(ofn);
    ofn.hwndOwner=hwndMain;
    ofn.lpstrFilter=L"Macro Config (*.mcfg)\0*.mcfg\0";
    ofn.lpstrFile=path;
    ofn.nMaxFile=MAX_PATH;
    ofn.Flags=OFN_FILEMUSTEXIST;
    if (!GetOpenFileName(&ofn)) return;
    std::wifstream f(path);
    if (!f.is_open()) { LOG_ERR(L"Import failed - could not read file"); return; }
    int ti, k, hk, hm, fi, ksz;
    if (!(f>>ti>>k>>hk>>hm>>fi>>ksz)) { LOG_ERR(L"Import failed - file is corrupt or wrong format"); return; }
    if (ti>=0&&ti<6) themeIdx=ti;
    kps=std::max(MIN_KPS,std::min(MAX_KPS,k));
    hotkeyVK=hk?hk:VK_F8;
    holdMode=hm!=0;
    if (fi>=0&&fi<5) fontIdx=fi;
    // importConfig does not touch autoLaunch or changelog state
    EnterCriticalSection(&keyListCS);
    keysToSend.clear();
    for (int i=0;i<ksz;i++){int v;if(f>>v&&v>0&&v<256)keysToSend.push_back(v);}
    LeaveCriticalSection(&keyListCS);
    saveSettings();
    LOG_OK(L"Config imported successfully - settings applied");
    InvalidateRect(hwndMain,NULL,FALSE);
}

// ===================== FONTS =====================
// ===================== AUTO-LAUNCH =====================
// Auto-launch with Roblox - monitors for RobloxPlayerBeta.exe
std::atomic<bool> robloxWasRunning(false);

bool isRobloxProcessRunning(){
    HANDLE hSnap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(hSnap==INVALID_HANDLE_VALUE)return false;
    PROCESSENTRY32 pe={};pe.dwSize=sizeof(pe);
    bool found=false;
    if(Process32First(hSnap,&pe)){
        do{
            if(_wcsicmp(pe.szExeFile,L"RobloxPlayerBeta.exe")==0||
               _wcsicmp(pe.szExeFile,L"RobloxPlayer.exe")==0){
                found=true;break;
            }
        }while(Process32Next(hSnap,&pe));
    }
    CloseHandle(hSnap);
    return found;
}

void cleanMemory(); // forward declaration
void robloxLaunchWatchThread(){
    while(appRunning){
        if(autoLaunchEnabled){
            bool running=isRobloxProcessRunning();
            if(running&&!robloxWasRunning){
                // Roblox just launched - show macro window
                if(hwndMain&&IsWindow(hwndMain)){
                    ShowWindow(hwndMain,SW_RESTORE);
                    SetForegroundWindow(hwndMain);
                    LOG_OK(L"Roblox detected - macro window shown");
                }
                // Auto RAM cleanup on Roblox launch
                LOG_INFO(L"Running automatic RAM cleanup for Roblox...");
                std::thread(cleanMemory).detach();
            }
            if(!running&&robloxWasRunning){
                // Roblox closed - hide macro
                if(hwndMain&&IsWindow(hwndMain)){
                    ShowWindow(hwndMain,SW_HIDE);
                    LOG_INFO(L"Roblox closed - macro window hidden");
                }
            }
            robloxWasRunning=running;
        }
        Sleep(2000);
    }
}

void setAutoLaunch(bool enable){
    autoLaunchEnabled=enable;
    // Also add to Windows startup so macro is running when Roblox launches
    HKEY hKey;
    if(RegOpenKeyEx(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,KEY_SET_VALUE,&hKey)==ERROR_SUCCESS){
        if(enable){
            wchar_t exe[MAX_PATH];GetModuleFileName(NULL,exe,MAX_PATH);
            // Launch minimised to tray so it is running but hidden
            std::wstring cmd=std::wstring(exe)+L" /tray";
            RegSetValueEx(hKey,L"MacroApp",0,REG_SZ,(BYTE*)cmd.c_str(),(DWORD)((cmd.size()+1)*sizeof(wchar_t)));
            LOG_OK(L"Auto-launch with Roblox enabled - macro starts with Windows (hidden), shows when Roblox opens");
        } else {
            RegDeleteValue(hKey,L"MacroApp");
            LOG_INFO(L"Auto-launch with Roblox disabled");
        }
        RegCloseKey(hKey);
    }
}
bool checkAutoLaunchReg(){
    HKEY hKey;
    if(RegOpenKeyEx(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,KEY_QUERY_VALUE,&hKey)!=ERROR_SUCCESS)return false;
    bool ex=RegQueryValueEx(hKey,L"MacroApp",NULL,NULL,NULL,NULL)==ERROR_SUCCESS;
    RegCloseKey(hKey);
    return ex;
}

// ===================== TRIAL MODE =====================
void initTrial() {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, TRIAL_REG_KEY, 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS) return;

    DWORD startDate = 0;
    DWORD sz = sizeof(DWORD);
    if (RegQueryValueEx(hKey, L"StartDate", NULL, NULL, (BYTE*)&startDate, &sz) != ERROR_SUCCESS) {
        // First run - store today as YYYYMMDD
        SYSTEMTIME st; GetLocalTime(&st);
        startDate = st.wYear*10000 + st.wMonth*100 + st.wDay;
        RegSetValueEx(hKey, L"StartDate", 0, REG_DWORD, (BYTE*)&startDate, sizeof(DWORD));
        LOG_INFO(L"Trial started - 2 day free trial activated");
    }

    // Calculate days elapsed
    SYSTEMTIME st; GetLocalTime(&st);
    DWORD today = st.wYear*10000 + st.wMonth*100 + st.wDay;

    // Simple day diff (good enough for 2 day trial)
    SYSTEMTIME trialSt = {};
    trialSt.wYear  = (WORD)(startDate/10000);
    trialSt.wMonth = (WORD)((startDate%10000)/100);
    trialSt.wDay   = (WORD)(startDate%100);
    FILETIME ftTrial, ftNow;
    SystemTimeToFileTime(&trialSt, &ftTrial);
    SystemTimeToFileTime(&st, &ftNow);
    ULARGE_INTEGER uTrial, uNow;
    uTrial.LowPart=ftTrial.dwLowDateTime; uTrial.HighPart=ftTrial.dwHighDateTime;
    uNow.LowPart=ftNow.dwLowDateTime;     uNow.HighPart=ftNow.dwHighDateTime;
    // 1 day = 864000000000 * 100ns intervals
    int daysElapsed = (int)((uNow.QuadPart - uTrial.QuadPart) / 864000000000ULL);

    trialDaysLeft = TRIAL_DAYS - daysElapsed;
    trialMode = true;

    wchar_t buf[128];
    if (trialDaysLeft > 0) {
        swprintf(buf,128,L"Trial: %d day(s) remaining",trialDaysLeft);
        LOG_INFO(buf);
    } else {
        LOG_ERR(L"Trial expired - license key required to continue");
    }
    RegCloseKey(hKey);
}

// ===================== AUTO-UPDATE =====================
std::wstring latestVersion = L"";

void checkForUpdate() {
    LOG_INFO(L"Checking for updates...");
    HINTERNET hN = InternetOpen(L"Macro/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hN) { LOG_ERR(L"Update check failed - no internet connection"); return; }
    // Point this to wherever you host the version file
    HINTERNET hUrl = InternetOpenUrl(hN,
        L"about:blank",
        NULL, 0, INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) { InternetCloseHandle(hN); LOG_ERR(L"Update server not reachable"); return; }
    char buf[64]={};DWORD rd=0;
    InternetReadFile(hUrl, buf, sizeof(buf)-1, &rd);
    InternetCloseHandle(hUrl); InternetCloseHandle(hN);
    if (rd == 0) { LOG_ERR(L"Update check failed - empty response from server"); return; }
    buf[rd] = 0;
    // Trim whitespace
    int end = (int)strlen(buf)-1;
    while (end>=0 && (buf[end]=='\n'||buf[end]=='\r'||buf[end]==' ')) buf[end--]=0;
    wchar_t wver[64]={};
    MultiByteToWideChar(CP_ACP, 0, buf, -1, wver, 64);
    latestVersion = std::wstring(wver);
    if (latestVersion != std::wstring(APP_VERSION)) {
        wchar_t msg[128];
        swprintf(msg,128,L"Update available: %s (current: %s)",wver,APP_VERSION);
        LOG_OK(msg);
    } else {
        LOG_OK(L"Macro is up to date");
    }
}

// ===================== SYSTEM TRAY =====================
HICON gAppIcon = NULL; // forward declared here, initialized in loadAppIcon()
NOTIFYICONDATA nid = {};

void addTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd   = hwnd;
    nid.uID    = ID_TRAY_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon  = gAppIcon ? gAppIcon : LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"PulseKPS");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void removeTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void showTrayMenu(HWND hwnd) {
    POINT pt; GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW, L"Show Macro");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN|TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

// Changelog is shown inside splash screen

void createFonts() {
    const wchar_t* fn = FONT_NAMES[fontIdx];
    // Per-font size adjustments - each font renders at different visual sizes
    // Index: 0=Calibri, 1=Segoe UI, 2=Consolas, 3=Arial, 4=Bahnschrift
    int szBig, szMed, szSm, szMono;
    switch(fontIdx){
        case 0:  szBig=24;szMed=17;szSm=15;szMono=13;break; // Calibri - needs larger
        case 1:  szBig=22;szMed=16;szSm=14;szMono=13;break; // Segoe UI
        case 2:  szBig=20;szMed=14;szSm=13;szMono=13;break; // Consolas - slightly smaller
        case 3:  szBig=23;szMed=16;szSm=15;szMono=13;break; // Arial - slightly bigger
        case 4:  szBig=22;szMed=16;szSm=14;szMono=13;break; // Bahnschrift
        default: szBig=22;szMed=16;szSm=14;szMono=13;break;
    }
    auto mk=[&](int sz,int wt){
        return CreateFont(sz,0,0,0,wt,0,0,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,fn);
    };
    if(hFontBig)   DeleteObject(hFontBig);
    if(hFontMed)   DeleteObject(hFontMed);
    if(hFontSmall) DeleteObject(hFontSmall);
    if(hFontMono)  DeleteObject(hFontMono);
    hFontBig  =mk(szBig, FW_BOLD);
    hFontMed  =mk(szMed, FW_BOLD);
    hFontSmall=mk(szSm,  FW_BOLD);
    hFontMono =CreateFont(szMono,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Consolas");
}

// ===================== LOGO =====================
// Load HICON from embedded PNG bytes using GDI+
// We use CreateIconFromResourceEx which accepts PNG data directly on Win Vista+
void loadAppIcon(){
    // CreateIconFromResourceEx can load PNG icons directly
    gAppIcon = (HICON)CreateIconFromResourceEx(
        (PBYTE)LOGO_32_PNG, LOGO_32_PNG_LEN,
        TRUE, 0x00030000, 32, 32, LR_DEFAULTCOLOR
    );
    if(!gAppIcon){
        // Fallback: load from file if it exists next to exe
        gAppIcon = (HICON)LoadImage(NULL, L"app.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
    }
}

HICON getSmallIcon(){
    HICON small = (HICON)CreateIconFromResourceEx(
        (PBYTE)LOGO_16_PNG, LOGO_16_PNG_LEN,
        TRUE, 0x00030000, 16, 16, LR_DEFAULTCOLOR
    );
    if(!small) small = gAppIcon;
    return small;
}

// ===================== HWID =====================
static unsigned long long hashStr(const std::wstring& s){
    unsigned long long h=14695981039346656037ULL;
    for(wchar_t c:s){h^=(unsigned long long)c;h*=1099511628211ULL;}
    return h;
}
static std::wstring getCPUID(){int i[4]={};__cpuid(i,1);std::wostringstream ss;ss<<std::hex<<i[0]<<i[1]<<i[2]<<i[3];return ss.str();}
static std::wstring getMAC(){IP_ADAPTER_INFO a[16];DWORD b=sizeof(a);if(GetAdaptersInfo(a,&b)!=ERROR_SUCCESS)return L"NOMAC";std::wostringstream ss;for(int i=0;i<6;i++)ss<<std::hex<<std::setw(2)<<std::setfill(L'0')<<a[0].Address[i];return ss.str();}
static std::wstring getVol(){DWORD s=0;GetVolumeInformation(L"C:\\",NULL,0,&s,NULL,NULL,NULL,0);std::wostringstream ss;ss<<std::hex<<s;return ss.str();}
static std::wstring generateHWID(){
    std::wstring raw=getCPUID()+L"-"+getMAC()+L"-"+getVol();
    unsigned long long h=hashStr(raw);
    std::wostringstream ss;ss<<std::uppercase<<std::hex<<std::setw(16)<<std::setfill(L'0')<<h;
    std::wstring r=ss.str();
    return r.substr(0,4)+L"-"+r.substr(4,4)+L"-"+r.substr(8,4)+L"-"+r.substr(12,4);
}

// ===================== HTTP =====================
std::string wtos(const std::wstring& w){return std::string(w.begin(),w.end());}
std::string httpCall(const wchar_t* path,const std::string& body,const std::string& key,const std::string& method){
    HINTERNET hN=InternetOpen(L"Macro/1.0",INTERNET_OPEN_TYPE_DIRECT,NULL,NULL,0);if(!hN)return"";
    HINTERNET hC=InternetConnect(hN,SUPABASE_URL,INTERNET_DEFAULT_HTTPS_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,0);if(!hC){InternetCloseHandle(hN);return"";}
    std::wstring wm(method.begin(),method.end());LPCWSTR t[]={L"application/json",NULL};
    HINTERNET hR=HttpOpenRequest(hC,wm.c_str(),path,NULL,NULL,t,INTERNET_FLAG_SECURE|INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE,0);if(!hR){InternetCloseHandle(hC);InternetCloseHandle(hN);return"";}
    std::string h="Content-Type: application/json\r\napikey: "+key+"\r\nAuthorization: Bearer "+key+"\r\nPrefer: return=representation\r\n";
    HttpSendRequestA(hR,h.c_str(),(DWORD)h.size(),(LPVOID)body.c_str(),(DWORD)body.size());
    std::string resp;char buf[4096];DWORD rd=0;
    while(InternetReadFile(hR,buf,sizeof(buf)-1,&rd)&&rd>0){buf[rd]=0;resp+=buf;rd=0;}
    InternetCloseHandle(hR);InternetCloseHandle(hC);InternetCloseHandle(hN);
    return resp;
}

// GitHub API GET — separate from Supabase httpCall
std::string githubGet(const char* path){
    HINTERNET hN=InternetOpen(L"PulseKPS/1.0",INTERNET_OPEN_TYPE_DIRECT,NULL,NULL,0);if(!hN)return"";
    HINTERNET hC=InternetConnect(hN,L"api.github.com",INTERNET_DEFAULT_HTTPS_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,0);
    if(!hC){InternetCloseHandle(hN);return"";}
    std::wstring wp(path,path+strlen(path));
    HINTERNET hR=HttpOpenRequest(hC,L"GET",wp.c_str(),NULL,NULL,NULL,INTERNET_FLAG_SECURE|INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE,0);
    if(!hR){InternetCloseHandle(hC);InternetCloseHandle(hN);return"";}
    std::string h="User-Agent: PulseKPS/1.0\r\nAccept: application/vnd.github.v3+json\r\n";
    HttpSendRequestA(hR,h.c_str(),(DWORD)h.size(),NULL,0);
    std::string resp;char buf[4096];DWORD rd=0;
    while(InternetReadFile(hR,buf,sizeof(buf)-1,&rd)&&rd>0){buf[rd]=0;resp+=buf;rd=0;}
    InternetCloseHandle(hR);InternetCloseHandle(hC);InternetCloseHandle(hN);
    return resp;
}

// ===================== LICENSE =====================
void saveLicense(const std::wstring& k){std::wofstream f(LICENSE_FILE);if(f.is_open())f<<k;}
std::wstring loadLicense(){std::wifstream f(LICENSE_FILE);if(!f.is_open())return L"";std::wstring k;std::getline(f,k);return k;}

// Returns: -1=network error, 0=invalid, 1=valid+match, 2=valid+just activated
int validateLicense(const std::wstring& key, const std::wstring& hwid) {
    LOG_INFO(L"Checking license...");
    std::wstring path=L"/rest/v1/licenses?license_key=eq."+key+L"&select=license_key,hwid,is_active,trial,expires_at";
    std::string resp=httpCall(path.c_str(),"",SUPABASE_ANON,"GET");
    if (resp.empty()) { LOG_ERR(L"Network error - using cached license"); return -1; }
    if (resp=="[]") { LOG_ERR(L"License key not found"); return 0; }
    if (resp.find("\"is_active\":false")!=std::string::npos) { LOG_ERR(L"License deactivated"); return 0; }

    // Check trial expiry server-side
    bool isTrial=resp.find("\"trial\":true")!=std::string::npos;
    if(isTrial){
        auto ep=resp.find("\"expires_at\":\"");
        if(ep!=std::string::npos){
            ep+=14;
            std::string exStr=resp.substr(ep,resp.find("\"",ep)-ep);
            SYSTEMTIME st;GetSystemTime(&st);
            char now[32];
            snprintf(now,sizeof(now),"%04d-%02d-%02dT%02d:%02d:%02dZ",
                st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
            if(std::string(now)>exStr){LOG_ERR(L"Trial expired - purchase a license");return 4;}
        }
    }

    bool noHWID=resp.find("\"hwid\":null")!=std::string::npos;
    bool match =resp.find("\"hwid\":\""+wtos(hwid)+"\"")!=std::string::npos;
    if(noHWID){
        std::wstring up=L"/rest/v1/licenses?license_key=eq."+key;
        httpCall(up.c_str(),"{\"hwid\":\""+wtos(hwid)+"\",\"activated_at\":\"now()\"}",SERVICE_KEY,"PATCH");
        LOG_OK(isTrial?L"Trial activated":L"License activated");
        return 2;
    }
    if(match){LOG_OK(isTrial?L"Trial valid":L"License valid");return 1;}
    LOG_ERR(L"HWID mismatch - contact support");
    return 3;
}

// ===================== VK STRING =====================
std::wstring vkToString(int vk){
    if(vk>='A'&&vk<='Z')return std::wstring(1,(wchar_t)vk);
    if(vk>='0'&&vk<='9')return std::wstring(1,(wchar_t)vk);
    switch(vk){
        case VK_F1:return L"F1";case VK_F2:return L"F2";case VK_F3:return L"F3";
        case VK_F4:return L"F4";case VK_F5:return L"F5";case VK_F6:return L"F6";
        case VK_F7:return L"F7";case VK_F8:return L"F8";case VK_F9:return L"F9";
        case VK_F10:return L"F10";case VK_F11:return L"F11";case VK_F12:return L"F12";
        case VK_SPACE:return L"Space";case VK_RETURN:return L"Enter";
        case VK_SHIFT:return L"Shift";case VK_CONTROL:return L"Ctrl";
        case VK_MENU:return L"Alt";case VK_TAB:return L"Tab";
        case VK_ESCAPE:return L"Esc";case VK_LEFT:return L"Left";
        case VK_RIGHT:return L"Right";case VK_UP:return L"Up";case VK_DOWN:return L"Down";
        case VK_LBUTTON:return L"M1";case VK_RBUTTON:return L"M2";
        case VK_MBUTTON:return L"M3";case VK_XBUTTON1:return L"M4";case VK_XBUTTON2:return L"M5";
        case VK_BACK:return L"Bksp";case VK_DELETE:return L"Del";case VK_CAPITAL:return L"Caps";
        case VK_NUMPAD0:return L"N0";case VK_NUMPAD1:return L"N1";case VK_NUMPAD2:return L"N2";
        case VK_NUMPAD3:return L"N3";case VK_NUMPAD4:return L"N4";case VK_NUMPAD5:return L"N5";
        case VK_NUMPAD6:return L"N6";case VK_NUMPAD7:return L"N7";
        case VK_NUMPAD8:return L"N8";case VK_NUMPAD9:return L"N9";
        default:{wchar_t b[16];swprintf(b,16,L"VK%d",vk);return b;}
    }
}

// ===================== MACRO =====================
bool checkRoblox(){
    HWND rWnd=NULL;
    EnumWindows([](HWND h,LPARAM lp)->BOOL{
        wchar_t t[256];GetWindowText(h,t,256);
        if(wcsstr(t,L"Roblox")&&IsWindowVisible(h)){*(HWND*)lp=h;return FALSE;}
        return TRUE;
    },(LPARAM)&rWnd);
    if(!rWnd)return false;
    if(GetForegroundWindow()!=rWnd)return false;
    WINDOWPLACEMENT wp={};wp.length=sizeof(wp);
    GetWindowPlacement(rWnd,&wp);
    if(wp.showCmd==SW_SHOWMAXIMIZED)return true;
    HMONITOR hm=MonitorFromWindow(rWnd,MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi={};mi.cbSize=sizeof(mi);GetMonitorInfo(hm,&mi);
    RECT wr;GetWindowRect(rWnd,&wr);
    return(wr.left<=mi.rcMonitor.left&&wr.top<=mi.rcMonitor.top&&
           wr.right>=mi.rcMonitor.right&&wr.bottom>=mi.rcMonitor.bottom);
}

void sendKey(int vk){
    UINT sc=MapVirtualKey(vk,MAPVK_VK_TO_VSC);
    INPUT in[2]={};
    in[0].type=INPUT_KEYBOARD;in[0].ki.wScan=(WORD)sc;in[0].ki.dwFlags=KEYEVENTF_SCANCODE;
    in[1].type=INPUT_KEYBOARD;in[1].ki.wScan=(WORD)sc;in[1].ki.dwFlags=KEYEVENTF_SCANCODE|KEYEVENTF_KEYUP;
    SendInput(2,in,sizeof(INPUT));
    keypressCount++;
}

void focusThread(){while(appRunning){robloxFocused=checkRoblox();Sleep(500);}}

void macroThread(){
    timeBeginPeriod(1);
    SetThreadAffinityMask(GetCurrentThread(),2);
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
    LARGE_INTEGER freq;QueryPerformanceFrequency(&freq);
    LOG_OK(L"Macro engine ready");
    while(appRunning){
        if(macroRunning&&robloxFocused){
            EnterCriticalSection(&keyListCS);
            std::vector<int> keys=keysToSend;
            LeaveCriticalSection(&keyListCS);
            if(!keys.empty()){
                LARGE_INTEGER start;QueryPerformanceCounter(&start);
                for(int vk:keys){if(!macroRunning)break;sendKey(vk);}
                int targetKps=kps.load();
                long long intervalUs=1000000LL/targetKps;
                long long sleepUs=intervalUs-300;
                if(sleepUs>1000)Sleep((DWORD)(sleepUs/1000));
                LARGE_INTEGER now;
                long long target=start.QuadPart+(intervalUs*freq.QuadPart/1000000LL);
                do{_mm_pause();QueryPerformanceCounter(&now);}while(now.QuadPart<target&&macroRunning);
            }else Sleep(10);
        }else{
            // Roblox not active - sleep hard to use almost no CPU
            Sleep(50);
        }
    }
    timeEndPeriod(1);
}

void captureThread(bool isHotkey){
    Sleep(300);
    while(appRunning){
        for(int vk=1;vk<255;vk++){
            // Allow all keys including mouse buttons as hotkey
            if(GetAsyncKeyState(vk)&0x8000){
                if(isHotkey){
                    hotkeyVK=vk;capturingHotkey=false;
                    std::wstring msg=L"Hotkey set to: "+vkToString(vk);
                    LOG_OK(msg.c_str());
                }else{
                    EnterCriticalSection(&keyListCS);
                    keysToSend.push_back(vk);
                    LeaveCriticalSection(&keyListCS);
                    capturingKey=false;
                    std::wstring msg=L"Key added to spam list: "+vkToString(vk);
                    LOG_OK(msg.c_str());
                }
                InvalidateRect(hwndMain,NULL,FALSE);
                return;
            }
        }
        Sleep(10);
    }
}

void hotkeyThread(){
    bool was=false;
    while(appRunning){
        if(!capturingHotkey&&!capturingKey){
            bool focused=robloxFocused.load();
            bool pressed=(GetAsyncKeyState(hotkeyVK.load())&0x8000)!=0;
            if(holdMode){
                bool prev=macroRunning.load();
                macroRunning=pressed&&focused;
                if(!prev&&macroRunning.load()) LOG_OK(L"Macro activated - hold mode engaged");
                else if(prev&&!macroRunning.load()) LOG_INFO(L"Macro deactivated - hold released");
            }else{
                if(pressed&&!was){
                    if(focused){
                        macroRunning=!macroRunning;
                        if(macroRunning.load()) LOG_OK(L"Macro activated - toggle on");
                        else LOG_INFO(L"Macro deactivated - toggle off");
                    }else if(macroRunning.load()){
                        macroRunning=false;
                        LOG_INFO(L"Macro stopped - Roblox is not active or maximised");
                    }
                }
            }
            was=pressed;
            // Only redraw on macro tab - settings handles its own refresh
            if(activeTab==0) InvalidateRect(hwndMain,NULL,FALSE);
            if(hwndOverlay)InvalidateRect(hwndOverlay,NULL,FALSE);
        }
        Sleep(5);
    }
}

// CPU via GetProcessTimes - accurate, no PDH needed
void resourceThread(){
    HANDLE hProc=GetCurrentProcess();
    FILETIME prevIdle,prevKernel,prevUser;
    FILETIME prevPK,prevPU,dummy;
    GetSystemTimes(&prevIdle,&prevKernel,&prevUser);
    GetProcessTimes(hProc,&dummy,&dummy,&prevPK,&prevPU);
    lastKpsTick=GetTickCount();
    LOG_OK(L"Resource monitor started");

    while(appRunning){
        Sleep(1000);

        FILETIME idle,kernel,user,pk,pu;
        GetSystemTimes(&idle,&kernel,&user);
        GetProcessTimes(hProc,&dummy,&dummy,&pk,&pu);

        // Convert to ULARGE_INTEGER for arithmetic
        auto toU=[](FILETIME ft)->unsigned long long{
            ULARGE_INTEGER u;u.LowPart=ft.dwLowDateTime;u.HighPart=ft.dwHighDateTime;return u.QuadPart;
        };

        unsigned long long sysKernel=toU(kernel)-toU(prevKernel);
        unsigned long long sysUser  =toU(user)  -toU(prevUser);
        unsigned long long sysTotal =sysKernel+sysUser;
        unsigned long long procTime =(toU(pk)-toU(prevPK))+(toU(pu)-toU(prevPU));

        if(sysTotal>0)
            cpuUsage=(float)((double)procTime/(double)sysTotal*100.0);
        else
            cpuUsage=0.0f;

        prevKernel=kernel;prevUser=user;prevPK=pk;prevPU=pu;

        PROCESS_MEMORY_COUNTERS pmc={};
        GetProcessMemoryInfo(hProc,&pmc,sizeof(pmc));
        memUsageKB=pmc.WorkingSetSize/1024;

        // Actual KPS
        DWORD now=GetTickCount();
        DWORD elapsed=now-lastKpsTick;
        if(elapsed>=1000){
            int count=keypressCount.exchange(0);
            actualKPS=(int)(count*1000.0f/elapsed);
            lastKpsTick=now;
        }

        if(hwndOverlay)InvalidateRect(hwndOverlay,NULL,FALSE);
    }
}

// ===================== BENCHMARK =====================
int benchmarkSystem(){
    LOG_INFO(L"Benchmarking your system - measuring SendInput throughput...");
    LARGE_INTEGER freq,start,end;
    QueryPerformanceFrequency(&freq);
    INPUT dummy[2]={};
    dummy[0].type=INPUT_KEYBOARD;dummy[0].ki.dwFlags=KEYEVENTF_KEYUP;
    dummy[1]=dummy[0];
    for(int i=0;i<100;i++)SendInput(2,dummy,sizeof(INPUT));
    const int trials=500;
    QueryPerformanceCounter(&start);
    for(int i=0;i<trials;i++)SendInput(2,dummy,sizeof(INPUT));
    QueryPerformanceCounter(&end);
    double totalUs=((double)(end.QuadPart-start.QuadPart)/freq.QuadPart)*1000000.0;
    double perCallUs=totalUs/trials;
    int maxTheo=(int)(1000000.0/perCallUs);
    int recommended=(int)(maxTheo*0.4);
    recommended=std::max(20,std::min(500,recommended));
    wchar_t buf[256];
    swprintf(buf,256,L"SendInput avg: %.1f us/call | Theoretical max: %d KPS | Recommended (40%%): %d KPS",
        perCallUs,maxTheo,recommended);
    LOG_INFO(buf);
    swprintf(buf,256,L"KPS auto-set to recommended value: %d",recommended);
    LOG_OK(buf);
    return recommended;
}

// ===================== MEMORY CLEANER =====================
std::wstring cleanRamStatus = L"";
bool licCopied = false;
DWORD licCopiedTick = 0;

// NtSetSystemInformation for standby list flush
typedef LONG(WINAPI*NtSetSysInfo_t)(UINT,PVOID,ULONG);

void cleanMemory(){
    auto logAndRefresh=[](const wchar_t* t,const wchar_t* m,const wchar_t* status){
        addLog(t,m);
        cleanRamStatus=std::wstring(status);
        // Settings now inline — invalidate main window
        InvalidateRect(hwndMain,NULL,FALSE);
    };

    logAndRefresh(L"INFO",L"Starting system-wide RAM cleanup...",L"Starting...");

    // Measure system RAM before
    MEMORYSTATUSEX ms={};ms.dwLength=sizeof(ms);GlobalMemoryStatusEx(&ms);
    SIZE_T beforeFree=ms.ullAvailPhys/1024;
    SIZE_T totalKB=ms.ullTotalPhys/1024;
    wchar_t buf[256];
    swprintf(buf,256,L"System RAM: %zu KB free of %zu KB total",beforeFree,totalKB);
    logAndRefresh(L"INFO",buf,L"Measuring system RAM...");
    Sleep(60);

    // Step 1: Trim working sets of ALL running processes
    logAndRefresh(L"INFO",L"Step 1/4: Trimming working sets of all processes...",L"Step 1/4: Trimming all processes...");
    // Run at below-normal priority so GDI/paint resources aren't starved
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
    {
        DWORD ownPid=GetCurrentProcessId(); // exclude ourselves to prevent GDI flush
        HANDLE hSnap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
        if(hSnap!=INVALID_HANDLE_VALUE){
            PROCESSENTRY32 pe={};pe.dwSize=sizeof(pe);
            int trimmed=0;
            if(Process32First(hSnap,&pe)){
                do{
                    if(pe.th32ProcessID==ownPid) continue; // skip self
                    HANDLE hProc=OpenProcess(PROCESS_SET_QUOTA|PROCESS_QUERY_INFORMATION,FALSE,pe.th32ProcessID);
                    if(hProc){
                        EmptyWorkingSet(hProc);
                        SetProcessWorkingSetSize(hProc,(SIZE_T)-1,(SIZE_T)-1);
                        CloseHandle(hProc);
                        trimmed++;
                        // Small yield every 10 processes so UI stays responsive
                        if(trimmed%10==0) Sleep(1);
                    }
                }while(Process32Next(hSnap,&pe));
            }
            CloseHandle(hSnap);
            swprintf(buf,256,L"Trimmed working sets of %d processes",trimmed);
            logAndRefresh(L"OK",buf,L"Step 1/4: Done");
        }
    }
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL);
    Sleep(60);

    // Step 2: Flush standby list (frees cached pages Windows holds speculatively)
    // Requires SE_PROF_SINGLE_PROCESS_NAME privilege
    logAndRefresh(L"INFO",L"Step 2/4: Flushing system standby memory list...",L"Step 2/4: Flushing standby list...");
    {
        // Enable SeProfileSingleProcessPrivilege
        HANDLE hToken=NULL;
        if(OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken)){
            TOKEN_PRIVILEGES tp={};
            LookupPrivilegeValue(NULL,SE_PROF_SINGLE_PROCESS_NAME,&tp.Privileges[0].Luid);
            tp.PrivilegeCount=1;
            tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken,FALSE,&tp,0,NULL,NULL);
            CloseHandle(hToken);
        }
        // NtSetSystemInformation with SystemMemoryListInformation (80)
        // Command 4 = MemoryPurgeStandbyList
        NtSetSysInfo_t NtSetSI=(NtSetSysInfo_t)GetProcAddress(GetModuleHandle(L"ntdll.dll"),"NtSetSystemInformation");
        if(NtSetSI){
            UINT cmd=4; // MemoryPurgeStandbyList
            LONG r=NtSetSI(80,&cmd,sizeof(cmd));
            if(r==0)logAndRefresh(L"OK",L"Standby list flushed successfully",L"Step 2/4: Done");
            else{
                swprintf(buf,256,L"Standby flush returned 0x%lX - may need admin rights",r);
                logAndRefresh(L"INFO",buf,L"Step 2/4: Partial (needs admin)");
            }
        } else {
            logAndRefresh(L"INFO",L"NtSetSystemInformation unavailable on this system",L"Step 2/4: Skipped");
        }
    }
    Sleep(60);

    // Step 3: Flush modified page list (dirty pages waiting to be written)
    logAndRefresh(L"INFO",L"Step 3/4: Flushing modified page list...",L"Step 3/4: Flushing modified pages...");
    {
        NtSetSysInfo_t NtSetSI=(NtSetSysInfo_t)GetProcAddress(GetModuleHandle(L"ntdll.dll"),"NtSetSystemInformation");
        if(NtSetSI){
            UINT cmd=3; // MemoryFlushModifiedList
            NtSetSI(80,&cmd,sizeof(cmd));
        }
        logAndRefresh(L"OK",L"Modified page list flushed",L"Step 3/4: Done");
    }
    Sleep(60);

    // Step 4: Final measure
    logAndRefresh(L"INFO",L"Step 4/4: Measuring results...",L"Step 4/4: Measuring results...");
    GlobalMemoryStatusEx(&ms);
    SIZE_T afterFree=ms.ullAvailPhys/1024;
    SIZE_T freed=afterFree>beforeFree?afterFree-beforeFree:0;
    swprintf(buf,256,L"Freed %zu KB - RAM free: %zu KB -> %zu KB",freed,beforeFree,afterFree);
    logAndRefresh(L"OK",buf,buf);
}

// ===================== DRAW HELPERS =====================
void fillRect(HDC hdc,int x,int y,int w,int h,COLORREF c){
    RECT r={x,y,x+w,y+h};HBRUSH b=CreateSolidBrush(c);FillRect(hdc,&r,b);DeleteObject(b);
}
void drawRR(HDC hdc,int x,int y,int w,int h,int r,COLORREF fill,COLORREF border=0,int bw=0,float hov=0.0f){
    // Hover lift: shift up 1px and draw drop shadow
    if(hov>0.01f){
        int lift=(int)(hov*2);
        // Shadow
        HRGN sr=CreateRoundRectRgn(x+2,y+lift+2,x+w+2,y+h+lift+2,r,r);
        HBRUSH sb=CreateSolidBrush(RGB(0,0,0));
        HRGN cr2=CreateRectRgn(0,0,10000,10000);
        // Simple shadow via offset fill
        HBRUSH shadow=CreateSolidBrush(RGB(4,4,8));
        FillRgn(hdc,sr,shadow);DeleteObject(shadow);DeleteObject(sr);DeleteObject(cr2);
        y-=lift;
    }
    HRGN rgn=CreateRoundRectRgn(x,y,x+w,y+h,r,r);
    HBRUSH br=CreateSolidBrush(fill);FillRgn(hdc,rgn,br);DeleteObject(br);
    if(bw>0){
        HPEN pen=CreatePen(PS_SOLID,bw,border);HPEN op=(HPEN)SelectObject(hdc,pen);
        HBRUSH nb=(HBRUSH)GetStockObject(NULL_BRUSH);HBRUSH ob=(HBRUSH)SelectObject(hdc,nb);
        RoundRect(hdc,x,y,x+w,y+h,r,r);
        SelectObject(hdc,op);SelectObject(hdc,ob);DeleteObject(pen);
    }
    DeleteObject(rgn);
}
void drawText(HDC hdc,const wchar_t* text,int x,int y,int w,int h,
              COLORREF c,HFONT font,UINT fmt=DT_LEFT|DT_VCENTER|DT_SINGLELINE){
    SelectObject(hdc,font);SetTextColor(hdc,c);SetBkMode(hdc,TRANSPARENT);
    RECT r={x,y,x+w,y+h};DrawText(hdc,text,-1,&r,fmt);
}
void drawCard(HDC hdc,int x,int y,int w,int h){
    // Subtle shadow effect - slightly lighter inner, no visible border
    drawRR(hdc,x,y,w,h,14,T.card,RGB(0,0,0),0);
    // Thin top highlight line for depth
    HPEN hp=CreatePen(PS_SOLID,1,RGB(
        std::min(255,GetRValue(T.card)+12),
        std::min(255,GetGValue(T.card)+12),
        std::min(255,GetBValue(T.card)+12)));
    HPEN op=(HPEN)SelectObject(hdc,hp);
    MoveToEx(hdc,x+14,y+1,NULL);LineTo(hdc,x+w-14,y+1);
    SelectObject(hdc,op);DeleteObject(hp);
}
void drawDot(HDC hdc,int cx,int cy,int r,COLORREF c){
    HBRUSH b=CreateSolidBrush(c);HPEN p=CreatePen(PS_SOLID,1,c);
    HBRUSH ob=(HBRUSH)SelectObject(hdc,b);HPEN op=(HPEN)SelectObject(hdc,p);
    Ellipse(hdc,cx-r,cy-r,cx+r,cy+r);
    SelectObject(hdc,ob);SelectObject(hdc,op);DeleteObject(b);DeleteObject(p);
}
void drawToggle(HDC hdc,int x,int y,bool on){
    int W=44,H=22;
    drawRR(hdc,x,y,W,H,12,on?T.accent:T.btn,T.border,1);
    int kx=on?x+W-H+2:x+2;
    drawRR(hdc,kx,y+2,H-4,H-4,H-4,T.text);
}
void drawSlider(HDC hdc,int x,int y,int w,int val,int minV,int maxV){
    int H=6,KW=20,KH=20;
    int ty=y+(KH-H)/2;
    float pct=(float)(val-minV)/(maxV-minV);
    pct=std::max(0.0f,std::min(1.0f,pct));
    int filled=(int)(pct*(w-KW));
    drawRR(hdc,x+KW/2,ty,w-KW,H,3,T.btn);
    if(filled>0)drawRR(hdc,x+KW/2,ty,filled,H,3,T.accent);
    int kx=x+(int)(pct*(w-KW));
    drawRR(hdc,kx-2,y-2,KW+4,KH+4,KW+4,
        RGB(GetRValue(T.accent)/5,GetGValue(T.accent)/5,GetBValue(T.accent)/5));
    drawRR(hdc,kx,y,KW,KH,KW,T.text,T.accent,2);
}

// ===================== LAYOUT =====================
struct Layout{int W,pad,cw,yStatus,yHotkey,yMode,yKeys,yKps;};
Layout getLayout(HWND hwnd){
    RECT cr;GetClientRect(hwnd,&cr);
    Layout l;l.W=cr.right;l.pad=14;l.cw=l.W-l.pad*2;
    int off=-mainScrollPos; // scroll offset
    l.yStatus      =70 +off;
    l.yHotkey      =l.yStatus +74;
    l.yMode        =l.yHotkey +68;
    l.yKeys        =l.yMode   +68;
    l.yKps         =l.yKeys   +108;
    mainTotalHeight=l.yKps+102+mainScrollPos; // last card + padding
    return l;
}

int hitTest(HWND hwnd,int mx,int my){
    Layout l=getLayout(hwnd);int p=l.pad;
    if(mx>=p+108&&mx<=p+224&&my>=l.yHotkey+22&&my<=l.yHotkey+50) return ID_SET_HOTKEY;
    if(mx>=p+14 &&mx<=p+112&&my>=l.yMode+22  &&my<=l.yMode+50)   return ID_MODE_TOGGLE;
    if(mx>=p+120&&mx<=p+218&&my>=l.yMode+22  &&my<=l.yMode+50)   return ID_MODE_HOLD;
    if(mx>=p+14 &&mx<=p+50 &&my>=l.yKeys+66  &&my<=l.yKeys+90)   return ID_ADD_KEY;
    if(mx>=p+56 &&mx<=p+92 &&my>=l.yKeys+66  &&my<=l.yKeys+90)   return ID_REMOVE_KEY;
    int pw=44,pg=5,px=p+14;
    for(int i=0;i<KPS_PRESET_COUNT;i++){
        if(mx>=px&&mx<=px+pw&&my>=l.yKps+56&&my<=l.yKps+78)return 500+i;
        px+=pw+pg;
    }
    return 0;
}
bool isInSlider(HWND hwnd,int mx,int my){
    Layout l=getLayout(hwnd);
    return(my>=l.yKps+20&&my<=l.yKps+50&&mx>=l.pad+14&&mx<=l.W-l.pad-14);
}
int sliderVal(HWND hwnd,int mx){
    Layout l=getLayout(hwnd);
    int sx=l.pad+14,sw=l.cw-28,KW=20;
    float pct=(float)(mx-sx-KW/2)/(sw-KW);
    pct=std::max(0.0f,std::min(1.0f,pct));
    // Snap to multiples of 5
    int raw=MIN_KPS+(int)(pct*(MAX_KPS-MIN_KPS));
    return std::max(MIN_KPS,std::min(MAX_KPS,(raw/5)*5));
}

// ===================== PAINT MAIN =====================
// Forward declarations
void drawToasts(HDC hdc,int W,int H);
void paintSettingsInto(HDC hdc,int W,int H);

void paintMain(HWND hwnd){
    PAINTSTRUCT ps;HDC hdc_real=BeginPaint(hwnd,&ps);
    RECT cr;GetClientRect(hwnd,&cr);int W=cr.right,H=cr.bottom;
    HDC hdc=CreateCompatibleDC(hdc_real);
    HBITMAP bmp=CreateCompatibleBitmap(hdc_real,W,H);
    HBITMAP obmp=(HBITMAP)SelectObject(hdc,bmp);

    Layout l=getLayout(hwnd);int p=l.pad,cw=l.cw;

    // Update scrollbar range now that mainTotalHeight is computed
    {SCROLLINFO si={};si.cbSize=sizeof(si);si.fMask=SIF_RANGE|SIF_PAGE|SIF_POS;
    si.nMin=0;
    // nMax = total content height so scrollbar thumb size is proportional
    // Content height without scroll offset = mainTotalHeight - mainScrollPos (because layout adds it back)
    int contentH=mainTotalHeight-mainScrollPos;
    si.nMax=std::max(H,(int)contentH)-1;
    si.nPage=(UINT)H;
    si.nPos=mainScrollPos;
    SetScrollInfo(hwnd,SB_VERT,&si,TRUE);}

    // Compute fade-driven Y offset - elements float down into place
    int fadeY=(int)((1.0f-easeOut(fadeAlpha))*18.0f);

    fillRect(hdc,0,0,W,H,T.bg);

    if(activeTab==0){
    // ── TITLE BAR (macro tab) ─────────────────────────────
    fillRect(hdc,0,0,W,56,T.surface);
    fillRect(hdc,0,55,W,1,RGB(GetRValue(T.accent)/3,GetGValue(T.accent)/3,GetBValue(T.accent)/3));
    if(gAppIcon) DrawIconEx(hdc,14,12,gAppIcon,28,28,0,NULL,DI_NORMAL);
    drawText(hdc,L"PULSEKPS",gAppIcon?52:16,0,120,56,T.text,hFontBig,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawText(hdc,APP_VERSION,W-84,0,70,56,T.subtext,hFontSmall,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);

    int y=l.yStatus;

    // STATUS CARD
    drawCard(hdc,p,y,cw,58);
    bool running=macroRunning.load();
    // Smooth status colour transition using animRunning
    float ar=easeInOut(animRunning);
    COLORREF dotCol=lerpCol(T.red,T.green,ar);
    COLORREF textCol=lerpCol(T.red,T.green,ar);
    // Pulse glow when running
    if(animRunning>0.5f){
        int glowR=(int)(10+4*animMacroStatus);
        COLORREF glowCol=RGB(
            (int)(GetRValue(T.green)*animMacroStatus*0.3f),
            (int)(GetGValue(T.green)*animMacroStatus*0.3f),
            (int)(GetBValue(T.green)*animMacroStatus*0.3f)
        );
        drawDot(hdc,p+26,y+29,glowR,glowCol);
    }
    drawDot(hdc,p+26,y+29,7,dotCol);
    // Status text fades between STOPPED and RUNNING
    drawText(hdc,ar>0.5f?L"RUNNING":L"STOPPED",p+42,y,130,58,textCol,hFontMed,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    y=l.yHotkey;

    // ---- HOTKEY CARD ----
    drawCard(hdc,p,y,cw,62);
    drawText(hdc,L"HOTKEY",p+16,y+10,100,11,T.subtext,hFontSmall);
    std::wstring hkStr=capturingHotkey?L"...":vkToString(hotkeyVK.load());
    // Large key badge
    drawRR(hdc,p+16,y+26,72,26,8,T.btn,T.border,1);
    drawText(hdc,hkStr.c_str(),p+16,y+26,72,26,T.accent,hFontMed,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    // Set button - pill shaped
    float hkHov=getHoverAlpha(ID_SET_HOTKEY);
    COLORREF hkBg=capturingHotkey?T.accent:lerpCol(T.btn,T.btnHov,hkHov);
    drawRR(hdc,p+96,y+26,110,26,14,hkBg,RGB(0,0,0),0);
    drawText(hdc,capturingHotkey?L"Listening...":L"Set Hotkey",p+96,y+26,110,26,capturingHotkey?T.bg:lerpCol(T.subtext,T.text,hkHov),hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    y=l.yMode;

    // MODE CARD
    drawCard(hdc,p,y,cw,58);
    drawText(hdc,L"MODE",p+16,y+10,200,14,T.subtext,hFontSmall);
    bool isHold=holdMode.load();
    // Smooth mode buttons using animModeHold (0=Toggle active, 1=Hold active)
    float mh=easeInOut(animModeHold);
    COLORREF togBg=lerpCol(T.accent,lerpCol(T.btn,T.btnHov,getHoverAlpha(ID_MODE_TOGGLE)),mh);
    COLORREF holBg=lerpCol(lerpCol(T.btn,T.btnHov,getHoverAlpha(ID_MODE_HOLD)),T.accent,mh);
    COLORREF togBorder=lerpCol(T.accent,T.border,mh);
    COLORREF holBorder=lerpCol(T.border,T.accent,mh);
    COLORREF togText=lerpCol(T.text,T.subtext,mh);
    COLORREF holText=lerpCol(T.subtext,T.text,mh);
    float togH=getHoverAlpha(ID_MODE_TOGGLE),holH=getHoverAlpha(ID_MODE_HOLD);
    drawRR(hdc,p+16, y+26,96,26,14,togBg,RGB(0,0,0),0,!isHold?0.0f:togH);
    drawText(hdc,L"Toggle",p+16, y+26,96,26,togText,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    drawRR(hdc,p+120,y+26,96,26,14,holBg,RGB(0,0,0),0,isHold?0.0f:holH);
    drawText(hdc,L"Hold",  p+120,y+26,96,26,holText,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    y=l.yKeys;

    // KEYS CARD
    drawCard(hdc,p,y,cw,98);
    drawText(hdc,L"KEYS TO SPAM",p+16,y+10,200,14,T.subtext,hFontSmall);
    EnterCriticalSection(&keyListCS);
    std::vector<int> keys=keysToSend;
    LeaveCriticalSection(&keyListCS);
    int kx=p+14,ky=y+24;
    if(keys.empty()){
        drawText(hdc,L"No keys - tap + to add",kx,ky,cw-28,24,T.subtext,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    }else{
        for(int vk:keys){
            drawRR(hdc,kx,ky,50,22,6,T.btn,T.border,1);
            drawText(hdc,vkToString(vk).c_str(),kx,ky,50,22,T.text,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            kx+=56;if(kx>W-p-70){kx=p+14;ky+=26;}
        }
    }
    float addHov=getHoverAlpha(ID_ADD_KEY);
    float remHov=getHoverAlpha(ID_REMOVE_KEY);
    drawRR(hdc,p+16,y+68,32,22,12,lerpCol(T.btn,T.btnHov,addHov),RGB(0,0,0),0);
    drawText(hdc,capturingKey?L"...":L"+",p+16,y+68,32,22,lerpCol(T.subtext,T.accent,addHov),hFontMed,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    drawRR(hdc,p+54,y+68,32,22,12,lerpCol(T.btn,T.btnHov,remHov),RGB(0,0,0),0);
    drawText(hdc,L"−",p+54,y+68,32,22,lerpCol(T.subtext,T.text,remHov),hFontMed,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    y=l.yKps;

    // KPS CARD - presets inside
    drawCard(hdc,p,y,cw,90);
    wchar_t kpsBuf[32];swprintf(kpsBuf,32,L"%d KPS",kps.load());
    drawText(hdc,L"SPEED",p+16,y+10,60,12,T.subtext,hFontSmall);
    drawText(hdc,kpsBuf,p+76,y+6,140,18,T.accent,hFontMed);
    drawSlider(hdc,p+16,y+30,cw-32,kps.load(),MIN_KPS,MAX_KPS);
    // Presets inside card
    int pw=44,pg=5,px=p+14;
    for(int i=0;i<KPS_PRESET_COUNT;i++){
        bool active=(kps.load()==KPS_PRESETS[i]);
        float hov=getHoverAlpha(500+i);
        COLORREF pbg=active?T.accent:lerpCol(T.btn,T.btnHov,hov);
        COLORREF pbd=active?T.accent:lerpCol(T.border,T.accent,hov*0.5f);
        COLORREF ptx=active?T.text:lerpCol(T.subtext,T.text,hov);
        drawRR(hdc,px,y+58,pw,20,12,pbg,RGB(0,0,0),0,hov);
        wchar_t pb[8];
        if(KPS_PRESETS[i]>=1000)swprintf(pb,8,L"%dk",KPS_PRESETS[i]/1000);
        else swprintf(pb,8,L"%d",KPS_PRESETS[i]);
        drawText(hdc,pb,px,y+56,pw,22,ptx,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        px+=pw+pg;
    }

    } // end activeTab==0

    // ── SETTINGS TAB CONTENT ─────────────────────────────────
    if(activeTab==1){
        paintSettingsInto(hdc,W,H-DOCK_H);
    }

    // ── BOTTOM DOCK ───────────────────────────────────────────
    {int dy=H-DOCK_H;
    fillRect(hdc,0,dy,W,DOCK_H,T.surface);
    fillRect(hdc,0,dy,W,1,T.border);
    int tw=W/2;
    float mA=easeInOut(1.0f-tabAnim);
    float sA=easeInOut(tabAnim);
    fillRect(hdc,0,dy+1,tw,DOCK_H-1,lerpCol(T.surface,T.card,mA*0.7f));
    {int bw=(int)(mA*(tw-24));if(bw>0)fillRect(hdc,(tw-bw)/2,dy+DOCK_H-3,bw,3,T.accent);}
    drawText(hdc,L"PulseKPS",0,dy,tw,DOCK_H,lerpCol(T.subtext,T.text,mA),hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    fillRect(hdc,tw,dy+1,tw,DOCK_H-1,lerpCol(T.surface,T.card,sA*0.7f));
    {int bw=(int)(sA*(tw-24));if(bw>0)fillRect(hdc,tw+(tw-bw)/2,dy+DOCK_H-3,bw,3,T.accent);}
    drawText(hdc,L"Settings",tw,dy,tw,DOCK_H,lerpCol(T.subtext,T.text,sA),hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    fillRect(hdc,tw,dy+10,1,DOCK_H-20,T.border);}

    drawToasts(hdc,W,H);

    BitBlt(hdc_real,0,0,W,H,hdc,0,0,SRCCOPY);
    SelectObject(hdc,obmp);
    DeleteObject(bmp);DeleteDC(hdc);
    EndPaint(hwnd,&ps);
}

// ===================== SETTINGS LAYOUT =====================
struct SLayout{int W,pad,cw,yRes,yLic,yFont,yFontDrop,yTheme,yAutoLaunch,yMinimise,yUpdate,yTrial;};
SLayout getSLayout(HWND hwnd){
    RECT cr;GetClientRect(hwnd,&cr);
    SLayout l;l.W=cr.right;l.pad=14;l.cw=l.W-l.pad*2-16; // -16 for scrollbar
    int y=56-settScrollPos; // start below fixed 48px header + gap
    // Resources card includes overlay toggles + utility buttons
    bool hasStatusL=!cleanRamStatus.empty();
    int resH=222+(hasStatusL?28:0);
    l.yRes       =y;      y+=resH+8;
    l.yLic       =y;      y+=(licCopied?54:40)+8;
    l.yFont      =y;      y+=52+8;
    l.yFontDrop  =l.yFont+52;
    if(fontDropOpen) y+=5*22;
    l.yTheme     =y;      y+=74+8;
    l.yAutoLaunch=y;      y+=58+8;
    l.yMinimise  =y;      y+=58+8;
    l.yUpdate    =y;      y+=58+8;
    l.yTrial     =y;      if(trialMode){y+=52+8;}

    // Store total content height for scrollbar
    settTotalHeight=y+settScrollPos+8;
    return l;
}

// ===================== PAINT SETTINGS =====================
void paintSettings(HWND hwnd){
    PAINTSTRUCT ps;HDC hdc_real=BeginPaint(hwnd,&ps);
    RECT cr;GetClientRect(hwnd,&cr);int W=cr.right,H=cr.bottom;
    HDC hdc=CreateCompatibleDC(hdc_real);
    HBITMAP bmp=CreateCompatibleBitmap(hdc_real,W,H);
    HBITMAP obmp=(HBITMAP)SelectObject(hdc,bmp);
    SLayout l=getSLayout(hwnd);int p=l.pad,cw=l.cw;

    fillRect(hdc,0,0,W,H,T.bg);
    // Fixed title bar (not scrolled)
    fillRect(hdc,0,0,W,44,T.surface);
    fillRect(hdc,0,43,W,2,T.accent);
    drawText(hdc,L"SETTINGS",p,0,200,44,T.text,hFontBig,DT_LEFT|DT_VCENTER|DT_SINGLELINE);

    // Update scrollbar
    SCROLLINFO si={};si.cbSize=sizeof(si);si.fMask=SIF_ALL;
    si.nMin=0;si.nMax=std::max(0,settTotalHeight-H+44);
    si.nPage=H-44;si.nPos=settScrollPos;
    SetScrollInfo(hwnd,SB_VERT,&si,TRUE);

    // Offset all cards by fade amount
    int sfadeY=(int)((1.0f-easeOut(settFadeAlpha))*16.0f);
    int y=l.yRes+sfadeY;

    // ── RESOURCES + OVERLAYS + UTILITY BUTTONS (unified card) ──
    bool hasStatus2=!cleanRamStatus.empty();
    // Heights: header=24 cpu=22 ram=22 bar=13 resToggle=28 kpsToggle=28 divider=9 btn1=34 btn2=34 pad=8 = 222; +status row 28
    int resCardH=222+(hasStatus2?28:0);
    drawCard(hdc,p,y,cw,resCardH);
    // Header + stats
    drawText(hdc,L"RESOURCES",p+14,y+8,200,14,T.subtext,hFontSmall);
    wchar_t cpuBuf[64],memBuf[64];
    swprintf(cpuBuf,64,L"CPU:  %.2f%%",cpuUsage);
    swprintf(memBuf,64,L"RAM:  %.1f MB",(float)memUsageKB/1024.0f);
    drawText(hdc,cpuBuf,p+14,y+26,cw-28,18,T.text,hFontMed,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawText(hdc,memBuf,p+14,y+48,cw-28,18,T.text,hFontMed,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawRR(hdc,p+14,y+70,cw-28,5,3,T.btn);
    {int cf2=(int)(std::min(cpuUsage/100.0f,1.0f)*(cw-28));
    if(cf2>0)drawRR(hdc,p+14,y+70,cf2,5,3,T.accent);}
    // Overlay toggles
    drawText(hdc,L"Resource Overlay",p+14,y+84,200,20,T.subtext,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawToggle(hdc,p+cw-48,y+84,resOverlayEnabled);
    drawText(hdc,L"KPS Overlay",p+14,y+112,200,20,T.subtext,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawToggle(hdc,p+cw-48,y+112,kpsOverlayEnabled);
    // Divider
    fillRect(hdc,p+14,y+140,cw-28,1,T.border);
    // Utility buttons 2x2
    {int halfW2=(cw-28-8)/2;
    int bx1=p+14,bx2=p+14+halfW2+8;
    int r1y=y+148,r2y=y+182;
    drawRR(hdc,bx1,r1y,halfW2,28,8,T.btn,T.border,1);
    drawText(hdc,L"Clean RAM",bx1,r1y,halfW2,28,T.text,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    drawRR(hdc,bx2,r1y,halfW2,28,8,T.btn,T.border,1);
    drawText(hdc,L"Open Logs",bx2,r1y,halfW2,28,T.text,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    drawRR(hdc,bx1,r2y,halfW2,28,8,T.btn,T.border,1);
    drawText(hdc,L"Export CFG",bx1,r2y,halfW2,28,T.text,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    drawRR(hdc,bx2,r2y,halfW2,28,8,T.btn,T.border,1);
    drawText(hdc,L"Import CFG",bx2,r2y,halfW2,28,T.text,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    if(hasStatus2){
        bool done2=cleanRamStatus.find(L"Freed")!=std::wstring::npos;
        COLORREF sc2=done2?T.green:T.accent;
        std::wstring sl2=(done2?L"+ ":L"~ ")+cleanRamStatus;
        SelectObject(hdc,hFontMono);SetTextColor(hdc,sc2);SetBkMode(hdc,TRANSPARENT);
        RECT sr2={p+14,y+216,p+14+cw-28,y+216+26};
        DrawText(hdc,sl2.c_str(),-1,&sr2,DT_LEFT|DT_TOP|DT_WORDBREAK);
    }}
    y=l.yLic;

    // License card
    // License card - taller to fit copy success message
    int licH = licCopied ? 54 : 40;
    drawCard(hdc,p,y,cw,licH);
    drawText(hdc,L"LICENSE",p+14,y+10,100,12,T.subtext,hFontSmall);
    std::wstring dk;
    if(savedLicenseKey.empty()) dk=L"No license key saved";
    else dk=keyVisible?savedLicenseKey:std::wstring(savedLicenseKey.size(),L'*');
    drawText(hdc,dk.c_str(),p+14,y+18,cw-106,18,T.text,hFontMono,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    // SHOW button
    drawRR(hdc,p+cw-92,y+10,40,20,6,T.btn,T.border,1);
    drawText(hdc,keyVisible?L"HIDE":L"SHOW",p+cw-92,y+10,40,20,T.subtext,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    COLORREF copyCol=licCopied?T.green:T.subtext;
    drawRR(hdc,p+cw-46,y+10,40,20,6,licCopied?RGB(20,60,30):T.btn,licCopied?T.green:T.border,1);
    drawText(hdc,licCopied?L"\u2713":L"COPY",p+cw-46,y+10,40,20,copyCol,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    // Copied confirmation message
    if(licCopied){
        drawText(hdc,L"Copied to clipboard",p+14,y+36,cw-28,14,T.green,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    }
    y=l.yFont;

    // Font card
    drawCard(hdc,p,y,cw,40);
    drawText(hdc,L"FONT",p+14,y+10,60,12,T.subtext,hFontSmall);
    drawRR(hdc,p+14,y+18,cw-28,18,4,T.btn,T.border,1);
    drawText(hdc,FONT_NAMES[fontIdx],p+18,y+18,cw-50,18,T.text,hFontMed,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawText(hdc,fontDropOpen?L"\u25B2":L"\u25BC",W-p-26,y+18,16,18,T.subtext,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    if(fontDropOpen){
        int dy=y+40;
        drawRR(hdc,p+14,dy,cw-28,5*22+4,6,T.card,T.border,1);
        for(int i=0;i<5;i++){
            bool hov=(fontDropHov==i),sel=(fontIdx==i);
            if(sel||hov)drawRR(hdc,p+16,dy+2+i*22,cw-32,20,4,sel?T.accent:T.btnHov);
            drawText(hdc,FONT_NAMES[i],p+22,dy+2+i*22,cw-40,20,sel?T.text:T.subtext,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        }
    }
    y=l.yTheme;

    // Theme card
    drawCard(hdc,p,y,cw,62);
    drawText(hdc,L"COLOUR THEME",p+14,y+10,200,12,T.subtext,hFontSmall);
    int bw=52,gap=5,tx=p+14;
    for(int i=0;i<6;i++){
        bool active=(i==themeIdx);
        drawRR(hdc,tx,y+26,bw,26,6,active?T.accent:T.btn,active?T.accent:T.border,1);
        drawText(hdc,THEMES[i].name,tx,y+26,bw,26,active?T.text:T.subtext,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        tx+=bw+gap;
    }
    y=l.yAutoLaunch+sfadeY;
    drawCard(hdc,p,y,cw,54);
    drawText(hdc,L"START WITH WINDOWS",p+14,y+10,300,12,T.subtext,hFontSmall);
    drawText(hdc,L"Launch macro automatically with Windows",p+14,y+28,cw-70,16,T.text,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawToggle(hdc,p+cw-48,y+16,autoLaunchEnabled);

    // Minimise to tray card
    // Minimise to tray card
    {int mY=l.yMinimise+sfadeY;
    drawCard(hdc,p,mY,cw,54);
    drawText(hdc,L"MINIMISE TO TRAY",p+14,mY+8,300,12,T.subtext,hFontSmall);
    drawText(hdc,L"Minimise to tray instead of closing",p+14,mY+24,cw-70,18,T.text,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawToggle(hdc,p+cw-48,mY+16,minimiseToTray);}

    y=l.yUpdate+sfadeY;
    drawCard(hdc,p,y,cw,54);
    drawText(hdc,L"UPDATE",p+14,y+10,200,12,T.subtext,hFontSmall);
    std::wstring updateStr=latestVersion.empty()?L"Click to check when update server is configured":
        (latestVersion==std::wstring(APP_VERSION)?L"Up to date - "+std::wstring(APP_VERSION):
        (latestVersion.find(L"404")!=std::wstring::npos||latestVersion.find(L"Not Found")!=std::wstring::npos)?
        L"Update server not configured yet":L"Update available: "+latestVersion);
    COLORREF uc=latestVersion.empty()?T.subtext:
        (latestVersion==std::wstring(APP_VERSION)?T.green:
        (latestVersion.find(L"404")!=std::wstring::npos||latestVersion.find(L"Not Found")!=std::wstring::npos)?T.subtext:T.accent);
    drawText(hdc,updateStr.c_str(),p+14,y+28,cw-28,16,uc,hFontMed,DT_LEFT|DT_VCENTER|DT_SINGLELINE);

    if(trialMode){
        y=l.yTrial+sfadeY;
        drawCard(hdc,p,y,cw,48);
        drawText(hdc,L"TRIAL",p+14,y+10,200,12,T.subtext,hFontSmall);
        wchar_t tb2[96];
        if(trialDaysLeft>0)swprintf(tb2,96,L"%d day(s) remaining - purchase a license to unlock",trialDaysLeft);
        else swprintf(tb2,96,L"Trial expired - enter a license key to continue");
        COLORREF tc=trialDaysLeft>0?T.accent:T.red;
        drawText(hdc,tb2,p+14,y+24,cw-28,18,tc,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    }

    // Utility buttons now merged into resources card above
    // Overlay position card (directly after res card)
    if(kpsOverlayEnabled||resOverlayEnabled){
        bool hasStatusOv=!cleanRamStatus.empty();
        int ovY2=l.yRes+(222+(hasStatusOv?28:0))+8;
        drawCard(hdc,p,ovY2,cw,90);
        drawText(hdc,L"OVERLAY POSITION",p+14,ovY2+10,200,14,T.subtext,hFontSmall);
        // X control
        drawText(hdc,L"X",p+14,ovY2+30,20,20,T.subtext,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        drawRR(hdc,p+30,ovY2+28,32,22,8,T.btn,T.border,1);
        drawText(hdc,L"-",p+30,ovY2+28,32,22,T.text,hFontMed,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        wchar_t xb[16];swprintf(xb,16,L"%d",overlayX);
        drawRR(hdc,p+66,ovY2+28,70,22,6,T.btn,T.border,1);
        drawText(hdc,xb,p+66,ovY2+28,70,22,T.accent,hFontMono,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        drawRR(hdc,p+140,ovY2+28,32,22,8,T.btn,T.border,1);
        drawText(hdc,L"+",p+140,ovY2+28,32,22,T.text,hFontMed,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        // Y control
        drawText(hdc,L"Y",p+14,ovY2+58,20,20,T.subtext,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        drawRR(hdc,p+30,ovY2+56,32,22,8,T.btn,T.border,1);
        drawText(hdc,L"-",p+30,ovY2+56,32,22,T.text,hFontMed,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        wchar_t yb[16];swprintf(yb,16,L"%d",overlayY);
        drawRR(hdc,p+66,ovY2+56,70,22,6,T.btn,T.border,1);
        drawText(hdc,yb,p+66,ovY2+56,70,22,T.accent,hFontMono,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        drawRR(hdc,p+140,ovY2+56,32,22,8,T.btn,T.border,1);
        drawText(hdc,L"+",p+140,ovY2+56,32,22,T.text,hFontMed,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        // Step size note
        drawText(hdc,L"Step: 10px",p+180,ovY2+38,100,18,T.subtext,hFontMono,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    }

    BitBlt(hdc_real,0,0,W,H,hdc,0,0,SRCCOPY);
    SelectObject(hdc,obmp);DeleteObject(bmp);DeleteDC(hdc);

    // Paint fixed header ON TOP of scrolled content
    fillRect(hdc_real,0,0,W,48,T.surface);
    fillRect(hdc_real,0,47,W,2,T.accent);
    // Use hdc_real directly for header text
    {HFONT oldF=(HFONT)SelectObject(hdc_real,hFontBig);
    SetTextColor(hdc_real,T.text);SetBkMode(hdc_real,TRANSPARENT);
    RECT hr={p,0,W-p,48};DrawText(hdc_real,L"SETTINGS",-1,&hr,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    SelectObject(hdc_real,oldF);}

    EndPaint(hwnd,&ps);
}

// ===================== OVERLAYS =====================
void paintKpsOverlay(HWND hwnd){
    PAINTSTRUCT ps;HDC hr=BeginPaint(hwnd,&ps);
    RECT cr;GetClientRect(hwnd,&cr);int W=cr.right,H=cr.bottom;
    HDC hdc=CreateCompatibleDC(hr);
    HBITMAP bmp=CreateCompatibleBitmap(hr,W,H);
    SelectObject(hdc,bmp);
    bool running=macroRunning.load();
    // Always dark regardless of theme
    fillRect(hdc,0,0,W,H,RGB(10,10,14));
    drawRR(hdc,0,0,W,H,10,RGB(18,18,26),RGB(40,40,60),1);

    if(resOverlayEnabled){
        // Combined card - KPS on top, resources below
        drawText(hdc,L"KPS",10,4,50,12,T.subtext,hFontSmall,DT_LEFT|DT_TOP|DT_SINGLELINE);
        drawDot(hdc,14,24,5,running?T.green:T.red);
        wchar_t buf[48];swprintf(buf,48,L"%s  %d KPS",running?L"ON":L"OFF",actualKPS.load());
        drawText(hdc,buf,0,14,W,20,RGB(230,230,255),hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        // divider
        fillRect(hdc,8,36,W-16,1,T.border);
        // Resources
        wchar_t cb[32],mb[32];
        swprintf(cb,32,L"CPU  %.1f%%",cpuUsage);
        swprintf(mb,32,L"RAM  %.1f MB",(float)memUsageKB/1024.0f);
        drawText(hdc,cb,10,40,W-20,16,T.text,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        drawRR(hdc,10,58,W-20,3,2,T.btn);
        int cf2=(int)(std::min(cpuUsage/100.0f,1.0f)*(W-20));
        if(cf2>0)drawRR(hdc,10,58,cf2,3,2,T.accent);
        drawText(hdc,mb,10,64,W-20,16,T.text,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    } else {
        // No dot in single overlay - text is centred
        wchar_t buf[48];swprintf(buf,48,L"%s  |  %d KPS",running?L"ON":L"OFF",actualKPS.load());
        drawText(hdc,buf,0,0,W,H,RGB(230,230,255),hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }

    BLENDFUNCTION bf={AC_SRC_OVER,0,235,0};
    AlphaBlend(hr,0,0,W,H,hdc,0,0,W,H,bf);
    DeleteObject(bmp);DeleteDC(hdc);EndPaint(hwnd,&ps);
}

void paintResOverlay(HWND hwnd){
    PAINTSTRUCT ps;HDC hr=BeginPaint(hwnd,&ps);
    RECT cr;GetClientRect(hwnd,&cr);int W=cr.right,H=cr.bottom;
    HDC hdc=CreateCompatibleDC(hr);
    HBITMAP bmp=CreateCompatibleBitmap(hr,W,H);
    SelectObject(hdc,bmp);
    fillRect(hdc,0,0,W,H,RGB(10,10,14));
    drawRR(hdc,0,0,W,H,10,RGB(18,18,26),RGB(40,40,60),1);
    drawText(hdc,L"RESOURCES",10,4,W-20,14,RGB(100,100,140),hFontSmall,DT_LEFT|DT_TOP|DT_SINGLELINE);
    wchar_t cb[32],mb[32];
    swprintf(cb,32,L"CPU  %.1f%%",cpuUsage);
    swprintf(mb,32,L"RAM  %.1f MB",(float)memUsageKB/1024.0f);
    drawText(hdc,cb,10,20,W-20,16,RGB(230,230,255),hFontMed,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawRR(hdc,10,38,W-20,4,2,T.btn);
    int cf=(int)(std::min(cpuUsage/100.0f,1.0f)*(W-20));
    if(cf>0)drawRR(hdc,10,38,cf,4,2,T.accent);
    drawText(hdc,mb,10,44,W-20,16,RGB(230,230,255),hFontMed,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    BLENDFUNCTION bf={AC_SRC_OVER,0,235,0};
    AlphaBlend(hr,0,0,W,H,hdc,0,0,W,H,bf);
    DeleteObject(bmp);DeleteDC(hdc);EndPaint(hwnd,&ps);
}

// ===================== TOAST NOTIFICATIONS =====================
struct Toast { std::wstring msg; COLORREF col; DWORD showTick; float alpha; int y; };
std::vector<Toast> toasts;
std::mutex toastMutex;
HWND hwndToast=NULL;

void showToast(const wchar_t* msg, COLORREF col=RGB(99,102,241)){
    std::lock_guard<std::mutex> lk(toastMutex);
    Toast t;t.msg=msg;t.col=col;t.showTick=GetTickCount();t.alpha=0.0f;t.y=0;
    toasts.push_back(t);
    if(hwndMain)InvalidateRect(hwndMain,NULL,FALSE);
}

void drawToasts(HDC hdc,int W,int H){
    std::lock_guard<std::mutex> lk(toastMutex);
    DWORD now=GetTickCount();
    // Remove expired
    toasts.erase(std::remove_if(toasts.begin(),toasts.end(),[&](const Toast&t){
        return now-t.showTick>3000;
    }),toasts.end());
    // Draw from bottom up
    int ty=H-20;
    for(int i=(int)toasts.size()-1;i>=0;i--){
        Toast&t=toasts[i];
        DWORD age=now-t.showTick;
        float a=age<300?(float)age/300.0f:age>2500?1.0f-(float)(age-2500)/500.0f:1.0f;
        t.alpha=a;
        ty-=36;
        // Toast pill
        int tw=220,tx=W/2-tw/2;
        // Slight border glow
        COLORREF glowC=RGB((int)(GetRValue(t.col)*0.3f),(int)(GetGValue(t.col)*0.3f),(int)(GetBValue(t.col)*0.3f));
        drawRR(hdc,tx-2,ty-2,tw+4,32,18,glowC);
        drawRR(hdc,tx,ty,tw,28,16,RGB(18,18,26));
        // Left accent bar
        HBRUSH ab=CreateSolidBrush(t.col);
        HPEN np=CreatePen(PS_NULL,0,0);
        HBRUSH ob=(HBRUSH)SelectObject(hdc,ab);HPEN op=(HPEN)SelectObject(hdc,np);
        RECT ar={tx+8,ty+6,tx+12,ty+22};
        HRGN rr=CreateRoundRectRgn(tx+8,ty+6,tx+12,ty+22,4,4);
        FillRgn(hdc,rr,ab);DeleteObject(rr);
        SelectObject(hdc,ob);SelectObject(hdc,op);DeleteObject(ab);DeleteObject(np);
        // Text
        HFONT sf=CreateFont(12,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,FONT_NAMES[fontIdx]);
        SelectObject(hdc,sf);SetTextColor(hdc,RGB(220,220,255));SetBkMode(hdc,TRANSPARENT);
        RECT tr={tx+18,ty,tx+tw-8,ty+28};DrawText(hdc,t.msg.c_str(),-1,&tr,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        DeleteObject(sf);
    }
}

// Single unified overlay - shows KPS and/or resource info
void paintOverlay(HWND hwnd){
    PAINTSTRUCT ps;HDC hr=BeginPaint(hwnd,&ps);
    RECT cr;GetClientRect(hwnd,&cr);int W=cr.right,H=cr.bottom;
    HDC hdc=CreateCompatibleDC(hr);
    HBITMAP bmp=CreateCompatibleBitmap(hr,W,H);
    HBITMAP ob=(HBITMAP)SelectObject(hdc,bmp);
    bool running=macroRunning.load();
    COLORREF runCol=running?RGB(52,211,153):RGB(248,113,113);
    fillRect(hdc,0,0,W,H,RGB(10,10,14));
    drawRR(hdc,0,0,W,H,10,RGB(18,18,26),RGB(40,40,60),1);
    int y=4;
    if(kpsOverlayEnabled){
        wchar_t buf[48];swprintf(buf,48,L"%s  |  %d KPS",running?L"ON":L"OFF",actualKPS.load());
        drawDot(hdc,14,y+9,5,runCol);
        drawText(hdc,buf,22,y,W-26,22,RGB(230,230,255),hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        y+=24;
    }
    if(resOverlayEnabled){
        if(kpsOverlayEnabled) fillRect(hdc,10,y,W-20,1,RGB(30,30,46));
        y+=4;
        wchar_t cb[32],mb[32];
        swprintf(cb,32,L"CPU  %.1f%%",cpuUsage);
        swprintf(mb,32,L"RAM  %.1f MB",(float)memUsageKB/1024.0f);
        drawText(hdc,cb,10,y,W-20,16,RGB(180,180,210),hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        y+=16;
        drawRR(hdc,10,y,W-20,3,2,RGB(30,30,46));
        int cf=(int)(std::min(cpuUsage/100.0f,1.0f)*(W-20));
        if(cf>0)drawRR(hdc,10,y,cf,3,2,RGB(99,102,241));
        y+=5;
        drawText(hdc,mb,10,y,W-20,16,RGB(180,180,210),hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    }
    BLENDFUNCTION bf={AC_SRC_OVER,0,235,0};
    AlphaBlend(hr,0,0,W,H,hdc,0,0,W,H,bf);
    SelectObject(hdc,ob);DeleteObject(bmp);DeleteDC(hdc);EndPaint(hwnd,&ps);
}

LRESULT CALLBACK OverlayProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_PAINT:paintOverlay(hwnd);return 0;
    case WM_ERASEBKGND:return 1;
    case WM_DESTROY:hwndOverlay=NULL;break;
    default:return DefWindowProc(hwnd,msg,wp,lp);
    }return 0;
}

void updateOverlay(){
    if(!hwndOverlay||!IsWindow(hwndOverlay))return;
    bool any=kpsOverlayEnabled||resOverlayEnabled;
    if(!any){DestroyWindow(hwndOverlay);hwndOverlay=NULL;return;}
    // Resize to fit content
    int h=4;
    if(kpsOverlayEnabled)h+=24;
    if(resOverlayEnabled)h+=(kpsOverlayEnabled?5:0)+42;
    h+=4;
    RECT wr;GetWindowRect(hwndOverlay,&wr);
    SetWindowPos(hwndOverlay,NULL,0,0,180,h,SWP_NOMOVE|SWP_NOZORDER);
    InvalidateRect(hwndOverlay,NULL,TRUE);
}

void spawnOverlay(){
    bool any=kpsOverlayEnabled||resOverlayEnabled;
    if(!any){
        if(hwndOverlay&&IsWindow(hwndOverlay)){DestroyWindow(hwndOverlay);hwndOverlay=NULL;}
        return;
    }
    if(!hwndOverlay||!IsWindow(hwndOverlay)){
        WNDCLASS wc={};wc.lpfnWndProc=OverlayProc;wc.hInstance=gInst;
        wc.lpszClassName=L"OvWnd";wc.hCursor=NULL;
        RegisterClass(&wc);
        int sw=GetSystemMetrics(SM_CXSCREEN);
        int sw2=GetSystemMetrics(SM_CXSCREEN);
        if(overlayX==0) overlayX=sw2-200; // default right side
        hwndOverlay=CreateWindowEx(
            WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_NOACTIVATE,
            L"OvWnd",L"",WS_POPUP,overlayX,overlayY,180,60,NULL,NULL,gInst,NULL);
        SetLayeredWindowAttributes(hwndOverlay,0,235,LWA_ALPHA);
        ShowWindow(hwndOverlay,SW_SHOW);
        LOG_OK(L"Overlay shown");
    }
    updateOverlay();
}

// ===================== SETTINGS WNDPROC =====================
LRESULT CALLBACK SettingsWndProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        if(licCopied&&GetTickCount()-licCopiedTick>2000){licCopied=false;}
        paintSettings(hwnd);return 0;
    case WM_VSCROLL:{
        SCROLLINFO si={};si.cbSize=sizeof(si);si.fMask=SIF_ALL;
        GetScrollInfo(hwnd,SB_VERT,&si);
        int old=si.nPos;
        switch(LOWORD(wp)){
            case SB_LINEUP:   si.nPos-=20;break;
            case SB_LINEDOWN: si.nPos+=20;break;
            case SB_PAGEUP:   si.nPos-=si.nPage;break;
            case SB_PAGEDOWN: si.nPos+=si.nPage;break;
            case SB_THUMBTRACK:si.nPos=si.nTrackPos;break;
        }
        int maxScroll=std::max(0,si.nMax-(int)si.nPage);
        si.nPos=std::max(0,std::min(si.nPos,maxScroll));
        settScrollPos=si.nPos;
        SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
        if(si.nPos!=old)InvalidateRect(hwnd,NULL,FALSE);
        break;
    }
    case WM_MOUSEWHEEL:{
        int delta=GET_WHEEL_DELTA_WPARAM(wp)>0?-40:40;
        RECT cr2;GetClientRect(hwnd,&cr2);
        int maxS=std::max(0,(int)(settTotalHeight-cr2.bottom+48));
        settScrollPos=std::max(0,std::min(settScrollPos+delta,maxS));
        InvalidateRect(hwnd,NULL,FALSE);
        break;
    }
    case WM_MOUSEMOVE:{
        int mx=GET_X_LPARAM(lp),my=GET_Y_LPARAM(lp);
        SLayout l=getSLayout(hwnd);
        int newHov=-1;
        if(fontDropOpen){
            int dy=l.yFont+40;
            for(int i=0;i<5;i++){
                if(mx>=l.pad+14&&mx<=l.W-l.pad-14&&my>=dy+2+i*22&&my<=dy+2+i*22+20){newHov=i;break;}
            }
        }
        if(newHov!=fontDropHov){fontDropHov=newHov;InvalidateRect(hwnd,NULL,FALSE);}
        // Drag overlay via settings panel
        if((wp&MK_LBUTTON)&&overlayDragging){
            POINT cur;GetCursorPos(&cur);
            HWND ov=hwndOverlay;
            if(ov&&IsWindow(ov)){
                RECT r;GetWindowRect(ov,&r);
                int nx=r.left+(cur.x-overlayDrag.x),ny=r.top+(cur.y-overlayDrag.y);
                SetWindowPos(ov,NULL,nx,ny,0,0,SWP_NOSIZE|SWP_NOZORDER);
                overlayX=nx;overlayY=ny;
                overlayDrag=cur;
                InvalidateRect(hwnd,NULL,FALSE);
            }
        }
        break;
    }
    case WM_LBUTTONUP:
        if(overlayDragging){overlayDragging=false;ReleaseCapture();}
        break;
    case WM_LBUTTONDOWN:{
        int mx=GET_X_LPARAM(lp),my=GET_Y_LPARAM(lp);
        SLayout l=getSLayout(hwnd);
        int W=l.W,p=l.pad,cw=l.cw;
        (void)W; // suppress unused warning

        // Resource overlay toggle
        if(mx>=l.pad+l.cw-48&&mx<=l.pad+l.cw-4&&my>=l.yRes+84&&my<=l.yRes+106){
            resOverlayEnabled=!resOverlayEnabled;
            spawnOverlay();
            saveSettings();InvalidateRect(hwnd,NULL,FALSE);
        }
        if(mx>=l.pad+l.cw-48&&mx<=l.pad+l.cw-4&&my>=l.yRes+112&&my<=l.yRes+134){
            kpsOverlayEnabled=!kpsOverlayEnabled;
            spawnOverlay();
            saveSettings();InvalidateRect(hwnd,NULL,FALSE);
        }
        // Eye
        // SHOW/HIDE toggle
        if(mx>=l.pad+l.cw-92&&mx<=l.pad+l.cw-52&&my>=l.yLic+10&&my<=l.yLic+30){
            keyVisible=!keyVisible;InvalidateRect(hwnd,NULL,FALSE);
        }
        // COPY button
        if(mx>=l.pad+l.cw-46&&mx<=l.pad+l.cw-6&&my>=l.yLic+10&&my<=l.yLic+30){
            bool ok=false;
            if(OpenClipboard(hwnd)){
                EmptyClipboard();
                size_t sz=(savedLicenseKey.size()+1)*sizeof(wchar_t);
                HGLOBAL hm=GlobalAlloc(GMEM_MOVEABLE,sz);
                if(hm){
                    memcpy(GlobalLock(hm),savedLicenseKey.c_str(),sz);
                    GlobalUnlock(hm);
                    SetClipboardData(CF_UNICODETEXT,hm);
                    ok=true;
                }
                CloseClipboard();
            }
            if(ok){
                licCopied=true;
                licCopiedTick=GetTickCount();
                LOG_OK(L"License key copied to clipboard successfully");
            } else {
                LOG_ERR(L"Failed to copy license key to clipboard");
            }
            InvalidateRect(hwnd,NULL,FALSE);
        }
        // Font dropdown
        if(mx>=p+14&&mx<=W-p-14&&my>=l.yFont+18&&my<=l.yFont+40){
            fontDropOpen=!fontDropOpen;fontDropHov=-1;InvalidateRect(hwnd,NULL,FALSE);
        }
        if(fontDropOpen){
            int dy=l.yFont+40;
            for(int i=0;i<5;i++){
                if(mx>=p+14&&mx<=W-p-14&&my>=dy+2+i*22&&my<=dy+2+i*22+20){
                    fontIdx=i;fontDropOpen=false;fontDropHov=-1;
                    createFonts();saveSettings();
                    // Update KPS input font
                                InvalidateRect(hwnd,NULL,FALSE);InvalidateRect(hwndMain,NULL,FALSE);
                    wchar_t buf[64];swprintf(buf,64,L"Font changed to: %s",FONT_NAMES[i]);
                    LOG_OK(buf);
                    break;
                }
            }
        }
        // Theme
        {int bw=52,gap=5,tx=p+14;
        for(int i=0;i<6;i++){
            if(mx>=tx&&mx<=tx+bw&&my>=l.yTheme+26&&my<=l.yTheme+52){
                themeIdx=i;saveSettings();
                InvalidateRect(hwnd,NULL,FALSE);InvalidateRect(hwndMain,NULL,FALSE);
                wchar_t buf[64];swprintf(buf,64,L"Theme changed to: %s",THEMES[i].name);
                LOG_OK(buf);
            }
            tx+=bw+gap;
        }}
        // Auto-launch toggle
        if(mx>=l.pad+l.cw-48&&mx<=l.pad+l.cw-4&&my>=l.yAutoLaunch+13&&my<=l.yAutoLaunch+35){
            autoLaunchEnabled=!autoLaunchEnabled;
            setAutoLaunch(autoLaunchEnabled);
            saveSettings();InvalidateRect(hwnd,NULL,FALSE);
        }
        // Minimise to tray toggle
        if(mx>=l.pad+l.cw-48&&mx<=l.pad+l.cw-4&&my>=l.yMinimise+13&&my<=l.yMinimise+35){
            minimiseToTray=!minimiseToTray;
            saveSettings();InvalidateRect(hwnd,NULL,FALSE);
        }
        // Update check (click anywhere on update card)
        if(mx>=p&&mx<=W-p&&my>=l.yUpdate&&my<=l.yUpdate+48){
            std::thread(checkForUpdate).detach();
            InvalidateRect(hwnd,NULL,FALSE);
        }
        // Utility buttons
        {int hw2=(l.cw-28-8)/2,bx1=l.pad+14,bx2=l.pad+14+hw2+8;
        int r1y=l.yRes+148,r2y=l.yRes+182;
        if(mx>=bx1&&mx<=bx1+hw2&&my>=r1y&&my<=r1y+26) std::thread(cleanMemory).detach();
        if(mx>=bx2&&mx<=bx2+hw2&&my>=r1y&&my<=r1y+26){ShellExecute(NULL,L"open",LOG_FILE,NULL,NULL,SW_SHOW);LOG_INFO(L"Opened logs file");}
        if(mx>=bx1&&mx<=bx1+hw2&&my>=r2y&&my<=r2y+26){exportConfig();InvalidateRect(hwnd,NULL,FALSE);}
        if(mx>=bx2&&mx<=bx2+hw2&&my>=r2y&&my<=r2y+26){importConfig();InvalidateRect(hwnd,NULL,FALSE);}
        }
        break;
    }
    case WM_GETMINMAXINFO:{
        MINMAXINFO*mm=(MINMAXINFO*)lp;
        mm->ptMinTrackSize.x=420;
        mm->ptMinTrackSize.y=600;
        mm->ptMaxTrackSize.x=420;
        mm->ptMaxTrackSize.y=900;
        break;
    }
    case WM_DESTROY:hwndSettings=NULL;break;
    default:return DefWindowProc(hwnd,msg,wp,lp);
    }
    return 0;
}

// Handle a click in the settings panel (called from main WndProc when settings tab active)
void settingsHandleClick(HWND hwnd,int mx,int my,int W,int H){
    // Build the same layout as paintSettingsInto uses
    SLayout l;l.W=W;l.pad=14;l.cw=W-l.pad*2-16;
    bool hasStatusL3=!cleanRamStatus.empty();
    int resH3=222+(hasStatusL3?28:0);
    int y3=56-settScrollPos;
    l.yRes=y3;        y3+=resH3+8;
    l.yLic=y3;        y3+=(licCopied?54:40)+8;
    l.yFont=y3;       y3+=52+8;
    l.yFontDrop=l.yFont+52;
    if(fontDropOpen)  y3+=5*22;
    l.yTheme=y3;      y3+=74+8;
    l.yAutoLaunch=y3; y3+=58+8;
    l.yMinimise=y3;   y3+=58+8;
    l.yUpdate=y3;     y3+=58+8;
    l.yTrial=y3;
    int p=l.pad,cw=l.cw;
    (void)H;

    // Res overlay toggle
    if(mx>=p+cw-48&&mx<=p+cw-4&&my>=l.yRes+84&&my<=l.yRes+106){
        resOverlayEnabled=!resOverlayEnabled;spawnOverlay();saveSettings();InvalidateRect(hwnd,NULL,FALSE);}
    // KPS overlay toggle
    if(mx>=p+cw-48&&mx<=p+cw-4&&my>=l.yRes+112&&my<=l.yRes+134){
        kpsOverlayEnabled=!kpsOverlayEnabled;spawnOverlay();saveSettings();InvalidateRect(hwnd,NULL,FALSE);}
    // Utility buttons
    {int hw3=(cw-28-8)/2,bx1=p+14,bx2=p+14+hw3+8;
    int r1y=l.yRes+148,r2y=l.yRes+182;
    if(mx>=bx1&&mx<=bx1+hw3&&my>=r1y&&my<=r1y+28) std::thread(cleanMemory).detach();
    if(mx>=bx2&&mx<=bx2+hw3&&my>=r1y&&my<=r1y+28){ShellExecute(NULL,L"open",LOG_FILE,NULL,NULL,SW_SHOW);}
    if(mx>=bx1&&mx<=bx1+hw3&&my>=r2y&&my<=r2y+28){exportConfig();InvalidateRect(hwnd,NULL,FALSE);}
    if(mx>=bx2&&mx<=bx2+hw3&&my>=r2y&&my<=r2y+28){importConfig();InvalidateRect(hwnd,NULL,FALSE);}}
    // License show/copy
    if(mx>=p+cw-92&&mx<=p+cw-52&&my>=l.yLic+10&&my<=l.yLic+30){keyVisible=!keyVisible;InvalidateRect(hwnd,NULL,FALSE);}
    if(mx>=p+cw-46&&mx<=p+cw-6&&my>=l.yLic+10&&my<=l.yLic+30){
        if(OpenClipboard(hwnd)){EmptyClipboard();
        size_t sz=(savedLicenseKey.size()+1)*sizeof(wchar_t);
        HGLOBAL hm=GlobalAlloc(GMEM_MOVEABLE,sz);
        if(hm){memcpy(GlobalLock(hm),savedLicenseKey.c_str(),sz);GlobalUnlock(hm);SetClipboardData(CF_UNICODETEXT,hm);}
        CloseClipboard();licCopied=true;licCopiedTick=GetTickCount();LOG_OK(L"License key copied");InvalidateRect(hwnd,NULL,FALSE);}}
    // Font dropdown
    if(mx>=p+14&&mx<=p+cw-14&&my>=l.yFont+18&&my<=l.yFont+40){fontDropOpen=!fontDropOpen;InvalidateRect(hwnd,NULL,FALSE);}
    if(fontDropOpen){int dy3=l.yFont+40;
    for(int i=0;i<5;i++){if(mx>=p+14&&mx<=p+cw-14&&my>=dy3+2+i*22&&my<=dy3+2+i*22+20){
        fontIdx=i;fontDropOpen=false;createFonts();saveSettings();
        InvalidateRect(hwnd,NULL,FALSE);}}}
    // Theme
    {int bw3=52,gap3=5,tx3=p+14;
    for(int i=0;i<6;i++){if(mx>=tx3&&mx<=tx3+bw3&&my>=l.yTheme+26&&my<=l.yTheme+52){
        themeIdx=i;saveSettings();InvalidateRect(hwnd,NULL,FALSE);}tx3+=bw3+gap3;}}
    // Auto-launch toggle
    if(mx>=p+cw-48&&mx<=p+cw-4&&my>=l.yAutoLaunch+16&&my<=l.yAutoLaunch+38){
        autoLaunchEnabled=!autoLaunchEnabled;setAutoLaunch(autoLaunchEnabled);saveSettings();InvalidateRect(hwnd,NULL,FALSE);}
    // Minimise to tray toggle
    if(mx>=p+cw-48&&mx<=p+cw-4&&my>=l.yMinimise+16&&my<=l.yMinimise+38){
        minimiseToTray=!minimiseToTray;saveSettings();InvalidateRect(hwnd,NULL,FALSE);}
    // Changelog on startup toggle
    {int clY2=l.yMinimise+62;
    if(mx>=p+cw-48&&mx<=p+cw-4&&my>=clY2+16&&my<=clY2+38){
        showChangelogOnStartup=!showChangelogOnStartup;saveSettings();InvalidateRect(hwnd,NULL,FALSE);}}
    // Update check
    if(mx>=p&&mx<=p+cw&&my>=l.yUpdate&&my<=l.yUpdate+54){
        std::thread(checkForUpdate).detach();}
    // Overlay X/Y buttons
    if(kpsOverlayEnabled||resOverlayEnabled){
        bool hsOv=!cleanRamStatus.empty();int ovY3=l.yRes+(222+(hsOv?28:0))+8;
        if(mx>=p+30&&mx<=p+62&&my>=ovY3+28&&my<=ovY3+50){overlayX=std::max(0,overlayX-10);if(hwndOverlay&&IsWindow(hwndOverlay))SetWindowPos(hwndOverlay,NULL,overlayX,overlayY,0,0,SWP_NOSIZE|SWP_NOZORDER);saveSettings();InvalidateRect(hwnd,NULL,FALSE);}
        if(mx>=p+140&&mx<=p+172&&my>=ovY3+28&&my<=ovY3+50){overlayX+=10;if(hwndOverlay&&IsWindow(hwndOverlay))SetWindowPos(hwndOverlay,NULL,overlayX,overlayY,0,0,SWP_NOSIZE|SWP_NOZORDER);saveSettings();InvalidateRect(hwnd,NULL,FALSE);}
        if(mx>=p+30&&mx<=p+62&&my>=ovY3+56&&my<=ovY3+78){overlayY=std::max(0,overlayY-10);if(hwndOverlay&&IsWindow(hwndOverlay))SetWindowPos(hwndOverlay,NULL,overlayX,overlayY,0,0,SWP_NOSIZE|SWP_NOZORDER);saveSettings();InvalidateRect(hwnd,NULL,FALSE);}
        if(mx>=p+140&&mx<=p+172&&my>=ovY3+56&&my<=ovY3+78){overlayY+=10;if(hwndOverlay&&IsWindow(hwndOverlay))SetWindowPos(hwndOverlay,NULL,overlayX,overlayY,0,0,SWP_NOSIZE|SWP_NOZORDER);saveSettings();InvalidateRect(hwnd,NULL,FALSE);}
    }
}

// paintSettingsInto — draw settings directly into any HDC
void paintSettingsInto(HDC hdc,int W,int H){
    // Build layout same way getSLayout does but using W/H params
    SLayout l;
    l.W=W;l.pad=14;l.cw=W-l.pad*2-16;
    bool hasStatusL2=!cleanRamStatus.empty();
    int resH2=222+(hasStatusL2?28:0);
    int y2=56-settScrollPos;
    l.yRes=y2;        y2+=resH2+8;
    l.yLic=y2;        y2+=(licCopied?54:40)+8;
    l.yFont=y2;       y2+=52+8;
    l.yFontDrop=l.yFont+52;
    if(fontDropOpen)  y2+=5*22;
    l.yTheme=y2;      y2+=74+8;
    l.yAutoLaunch=y2; y2+=58+8;
    l.yMinimise=y2;   y2+=58+8;
    l.yUpdate=y2;     y2+=58+8;
    l.yTrial=y2;      if(trialMode){y2+=58+8;}
    settTotalHeight=y2+settScrollPos+8;
    int p=l.pad,cw=l.cw;



    // Background
    fillRect(hdc,0,0,W,H,T.bg);

    int sfadeY=(int)((1.0f-easeOut(settFadeAlpha))*16.0f);
    int y=l.yRes+sfadeY;

    // ── RESOURCES CARD ──────────────────────────────────────
    bool hasStatus2b=!cleanRamStatus.empty();
    int resCardH2=222+(hasStatus2b?28:0);
    drawCard(hdc,p,y,cw,resCardH2);
    drawText(hdc,L"RESOURCES",p+14,y+8,200,14,T.subtext,hFontSmall);
    wchar_t cpuBuf2[64],memBuf2[64];
    swprintf(cpuBuf2,64,L"CPU:  %.2f%%",cpuUsage);
    swprintf(memBuf2,64,L"RAM:  %.1f MB",(float)memUsageKB/1024.0f);
    drawText(hdc,cpuBuf2,p+14,y+26,cw-28,18,T.text,hFontMed,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawText(hdc,memBuf2,p+14,y+48,cw-28,18,T.text,hFontMed,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    {int cf2=(int)(std::min(cpuUsage/100.0f,1.0f)*(cw-28));
    drawRR(hdc,p+14,y+70,cw-28,5,3,T.btn);
    if(cf2>0)drawRR(hdc,p+14,y+70,cf2,5,3,T.accent);}
    drawText(hdc,L"Resource Overlay",p+14,y+84,200,20,T.subtext,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawToggle(hdc,p+cw-48,y+84,resOverlayEnabled);
    drawText(hdc,L"KPS Overlay",p+14,y+112,200,20,T.subtext,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawToggle(hdc,p+cw-48,y+112,kpsOverlayEnabled);
    fillRect(hdc,p+14,y+140,cw-28,1,T.border);
    {int hw2=(cw-28-8)/2,bx1=p+14,bx2=p+14+hw2+8;
    int r1y=y+148,r2y=y+182;
    drawRR(hdc,bx1,r1y,hw2,28,8,T.btn,T.border,1);drawText(hdc,L"Clean RAM",bx1,r1y,hw2,28,T.text,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    drawRR(hdc,bx2,r1y,hw2,28,8,T.btn,T.border,1);drawText(hdc,L"Open Logs",bx2,r1y,hw2,28,T.text,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    drawRR(hdc,bx1,r2y,hw2,28,8,T.btn,T.border,1);drawText(hdc,L"Export CFG",bx1,r2y,hw2,28,T.text,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    drawRR(hdc,bx2,r2y,hw2,28,8,T.btn,T.border,1);drawText(hdc,L"Import CFG",bx2,r2y,hw2,28,T.text,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    if(hasStatus2b){
        bool done2=cleanRamStatus.find(L"Freed")!=std::wstring::npos;
        std::wstring sl2=(done2?L"+ ":L"~ ")+cleanRamStatus;
        SelectObject(hdc,hFontMono);SetTextColor(hdc,done2?T.green:T.accent);SetBkMode(hdc,TRANSPARENT);
        RECT sr2={p+14,y+216,p+14+cw-28,y+244};DrawText(hdc,sl2.c_str(),-1,&sr2,DT_LEFT|DT_TOP|DT_WORDBREAK);
    }}
    // Overlay position card
    if(kpsOverlayEnabled||resOverlayEnabled){
        bool hsOv=!cleanRamStatus.empty();int ovY2=l.yRes+resCardH2+sfadeY+8;
        drawCard(hdc,p,ovY2,cw,90);
        drawText(hdc,L"OVERLAY POSITION",p+14,ovY2+10,200,14,T.subtext,hFontSmall);
        wchar_t xb[16],yb[16];swprintf(xb,16,L"%d",overlayX);swprintf(yb,16,L"%d",overlayY);
        drawText(hdc,L"X",p+14,ovY2+30,20,20,T.subtext,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        drawRR(hdc,p+30,ovY2+28,32,22,8,T.btn,T.border,1);drawText(hdc,L"-",p+30,ovY2+28,32,22,T.text,hFontMed,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        drawRR(hdc,p+66,ovY2+28,70,22,6,T.btn,T.border,1);drawText(hdc,xb,p+66,ovY2+28,70,22,T.accent,hFontMono,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        drawRR(hdc,p+140,ovY2+28,32,22,8,T.btn,T.border,1);drawText(hdc,L"+",p+140,ovY2+28,32,22,T.text,hFontMed,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        drawText(hdc,L"Y",p+14,ovY2+58,20,20,T.subtext,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        drawRR(hdc,p+30,ovY2+56,32,22,8,T.btn,T.border,1);drawText(hdc,L"-",p+30,ovY2+56,32,22,T.text,hFontMed,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        drawRR(hdc,p+66,ovY2+56,70,22,6,T.btn,T.border,1);drawText(hdc,yb,p+66,ovY2+56,70,22,T.accent,hFontMono,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        drawRR(hdc,p+140,ovY2+56,32,22,8,T.btn,T.border,1);drawText(hdc,L"+",p+140,ovY2+56,32,22,T.text,hFontMed,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        drawText(hdc,L"Step: 10px",p+180,ovY2+38,100,18,T.subtext,hFontMono,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    }
    y=l.yLic+sfadeY;
    // ── LICENSE CARD ────────────────────────────────────────
    {int licH=licCopied?54:40;
    drawCard(hdc,p,y,cw,licH);
    drawText(hdc,L"LICENSE",p+14,y+8,100,14,T.subtext,hFontSmall);
    std::wstring dk2;
    if(savedLicenseKey.empty())dk2=L"No license key saved";
    else dk2=keyVisible?savedLicenseKey:std::wstring(savedLicenseKey.size(),L'*');
    drawText(hdc,dk2.c_str(),p+14,y+18,cw-106,18,T.text,hFontMono,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawRR(hdc,p+cw-92,y+10,40,20,6,T.btn,T.border,1);
    drawText(hdc,keyVisible?L"HIDE":L"SHOW",p+cw-92,y+10,40,20,T.subtext,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    COLORREF cc=licCopied?T.green:T.subtext;
    drawRR(hdc,p+cw-46,y+10,40,20,6,licCopied?RGB(20,60,30):T.btn,licCopied?T.green:T.border,1);
    drawText(hdc,licCopied?L"✓":L"COPY",p+cw-46,y+10,40,20,cc,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    if(licCopied)drawText(hdc,L"Copied to clipboard",p+14,y+36,cw-28,14,T.green,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);}
    y=l.yFont+sfadeY;
    // ── FONT CARD ───────────────────────────────────────────
    drawCard(hdc,p,y,cw,40);
    drawText(hdc,L"FONT",p+14,y+10,60,14,T.subtext,hFontSmall);
    drawRR(hdc,p+14,y+18,cw-28,18,4,T.btn,T.border,1);
    drawText(hdc,FONT_NAMES[fontIdx],p+18,y+18,cw-50,18,T.text,hFontMed,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawText(hdc,fontDropOpen?L"▲":L"▼",p+cw-26,y+18,16,18,T.subtext,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    if(fontDropOpen){int dy2=y+40;drawRR(hdc,p+14,dy2,cw-28,5*22+4,6,T.card,T.border,1);
    for(int i=0;i<5;i++){bool hov=(fontDropHov==i),sel=(fontIdx==i);
    if(sel||hov)drawRR(hdc,p+16,dy2+2+i*22,cw-32,20,4,sel?T.accent:T.btnHov);
    drawText(hdc,FONT_NAMES[i],p+22,dy2+2+i*22,cw-40,20,sel?T.text:T.subtext,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);}}
    y=l.yTheme+sfadeY;
    // ── THEME CARD ──────────────────────────────────────────
    drawCard(hdc,p,y,cw,62);
    drawText(hdc,L"COLOUR THEME",p+14,y+10,200,14,T.subtext,hFontSmall);
    {int bw2=52,gap2=5,tx2=p+14;
    for(int i=0;i<6;i++){bool act=(i==themeIdx);
    drawRR(hdc,tx2,y+26,bw2,26,6,act?T.accent:T.btn,act?T.accent:T.border,1);
    drawText(hdc,THEMES[i].name,tx2,y+26,bw2,26,act?T.text:T.subtext,hFontSmall,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    tx2+=bw2+gap2;}}
    y=l.yAutoLaunch+sfadeY;
    // ── AUTO-LAUNCH CARD ────────────────────────────────────
    drawCard(hdc,p,y,cw,54);
    drawText(hdc,L"START WITH WINDOWS",p+14,y+10,300,14,T.subtext,hFontSmall);
    drawText(hdc,L"Launch macro automatically with Windows",p+14,y+28,cw-70,16,T.text,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawToggle(hdc,p+cw-48,y+16,autoLaunchEnabled);
    {int mY2=l.yMinimise+sfadeY;
    drawCard(hdc,p,mY2,cw,54);
    drawText(hdc,L"MINIMISE TO TRAY",p+14,mY2+10,300,14,T.subtext,hFontSmall);
    drawText(hdc,L"Minimise to tray instead of closing",p+14,mY2+28,cw-70,16,T.text,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawToggle(hdc,p+cw-48,mY2+16,minimiseToTray);}
    // Changelog on startup toggle
    {int clY=l.yMinimise+sfadeY+62;
    drawCard(hdc,p,clY,cw,54);
    drawText(hdc,L"CHANGELOG ON STARTUP",p+14,clY+10,300,14,T.subtext,hFontSmall);
    drawText(hdc,L"Show changelog when a new version is found",p+14,clY+28,cw-70,16,T.text,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    drawToggle(hdc,p+cw-48,clY+16,showChangelogOnStartup);}
    y=l.yUpdate+sfadeY;
    drawCard(hdc,p,y,cw,54);
    drawText(hdc,L"UPDATE",p+14,y+10,200,14,T.subtext,hFontSmall);
    {std::wstring ustr=latestVersion.empty()?L"Click to check for updates":
    (latestVersion==std::wstring(APP_VERSION)?L"Up to date - "+std::wstring(APP_VERSION):L"Update available: "+latestVersion);
    COLORREF uc=latestVersion.empty()?T.subtext:(latestVersion==std::wstring(APP_VERSION)?T.green:T.accent);
    drawText(hdc,ustr.c_str(),p+14,y+28,cw-28,16,uc,hFontMed,DT_LEFT|DT_VCENTER|DT_SINGLELINE);}
    if(trialMode){y=l.yTrial+sfadeY;
    drawCard(hdc,p,y,cw,54);
    drawText(hdc,L"TRIAL",p+14,y+10,200,14,T.subtext,hFontSmall);
    wchar_t tb3[96];
    if(trialDaysLeft>0)swprintf(tb3,96,L"%d day(s) remaining - purchase a license to unlock",trialDaysLeft);
    else swprintf(tb3,96,L"Trial expired - enter a license key to continue");
    drawText(hdc,tb3,p+14,y+28,cw-28,16,trialDaysLeft>0?T.accent:T.red,hFontSmall,DT_LEFT|DT_VCENTER|DT_SINGLELINE);}

    // Fixed settings header painted last (on top)
    fillRect(hdc,0,0,W,48,T.surface);
    fillRect(hdc,0,47,W,2,T.accent);
    {HFONT oldF=(HFONT)SelectObject(hdc,hFontBig);
    SetTextColor(hdc,T.text);SetBkMode(hdc,TRANSPARENT);
    RECT hr={14,0,W-14,48};DrawText(hdc,L"SETTINGS",-1,&hr,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    SelectObject(hdc,oldF);}
}

void openSettings(){} // settings now inline

// ===================== MAIN WNDPROC =====================
LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_ERASEBKGND:
        return 1; // double buffer handles all painting
    case WM_MOVING:
        return TRUE; // suppress nc repaint during drag
    case WM_NCACTIVATE:
        // Return TRUE to suppress white flash when window loses/gains focus.
        // Do NOT call DefWindowProc here — it redraws non-client area white.
        return TRUE;
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
        InvalidateRect(hwnd,NULL,FALSE);
        break;
    case WM_PAINT:
        // Uncomment to debug paint frequency: LOG_INFO(L"WM_PAINT");
        paintMain(hwnd);return 0;
    case WM_TIMER:
        if(wp==TIMER_SETT){
            if(activeTab==1&&settingsDirty){
                settingsDirty=false;
                InvalidateRect(hwnd,NULL,FALSE);
            }
            break;
        }
        if(wp==TIMER_ANIM){

            // Mode and running state - instant
            animModeHold=(float)holdMode.load();
            animRunning=(float)macroRunning.load();
            // Tab dock indicator animation (smooth bar, instant content)
            // Dock tab animation — fixed speed, always finishes in ~600ms
            float tabTarget=(float)activeTab;
            float tabDiff=tabTarget-tabAnim;
            if(fabsf(tabDiff)<0.01f){
                tabAnim=tabTarget;
            } else {
                tabAnim+=tabDiff*0.18f; // linear-ish, consistent speed
            }

            // Hover states - fast ease out
            int curHov=(int)(INT_PTR)hwndHovered;
            for(int i=0;i<16;i++){
                float tgt=(hoverBtnId[i]!=0&&hoverBtnId[i]==curHov)?1.0f:0.0f;
                animateTo(hoverBtn[i],tgt,tgt>0.5f?0.6f:0.35f);
            }

            // Status dot pulse
            DWORD now=GetTickCount();
            if(macroRunning.load()){
                animMacroStatus=0.55f+0.45f*(float)sin((double)now/500.0);
            } else {
                animMacroStatus=0.0f;
            }

            // Window fade-in animation
            if(fadeAlpha<1.0f){ fadeAlpha=std::min(1.0f,fadeAlpha+0.08f); }
            if(settFadeAlpha<1.0f){ settFadeAlpha=std::min(1.0f,settFadeAlpha+0.08f); }
            // Tab animation runs regardless of which tab is active
            if(fabsf(tabAnim-(float)activeTab)>0.005f){
                InvalidateRect(hwnd,NULL,FALSE); // need repaint for dock animation
            }
            if(activeTab==0){
                InvalidateRect(hwnd,NULL,FALSE);
            }
            // Settings tab redraws handled by TIMER_SETT
            if(hwndOverlay)InvalidateRect(hwndOverlay,NULL,FALSE);
            }
        break;
    case WM_MOUSEMOVE:{
        int mx=GET_X_LPARAM(lp),my=GET_Y_LPARAM(lp);
        HWND prev=hwndHovered;
        hwndHovered=(HWND)(INT_PTR)hitTest(hwnd,mx,my);
        if(hwndHovered!=prev&&activeTab==0)InvalidateRect(hwnd,NULL,FALSE);

        if((wp&MK_LBUTTON)&&isInSlider(hwnd,mx,my)){
            kps=sliderVal(hwnd,mx);
            InvalidateRect(hwnd,NULL,FALSE);
        }
        break;
    }
    case WM_LBUTTONDOWN:{
        int mx=GET_X_LPARAM(lp),my=GET_Y_LPARAM(lp);
        // Dock tab clicks
        {RECT cr10;GetClientRect(hwnd,&cr10);
        int dockY=cr10.bottom-DOCK_H;
        if(my>=dockY){
            if(mx<cr10.right/2){
                activeTab=0;
                // Hide settings window if open
                if(hwndSettings&&IsWindow(hwndSettings))ShowWindow(hwndSettings,SW_HIDE);
            } else {
                activeTab=1;
                settScrollPos=0;
                settFadeAlpha=0.0f;
                settingsDirty=true;
                SetTimer(hwnd,TIMER_SETT,500,NULL); // settings refresh every 500ms
            }
            InvalidateRect(hwnd,NULL,FALSE);
            break;
        }
        if(activeTab==1){
            settingsHandleClick(hwnd,mx,my,cr10.right,cr10.bottom-DOCK_H);
            settingsDirty=true;
            InvalidateRect(hwnd,NULL,FALSE);
            break;
        }}
        int hit=hitTest(hwnd,mx,my);
        if(hit==ID_SETTINGS_BTN){activeTab=1;settScrollPos=0;InvalidateRect(hwnd,NULL,FALSE);}

        if(hit==ID_SET_HOTKEY&&!capturingKey){capturingHotkey=true;macroRunning=false;InvalidateRect(hwnd,NULL,FALSE);std::thread(captureThread,true).detach();}
        if(hit==ID_ADD_KEY&&!capturingHotkey){capturingKey=true;InvalidateRect(hwnd,NULL,FALSE);std::thread(captureThread,false).detach();}
        if(hit==ID_REMOVE_KEY){
            EnterCriticalSection(&keyListCS);
            if(!keysToSend.empty()){
                wchar_t buf[64];swprintf(buf,64,L"Key removed: %s",vkToString(keysToSend.back()).c_str());
                LOG_INFO(buf);keysToSend.pop_back();
            }
            LeaveCriticalSection(&keyListCS);
            InvalidateRect(hwnd,NULL,FALSE);
        }
        if(hit==ID_MODE_TOGGLE){holdMode=false;LOG_OK(L"Mode set to Toggle");showToast(L"Mode: Toggle",RGB(99,102,241));InvalidateRect(hwnd,NULL,FALSE);}
        if(hit==ID_MODE_HOLD){holdMode=true;macroRunning=false;LOG_OK(L"Mode set to Hold");showToast(L"Mode: Hold",RGB(139,92,246));InvalidateRect(hwnd,NULL,FALSE);}
        if(hit>=500&&hit<500+KPS_PRESET_COUNT){
            int v=KPS_PRESETS[hit-500];kps=v;
            wchar_t buf[48];swprintf(buf,48,L"KPS preset selected: %d",v);LOG_INFO(buf);
            InvalidateRect(hwnd,NULL,FALSE);
        }
        if(isInSlider(hwnd,mx,my)){
            kps=sliderVal(hwnd,mx);
            InvalidateRect(hwnd,NULL,FALSE);
        }
        break;
    }
    case WM_VSCROLL:{
        SCROLLINFO si={};si.cbSize=sizeof(si);si.fMask=SIF_ALL;
        GetScrollInfo(hwnd,SB_VERT,&si);
        int old=si.nPos;
        switch(LOWORD(wp)){
            case SB_LINEUP:   si.nPos-=20;break;
            case SB_LINEDOWN: si.nPos+=20;break;
            case SB_PAGEUP:   si.nPos-=si.nPage;break;
            case SB_PAGEDOWN: si.nPos+=si.nPage;break;
            case SB_THUMBTRACK:si.nPos=si.nTrackPos;break;
        }
        RECT cr5;GetClientRect(hwnd,&cr5);
        int contentH5=mainTotalHeight-mainScrollPos;
int maxS5=std::max(0,(int)contentH5-(int)cr5.bottom);
        si.nPos=std::max(0,std::min(si.nPos,maxS5));
        mainScrollPos=si.nPos;
        if(si.nPos!=old) InvalidateRect(hwnd,NULL,FALSE);
        break;
    }
    case WM_MOUSEWHEEL:{
        POINT pt={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};ScreenToClient(hwnd,&pt);

        if(activeTab==1){
            int delta2=GET_WHEEL_DELTA_WPARAM(wp)>0?-40:40;
            RECT cr9;GetClientRect(hwnd,&cr9);
            int maxS9=std::max(0,(int)settTotalHeight-(int)(cr9.bottom-DOCK_H)+48);
            settScrollPos=std::max(0,std::min(settScrollPos+delta2,maxS9));

            settingsDirty=true;
            InvalidateRect(hwnd,NULL,FALSE);
            break;
        }
        if(isInSlider(hwnd,pt.x,pt.y)){
            int delta=(GET_WHEEL_DELTA_WPARAM(wp)>0?5:-5);
            kps=std::max(MIN_KPS,std::min(MAX_KPS,kps.load()+delta));
            InvalidateRect(hwnd,NULL,FALSE);
        } else {
            RECT cr5;GetClientRect(hwnd,&cr5);
            int delta=GET_WHEEL_DELTA_WPARAM(wp)>0?-40:40;
            int contentH6=mainTotalHeight-mainScrollPos;
int maxS=std::max(0,(int)contentH6-(int)cr5.bottom);
            mainScrollPos=std::max(0,std::min(mainScrollPos+delta,maxS));
            // Update scrollbar thumb position
            SCROLLINFO si={};si.cbSize=sizeof(si);si.fMask=SIF_POS;
            si.nPos=mainScrollPos;
            SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
            InvalidateRect(hwnd,NULL,FALSE);
        }
        break;
    }
    case WM_COMMAND:
        break;
    case WM_CHAR:
        break;
    case WM_SIZE:
        if(wp==SIZE_MINIMIZED&&minimiseToTray){
                ShowWindow(hwnd,SW_HIDE);
        } else if(wp!=SIZE_MINIMIZED){
            InvalidateRect(hwnd,NULL,FALSE);
        }
        break;
    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED:
    case WM_DWMCOMPOSITIONCHANGED:
        break;
    case WM_GETMINMAXINFO:{
        MINMAXINFO*mm=(MINMAXINFO*)lp;
        mm->ptMinTrackSize.x=APP_W;
        mm->ptMinTrackSize.y=APP_H;
        mm->ptMaxTrackSize.x=APP_W;
        mm->ptMaxTrackSize.y=APP_H;
        break;
    }
    case WM_DESTROY:
        saveSettings();KillTimer(hwnd,TIMER_ANIM);
        appRunning=false;macroRunning=false;
        PostQuitMessage(0);break;
    default:return DefWindowProc(hwnd,msg,wp,lp);
    }
    return 0;
}

// ===================== SPLASH =====================
HWND hwndSplash=NULL;
std::wstring splashMsg=L"Starting...";
bool splashShowChangelog=false;
int  splashChangelogScroll=0;
const wchar_t* CHANGELOG_TEXT =
    L"MACRO v2.1.1\n"
    L"====================\n\n"
    L"NEW FEATURES\n"
    L"--------------------\n"
    L"+ Bottom dock navigation (Macro / Settings tabs)\n"
    L"+ Settings now inline - no separate window\n"
    L"+ Animated tab transitions with easing\n"
    L"+ App icon (M logo) in taskbar, tray, title bar\n"
    L"+ Colour-coded changelog view\n"
    L"+ Per-font size calibration (5 fonts)\n\n"
    L"BUG FIXES\n"
    L"--------------------\n"
    L"~ Fixed drag flicker (removed CS_HREDRAW/CS_VREDRAW)\n"
    L"~ Fixed title bar flickering on repaint\n"
    L"~ Fixed minimise to tray deferred hide\n"
    L"~ Fixed log messages showing ? instead of -\n"
    L"~ Fixed gear button removed from status card\n"
    L"~ Fixed light theme contrast and readability\n\n"
    L"v2.0.0\n"
    L"====================\n\n"
    L"NEW FEATURES\n"
    L"--------------------\n"
    L"+ Settings scrollable with full layout\n"
    L"+ Unified resources card with utility buttons\n"
    L"+ Overlay X/Y position controls\n"
    L"+ Roblox auto-launch with RAM cleanup\n"
    L"+ Fade-in animations on window open\n\n"
    L"BUG FIXES\n"
    L"--------------------\n"
    L"~ Fixed overlay duplicating when toggled\n"
    L"~ Fixed RAM cleanup causing white flash\n"
    L"~ Fixed license deleting on network error\n"
    L"~ Fixed logs truncating to one character";

LRESULT CALLBACK SplashProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_PAINT:{
        PAINTSTRUCT ps;HDC hdc=BeginPaint(hwnd,&ps);
        RECT cr;GetClientRect(hwnd,&cr);
        int W=cr.right,H=cr.bottom;

        fillRect(hdc,0,0,W,H,RGB(10,10,15));
        drawRR(hdc,0,0,W,H,14,RGB(18,18,26),RGB(32,32,48),1);

        HFONT fb=CreateFont(40,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,FONT_NAMES[fontIdx]);
        HFONT fs=CreateFont(12,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,FONT_NAMES[fontIdx]);
        HFONT fm=CreateFont(13,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,FONT_NAMES[fontIdx]);

        if(!splashShowChangelog){
            // Normal loading screen
            SelectObject(hdc,fb);SetTextColor(hdc,RGB(99,102,241));SetBkMode(hdc,TRANSPARENT);
            RECT lr={0,H/2-60,W,H/2};DrawText(hdc,L"PULSEKPS",-1,&lr,DT_CENTER|DT_VCENTER|DT_SINGLELINE);

            // Loading dots animation
            DWORD tick=GetTickCount();
            int dots=(tick/400)%4;
            wchar_t dotStr[8]={};
            for(int i=0;i<dots;i++) dotStr[i]=L'.';

            SelectObject(hdc,fs);SetTextColor(hdc,RGB(100,100,140));
            RECT sr={0,H/2+10,W,H/2+32};DrawText(hdc,splashMsg.c_str(),-1,&sr,DT_CENTER|DT_TOP|DT_SINGLELINE);
            RECT dr={0,H/2+30,W,H/2+50};DrawText(hdc,dotStr,-1,&dr,DT_CENTER|DT_TOP|DT_SINGLELINE);

            SetTextColor(hdc,RGB(40,40,60));
            RECT vr={0,H-22,W,H-4};DrawText(hdc,APP_VERSION,-1,&vr,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        } else {
            // Changelog screen
            // Header
            fillRect(hdc,0,0,W,44,RGB(14,14,22));
            SelectObject(hdc,fm);SetTextColor(hdc,RGB(99,102,241));SetBkMode(hdc,TRANSPARENT);
            RECT hr={14,0,W-14,44};DrawText(hdc,L"PULSEKPS",-1,&hr,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
            SelectObject(hdc,fs);SetTextColor(hdc,RGB(80,80,110));
            RECT vhr={0,0,W-14,44};DrawText(hdc,L"WHAT'S NEW",-1,&vhr,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
            fillRect(hdc,0,43,W,1,RGB(99,102,241));

            // Scrollable changelog text
            // Clip to content area
            HRGN clipRgn=CreateRectRgn(14,48,W-14,H-44);
            SelectClipRgn(hdc,clipRgn);

            SelectObject(hdc,fs);SetTextColor(hdc,RGB(180,180,210));SetBkMode(hdc,TRANSPARENT);
            RECT tr={14,48-splashChangelogScroll,W-14,48-splashChangelogScroll+2000};
            DrawText(hdc,CHANGELOG_TEXT,-1,&tr,DT_LEFT|DT_TOP|DT_WORDBREAK);

            SelectClipRgn(hdc,NULL);
            DeleteObject(clipRgn);

            // Bottom bar with continue button
            fillRect(hdc,0,H-40,W,40,RGB(14,14,22));
            fillRect(hdc,0,H-41,W,1,RGB(32,32,48));
            // Continue pill button
            drawRR(hdc,W/2-60,H-32,120,24,12,RGB(99,102,241),RGB(0,0,0),0);
            SelectObject(hdc,fs);SetTextColor(hdc,RGB(255,255,255));
            RECT br={W/2-60,H-32,W/2+60,H-8};DrawText(hdc,L"Continue",-1,&br,DT_CENTER|DT_VCENTER|DT_SINGLELINE);

            // Scroll hint if content is long
            SelectObject(hdc,fs);SetTextColor(hdc,RGB(50,50,70));
            RECT sh={0,H-40,W/2-70,H};DrawText(hdc,L"scroll to read",-1,&sh,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        }

        DeleteObject(fb);DeleteObject(fs);DeleteObject(fm);
        EndPaint(hwnd,&ps);return 0;
    }
    case WM_MOUSEWHEEL:
        if(splashShowChangelog){
            splashChangelogScroll+=GET_WHEEL_DELTA_WPARAM(wp)>0?-30:30;
            splashChangelogScroll=std::max(0,splashChangelogScroll);
            InvalidateRect(hwnd,NULL,FALSE);
        }
        return 0;
    case WM_LBUTTONDOWN:
        if(splashShowChangelog){
            RECT cr;GetClientRect(hwnd,&cr);
            int mx=GET_X_LPARAM(lp),my=GET_Y_LPARAM(lp);
            int W=cr.right,H=cr.bottom;
            // Continue button hit area
            if(mx>=W/2-60&&mx<=W/2+60&&my>=H-32&&my<=H-8){
                DestroyWindow(hwnd);
            }
        }
        return 0;
    case WM_ERASEBKGND:return 1;
    default:return DefWindowProc(hwnd,msg,wp,lp);
    }
}
void updateSplash(const wchar_t* s){
    splashMsg=s;LOG_INFO(s);
    if(hwndSplash){InvalidateRect(hwndSplash,NULL,TRUE);UpdateWindow(hwndSplash);}
}
void showSplash(HINSTANCE hInst){
    WNDCLASS wc={};wc.lpfnWndProc=SplashProc;wc.hInstance=hInst;wc.lpszClassName=L"Sp4";wc.hCursor=LoadCursor(NULL,IDC_ARROW);RegisterClass(&wc);
    int sw=GetSystemMetrics(SM_CXSCREEN),sh=GetSystemMetrics(SM_CYSCREEN);
    hwndSplash=CreateWindowEx(WS_EX_TOPMOST|WS_EX_LAYERED,L"Sp4",L"",WS_POPUP,sw/2-185,sh/2-230,370,460,NULL,NULL,hInst,NULL);
    SetLayeredWindowAttributes(hwndSplash,0,0,LWA_ALPHA);
    ShowWindow(hwndSplash,SW_SHOW);UpdateWindow(hwndSplash);
    for(int a=0;a<=255;a+=10){SetLayeredWindowAttributes(hwndSplash,0,(BYTE)a,LWA_ALPHA);Sleep(8);}
}
void hideSplash(){
    for(int a=255;a>=0;a-=10){SetLayeredWindowAttributes(hwndSplash,0,(BYTE)a,LWA_ALPHA);Sleep(8);}
    DestroyWindow(hwndSplash);hwndSplash=NULL;
}

// ===================== LICENSE WINDOW =====================
LRESULT CALLBACK LicenseProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    static HWND hwndInput=NULL,hwndBtn=NULL,hwndMsg=NULL;
    switch(msg){
    case WM_CREATE:{
        HFONT f=CreateFont(14,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,FONT_NAMES[fontIdx]);
        HFONT fb=CreateFont(17,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,FONT_NAMES[fontIdx]);
        HWND l1=CreateWindow(L"STATIC",L"Activate Macro",WS_CHILD|WS_VISIBLE|SS_CENTER,20,52,330,22,hwnd,NULL,NULL,NULL);SendMessage(l1,WM_SETFONT,(WPARAM)fb,TRUE);
        HWND l2=CreateWindow(L"STATIC",L"Enter the license key received after purchase.",WS_CHILD|WS_VISIBLE|SS_CENTER,20,78,330,16,hwnd,NULL,NULL,NULL);SendMessage(l2,WM_SETFONT,(WPARAM)f,TRUE);
        hwndInput=CreateWindow(L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_CENTER,20,106,330,26,hwnd,(HMENU)ID_LICENSE_INPUT,NULL,NULL);SendMessage(hwndInput,WM_SETFONT,(WPARAM)f,TRUE);
        SendMessage(hwndInput,EM_SETCUEBANNER,FALSE,(LPARAM)L"MACRO-XXXX-XXXX-XXXX-XXXX");
        hwndBtn=CreateWindow(L"BUTTON",L"Activate",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,20,144,330,30,hwnd,(HMENU)ID_LICENSE_BTN,NULL,NULL);SendMessage(hwndBtn,WM_SETFONT,(WPARAM)f,TRUE);
        hwndMsg=CreateWindow(L"STATIC",L"",WS_CHILD|WS_VISIBLE|SS_CENTER,20,184,330,18,hwnd,NULL,NULL,NULL);SendMessage(hwndMsg,WM_SETFONT,(WPARAM)f,TRUE);
        break;
    }
    case WM_CTLCOLORSTATIC:{HDC h=(HDC)wp;SetBkMode(h,TRANSPARENT);SetTextColor(h,T.text);static HBRUSH b=NULL;if(b)DeleteObject(b);b=CreateSolidBrush(T.surface);return(LRESULT)b;}
    case WM_CTLCOLOREDIT:{HDC h=(HDC)wp;SetTextColor(h,T.text);SetBkColor(h,T.btn);static HBRUSH b=NULL;if(b)DeleteObject(b);b=CreateSolidBrush(T.btn);return(LRESULT)b;}
    case WM_CTLCOLORBTN:{HDC h=(HDC)wp;SetTextColor(h,T.text);SetBkColor(h,T.btn);static HBRUSH b=NULL;if(b)DeleteObject(b);b=CreateSolidBrush(T.btn);return(LRESULT)b;}
    case WM_ERASEBKGND:{HDC h=(HDC)wp;RECT r;GetClientRect(hwnd,&r);HBRUSH b=CreateSolidBrush(T.surface);FillRect(h,&r,b);DeleteObject(b);return 1;}
    case WM_PAINT:{
        PAINTSTRUCT ps;HDC hdc=BeginPaint(hwnd,&ps);
        fillRect(hdc,0,0,370,42,T.bg);
        SelectObject(hdc,hFontBig);SetTextColor(hdc,T.text);SetBkMode(hdc,TRANSPARENT);
        RECT tr={14,0,300,42};DrawText(hdc,L"PULSEKPS",-1,&tr,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        fillRect(hdc,0,41,370,2,T.accent);
        EndPaint(hwnd,&ps);return 0;
    }
    case WM_COMMAND:{
        if(LOWORD(wp)==ID_LICENSE_BTN){
            wchar_t buf[64]={};GetWindowText(hwndInput,buf,64);
            std::wstring key(buf);
            if(key.empty()){SetWindowText(hwndMsg,L"Please enter your license key.");break;}
            SetWindowText(hwndMsg,L"Validating - please wait...");UpdateWindow(hwnd);
            int r=validateLicense(key,machineHWID);
            if(r==-1)SetWindowText(hwndMsg,L"Network error - check your connection and try again.");
            else if(r==0)SetWindowText(hwndMsg,L"Invalid key or already used on another machine.");
            else{saveLicense(key);savedLicenseKey=key;SetWindowText(hwndMsg,L"Activated successfully!");Sleep(700);DestroyWindow(hwnd);}
        }
        break;
    }
    case WM_DESTROY:
        KillTimer(hwnd,TIMER_SETT);
break;
    default:return DefWindowProc(hwnd,msg,wp,lp);
    }return 0;
}

bool showLicenseWindow(HINSTANCE hInst){
    WNDCLASS wc={};wc.lpfnWndProc=LicenseProc;wc.hInstance=hInst;wc.lpszClassName=L"Lic4";wc.hCursor=LoadCursor(NULL,IDC_ARROW);RegisterClass(&wc);
    HWND hwnd=CreateWindow(L"Lic4",L"Macro \u2014 Activate",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,370,250,NULL,NULL,hInst,NULL);
    BOOL dark=TRUE;DwmSetWindowAttribute(hwnd,(DWORD)20,&dark,sizeof(dark));
    ShowWindow(hwnd,SW_SHOW);UpdateWindow(hwnd);
    MSG msg;
    while(IsWindow(hwnd)){if(PeekMessage(&msg,NULL,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessage(&msg);}else Sleep(10);}
    return !savedLicenseKey.empty();
}

// ===================== WINMAIN =====================
int WINAPI WinMain(HINSTANCE hInst,HINSTANCE,LPSTR lpCmdLine,int nCmdShow){
    // If launched with /tray, start hidden in tray
    bool startHidden=(lpCmdLine&&strstr(lpCmdLine,"/tray")!=nullptr);
    if(startHidden)nCmdShow=SW_HIDE;
    gInst=hInst;
    InitializeCriticalSection(&keyListCS);
    loadAppIcon();
    lastKpsTick=GetTickCount();

    // Delete logs if older than 24 hours
    {
        HANDLE hf=CreateFile(LOG_FILE,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
        if(hf!=INVALID_HANDLE_VALUE){
            FILETIME ft;GetFileTime(hf,NULL,NULL,&ft);CloseHandle(hf);
            FILETIME now;SYSTEMTIME st;GetSystemTime(&st);SystemTimeToFileTime(&st,&now);
            ULARGE_INTEGER tNow,tFile;
            tNow.LowPart=now.dwLowDateTime;tNow.HighPart=now.dwHighDateTime;
            tFile.LowPart=ft.dwLowDateTime;tFile.HighPart=ft.dwHighDateTime;
            // 24 hours in 100-nanosecond intervals = 864000000000
            if(tNow.QuadPart-tFile.QuadPart>864000000000ULL)
                DeleteFile(LOG_FILE);
        }
    }
    LOG_INFO(L"Macro " APP_VERSION L" starting up");

    machineHWID=generateHWID();

    // Load settings first (font needed for everything)
    loadSettings();
    createFonts();

    // License check
    savedLicenseKey=loadLicense();
    if(!savedLicenseKey.empty()){
        int r=validateLicense(savedLicenseKey,machineHWID);
        if(r==0){
            // Key genuinely not found or deactivated - delete and re-ask
            DeleteFile(LICENSE_FILE);
            savedLicenseKey=L"";
            LOG_ERR(L"License removed - key was not found or has been deactivated");
        }
        // r==-1 (network error), r==3 (hwid mismatch) - keep license, just warn
    }

    // License / trial gate
    if(savedLicenseKey.empty()){
        // Always require activation - no free entry without a key
        LOG_INFO(L"No license - please activate");
        if(!showLicenseWindow(hInst)){LOG_ERR(L"Activation cancelled - exiting");return 0;}
        // After activation, check if it was a trial key (2-day keys are tracked via Supabase)
        // Trial tracking is handled server-side via the license expiry
    }
    // Check trial status for display purposes
    initTrial();

    showSplash(hInst);
    updateSplash(L"Loading settings...");Sleep(200);
    updateSplash(L"Benchmarking your system...");
    recommendedKPS=benchmarkSystem();
    {std::wifstream chk(SETTINGS_FILE);if(!chk.is_open())kps=recommendedKPS;}
    Sleep(300);
    updateSplash(L"Starting macro engine...");Sleep(180);
    updateSplash(L"Ready.");Sleep(300);
    // Show changelog inline if new version
    if(!changelogShown&&showChangelogOnStartup){
        splashShowChangelog=true;
        splashChangelogScroll=0;
        InvalidateRect(hwndSplash,NULL,TRUE);
        UpdateWindow(hwndSplash);
        // Wait for user to click Continue - block here until dismissed
        MSG msg;
        while(hwndSplash&&IsWindow(hwndSplash)){
            if(PeekMessage(&msg,NULL,0,0,PM_REMOVE)){
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } else {
                Sleep(16);
            }
        }
        hwndSplash=NULL;
        changelogShown=true;
        saveSettings();
    } else {
        hideSplash();
    }

    InitCommonControls();

    WNDCLASS wc={};
    wc.lpfnWndProc=WndProc;wc.hInstance=hInst;wc.lpszClassName=L"MApp_91x";
    wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground=CreateSolidBrush(THEMES[themeIdx].bg); // Force solid bg
    wc.style=CS_OWNDC;
    RegisterClass(&wc);

    HWND hwnd=CreateWindow(L"MApp_91x",L"PulseKPS",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,APP_W,APP_H,
        NULL,NULL,hInst,NULL);

    // Dark title bar
    BOOL dark=TRUE;
    DwmSetWindowAttribute(hwnd,(DWORD)20,&dark,sizeof(dark));

    // Explicitly disable all DWM backdrop/mica/acrylic effects
    // DWMWA_SYSTEMBACKDROP_TYPE = 1 means DWMSBT_NONE
    int backdropNone=1;
    DwmSetWindowAttribute(hwnd,(DWORD)38,&backdropNone,sizeof(backdropNone));
    // DWMWA_USE_IMMERSIVE_DARK_MODE already set above
    // DWMWA_MICA_EFFECT = 0 (disable mica)
    int micaOff=0;
    DwmSetWindowAttribute(hwnd,(DWORD)1029,&micaOff,sizeof(micaOff));

    // Pin caption/border to theme bg
    COLORREF bgCol=T.bg;
    DwmSetWindowAttribute(hwnd,(DWORD)35,&bgCol,sizeof(bgCol));
    DwmSetWindowAttribute(hwnd,(DWORD)34,&bgCol,sizeof(bgCol));

    // Disable DWM blur
    DWM_BLURBEHIND bb={};
    bb.dwFlags=DWM_BB_ENABLE;
    bb.fEnable=FALSE;
    DwmEnableBlurBehindWindow(hwnd,&bb);

    // Solid class background
    SetClassLongPtr(hwnd,GCLP_HBRBACKGROUND,(LONG_PTR)CreateSolidBrush(T.bg));

    hwndMain=hwnd;

    // Set window icon (taskbar + title bar)
    if(gAppIcon){
        SendMessage(hwnd,WM_SETICON,ICON_BIG,(LPARAM)gAppIcon);
        SendMessage(hwnd,WM_SETICON,ICON_SMALL,(LPARAM)getSmallIcon());
    }
    ShowWindow(hwnd,nCmdShow);UpdateWindow(hwnd);
    fadeAlpha=0.0f;
    SetTimer(hwnd,TIMER_ANIM,50,NULL);

    spawnOverlay(); // Spawn if either enabled

    // System tray
    addTrayIcon(hwnd);

    // Sync auto-launch toggle with registry
    autoLaunchEnabled=checkAutoLaunchReg();

    // Check for updates in background
    std::thread(checkForUpdate).detach();

    // Changelog handled inside splash - no action needed here

    LOG_OK(L"Ready!");

    std::thread(focusThread).detach();
    std::thread(macroThread).detach();
    std::thread(hotkeyThread).detach();
    std::thread(resourceThread).detach();
    std::thread(robloxLaunchWatchThread).detach();

    MSG flush;while(PeekMessage(&flush,NULL,WM_QUIT,WM_QUIT,PM_REMOVE)){}

    MSG msg;
    while(GetMessage(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);}

    DeleteCriticalSection(&keyListCS);
    return 0;
}