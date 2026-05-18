#define UNICODE
#define _UNICODE
#include <windows.h>
#include <wininet.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "user32.lib")

#define SUPABASE_URL L"autxjoshycjhnvdpbtmn.supabase.co"
#define SUPABASE_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImF1dHhqb3NoeWNqaG52ZHBidG1uIiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImlhdCI6MTc3ODg1MTAxMSwiZXhwIjoyMDk0NDI3MDExfQ.jZUlYNFIPInicnl8EWnrqL_XBo-uggQIlohg9GxAUeg"

// Generate a random license key: MACRO-XXXX-XXXX-XXXX-XXXX
std::wstring generateLicenseKey() {
    srand((unsigned int)time(NULL) ^ GetTickCount());
    const wchar_t* chars = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int len = 36;
    auto rndSeg = [&]() {
        std::wstring s;
        for (int i = 0; i < 4; i++)
            s += chars[rand() % 36];
        return s;
    };
    return L"MACRO-" + rndSeg() + L"-" + rndSeg() + L"-" + rndSeg() + L"-" + rndSeg();
}

// Narrow string helper
std::string wToStr(const std::wstring& w) {
    std::string s(w.begin(), w.end());
    return s;
}

// Send license key to Supabase via HTTP POST
bool insertKeyToSupabase(const std::wstring& key) {
    HINTERNET hInternet = InternetOpen(L"KeyGen/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hConnect = InternetConnect(hInternet, SUPABASE_URL,
        INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) { InternetCloseHandle(hInternet); return false; }

    LPCWSTR acceptTypes[] = { L"application/json", NULL };
    HINTERNET hRequest = HttpOpenRequest(hConnect, L"POST", L"/rest/v1/licenses",
        NULL, NULL, acceptTypes,
        INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hRequest) { InternetCloseHandle(hConnect); InternetCloseHandle(hInternet); return false; }

    std::string headers =
        "Content-Type: application/json\r\n"
        "apikey: " SUPABASE_KEY "\r\n"
        "Authorization: Bearer " SUPABASE_KEY "\r\n"
        "Prefer: return=minimal\r\n";

    std::string keyNarrow = wToStr(key);
    std::string body = "{\"license_key\":\"" + keyNarrow + "\",\"is_active\":true}";

    BOOL ok = HttpSendRequestA(hRequest,
        headers.c_str(), (DWORD)headers.length(),
        (LPVOID)body.c_str(), (DWORD)body.length());

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
        &statusCode, &statusSize, NULL);

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return (statusCode == 201 || statusCode == 200);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    std::wstring key = generateLicenseKey();

    // Try to insert into Supabase
    bool success = insertKeyToSupabase(key);

    if (success) {
        std::wstring msg = L"License key generated and saved to Supabase:\n\n" + key + L"\n\nCopy and send this to the buyer.";
        // Copy to clipboard
        if (OpenClipboard(NULL)) {
            EmptyClipboard();
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (key.size() + 1) * sizeof(wchar_t));
            if (hMem) {
                memcpy(GlobalLock(hMem), key.c_str(), (key.size() + 1) * sizeof(wchar_t));
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
            CloseClipboard();
        }
        msg += L"\n\n(Key copied to clipboard)";
        MessageBox(NULL, msg.c_str(), L"Key Generated", MB_OK | MB_ICONINFORMATION);
    } else {
        std::wstring msg = L"Failed to save to Supabase. Check your internet connection.\n\nKey was:\n" + key;
        MessageBox(NULL, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
    }

    return 0;
}