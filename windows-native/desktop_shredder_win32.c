/*
 * DesktopShredder v1.2 — Standalone Zero-Dependency Native Win64 GUI Application
 * Pure Win32 + OLE DragDrop API (Zero CRT, Zero Network DLLs, Zero Runtime Dependencies)
 *
 * v1.2 New Features:
 * - Live OLE IDropTarget hover detection: switches floating icon to a crisp single Document/File
 *   icon immediately when dragging files over the widget, and restores on DragLeave.
 * - Multi-stage 40 FPS Paper Shredding Animation: document sheet feeds smoothly into the vibrating
 *   cutter slot, blades spin, crimson LED pulses, and sliced paper strips cascade down the window.
 */

#include <stdint.h>
#include <stddef.h>

#define WINAPI __attribute__((ms_abi))

/* Exact LLP64 Win64 Primitive Types */
typedef int32_t   BOOL;
typedef int32_t   LONG;
typedef int32_t   HRESULT;
typedef uint32_t  DWORD;
typedef uint32_t  UINT;
typedef uint32_t  ULONG;
typedef uint16_t  WORD;
typedef uint16_t  WCHAR;
typedef uint16_t  ATOM;
typedef uint8_t   BYTE;
typedef int64_t   LONGLONG;
typedef uint64_t  ULONGLONG;
typedef int64_t   LONG_PTR;
typedef uint64_t  ULONG_PTR;
typedef uint64_t  UINT_PTR;
typedef int64_t   LRESULT;
typedef uint64_t  WPARAM;
typedef int64_t   LPARAM;
typedef uint64_t  SIZE_T;

typedef void *HANDLE;
typedef void *HWND;
typedef void *HINSTANCE;
typedef void *HICON;
typedef void *HCURSOR;
typedef void *HBRUSH;
typedef void *HFONT;
typedef void *HDC;
typedef void *HBITMAP;
typedef void *HGDIOBJ;
typedef void *HMENU;
typedef void *HDROP;
typedef void *HMONITOR;
typedef void *HKEY;

typedef LRESULT (WINAPI *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef DWORD   (WINAPI *LPTHREAD_START_ROUTINE)(void *);

/* Win32 & OLE Constants */
#define TRUE  1
#define FALSE 0
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

#define S_OK                ((HRESULT)0)
#define S_FALSE             ((HRESULT)1)
#define E_NOINTERFACE       ((HRESULT)0x80004002L)
#define E_POINTER           ((HRESULT)0x80004003L)

#define CF_HDROP            15U
#define DVASPECT_CONTENT    1U
#define TYMED_HGLOBAL       1U

#define DROPEFFECT_NONE     0U
#define DROPEFFECT_COPY     1U
#define DROPEFFECT_MOVE     2U
#define DROPEFFECT_LINK     4U

#define WS_POPUP            0x80000000U
#define WS_CAPTION          0x00C00000U
#define WS_SYSMENU          0x00080000U
#define WS_VISIBLE          0x10000000U
#define WS_CHILD            0x40000000U
#define WS_TABSTOP          0x00010000U
#define WS_VSCROLL          0x00200000U

#define WS_EX_TOPMOST       0x00000008U
#define WS_EX_ACCEPTFILES   0x00000010U
#define WS_EX_TOOLWINDOW    0x00000080U
#define WS_EX_LAYERED       0x00080000U
#define WS_EX_NOACTIVATE    0x08000000U

#define SWP_NOSIZE          0x0001U
#define SWP_NOZORDER        0x0004U
#define SWP_NOACTIVATE      0x0010U

#define BS_PUSHBUTTON       0x00000000U
#define BS_DEFPUSHBUTTON    0x00000001U
#define BS_AUTOCHECKBOX     0x00000003U
#define CBS_DROPDOWNLIST    0x00000003U

#define BM_GETCHECK         0x00F0U
#define BM_SETCHECK         0x00F1U
#define BST_UNCHECKED       0x0000U
#define BST_CHECKED         0x0001U

#define CB_ADDSTRING        0x0143U
#define CB_GETCURSEL        0x0147U
#define CB_SETCURSEL        0x014EU

#define WM_NULL             0x0000U
#define WM_CREATE           0x0001U
#define WM_DESTROY          0x0002U
#define WM_CLOSE            0x0010U
#define WM_COMMAND          0x0111U
#define WM_TIMER            0x0113U
#define WM_MOUSEMOVE        0x0200U
#define WM_LBUTTONDOWN      0x0201U
#define WM_LBUTTONUP        0x0202U
#define WM_RBUTTONUP        0x0205U
#define WM_DROPFILES        0x0233U
#define WM_SETFONT          0x0030U
#define WM_CTLCOLORSTATIC   0x0138U
#define WM_APP_SHRED_STATE  0x8001U

#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_SHOWNOACTIVATE   4

#define SM_CXSCREEN         0
#define SM_CYSCREEN         1
#define SM_XVIRTUALSCREEN   76
#define SM_YVIRTUALSCREEN   77
#define SM_CXVIRTUALSCREEN  78
#define SM_CYVIRTUALSCREEN  79

#define IDC_ARROW           ((const WCHAR *)(uintptr_t)32512)
#define IDC_SIZEALL         ((const WCHAR *)(uintptr_t)32646)

#define ULW_ALPHA           0x00000002U
#define AC_SRC_OVER         0x00
#define AC_SRC_ALPHA        0x01
#define DIB_RGB_COLORS      0

#define TPM_RIGHTBUTTON     0x0002U
#define TPM_RETURNCMD       0x0100U
#define MF_STRING           0x0000U
#define MF_CHECKED          0x0008U
#define MF_SEPARATOR        0x0800U

#define GWL_STYLE           (-16)
#define MONITOR_DEFAULTTONEAREST 2U
#define MSGFLT_ALLOW        1U

#define GENERIC_READ        0x80000000U
#define GENERIC_WRITE       0x40000000U
#define OPEN_EXISTING       3U
#define CREATE_ALWAYS       2U
#define FILE_ATTRIBUTE_DIRECTORY     0x00000010U
#define FILE_ATTRIBUTE_NORMAL        0x00000080U
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400U
#define FILE_FLAG_WRITE_THROUGH      0x80000000U
#define FILE_FLAG_SEQUENTIAL_SCAN    0x08000000U
#define FILE_FLAG_BACKUP_SEMANTICS   0x02000000U
#define FILE_BEGIN          0U

#define MEM_COMMIT          0x00001000U
#define MEM_RESERVE         0x00002000U
#define MEM_RELEASE         0x00008000U
#define PAGE_READWRITE      0x04U
#define HEAP_ZERO_MEMORY    0x00000008U

#define ERROR_ALREADY_EXISTS 183U
#define INFINITE            0xFFFFFFFFU

#define HKEY_CURRENT_USER   ((HKEY)(uintptr_t)0x80000001ULL)
#define KEY_READ            0x20019U
#define KEY_WRITE           0x20006U
#define REG_SZ              1U

#define BCRYPT_USE_SYSTEM_PREFERRED_RNG 0x00000002U
#define LOGPIXELSX          88

/* Exact Win64 & COM Struct Definitions */
typedef struct { LONG x; LONG y; } POINT;
typedef struct { LONG cx; LONG cy; } SIZE;
typedef struct { LONG left; LONG top; LONG right; LONG bottom; } RECT;
typedef struct { BYTE BlendOp; BYTE BlendFlags; BYTE SourceConstantAlpha; BYTE AlphaFormat; } BLENDFUNCTION;
typedef struct { void *Ptr; } SRWLOCK;
typedef struct { DWORD dwLowDateTime; DWORD dwHighDateTime; } FILETIME;

typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
} GUID;

typedef struct {
    WORD   cfFormat;
    void  *ptd;
    DWORD  dwAspect;
    LONG   lindex;
    DWORD  tymed;
} FORMATETC;

typedef struct {
    DWORD  tymed;
    HANDLE hGlobal;
    void  *pUnkForRelease;
} STGMEDIUM;

typedef struct IDataObject IDataObject;
typedef struct {
    HRESULT (WINAPI *QueryInterface)(IDataObject *, const GUID *, void **);
    ULONG   (WINAPI *AddRef)(IDataObject *);
    ULONG   (WINAPI *Release)(IDataObject *);
    HRESULT (WINAPI *GetData)(IDataObject *, FORMATETC *, STGMEDIUM *);
    HRESULT (WINAPI *GetDataHere)(IDataObject *, FORMATETC *, STGMEDIUM *);
    HRESULT (WINAPI *QueryGetData)(IDataObject *, FORMATETC *);
    HRESULT (WINAPI *GetCanonicalFormatEtc)(IDataObject *, FORMATETC *, FORMATETC *);
    HRESULT (WINAPI *SetData)(IDataObject *, FORMATETC *, STGMEDIUM *, BOOL);
    HRESULT (WINAPI *EnumFormatEtc)(IDataObject *, DWORD, void **);
    HRESULT (WINAPI *DAdvise)(IDataObject *, FORMATETC *, DWORD, void *, DWORD *);
    HRESULT (WINAPI *DUnadvise)(IDataObject *, DWORD);
    HRESULT (WINAPI *EnumDAdvise)(IDataObject *, void **);
} IDataObjectVtbl;

struct IDataObject {
    const IDataObjectVtbl *lpVtbl;
};

typedef struct IDropTarget IDropTarget;
typedef struct {
    HRESULT (WINAPI *QueryInterface)(IDropTarget *, const GUID *, void **);
    ULONG   (WINAPI *AddRef)(IDropTarget *);
    ULONG   (WINAPI *Release)(IDropTarget *);
    HRESULT (WINAPI *DragEnter)(IDropTarget *, IDataObject *, DWORD, POINT, DWORD *);
    HRESULT (WINAPI *DragOver)(IDropTarget *, DWORD, POINT, DWORD *);
    HRESULT (WINAPI *DragLeave)(IDropTarget *);
    HRESULT (WINAPI *Drop)(IDropTarget *, IDataObject *, DWORD, POINT, DWORD *);
} IDropTargetVtbl;

struct IDropTarget {
    const IDropTargetVtbl *lpVtbl;
};

typedef struct {
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER;

typedef struct {
    BITMAPINFOHEADER bmiHeader;
    DWORD bmiColors[1];
} BITMAPINFO;

typedef struct {
    UINT      cbSize;
    UINT      style;
    WNDPROC   lpfnWndProc;
    int32_t   cbClsExtra;
    int32_t   cbWndExtra;
    HINSTANCE hInstance;
    HICON     hIcon;
    HCURSOR   hCursor;
    HBRUSH    hbrBackground;
    const WCHAR *lpszMenuName;
    const WCHAR *lpszClassName;
    HICON     hIconSm;
} WNDCLASSEXW;

typedef struct {
    HWND     hwnd;
    UINT     message;
    WPARAM   wParam;
    LPARAM   lParam;
    DWORD    time;
    POINT    pt;
} MSG;

typedef struct {
    DWORD cbSize;
    RECT  rcMonitor;
    RECT  rcWork;
    DWORD dwFlags;
} MONITORINFO;

typedef struct {
    DWORD    dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD    nFileSizeHigh;
    DWORD    nFileSizeLow;
    DWORD    dwReserved0;
    DWORD    dwReserved1;
    WCHAR    cFileName[260];
    WCHAR    cAlternateFileName[14];
} WIN32_FIND_DATAW;

typedef struct {
    LONGLONG StreamSize;
    WCHAR    cStreamName[296];
} WIN32_FIND_STREAM_DATA;

/* Compile-Time Win64 ABI Assertions */
_Static_assert(sizeof(POINT) == 8, "POINT size mismatch");
_Static_assert(sizeof(RECT) == 16, "RECT size mismatch");
_Static_assert(sizeof(GUID) == 16, "GUID size mismatch");
_Static_assert(sizeof(FORMATETC) == 32, "FORMATETC size mismatch");
_Static_assert(offsetof(FORMATETC, ptd) == 8, "FORMATETC.ptd offset mismatch");
_Static_assert(offsetof(FORMATETC, tymed) == 24, "FORMATETC.tymed offset mismatch");
_Static_assert(sizeof(STGMEDIUM) == 24, "STGMEDIUM size mismatch");
_Static_assert(offsetof(STGMEDIUM, hGlobal) == 8, "STGMEDIUM.hGlobal offset mismatch");
_Static_assert(sizeof(BLENDFUNCTION) == 4, "BLENDFUNCTION size mismatch");
_Static_assert(sizeof(BITMAPINFOHEADER) == 40, "BITMAPINFOHEADER size mismatch");
_Static_assert(sizeof(WNDCLASSEXW) == 80, "WNDCLASSEXW size mismatch");
_Static_assert(sizeof(MSG) == 48, "MSG size mismatch");
_Static_assert(sizeof(MONITORINFO) == 40, "MONITORINFO size mismatch");
_Static_assert(sizeof(WIN32_FIND_DATAW) == 592, "WIN32_FIND_DATAW size mismatch");
_Static_assert(sizeof(WIN32_FIND_STREAM_DATA) == 600, "WIN32_FIND_STREAM_DATA size mismatch");

/* Imported Win32 API Pointer Declarations (__imp_*) */
#define IMP(ret, name, args) extern ret (WINAPI * const __imp_##name) args
#define W32(name) (*__imp_##name)

/* KERNEL32.dll */
IMP(void,      ExitProcess, (UINT));
IMP(HINSTANCE, GetModuleHandleW, (const WCHAR *));
IMP(DWORD,     GetModuleFileNameW, (HINSTANCE, WCHAR *, DWORD));
IMP(DWORD,     GetCurrentProcessId, (void));
IMP(DWORD,     GetLastError, (void));
IMP(HANDLE,    CreateMutexW, (void *, BOOL, const WCHAR *));
IMP(HANDLE,    CreateEventW, (void *, BOOL, BOOL, const WCHAR *));
IMP(BOOL,      SetEvent, (HANDLE));
IMP(DWORD,     WaitForSingleObject, (HANDLE, DWORD));
IMP(void,      Sleep, (DWORD));
IMP(HANDLE,    CreateThread, (void *, SIZE_T, LPTHREAD_START_ROUTINE, void *, DWORD, DWORD *));
IMP(void,      AcquireSRWLockExclusive, (SRWLOCK *));
IMP(void,      ReleaseSRWLockExclusive, (SRWLOCK *));
IMP(HANDLE,    GetProcessHeap, (void));
IMP(void *,    HeapAlloc, (HANDLE, DWORD, SIZE_T));
IMP(BOOL,      HeapFree, (HANDLE, DWORD, void *));
IMP(void *,    VirtualAlloc, (void *, SIZE_T, DWORD, DWORD));
IMP(BOOL,      VirtualFree, (void *, SIZE_T, DWORD));
IMP(DWORD,     GetEnvironmentVariableW, (const WCHAR *, WCHAR *, DWORD));
IMP(BOOL,      CreateDirectoryW, (const WCHAR *, void *));
IMP(DWORD,     GetFileAttributesW, (const WCHAR *));
IMP(BOOL,      SetFileAttributesW, (const WCHAR *, DWORD));
IMP(HANDLE,    CreateFileW, (const WCHAR *, DWORD, DWORD, void *, DWORD, DWORD, HANDLE));
IMP(BOOL,      GetFileSizeEx, (HANDLE, LONGLONG *));
IMP(BOOL,      SetFilePointerEx, (HANDLE, LONGLONG, LONGLONG *, DWORD));
IMP(BOOL,      SetEndOfFile, (HANDLE));
IMP(BOOL,      WriteFile, (HANDLE, const void *, DWORD, DWORD *, void *));
IMP(BOOL,      ReadFile, (HANDLE, void *, DWORD, DWORD *, void *));
IMP(BOOL,      FlushFileBuffers, (HANDLE));
IMP(BOOL,      SetFileTime, (HANDLE, const FILETIME *, const FILETIME *, const FILETIME *));
IMP(BOOL,      CloseHandle, (HANDLE));
IMP(BOOL,      MoveFileExW, (const WCHAR *, const WCHAR *, DWORD));
IMP(BOOL,      DeleteFileW, (const WCHAR *));
IMP(BOOL,      RemoveDirectoryW, (const WCHAR *));
IMP(HANDLE,    FindFirstFileW, (const WCHAR *, WIN32_FIND_DATAW *));
IMP(BOOL,      FindNextFileW, (HANDLE, WIN32_FIND_DATAW *));
IMP(BOOL,      FindClose, (HANDLE));
IMP(HANDLE,    FindFirstStreamW, (const WCHAR *, int32_t, WIN32_FIND_STREAM_DATA *, DWORD));
IMP(BOOL,      FindNextStreamW, (HANDLE, WIN32_FIND_STREAM_DATA *));

/* USER32.dll */
IMP(ATOM,      RegisterClassExW, (const WNDCLASSEXW *));
IMP(HWND,      FindWindowW, (const WCHAR *, const WCHAR *));
IMP(HWND,      CreateWindowExW, (DWORD, const WCHAR *, const WCHAR *, DWORD, int32_t, int32_t, int32_t, int32_t, HWND, HMENU, HINSTANCE, void *));
IMP(BOOL,      DestroyWindow, (HWND));
IMP(BOOL,      ShowWindow, (HWND, int32_t));
IMP(BOOL,      UpdateWindow, (HWND));
IMP(BOOL,      IsWindowVisible, (HWND));
IMP(BOOL,      IsIconic, (HWND));
IMP(BOOL,      SetForegroundWindow, (HWND));
IMP(BOOL,      GetMessageW, (MSG *, HWND, UINT, UINT));
IMP(BOOL,      TranslateMessage, (const MSG *));
IMP(LRESULT,   DispatchMessageW, (const MSG *));
IMP(void,      PostQuitMessage, (int32_t));
IMP(BOOL,      PostMessageW, (HWND, UINT, WPARAM, LPARAM));
IMP(LRESULT,   SendMessageW, (HWND, UINT, WPARAM, LPARAM));
IMP(LRESULT,   DefWindowProcW, (HWND, UINT, WPARAM, LPARAM));
IMP(HCURSOR,   LoadCursorW, (HINSTANCE, const WCHAR *));
IMP(HICON,     LoadIconW, (HINSTANCE, const WCHAR *));
IMP(HDC,       GetDC, (HWND));
IMP(int32_t,   ReleaseDC, (HWND, HDC));
IMP(BOOL,      UpdateLayeredWindow, (HWND, HDC, POINT *, SIZE *, HDC, POINT *, DWORD, BLENDFUNCTION *, DWORD));
IMP(BOOL,      GetWindowRect, (HWND, RECT *));
IMP(BOOL,      SetWindowPos, (HWND, HWND, int32_t, int32_t, int32_t, int32_t, UINT));
IMP(BOOL,      GetCursorPos, (POINT *));
IMP(HWND,      SetCapture, (HWND));
IMP(BOOL,      ReleaseCapture, (void));
IMP(UINT_PTR,  SetTimer, (HWND, UINT_PTR, UINT, void *));
IMP(BOOL,      KillTimer, (HWND, UINT_PTR));
IMP(HMENU,     CreatePopupMenu, (void));
IMP(BOOL,      AppendMenuW, (HMENU, UINT, UINT_PTR, const WCHAR *));
IMP(BOOL,      TrackPopupMenu, (HMENU, UINT, int32_t, int32_t, int32_t, HWND, const RECT *));
IMP(BOOL,      DestroyMenu, (HMENU));
IMP(int32_t,   GetSystemMetrics, (int32_t));
IMP(HWND,      GetForegroundWindow, (void));
IMP(DWORD,     GetWindowThreadProcessId, (HWND, DWORD *));
IMP(int32_t,   GetClassNameW, (HWND, WCHAR *, int32_t));
IMP(LONG_PTR,  GetWindowLongPtrW, (HWND, int32_t));
IMP(HMONITOR,  MonitorFromWindow, (HWND, DWORD));
IMP(BOOL,      GetMonitorInfoW, (HMONITOR, MONITORINFO *));
IMP(BOOL,      ChangeWindowMessageFilterEx, (HWND, UINT, DWORD, void *));

/* GDI32.dll */
IMP(HDC,       CreateCompatibleDC, (HDC));
IMP(BOOL,      DeleteDC, (HDC));
IMP(HBITMAP,   CreateDIBSection, (HDC, const BITMAPINFO *, UINT, void **, HANDLE, DWORD));
IMP(HGDIOBJ,   SelectObject, (HDC, HGDIOBJ));
IMP(BOOL,      DeleteObject, (HGDIOBJ));
IMP(HFONT,     CreateFontW, (int32_t, int32_t, int32_t, int32_t, int32_t, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, const WCHAR *));
IMP(HBRUSH,    CreateSolidBrush, (DWORD));
IMP(int32_t,   GetDeviceCaps, (HDC, int32_t));

/* SHELL32.dll */
IMP(void,      DragAcceptFiles, (HWND, BOOL));
IMP(UINT,      DragQueryFileW, (HDROP, UINT, WCHAR *, UINT));
IMP(void,      DragFinish, (HDROP));
IMP(int32_t,   SHQueryUserNotificationState, (int32_t *));

/* ADVAPI32.dll */
IMP(LONG,      RegCreateKeyExW, (HKEY, const WCHAR *, DWORD, WCHAR *, DWORD, DWORD, void *, HKEY *, DWORD *));
IMP(LONG,      RegOpenKeyExW, (HKEY, const WCHAR *, DWORD, DWORD, HKEY *));
IMP(LONG,      RegSetValueExW, (HKEY, const WCHAR *, DWORD, DWORD, const BYTE *, DWORD));
IMP(LONG,      RegQueryValueExW, (HKEY, const WCHAR *, DWORD *, DWORD *, BYTE *, DWORD *));
IMP(LONG,      RegDeleteValueW, (HKEY, const WCHAR *));
IMP(LONG,      RegCloseKey, (HKEY));

/* BCRYPT.dll */
IMP(int32_t,   BCryptGenRandom, (HANDLE, BYTE *, ULONG, ULONG));

/* OLE32.dll */
IMP(HRESULT,   OleInitialize, (void *));
IMP(void,      OleUninitialize, (void));
IMP(HRESULT,   RegisterDragDrop, (HWND, IDropTarget *));
IMP(HRESULT,   RevokeDragDrop, (HWND));
IMP(void,      ReleaseStgMedium, (STGMEDIUM *));

/* Minimal freestanding memory/string helpers */
void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memset(void *dst, int c, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    uint8_t v = (uint8_t)c;
    while (n--) *d++ = v;
    return dst;
}

static int guid_eq(const GUID *a, const GUID *b) {
    const uint64_t *pa = (const uint64_t *)a;
    const uint64_t *pb = (const uint64_t *)b;
    return (pa[0] == pb[0]) && (pa[1] == pb[1]);
}

static size_t wstr_len(const WCHAR *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static void wstr_copy(WCHAR *dst, size_t cap, const WCHAR *src) {
    if (cap == 0) return;
    size_t i = 0;
    while (i + 1 < cap && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static void wstr_cat(WCHAR *dst, size_t cap, const WCHAR *src) {
    size_t dlen = wstr_len(dst);
    if (dlen + 1 >= cap) return;
    wstr_copy(dst + dlen, cap - dlen, src);
}

static int wstr_ieq_ascii(const WCHAR *w, const char *a) {
    size_t i = 0;
    while (w[i] && a[i]) {
        WCHAR cw = w[i];
        WCHAR ca = (uint8_t)a[i];
        if (cw >= 'A' && cw <= 'Z') cw += 32;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cw != ca) return 0;
        i++;
    }
    return (w[i] == 0 && a[i] == 0);
}

static int wstr_ends_with_data_ads(const WCHAR *s) {
    if (wstr_ieq_ascii(s, "::$DATA")) return 0;
    size_t len = wstr_len(s);
    if (len < 7) return 0;
    return wstr_ieq_ascii(s + len - 6, ":$DATA");
}

/* Application Global State */
#define TIMER_FULLSCREEN 1
#define TIMER_ANIM       2

#define IDM_SETTINGS     1001
#define IDM_AUTOSTART    1002
#define IDM_LOCKPOS      1003
#define IDM_RESETPOS     1004
#define IDM_EXIT         1005

#define IDC_CHK_AUTO     2001
#define IDC_CHK_LOCK     2002
#define IDC_CMB_PASSES   2003
#define IDC_CMB_OPACITY  2004
#define IDC_BTN_SAVE     2005
#define IDC_BTN_CANCEL   2006

typedef struct ShredJob {
    WCHAR *path;
    struct ShredJob *next;
} ShredJob;

static HINSTANCE g_hInst;
static HWND      g_hwndMain;
static HWND      g_hwndSettings;
static HFONT     g_hFontUI;
static HBRUSH    g_hBrushDlgBg;
static DWORD     g_selfPid;
static HANDLE    g_heap;
static HANDLE    g_workEvent;
static SRWLOCK   g_queueLock = {0};
static ShredJob *g_jobHead = NULL;
static ShredJob *g_jobTail = NULL;

static volatile LONG g_activeShreds = 0;
static volatile LONG g_shredPasses  = 7;
static int32_t   g_winX        = -99999;
static int32_t   g_winY        = -99999;
static int32_t   g_widgetW     = 112;
static int32_t   g_widgetH     = 124;
static float     g_dpiScale    = 1.0f;
static uint8_t   g_opacityByte = 242;
static BOOL      g_lockPos     = FALSE;
static BOOL      g_dragging    = FALSE;
static POINT     g_dragStartMouse;
static POINT     g_dragStartWin;
static BOOL      g_hiddenByFs  = FALSE;

/* Drag-Over File Icon & Shredding Animation State */
static BOOL      g_dragHoverFile    = FALSE;
static BOOL      g_canAcceptDrop    = FALSE;
static BOOL      g_shredAnimActive  = FALSE;
static int32_t   g_animTick         = 0;
static int32_t   g_animMinRemaining = 0;

static HWND g_chkAuto, g_chkLock, g_cmbPasses, g_cmbOpacity;
static IDropTargetVtbl g_dropVtbl;
static IDropTarget     g_dropTarget;

static void RenderLayeredShredder(void);

/* ======================== Configuration Persistence (Offline INI) ======================== */

static BOOL GetConfigFilePath(WCHAR *outPath, size_t cap, BOOL createDir) {
    WCHAR appData[520];
    DWORD n = W32(GetEnvironmentVariableW)(u"LOCALAPPDATA", appData, 500);
    if (n == 0 || n >= 500) return FALSE;
    wstr_copy(outPath, cap, appData);
    wstr_cat(outPath, cap, u"\\DesktopShredder");
    if (createDir) {
        W32(CreateDirectoryW)(outPath, NULL);
    }
    wstr_cat(outPath, cap, u"\\settings.cfg");
    return TRUE;
}

static int parse_int(const char **p) {
    const char *s = *p;
    while (*s == ' ' || *s == '\t') s++;
    int sign = 1;
    if (*s == '-') { sign = -1; s++; }
    int val = 0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    *p = s;
    return val * sign;
}

static void LoadConfig(void) {
    WCHAR cfgPath[560];
    if (!GetConfigFilePath(cfgPath, 560, FALSE)) return;
    HANDLE h = W32(CreateFileW)(cfgPath, GENERIC_READ, 1, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    char buf[256];
    DWORD rd = 0;
    if (W32(ReadFile)(h, buf, sizeof(buf) - 1, &rd, NULL) && rd > 0) {
        buf[rd] = 0;
        const char *p = buf;
        while (*p) {
            if (p[0] == 'X' && p[1] == '=') { p += 2; g_winX = parse_int(&p); }
            else if (p[0] == 'Y' && p[1] == '=') { p += 2; g_winY = parse_int(&p); }
            else if (p[0] == 'P' && p[1] == '=') {
                p += 2;
                int passes = parse_int(&p);
                g_shredPasses = (passes >= 35) ? 35 : ((passes <= 3) ? 3 : 7);
            }
            else if (p[0] == 'O' && p[1] == '=') {
                p += 2;
                int op = parse_int(&p);
                if (op < 100) op = 100;
                if (op > 255) op = 255;
                g_opacityByte = (uint8_t)op;
            }
            else if (p[0] == 'L' && p[1] == '=') {
                p += 2;
                g_lockPos = parse_int(&p) ? TRUE : FALSE;
            }
            while (*p && *p != '\n') p++;
            if (*p == '\n') p++;
        }
    }
    W32(CloseHandle)(h);
}

static char *append_str(char *dst, const char *s) {
    while (*s) *dst++ = *s++;
    return dst;
}

static char *append_int(char *dst, int v) {
    if (v < 0) { *dst++ = '-'; v = -v; }
    char tmp[16];
    int n = 0;
    if (v == 0) tmp[n++] = '0';
    while (v > 0) { tmp[n++] = (char)('0' + (v % 10)); v /= 10; }
    while (n > 0) *dst++ = tmp[--n];
    *dst++ = '\r';
    *dst++ = '\n';
    return dst;
}

static void SaveConfig(void) {
    WCHAR cfgPath[560];
    if (!GetConfigFilePath(cfgPath, 560, TRUE)) return;
    HANDLE h = W32(CreateFileW)(cfgPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    char buf[160];
    char *p = buf;
    p = append_str(p, "X="); p = append_int(p, g_winX);
    p = append_str(p, "Y="); p = append_int(p, g_winY);
    p = append_str(p, "P="); p = append_int(p, (int)g_shredPasses);
    p = append_str(p, "O="); p = append_int(p, (int)g_opacityByte);
    p = append_str(p, "L="); p = append_int(p, g_lockPos ? 1 : 0);
    DWORD wr = 0;
    W32(WriteFile)(h, buf, (DWORD)(p - buf), &wr, NULL);
    W32(CloseHandle)(h);
}

/* ======================== Registry Launch-at-Startup ======================== */

static const WCHAR kRunSubKey[] = u"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const WCHAR kAppValName[] = u"DesktopShredder";

static BOOL IsAutoStartEnabled(void) {
    HKEY hKey = NULL;
    if (W32(RegOpenKeyExW)(HKEY_CURRENT_USER, kRunSubKey, 0, KEY_READ, &hKey) != 0) {
        return FALSE;
    }
    WCHAR val[560];
    DWORD cb = sizeof(val);
    DWORD type = 0;
    LONG rc = W32(RegQueryValueExW)(hKey, kAppValName, NULL, &type, (BYTE *)val, &cb);
    W32(RegCloseKey)(hKey);
    return (rc == 0 && type == REG_SZ && cb > 4);
}

static void SetAutoStart(BOOL enable) {
    HKEY hKey = NULL;
    if (W32(RegCreateKeyExW)(HKEY_CURRENT_USER, kRunSubKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != 0) {
        return;
    }
    if (enable) {
        WCHAR exePath[520];
        DWORD len = W32(GetModuleFileNameW)(NULL, exePath, 500);
        if (len > 0 && len < 500) {
            WCHAR quoted[530];
            quoted[0] = u'"';
            wstr_copy(quoted + 1, 520, exePath);
            wstr_cat(quoted, 530, u"\"");
            DWORD bytes = (DWORD)((wstr_len(quoted) + 1) * sizeof(WCHAR));
            W32(RegSetValueExW)(hKey, kAppValName, 0, REG_SZ, (const BYTE *)quoted, bytes);
        }
    } else {
        W32(RegDeleteValueW)(hKey, kAppValName);
    }
    W32(RegCloseKey)(hKey);
}

/* ======================== Unrecoverable 7-Pass Shredding Engine ======================== */

#define SHRED_BUF_SIZE (1024U * 1024U)
static const uint8_t kGutmannSingle[27][3] = {
    {0x55,0x55,0x55}, {0xAA,0xAA,0xAA}, {0x92,0x49,0x24}, {0x49,0x24,0x92}, {0x24,0x92,0x49},
    {0x00,0x00,0x00}, {0x11,0x11,0x11}, {0x22,0x22,0x22}, {0x33,0x33,0x33}, {0x44,0x44,0x44},
    {0x55,0x55,0x55}, {0x66,0x66,0x66}, {0x77,0x77,0x77}, {0x88,0x88,0x88}, {0x99,0x99,0x99},
    {0xAA,0xAA,0xAA}, {0xBB,0xBB,0xBB}, {0xCC,0xCC,0xCC}, {0xDD,0xDD,0xDD}, {0xEE,0xEE,0xEE},
    {0xFF,0xFF,0xFF}, {0x92,0x49,0x24}, {0x49,0x24,0x92}, {0x24,0x92,0x49}, {0x6D,0xB6,0xDB},
    {0xB6,0xDB,0x6D}, {0xDB,0x6D,0xB6}
};

static void FillRandomBytes(BYTE *buf, ULONG len) {
    if (W32(BCryptGenRandom)(NULL, buf, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        uint64_t x = 0x9E3779B97F4A7C15ULL ^ (uintptr_t)buf ^ g_selfPid;
        for (ULONG i = 0; i < len; i++) {
            x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
            buf[i] = (BYTE)((x * 0x2545F4914F6CDD1DULL) >> 56);
        }
    }
}

static void WriteStreamPass(HANDLE hFile, LONGLONG length, BYTE *buf, BOOL randomEachChunk) {
    LONGLONG zero = 0;
    W32(SetFilePointerEx)(hFile, zero, NULL, FILE_BEGIN);
    LONGLONG rem = length;
    while (rem > 0) {
        DWORD chunk = (rem > (LONGLONG)SHRED_BUF_SIZE) ? SHRED_BUF_SIZE : (DWORD)rem;
        if (randomEachChunk) {
            FillRandomBytes(buf, chunk);
        }
        DWORD written = 0;
        if (!W32(WriteFile)(hFile, buf, chunk, &written, NULL) || written == 0) {
            break;
        }
        rem -= (LONGLONG)written;
    }
    W32(FlushFileBuffers)(hFile);
}

static void OverwriteHandle(HANDLE hFile, LONGLONG length, int passes, BYTE *buf) {
    if (length <= 0) return;
    if (passes >= 35) {
        for (int i = 0; i < 4; i++) WriteStreamPass(hFile, length, buf, TRUE);
        for (int p = 0; p < 27; p++) {
            for (uint32_t i = 0; i < SHRED_BUF_SIZE; i++) buf[i] = kGutmannSingle[p][i % 3];
            WriteStreamPass(hFile, length, buf, FALSE);
        }
        for (int i = 0; i < 4; i++) WriteStreamPass(hFile, length, buf, TRUE);
    } else if (passes <= 3) {
        memset(buf, 0x00, SHRED_BUF_SIZE); WriteStreamPass(hFile, length, buf, FALSE);
        memset(buf, 0xFF, SHRED_BUF_SIZE); WriteStreamPass(hFile, length, buf, FALSE);
        WriteStreamPass(hFile, length, buf, TRUE);
    } else {
        memset(buf, 0x00, SHRED_BUF_SIZE); WriteStreamPass(hFile, length, buf, FALSE);
        memset(buf, 0xFF, SHRED_BUF_SIZE); WriteStreamPass(hFile, length, buf, FALSE);
        WriteStreamPass(hFile, length, buf, TRUE);
        memset(buf, 0x55, SHRED_BUF_SIZE); WriteStreamPass(hFile, length, buf, FALSE);
        memset(buf, 0xAA, SHRED_BUF_SIZE); WriteStreamPass(hFile, length, buf, FALSE);
        WriteStreamPass(hFile, length, buf, TRUE);
        memset(buf, 0x00, SHRED_BUF_SIZE); WriteStreamPass(hFile, length, buf, FALSE);
    }
}

static void WipeAdsStreams(const WCHAR *filePath, BYTE *buf) {
    WIN32_FIND_STREAM_DATA sd;
    HANDLE hFind = W32(FindFirstStreamW)(filePath, 0, &sd, 0);
    if (hFind == INVALID_HANDLE_VALUE || hFind == NULL) return;

    size_t baseLen = wstr_len(filePath);
    WCHAR *adsPath = (WCHAR *)W32(HeapAlloc)(g_heap, HEAP_ZERO_MEMORY, (baseLen + 310) * sizeof(WCHAR));
    if (adsPath) {
        do {
            if (wstr_ends_with_data_ads(sd.cStreamName)) {
                wstr_copy(adsPath, baseLen + 305, filePath);
                wstr_cat(adsPath, baseLen + 305, sd.cStreamName);
                HANDLE hAds = W32(CreateFileW)(adsPath, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                               OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, NULL);
                if (hAds != INVALID_HANDLE_VALUE) {
                    LONGLONG sz = 0;
                    if (W32(GetFileSizeEx)(hAds, &sz) && sz > 0) {
                        memset(buf, 0x00, SHRED_BUF_SIZE);
                        WriteStreamPass(hAds, sz, buf, FALSE);
                        WriteStreamPass(hAds, sz, buf, TRUE);
                    }
                    LONGLONG zero = 0;
                    W32(SetFilePointerEx)(hAds, zero, NULL, FILE_BEGIN);
                    W32(SetEndOfFile)(hAds);
                    W32(FlushFileBuffers)(hAds);
                    W32(CloseHandle)(hAds);
                }
                W32(DeleteFileW)(adsPath);
            }
        } while (W32(FindNextStreamW)(hFind, &sd));
        W32(HeapFree)(g_heap, 0, adsPath);
    }
    W32(FindClose)(hFind);
}

static void ResetAntiForensicEpoch(HANDLE hHandle) {
    FILETIME ft;
    ft.dwLowDateTime  = 0xE1D58000U;
    ft.dwHighDateTime = 0x01A8E79FU;
    W32(SetFileTime)(hHandle, &ft, &ft, &ft);
}

static void ScramblePathInPlace(WCHAR *inoutPath, size_t cap) {
    static const char kChars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    size_t totalLen = wstr_len(inoutPath);
    intptr_t slash = -1;
    for (size_t i = 0; i < totalLen; i++) {
        if (inoutPath[i] == u'\\' || inoutPath[i] == u'/') slash = (intptr_t)i;
    }
    if (slash < 0) return;

    size_t nameLen = totalLen - (size_t)(slash + 1);
    if (nameLen < 4) nameLen = 4;
    if (nameLen > 48) nameLen = 48;

    size_t rounds[3] = { nameLen, 8, 1 };
    WCHAR *candidate = (WCHAR *)W32(HeapAlloc)(g_heap, HEAP_ZERO_MEMORY, cap * sizeof(WCHAR));
    if (!candidate) return;

    BYTE rnd[64];
    for (int r = 0; r < 3; r++) {
        size_t targetLen = rounds[r];
        if ((size_t)(slash + 1) + targetLen + 2 >= cap) continue;

        for (int attempt = 0; attempt < 6; attempt++) {
            FillRandomBytes(rnd, (ULONG)targetLen);
            memcpy(candidate, inoutPath, (size_t)(slash + 1) * sizeof(WCHAR));
            for (size_t k = 0; k < targetLen; k++) {
                candidate[slash + 1 + k] = (WCHAR)kChars[rnd[k] % 36];
            }
            candidate[slash + 1 + targetLen] = 0;

            if (W32(GetFileAttributesW)(candidate) == 0xFFFFFFFFU) {
                if (W32(MoveFileExW)(inoutPath, candidate, 0)) {
                    wstr_copy(inoutPath, cap, candidate);
                }
                break;
            }
        }
    }
    W32(HeapFree)(g_heap, 0, candidate);
}

static void ShredEntryRecursive(const WCHAR *rawPath, int passes, BYTE *buf) {
    DWORD attrs = W32(GetFileAttributesW)(rawPath);
    if (attrs == 0xFFFFFFFFU) return;

    if ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
        W32(SetFileAttributesW)(rawPath, FILE_ATTRIBUTE_NORMAL);
        if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            W32(RemoveDirectoryW)(rawPath);
        } else {
            W32(DeleteFileW)(rawPath);
        }
        return;
    }

    size_t pathCap = wstr_len(rawPath) + 80;
    WCHAR *workPath = (WCHAR *)W32(HeapAlloc)(g_heap, HEAP_ZERO_MEMORY, pathCap * sizeof(WCHAR));
    if (!workPath) return;
    wstr_copy(workPath, pathCap, rawPath);

    if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        W32(SetFileAttributesW)(workPath, FILE_ATTRIBUTE_NORMAL);
        size_t searchCap = wstr_len(workPath) + 16;
        WCHAR *pattern = (WCHAR *)W32(HeapAlloc)(g_heap, HEAP_ZERO_MEMORY, searchCap * sizeof(WCHAR));
        WIN32_FIND_DATAW *fd = (WIN32_FIND_DATAW *)W32(HeapAlloc)(g_heap, HEAP_ZERO_MEMORY, sizeof(WIN32_FIND_DATAW));
        if (pattern && fd) {
            wstr_copy(pattern, searchCap, workPath);
            wstr_cat(pattern, searchCap, u"\\*");
            HANDLE hFind = W32(FindFirstFileW)(pattern, fd);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    const WCHAR *fn = fd->cFileName;
                    if (fn[0] == u'.' && (fn[1] == 0 || (fn[1] == u'.' && fn[2] == 0))) continue;
                    size_t childCap = wstr_len(workPath) + wstr_len(fn) + 8;
                    WCHAR *child = (WCHAR *)W32(HeapAlloc)(g_heap, HEAP_ZERO_MEMORY, childCap * sizeof(WCHAR));
                    if (child) {
                        wstr_copy(child, childCap, workPath);
                        wstr_cat(child, childCap, u"\\");
                        wstr_cat(child, childCap, fn);
                        ShredEntryRecursive(child, passes, buf);
                        W32(HeapFree)(g_heap, 0, child);
                    }
                } while (W32(FindNextFileW)(hFind, fd));
                W32(FindClose)(hFind);
            }
        }
        if (pattern) W32(HeapFree)(g_heap, 0, pattern);
        if (fd) W32(HeapFree)(g_heap, 0, fd);

        ScramblePathInPlace(workPath, pathCap);
        HANDLE hDir = W32(CreateFileW)(workPath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (hDir != INVALID_HANDLE_VALUE) {
            ResetAntiForensicEpoch(hDir);
            W32(CloseHandle)(hDir);
        }
        W32(RemoveDirectoryW)(workPath);
        W32(HeapFree)(g_heap, 0, workPath);
        return;
    }

    W32(SetFileAttributesW)(workPath, FILE_ATTRIBUTE_NORMAL);
    WipeAdsStreams(workPath, buf);

    HANDLE hFile = W32(CreateFileW)(
        workPath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_WRITE_THROUGH | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        LONGLONG fileSize = 0;
        if (W32(GetFileSizeEx)(hFile, &fileSize) && fileSize > 0) {
            OverwriteHandle(hFile, fileSize, passes, buf);
        }
        LONGLONG zero = 0;
        W32(SetFilePointerEx)(hFile, zero, NULL, FILE_BEGIN);
        W32(SetEndOfFile)(hFile);
        W32(FlushFileBuffers)(hFile);
        ResetAntiForensicEpoch(hFile);
        W32(CloseHandle)(hFile);
    }

    ScramblePathInPlace(workPath, pathCap);
    W32(DeleteFileW)(workPath);
    W32(HeapFree)(g_heap, 0, workPath);
}

static DWORD WINAPI ShredWorkerThread(void *unused) {
    (void)unused;
    for (;;) {
        W32(WaitForSingleObject)(g_workEvent, INFINITE);

        for (;;) {
            W32(AcquireSRWLockExclusive)(&g_queueLock);
            ShredJob *job = g_jobHead;
            if (job) {
                g_jobHead = job->next;
                if (!g_jobHead) g_jobTail = NULL;
            }
            W32(ReleaseSRWLockExclusive)(&g_queueLock);

            if (!job) break;

            __atomic_add_fetch(&g_activeShreds, 1, __ATOMIC_SEQ_CST);
            W32(PostMessageW)(g_hwndMain, WM_APP_SHRED_STATE, 1, 0);

            BYTE *buf = (BYTE *)W32(VirtualAlloc)(NULL, SHRED_BUF_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (buf) {
                ShredEntryRecursive(job->path, (int)g_shredPasses, buf);
                volatile BYTE *vp = (volatile BYTE *)buf;
                for (size_t i = 0; i < SHRED_BUF_SIZE; i++) vp[i] = 0;
                W32(VirtualFree)(buf, 0, MEM_RELEASE);
            }

            W32(HeapFree)(g_heap, 0, job->path);
            W32(HeapFree)(g_heap, 0, job);

            __atomic_sub_fetch(&g_activeShreds, 1, __ATOMIC_SEQ_CST);
            W32(PostMessageW)(g_hwndMain, WM_APP_SHRED_STATE, 0, 0);
        }
    }
    return 0;
}

/* Enqueue all dropped file/folder paths from an HDROP handle and trigger shredding animation */
static void EnqueueDroppedHDrop(HDROP hDrop) {
    UINT count = W32(DragQueryFileW)(hDrop, 0xFFFFFFFFU, NULL, 0);
    if (count == 0) return;

    UINT queued = 0;
    for (UINT i = 0; i < count; i++) {
        UINT cch = W32(DragQueryFileW)(hDrop, i, NULL, 0);
        if (cch == 0) continue;
        WCHAR *pathCopy = (WCHAR *)W32(HeapAlloc)(g_heap, HEAP_ZERO_MEMORY, (size_t)(cch + 4) * sizeof(WCHAR));
        ShredJob *job = (ShredJob *)W32(HeapAlloc)(g_heap, HEAP_ZERO_MEMORY, sizeof(ShredJob));
        if (pathCopy && job) {
            W32(DragQueryFileW)(hDrop, i, pathCopy, cch + 2);
            job->path = pathCopy;
            job->next = NULL;

            W32(AcquireSRWLockExclusive)(&g_queueLock);
            if (g_jobTail) g_jobTail->next = job;
            else           g_jobHead = job;
            g_jobTail = job;
            W32(ReleaseSRWLockExclusive)(&g_queueLock);
            queued++;
        } else {
            if (pathCopy) W32(HeapFree)(g_heap, 0, pathCopy);
            if (job) W32(HeapFree)(g_heap, 0, job);
        }
    }

    if (queued > 0) {
        W32(SetEvent)(g_workEvent);
        /* Guarantee at least 60 frames (~1.5s at 40 FPS) of visible paper-shredding animation
         * even when small files finish wiping in a few milliseconds on fast NVMe SSDs! */
        g_shredAnimActive = TRUE;
        g_animTick = 0;
        g_animMinRemaining = 60;
        W32(SetTimer)(g_hwndMain, TIMER_ANIM, 25, NULL);
        RenderLayeredShredder();
    }
}

/* ======================== OLE COM IDropTarget Implementation ======================== */

static const GUID kIID_IUnknown    = { 0x00000000U, 0x0000U, 0x0000U, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };
static const GUID kIID_IDropTarget = { 0x00000122U, 0x0000U, 0x0000U, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };

static HRESULT WINAPI Drop_QueryInterface(IDropTarget *This, const GUID *riid, void **ppvObj) {
    if (!ppvObj || !riid) return E_POINTER;
    if (guid_eq(riid, &kIID_IUnknown) || guid_eq(riid, &kIID_IDropTarget)) {
        *ppvObj = This;
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Drop_AddRef(IDropTarget *This) {
    (void)This;
    return 1;
}

static ULONG WINAPI Drop_Release(IDropTarget *This) {
    (void)This;
    return 1;
}

static DWORD ChooseDropEffect(DWORD allowed) {
    if (allowed & DROPEFFECT_COPY) return DROPEFFECT_COPY;
    if (allowed & DROPEFFECT_MOVE) return DROPEFFECT_MOVE;
    if (allowed & DROPEFFECT_LINK) return DROPEFFECT_LINK;
    return DROPEFFECT_COPY;
}

static HRESULT WINAPI Drop_DragEnter(IDropTarget *This, IDataObject *pDataObj, DWORD grfKeyState, POINT pt, DWORD *pdwEffect) {
    (void)This; (void)grfKeyState; (void)pt;
    if (!pdwEffect) return E_POINTER;

    g_canAcceptDrop = FALSE;
    if (pDataObj && pDataObj->lpVtbl && pDataObj->lpVtbl->QueryGetData) {
        FORMATETC fmt;
        memset(&fmt, 0, sizeof(fmt));
        fmt.cfFormat = CF_HDROP;
        fmt.ptd      = NULL;
        fmt.dwAspect = DVASPECT_CONTENT;
        fmt.lindex   = -1;
        fmt.tymed    = TYMED_HGLOBAL;
        if (pDataObj->lpVtbl->QueryGetData(pDataObj, &fmt) == S_OK) {
            g_canAcceptDrop = TRUE;
        }
    }

    if (g_canAcceptDrop) {
        *pdwEffect = ChooseDropEffect(*pdwEffect);
        if (!g_dragHoverFile) {
            g_dragHoverFile = TRUE;
            RenderLayeredShredder();
        }
    } else {
        *pdwEffect = DROPEFFECT_NONE;
    }
    return S_OK;
}

static HRESULT WINAPI Drop_DragOver(IDropTarget *This, DWORD grfKeyState, POINT pt, DWORD *pdwEffect) {
    (void)This; (void)grfKeyState; (void)pt;
    if (!pdwEffect) return E_POINTER;
    *pdwEffect = g_canAcceptDrop ? ChooseDropEffect(*pdwEffect) : DROPEFFECT_NONE;
    return S_OK;
}

static HRESULT WINAPI Drop_DragLeave(IDropTarget *This) {
    (void)This;
    g_canAcceptDrop = FALSE;
    if (g_dragHoverFile) {
        g_dragHoverFile = FALSE;
        RenderLayeredShredder();
    }
    return S_OK;
}

static HRESULT WINAPI Drop_Drop(IDropTarget *This, IDataObject *pDataObj, DWORD grfKeyState, POINT pt, DWORD *pdwEffect) {
    (void)This; (void)grfKeyState; (void)pt;
    BOOL wasHover = g_dragHoverFile;
    g_dragHoverFile = FALSE;
    g_canAcceptDrop = FALSE;

    if (pDataObj && pDataObj->lpVtbl && pDataObj->lpVtbl->GetData) {
        FORMATETC fmt;
        STGMEDIUM stg;
        memset(&fmt, 0, sizeof(fmt));
        memset(&stg, 0, sizeof(stg));
        fmt.cfFormat = CF_HDROP;
        fmt.ptd      = NULL;
        fmt.dwAspect = DVASPECT_CONTENT;
        fmt.lindex   = -1;
        fmt.tymed    = TYMED_HGLOBAL;

        if (pDataObj->lpVtbl->GetData(pDataObj, &fmt, &stg) == S_OK) {
            if (stg.hGlobal != NULL) {
                EnqueueDroppedHDrop((HDROP)stg.hGlobal);
            }
            W32(ReleaseStgMedium)(&stg);
            if (pdwEffect) *pdwEffect = DROPEFFECT_COPY; /* Never let drag source race our 7-pass shredder */
            return S_OK;
        }
    }

    if (wasHover) {
        RenderLayeredShredder();
    }
    if (pdwEffect) *pdwEffect = DROPEFFECT_NONE;
    return S_OK;
}

static void InitDropTarget(void) {
    /* Populate function pointers at runtime using RIP-relative addresses (100% ASLR-safe) */
    g_dropVtbl.QueryInterface = Drop_QueryInterface;
    g_dropVtbl.AddRef         = Drop_AddRef;
    g_dropVtbl.Release        = Drop_Release;
    g_dropVtbl.DragEnter      = Drop_DragEnter;
    g_dropVtbl.DragOver       = Drop_DragOver;
    g_dropVtbl.DragLeave      = Drop_DragLeave;
    g_dropVtbl.Drop           = Drop_Drop;
    g_dropTarget.lpVtbl       = &g_dropVtbl;
}

/* ======================== Three-State SDF Layered Widget Renderer ======================== */

static float f_abs(float x) { return x < 0.0f ? -x : x; }
static float f_max(float a, float b) { return a > b ? a : b; }
static float f_min(float a, float b) { return a < b ? a : b; }
static float f_clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }

static float fast_sqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    float g = x * 0.5f + 0.5f;
    for (int i = 0; i < 4; i++) g = 0.5f * (g + x / g);
    return g;
}

static float fast_sin(float x) {
    const float pi = 3.14159265f;
    const float two_pi = 6.28318530f;
    while (x > pi) x -= two_pi;
    while (x < -pi) x += two_pi;
    float y = 1.27323954f * x - 0.405284735f * x * f_abs(x);
    return 0.225f * (y * f_abs(y) - y) + y;
}

static float sdf_rrect(float px, float py, float x0, float y0, float x1, float y1, float r) {
    float cx = (x0 + x1) * 0.5f;
    float cy = (y0 + y1) * 0.5f;
    float hx = (x1 - x0) * 0.5f - r;
    float hy = (y1 - y0) * 0.5f - r;
    float dx = f_max(f_abs(px - cx) - hx, 0.0f);
    float dy = f_max(f_abs(py - cy) - hy, 0.0f);
    float out_d = fast_sqrt(dx * dx + dy * dy);
    float in_d = f_min(f_max(f_abs(px - cx) - hx, f_abs(py - cy) - hy), 0.0f);
    return out_d + in_d - r;
}

static void blend_px(uint32_t *dst, uint8_t r, uint8_t g, uint8_t b, float alpha01) {
    if (alpha01 <= 0.002f) return;
    float sa = f_clamp01(alpha01);
    uint32_t cur = *dst;
    float da = ((cur >> 24) & 0xFF) / 255.0f;
    float db = (cur & 0xFF) / 255.0f;
    float dg = ((cur >> 8) & 0xFF) / 255.0f;
    float dr = ((cur >> 16) & 0xFF) / 255.0f;

    float sr = (r / 255.0f) * sa;
    float sg = (g / 255.0f) * sa;
    float sb = (b / 255.0f) * sa;

    float inv = 1.0f - sa;
    float oa = sa + da * inv;
    float or_ = sr + dr * inv;
    float og = sg + dg * inv;
    float ob = sb + db * inv;

    uint32_t A = (uint32_t)(f_clamp01(oa) * 255.0f + 0.5f);
    uint32_t R = (uint32_t)(f_clamp01(or_) * 255.0f + 0.5f);
    uint32_t G = (uint32_t)(f_clamp01(og) * 255.0f + 0.5f);
    uint32_t B = (uint32_t)(f_clamp01(ob) * 255.0f + 0.5f);
    *dst = (A << 24) | (R << 16) | (G << 8) | B;
}

static void RenderLayeredShredder(void) {
    if (!g_hwndMain) return;
    HDC hdcScreen = W32(GetDC)(NULL);
    HDC hdcMem = W32(CreateCompatibleDC)(hdcScreen);

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_widgetW;
    bmi.bmiHeader.biHeight = -g_widgetH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;

    uint32_t *pixels = NULL;
    HBITMAP hBmp = W32(CreateDIBSection)(hdcMem, &bmi, DIB_RGB_COLORS, (void **)&pixels, NULL, 0);
    if (hBmp && pixels) {
        HGDIOBJ oldBmp = W32(SelectObject)(hdcMem, hBmp);
        float s = g_dpiScale;

        BOOL hoverFile = g_dragHoverFile;
        BOOL shredding = (!hoverFile) && g_shredAnimActive;
        float phase = (float)g_animTick * 0.25f;
        float shakeX = shredding ? (1.1f * fast_sin((float)g_animTick * 1.85f)) : 0.0f;

        /* During shredding: paper sheet slides smoothly from y=3 all the way down into cutter slot y=45 */
        int cycleTick = g_animTick % 46;
        BOOL showFeedingSheet = shredding && (g_animTick < 46 || __atomic_load_n(&g_activeShreds, __ATOMIC_SEQ_CST) > 0);
        float pyTop = 3.0f + ((float)cycleTick / 45.0f) * 42.0f;

        for (int32_t py = 0; py < g_widgetH; py++) {
            float y = ((float)py + 0.5f) / s;
            uint32_t *row = pixels + (size_t)py * (size_t)g_widgetW;

            for (int32_t px = 0; px < g_widgetW; px++) {
                float x = ((float)px + 0.5f) / s;
                /* Uniform invisible hit-region across ALL states prevents edge hover flicker */
                uint32_t out = (x >= 10.0f && x <= 102.0f && y >= 8.0f && y <= 116.0f) ? 0x04000000U : 0x00000000U;

                if (hoverFile) {
                    /* ======== STATE B: SINGLE DOCUMENT / FILE ICON ON DRAG-OVER ======== */
                    float foldSize = 21.0f;
                    float dCut = (x - (90.0f - foldSize)) - (y - 12.0f);

                    float dShad = f_max(sdf_rrect(x, y, 22.0f, 15.0f, 90.0f, 113.0f, 7.0f), dCut - 2.0f);
                    if (dShad < 6.5f) {
                        blend_px(&out, 15, 23, 42, f_clamp01((6.5f - dShad) / 8.0f) * 0.34f);
                    }

                    float dDoc = sdf_rrect(x, y, 22.0f, 12.0f, 90.0f, 110.0f, 6.0f);
                    float dSheet = f_max(dDoc, dCut);
                    float covDoc = f_clamp01(0.5f - dSheet * s);

                    if (covDoc > 0.0f) {
                        blend_px(&out, 248, 250, 252, covDoc);

                        if (dSheet > -1.6f) {
                            blend_px(&out, 59, 130, 246, covDoc * f_clamp01(1.6f + dSheet));
                        }

                        if (x >= 90.0f - foldSize - 0.5f && y <= 12.0f + foldSize + 0.5f && dCut <= 0.5f) {
                            float dFlapBox = sdf_rrect(x, y, 90.0f - foldSize, 12.0f, 92.0f, 12.0f + foldSize, 3.0f);
                            blend_px(&out, 203, 213, 225, f_clamp01(0.5f - dFlapBox * s) * covDoc);
                        }

                        blend_px(&out, 239, 68, 68,  f_clamp01(0.5f - sdf_rrect(x, y, 32.0f, 30.0f, 60.0f, 34.5f, 2.2f) * s));
                        blend_px(&out, 148, 163, 184, f_clamp01(0.5f - sdf_rrect(x, y, 32.0f, 43.0f, 78.0f, 46.8f, 1.8f) * s));
                        blend_px(&out, 148, 163, 184, f_clamp01(0.5f - sdf_rrect(x, y, 32.0f, 54.0f, 78.0f, 57.8f, 1.8f) * s));
                        blend_px(&out, 148, 163, 184, f_clamp01(0.5f - sdf_rrect(x, y, 32.0f, 65.0f, 68.0f, 68.8f, 1.8f) * s));

                        for (int dash = 0; dash < 5; dash++) {
                            float dx0 = 28.0f + (float)dash * 12.0f;
                            blend_px(&out, 239, 68, 68, f_clamp01(0.5f - sdf_rrect(x, y, dx0, 81.0f, dx0 + 7.5f, 83.2f, 1.0f) * s) * 0.85f);
                        }
                        float dShaft = sdf_rrect(x, y, 54.2f, 87.0f, 57.8f, 97.5f, 1.2f);
                        blend_px(&out, 239, 68, 68, f_clamp01(0.5f - dShaft * s));
                        if (y >= 94.0f && y <= 103.0f) {
                            float dTri = f_abs(x - 56.0f) - (103.0f - y);
                            blend_px(&out, 239, 68, 68, f_clamp01(0.5f - dTri * s));
                        }
                    }
                } else {
                    /* ======== STATE A (IDLE SHREDDER) & STATE C (SHREDDING ANIMATION) ======== */
                    float sx = x - shakeX;

                    float dShadow = sdf_rrect(sx, y, 16.0f, 47.0f, 96.0f, 115.0f, 10.0f);
                    if (dShadow < 6.0f) {
                        blend_px(&out, 0, 0, 0, f_clamp01((6.0f - dShadow) / 8.0f) * 0.36f);
                    }

                    /* Feeding document sheet sliding down into top slot during shredding */
                    if (showFeedingSheet && y <= 41.5f) {
                        float fold = 11.0f;
                        float dPaper = sdf_rrect(sx, y, 28.0f, pyTop, 84.0f, pyTop + 40.0f, 3.2f);
                        float dCut = (sx - (84.0f - fold)) - (y - pyTop);
                        float dSheet = f_max(dPaper, dCut);
                        float covP = f_clamp01(0.5f - dSheet * s);
                        if (covP > 0.0f) {
                            blend_px(&out, 248, 250, 252, covP);
                            if (dSheet > -1.1f) {
                                blend_px(&out, 203, 213, 225, covP * f_clamp01(1.1f + dSheet));
                            }
                            if (sx >= 84.0f - fold - 0.5f && y <= pyTop + fold + 0.5f && dCut <= 0.5f) {
                                float dFlap = sdf_rrect(sx, y, 84.0f - fold, pyTop, 85.5f, pyTop + fold, 2.0f);
                                blend_px(&out, 203, 213, 225, f_clamp01(0.5f - dFlap * s) * covP);
                            }
                            blend_px(&out, 239, 68, 68,  f_clamp01(0.5f - sdf_rrect(sx, y, 35.0f, pyTop + 8.0f, 60.0f, pyTop + 11.2f, 1.5f) * s) * 0.9f);
                            blend_px(&out, 148, 163, 184, f_clamp01(0.5f - sdf_rrect(sx, y, 35.0f, pyTop + 15.5f, 75.0f, pyTop + 18.2f, 1.2f) * s) * 0.9f);
                            blend_px(&out, 148, 163, 184, f_clamp01(0.5f - sdf_rrect(sx, y, 35.0f, pyTop + 22.0f, 71.0f, pyTop + 24.7f, 1.2f) * s) * 0.9f);
                            blend_px(&out, 148, 163, 184, f_clamp01(0.5f - sdf_rrect(sx, y, 35.0f, pyTop + 28.5f, 66.0f, pyTop + 31.2f, 1.2f) * s) * 0.9f);
                        }
                    } else if (!shredding && y <= 41.5f) {
                        /* Subtle idle paper lip peeking out so the icon is instantly recognizable as a shredder */
                        float dPeek = sdf_rrect(sx, y, 30.0f, 18.0f, 82.0f, 42.0f, 3.0f);
                        float covP = f_clamp01(0.5f - dPeek * s);
                        if (covP > 0.0f) {
                            blend_px(&out, 248, 250, 252, covP);
                            if (dPeek > -1.1f) blend_px(&out, 203, 213, 225, covP * f_clamp01(1.1f + dPeek));
                            blend_px(&out, 239, 68, 68,  f_clamp01(0.5f - sdf_rrect(sx, y, 37.0f, 24.0f, 61.0f, 27.0f, 1.4f) * s) * 0.88f);
                            blend_px(&out, 148, 163, 184, f_clamp01(0.5f - sdf_rrect(sx, y, 37.0f, 30.5f, 74.0f, 33.0f, 1.2f) * s) * 0.88f);
                        }
                    }

                    /* Lower Shredder Bin Body */
                    float dBin = sdf_rrect(sx, y, 16.0f, 50.0f, 96.0f, 112.0f, 9.5f);
                    float covBin = f_clamp01(0.5f - dBin * s);
                    if (covBin > 0.0f) {
                        blend_px(&out, 30, 41, 59, covBin);
                        if (dBin > -1.2f) {
                            blend_px(&out, 51, 65, 85, covBin * f_clamp01(1.2f + dBin));
                        }

                        /* Recessed Viewing Window */
                        float dWin = sdf_rrect(sx, y, 26.0f, 61.0f, 86.0f, 102.0f, 5.0f);
                        float covWin = f_clamp01(0.5f - dWin * s);
                        if (covWin > 0.0f) {
                            blend_px(&out, 15, 23, 42, covWin);

                            if (shredding) {
                                /* Active cascading paper strips pouring down inside the window */
                                for (int i = 0; i < 7; i++) {
                                    float colX = 30.5f + (float)i * 7.6f;
                                    float speed = 1.75f + (float)(i % 3) * 0.45f;
                                    float offset = (float)g_animTick * speed + (float)i * 9.0f;
                                    for (int seg = 0; seg < 2; seg++) {
                                        float tot = offset + (float)seg * 24.0f;
                                        int wraps = (int)(tot / 48.0f);
                                        float rawY = 58.0f + (tot - 48.0f * (float)wraps);
                                        float segLen = (seg == 0) ? 16.0f : 11.0f;
                                        float dSt = sdf_rrect(sx, y, colX, rawY, colX + 4.8f, rawY + segLen, 1.1f);
                                        float covSt = f_clamp01(0.5f - dSt * s) * covWin;
                                        if (covSt > 0.0f) {
                                            if (i == 3 && seg == 0) blend_px(&out, 254, 202, 202, covSt * 0.96f);
                                            else                    blend_px(&out, 241, 245, 249, covSt * 0.96f);
                                        }
                                    }
                                }
                            } else {
                                float idleH[7] = { 20.0f, 27.0f, 16.0f, 29.0f, 22.0f, 25.0f, 19.0f };
                                for (int i = 0; i < 7; i++) {
                                    float colX = 30.5f + (float)i * 7.6f;
                                    float dSt = sdf_rrect(sx, y, colX, 63.0f, colX + 4.8f, 63.0f + idleH[i], 1.1f);
                                    float covSt = f_clamp01(0.5f - dSt * s) * covWin;
                                    if (covSt > 0.0f) {
                                        if (i == 3) blend_px(&out, 254, 202, 202, covSt * 0.94f);
                                        else        blend_px(&out, 226, 232, 240, covSt * 0.94f);
                                    }
                                }
                            }

                            if (dWin > -1.0f) {
                                blend_px(&out, 71, 85, 105, covWin * f_clamp01(1.0f + dWin));
                            }
                        }
                    }

                    /* Metallic Cutter Head Housing */
                    float dHead = sdf_rrect(sx, y, 12.0f, 35.0f, 100.0f, 58.0f, 7.0f);
                    float covHead = f_clamp01(0.5f - dHead * s);
                    if (covHead > 0.0f) {
                        uint8_t hc = (y < 45.0f) ? 96 : 71;
                        uint8_t hg = (y < 45.0f) ? 111 : 85;
                        uint8_t hb = (y < 45.0f) ? 133 : 105;
                        blend_px(&out, hc, hg, hb, covHead);
                        if (dHead > -1.0f) {
                            blend_px(&out, 118, 134, 156, covHead * f_clamp01(1.0f + dHead));
                        }

                        /* Cutter Slot Throat + Spinning Cutter Teeth */
                        float dSlot = sdf_rrect(sx, y, 22.0f, 39.0f, 90.0f, 45.0f, 2.5f);
                        float covSlot = f_clamp01(0.5f - dSlot * s);
                        if (covSlot > 0.0f) {
                            blend_px(&out, 15, 23, 42, covSlot);
                            float toothShift = shredding ? ((float)(g_animTick % 6) * 1.05f) : 0.0f;
                            for (int t = 0; t < 10; t++) {
                                float tx = 26.0f + (float)t * 6.2f + toothShift;
                                if (tx < 87.5f) {
                                    float dTooth = sdf_rrect(sx, y, tx, 40.5f, tx + 2.0f, 43.5f, 0.4f);
                                    blend_px(&out, 148, 163, 184, f_clamp01(0.5f - dTooth * s) * covSlot * 0.78f);
                                }
                            }
                        }

                        /* Crimson Safety Accent Bar */
                        float dBar = sdf_rrect(sx, y, 23.0f, 50.5f, 74.0f, 53.2f, 1.3f);
                        blend_px(&out, 239, 68, 68, f_clamp01(0.5f - dBar * s) * 0.95f);

                        /* Status LED */
                        float dx = sx - 84.5f;
                        float dy = y - 51.8f;
                        float dist = fast_sqrt(dx * dx + dy * dy);
                        float covLed = f_clamp01(0.5f - (dist - 3.4f) * s);
                        if (covLed > 0.0f) {
                            if (shredding) {
                                float pulse = 0.60f + 0.40f * f_abs(fast_sin(phase * 2.5f));
                                blend_px(&out, 252, 85, 85, covLed * pulse);
                            } else {
                                blend_px(&out, 34, 197, 94, covLed);
                            }
                        }
                    }
                }

                row[px] = out;
            }
        }

        POINT ptSrc = { 0, 0 };
        POINT ptDst = { g_winX, g_winY };
        SIZE  szWnd = { g_widgetW, g_widgetH };
        BLENDFUNCTION bf = { AC_SRC_OVER, 0, g_opacityByte, AC_SRC_ALPHA };
        W32(UpdateLayeredWindow)(g_hwndMain, hdcScreen, &ptDst, &szWnd, hdcMem, &ptSrc, 0, &bf, ULW_ALPHA);

        W32(SelectObject)(hdcMem, oldBmp);
        W32(DeleteObject)(hBmp);
    }

    W32(DeleteDC)(hdcMem);
    W32(ReleaseDC)(NULL, hdcScreen);
}

/* ======================== Fullscreen App Auto-Hide Detection ======================== */

static BOOL IsAnotherAppFullscreen(void) {
    int32_t quns = 0;
    if (W32(SHQueryUserNotificationState)(&quns) == 0) {
        if (quns == 3 || quns == 4) return TRUE;
    }

    HWND fg = W32(GetForegroundWindow)();
    if (!fg || !W32(IsWindowVisible)(fg) || W32(IsIconic)(fg)) return FALSE;

    DWORD pid = 0;
    W32(GetWindowThreadProcessId)(fg, &pid);
    if (pid == 0 || pid == g_selfPid) return FALSE;

    WCHAR cls[96];
    int n = W32(GetClassNameW)(fg, cls, 95);
    if (n > 0) {
        cls[n] = 0;
        if (wstr_ieq_ascii(cls, "Progman") ||
            wstr_ieq_ascii(cls, "WorkerW") ||
            wstr_ieq_ascii(cls, "Shell_TrayWnd") ||
            wstr_ieq_ascii(cls, "Shell_SecondaryTrayWnd")) {
            return FALSE;
        }
    }

    RECT wr;
    if (!W32(GetWindowRect)(fg, &wr)) return FALSE;

    HMONITOR mon = W32(MonitorFromWindow)(fg, MONITOR_DEFAULTTONEAREST);
    if (!mon) return FALSE;

    MONITORINFO mi;
    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (!W32(GetMonitorInfoW)(mon, &mi)) return FALSE;

    BOOL coversMonitor =
        (wr.left <= mi.rcMonitor.left) &&
        (wr.top <= mi.rcMonitor.top) &&
        (wr.right >= mi.rcMonitor.right) &&
        (wr.bottom >= mi.rcMonitor.bottom);

    if (!coversMonitor) return FALSE;

    BOOL taskbarAutoHidden =
        (mi.rcWork.left == mi.rcMonitor.left) &&
        (mi.rcWork.top == mi.rcMonitor.top) &&
        (mi.rcWork.right == mi.rcMonitor.right) &&
        (mi.rcWork.bottom == mi.rcMonitor.bottom);

    if (taskbarAutoHidden) {
        LONG_PTR style = W32(GetWindowLongPtrW)(fg, GWL_STYLE);
        if ((style & WS_CAPTION) == WS_CAPTION) return FALSE;
    }

    return TRUE;
}

/* ======================== Settings Dialog Window ======================== */

static int S(int px) { return (int)((float)px * g_dpiScale + 0.5f); }

static LRESULT WINAPI SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HWND lblTitle = W32(CreateWindowExW)(0, u"STATIC", u"桌面碎纸机设置（纯本地离线 · 免登录 · 静默销毁）",
            WS_CHILD | WS_VISIBLE, S(20), S(16), S(370), S(22), hwnd, NULL, g_hInst, NULL);
        W32(SendMessageW)(lblTitle, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

        g_chkAuto = W32(CreateWindowExW)(0, u"BUTTON", u"开机自动启动（登录系统时自动悬浮在桌面）",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            S(20), S(48), S(370), S(24), hwnd, (HMENU)(uintptr_t)IDC_CHK_AUTO, g_hInst, NULL);
        W32(SendMessageW)(g_chkAuto, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        W32(SendMessageW)(g_chkAuto, BM_SETCHECK, IsAutoStartEnabled() ? BST_CHECKED : BST_UNCHECKED, 0);

        g_chkLock = W32(CreateWindowExW)(0, u"BUTTON", u"锁定桌面悬浮窗位置（防止鼠标误拖动）",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            S(20), S(80), S(370), S(24), hwnd, (HMENU)(uintptr_t)IDC_CHK_LOCK, g_hInst, NULL);
        W32(SendMessageW)(g_chkLock, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        W32(SendMessageW)(g_chkLock, BM_SETCHECK, g_lockPos ? BST_CHECKED : BST_UNCHECKED, 0);

        HWND lblPass = W32(CreateWindowExW)(0, u"STATIC", u"安全粉碎覆写标准（拖入即后台静默销毁，无法恢复）：",
            WS_CHILD | WS_VISIBLE, S(20), S(116), S(370), S(20), hwnd, NULL, g_hInst, NULL);
        W32(SendMessageW)(lblPass, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

        g_cmbPasses = W32(CreateWindowExW)(0, u"COMBOBOX", u"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            S(20), S(140), S(370), S(120), hwnd, (HMENU)(uintptr_t)IDC_CMB_PASSES, g_hInst, NULL);
        W32(SendMessageW)(g_cmbPasses, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        W32(SendMessageW)(g_cmbPasses, CB_ADDSTRING, 0, (LPARAM)u"7 次覆写 — DoD 5220.22-M ECE + CSPRNG（默认推荐）");
        W32(SendMessageW)(g_cmbPasses, CB_ADDSTRING, 0, (LPARAM)u"3 次覆写 — DoD 5220.22-M 标准（兼顾大文件速度）");
        W32(SendMessageW)(g_cmbPasses, CB_ADDSTRING, 0, (LPARAM)u"35 次覆写 — Gutmann 极限覆写标准（高敏小文件）");
        int selPass = (g_shredPasses >= 35) ? 2 : ((g_shredPasses <= 3) ? 1 : 0);
        W32(SendMessageW)(g_cmbPasses, CB_SETCURSEL, (WPARAM)selPass, 0);

        HWND lblOp = W32(CreateWindowExW)(0, u"STATIC", u"桌面碎纸机悬浮窗不透明度：",
            WS_CHILD | WS_VISIBLE, S(20), S(178), S(370), S(20), hwnd, NULL, g_hInst, NULL);
        W32(SendMessageW)(lblOp, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

        g_cmbOpacity = W32(CreateWindowExW)(0, u"COMBOBOX", u"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            S(20), S(202), S(370), S(140), hwnd, (HMENU)(uintptr_t)IDC_CMB_OPACITY, g_hInst, NULL);
        W32(SendMessageW)(g_cmbOpacity, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        W32(SendMessageW)(g_cmbOpacity, CB_ADDSTRING, 0, (LPARAM)u"95%（默认推荐）");
        W32(SendMessageW)(g_cmbOpacity, CB_ADDSTRING, 0, (LPARAM)u"100%（完全不透明）");
        W32(SendMessageW)(g_cmbOpacity, CB_ADDSTRING, 0, (LPARAM)u"80%（轻度半透明）");
        W32(SendMessageW)(g_cmbOpacity, CB_ADDSTRING, 0, (LPARAM)u"60%（高透明隐蔽）");
        int selOp = 0;
        if (g_opacityByte >= 253) selOp = 1;
        else if (g_opacityByte <= 165) selOp = 3;
        else if (g_opacityByte <= 220) selOp = 2;
        W32(SendMessageW)(g_cmbOpacity, CB_SETCURSEL, (WPARAM)selOp, 0);

        HWND btnCancel = W32(CreateWindowExW)(0, u"BUTTON", u"取消",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            S(212), S(248), S(84), S(30), hwnd, (HMENU)(uintptr_t)IDC_BTN_CANCEL, g_hInst, NULL);
        W32(SendMessageW)(btnCancel, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

        HWND btnSave = W32(CreateWindowExW)(0, u"BUTTON", u"保存设置",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            S(306), S(248), S(84), S(30), hwnd, (HMENU)(uintptr_t)IDC_BTN_SAVE, g_hInst, NULL);
        W32(SendMessageW)(btnSave, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        return 0;
    }

    case WM_CTLCOLORSTATIC:
        return (LRESULT)g_hBrushDlgBg;

    case WM_COMMAND: {
        WORD id = (WORD)(wParam & 0xFFFF);
        if (id == IDC_BTN_SAVE) {
            BOOL wantAuto = (W32(SendMessageW)(g_chkAuto, BM_GETCHECK, 0, 0) == BST_CHECKED);
            SetAutoStart(wantAuto);

            g_lockPos = (W32(SendMessageW)(g_chkLock, BM_GETCHECK, 0, 0) == BST_CHECKED);

            int pIdx = (int)W32(SendMessageW)(g_cmbPasses, CB_GETCURSEL, 0, 0);
            g_shredPasses = (pIdx == 2) ? 35 : ((pIdx == 1) ? 3 : 7);

            int oIdx = (int)W32(SendMessageW)(g_cmbOpacity, CB_GETCURSEL, 0, 0);
            if (oIdx == 1)      g_opacityByte = 255;
            else if (oIdx == 2) g_opacityByte = 204;
            else if (oIdx == 3) g_opacityByte = 153;
            else                g_opacityByte = 242;

            SaveConfig();
            RenderLayeredShredder();
            W32(DestroyWindow)(hwnd);
            return 0;
        } else if (id == IDC_BTN_CANCEL) {
            W32(DestroyWindow)(hwnd);
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        W32(DestroyWindow)(hwnd);
        return 0;

    case WM_DESTROY:
        g_hwndSettings = NULL;
        return 0;
    }
    return W32(DefWindowProcW)(hwnd, msg, wParam, lParam);
}

static void OpenSettingsWindow(void) {
    if (g_hwndSettings) {
        W32(ShowWindow)(g_hwndSettings, SW_SHOWNORMAL);
        W32(SetForegroundWindow)(g_hwndSettings);
        return;
    }
    int w = S(422);
    int h = S(328);
    int sx = W32(GetSystemMetrics)(SM_CXSCREEN);
    int sy = W32(GetSystemMetrics)(SM_CYSCREEN);
    g_hwndSettings = W32(CreateWindowExW)(
        WS_EX_TOPMOST,
        u"DesktopShredderSettingsCls",
        u"桌面碎纸机 - 设置",
        WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        (sx - w) / 2,
        (sy - h) / 2,
        w,
        h,
        NULL,
        NULL,
        g_hInst,
        NULL);
    W32(SetForegroundWindow)(g_hwndSettings);
}

/* ======================== Main Floating Shredder Window Procedure ======================== */

static void ResetToBottomRight(void) {
    int sx = W32(GetSystemMetrics)(SM_CXSCREEN);
    int sy = W32(GetSystemMetrics)(SM_CYSCREEN);
    g_winX = sx - g_widgetW - S(32);
    g_winY = sy - g_widgetH - S(72);
    W32(SetWindowPos)(g_hwndMain, NULL, g_winX, g_winY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    SaveConfig();
    RenderLayeredShredder();
}

static LRESULT WINAPI MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_LBUTTONDOWN:
        if (!g_lockPos) {
            g_dragging = TRUE;
            W32(SetCapture)(hwnd);
            W32(GetCursorPos)(&g_dragStartMouse);
            g_dragStartWin.x = g_winX;
            g_dragStartWin.y = g_winY;
        }
        return 0;

    case WM_MOUSEMOVE:
        if (g_dragging && !g_lockPos) {
            POINT cur;
            W32(GetCursorPos)(&cur);
            g_winX = g_dragStartWin.x + (cur.x - g_dragStartMouse.x);
            g_winY = g_dragStartWin.y + (cur.y - g_dragStartMouse.y);
            W32(SetWindowPos)(hwnd, NULL, g_winX, g_winY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;

    case WM_LBUTTONUP:
        if (g_dragging) {
            g_dragging = FALSE;
            W32(ReleaseCapture)();
            SaveConfig();
        }
        return 0;

    case WM_RBUTTONUP: {
        POINT pt;
        W32(GetCursorPos)(&pt);
        HMENU menu = W32(CreatePopupMenu)();
        if (menu) {
            W32(AppendMenuW)(menu, MF_STRING, IDM_SETTINGS, u"设置 (Settings)...");
            W32(AppendMenuW)(menu, MF_STRING | (IsAutoStartEnabled() ? MF_CHECKED : 0), IDM_AUTOSTART, u"开机自动启动");
            W32(AppendMenuW)(menu, MF_STRING | (g_lockPos ? MF_CHECKED : 0), IDM_LOCKPOS, u"锁定悬浮窗位置");
            W32(AppendMenuW)(menu, MF_STRING, IDM_RESETPOS, u"重置位置到右下角");
            W32(AppendMenuW)(menu, MF_SEPARATOR, 0, NULL);
            W32(AppendMenuW)(menu, MF_STRING, IDM_EXIT, u"退出碎纸机");

            W32(SetForegroundWindow)(hwnd);
            UINT cmd = (UINT)W32(TrackPopupMenu)(menu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
            W32(PostMessageW)(hwnd, WM_NULL, 0, 0);
            W32(DestroyMenu)(menu);

            if (cmd == IDM_SETTINGS) {
                OpenSettingsWindow();
            } else if (cmd == IDM_AUTOSTART) {
                SetAutoStart(!IsAutoStartEnabled());
            } else if (cmd == IDM_LOCKPOS) {
                g_lockPos = !g_lockPos;
                SaveConfig();
            } else if (cmd == IDM_RESETPOS) {
                ResetToBottomRight();
            } else if (cmd == IDM_EXIT) {
                SaveConfig();
                W32(PostQuitMessage)(0);
            }
        }
        return 0;
    }

    case WM_DROPFILES: {
        /* Fallback handler if OLE drag-drop is bypassed by an elevated sender */
        HDROP hDrop = (HDROP)wParam;
        EnqueueDroppedHDrop(hDrop);
        W32(DragFinish)(hDrop);
        return 0;
    }

    case WM_APP_SHRED_STATE:
        if (__atomic_load_n(&g_activeShreds, __ATOMIC_SEQ_CST) > 0 && !g_shredAnimActive) {
            g_shredAnimActive = TRUE;
            W32(SetTimer)(hwnd, TIMER_ANIM, 25, NULL);
            RenderLayeredShredder();
        }
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_ANIM) {
            g_animTick++;
            if (g_animMinRemaining > 0) {
                g_animMinRemaining--;
            }
            if (g_animMinRemaining <= 0 && __atomic_load_n(&g_activeShreds, __ATOMIC_SEQ_CST) <= 0) {
                g_shredAnimActive = FALSE;
                g_animTick = 0;
                W32(KillTimer)(hwnd, TIMER_ANIM);
            }
            RenderLayeredShredder();
        } else if (wParam == TIMER_FULLSCREEN) {
            if (g_hwndSettings != NULL) return 0;
            BOOL fs = IsAnotherAppFullscreen();
            if (fs && !g_hiddenByFs) {
                g_hiddenByFs = TRUE;
                W32(ShowWindow)(hwnd, SW_HIDE);
            } else if (!fs && g_hiddenByFs) {
                g_hiddenByFs = FALSE;
                W32(ShowWindow)(hwnd, SW_SHOWNOACTIVATE);
                RenderLayeredShredder();
            }
        }
        return 0;

    case WM_DESTROY:
        W32(PostQuitMessage)(0);
        return 0;
    }
    return W32(DefWindowProcW)(hwnd, msg, wParam, lParam);
}

/* ======================== Freestanding Entry Point ======================== */

void WINAPI WinMainCRTStartup(void) {
    HWND prevWnd = W32(FindWindowW)(u"DesktopShredderFloatingCls", NULL);
    if (prevWnd != NULL) {
        W32(SendMessageW)(prevWnd, WM_CLOSE, 0, 0);
        W32(Sleep)(150);
    }
    HANDLE hMutex = W32(CreateMutexW)(NULL, TRUE, u"Local\\DesktopShredder_Native_SingleInstance");
    if (hMutex && W32(GetLastError)() == ERROR_ALREADY_EXISTS) {
        W32(ExitProcess)(0);
    }

    HRESULT oleHr = W32(OleInitialize)(NULL);

    g_hInst = W32(GetModuleHandleW)(NULL);
    g_selfPid = W32(GetCurrentProcessId)();
    g_heap = W32(GetProcessHeap)();
    g_workEvent = W32(CreateEventW)(NULL, FALSE, FALSE, NULL);

    HDC hdc = W32(GetDC)(NULL);
    int dpi = W32(GetDeviceCaps)(hdc, LOGPIXELSX);
    W32(ReleaseDC)(NULL, hdc);
    if (dpi >= 96) {
        g_dpiScale = (float)dpi / 96.0f;
    }
    g_widgetW = S(112);
    g_widgetH = S(124);

    LoadConfig();
    int vx = W32(GetSystemMetrics)(SM_XVIRTUALSCREEN);
    int vy = W32(GetSystemMetrics)(SM_YVIRTUALSCREEN);
    int vw = W32(GetSystemMetrics)(SM_CXVIRTUALSCREEN);
    int vh = W32(GetSystemMetrics)(SM_CYVIRTUALSCREEN);
    if (g_winX < vx - 20 || g_winX > vx + vw - 20 || g_winY < vy - 20 || g_winY > vy + vh - 20) {
        int sx = W32(GetSystemMetrics)(SM_CXSCREEN);
        int sy = W32(GetSystemMetrics)(SM_CYSCREEN);
        g_winX = sx - g_widgetW - S(32);
        g_winY = sy - g_widgetH - S(72);
    }

    g_hFontUI = W32(CreateFontW)(-S(14), 0, 0, 0, 400, 0, 0, 0, 1, 0, 0, 5, 0, u"Microsoft YaHei UI");
    g_hBrushDlgBg = W32(CreateSolidBrush)(0x00FFFFFFU);

    HICON hAppIcon = W32(LoadIconW)(g_hInst, (const WCHAR *)(uintptr_t)1);

    WNDCLASSEXW wcMain;
    memset(&wcMain, 0, sizeof(wcMain));
    wcMain.cbSize = sizeof(wcMain);
    wcMain.lpfnWndProc = MainWndProc;
    wcMain.hInstance = g_hInst;
    wcMain.hIcon = hAppIcon;
    wcMain.hCursor = W32(LoadCursorW)(NULL, IDC_SIZEALL);
    wcMain.lpszClassName = u"DesktopShredderFloatingCls";
    wcMain.hIconSm = hAppIcon;
    W32(RegisterClassExW)(&wcMain);

    WNDCLASSEXW wcSet;
    memset(&wcSet, 0, sizeof(wcSet));
    wcSet.cbSize = sizeof(wcSet);
    wcSet.lpfnWndProc = SettingsWndProc;
    wcSet.hInstance = g_hInst;
    wcSet.hIcon = hAppIcon;
    wcSet.hCursor = W32(LoadCursorW)(NULL, IDC_ARROW);
    wcSet.hbrBackground = g_hBrushDlgBg;
    wcSet.lpszClassName = u"DesktopShredderSettingsCls";
    wcSet.hIconSm = hAppIcon;
    W32(RegisterClassExW)(&wcSet);

    DWORD tid = 0;
    W32(CreateThread)(NULL, 0, ShredWorkerThread, NULL, 0, &tid);

    g_hwndMain = W32(CreateWindowExW)(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        u"DesktopShredderFloatingCls",
        u"桌面碎纸机",
        WS_POPUP | WS_VISIBLE,
        g_winX,
        g_winY,
        g_widgetW,
        g_widgetH,
        NULL,
        NULL,
        g_hInst,
        NULL);

    InitDropTarget();
    BOOL oleRegistered = FALSE;
    if (oleHr >= 0 && W32(RegisterDragDrop)(g_hwndMain, &g_dropTarget) == S_OK) {
        oleRegistered = TRUE;
    } else {
        W32(DragAcceptFiles)(g_hwndMain, TRUE);
        W32(ChangeWindowMessageFilterEx)(g_hwndMain, WM_DROPFILES, MSGFLT_ALLOW, NULL);
        W32(ChangeWindowMessageFilterEx)(g_hwndMain, 0x0049U, MSGFLT_ALLOW, NULL);
    }

    RenderLayeredShredder();
    W32(SetTimer)(g_hwndMain, TIMER_FULLSCREEN, 400, NULL);

    MSG msg;
    while (W32(GetMessageW)(&msg, NULL, 0, 0) > 0) {
        W32(TranslateMessage)(&msg);
        W32(DispatchMessageW)(&msg);
    }

    if (oleRegistered) {
        W32(RevokeDragDrop)(g_hwndMain);
    }
    if (oleHr >= 0) {
        W32(OleUninitialize)();
    }

    SaveConfig();
    W32(ExitProcess)(0);
}
