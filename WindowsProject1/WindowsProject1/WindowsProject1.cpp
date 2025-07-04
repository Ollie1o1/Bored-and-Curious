#include "framework.h"
#include <windowsx.h>
#include "WindowsProject1.h"
#include <vector>
#include <cmath>

#define MAX_LOADSTRING 100

// Menu IDs
#define ID_BG_RED 1001
#define ID_BG_GREEN 1002
#define ID_BG_BLUE 1003
#define ID_BALL_RED 1004
#define ID_BALL_GREEN 1005
#define ID_BALL_BLUE 1006

// Global Variables:
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

// Physics Variables:
struct Ball {
    float x, y;
    float vx, vy;
};
std::vector<Ball> balls;
const float radius = 20.0f;
const float gravity = 500.0f;
const float damping = 0.8f;
int window_width, window_height;
DWORD last_time;
COLORREF bg_color = RGB(0, 0, 0);
COLORREF ball_color = RGB(0, 0, 255);

// Virtual ball
bool virtual_ball_active = false;
float virtual_ball_x, virtual_ball_y;

// Holding variables
bool is_holding = false;
float start_x, start_y;
const float threshold = 5.0f;
const float velocity_scale = 5.0f;

// Forward declarations
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINDOWSPROJECT1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSPROJECT1));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 800, 600, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
    {
        return FALSE;
    }
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        // Initialize simulation
        balls.clear();
        last_time = GetTickCount();
        SetTimer(hWnd, 1, 17, NULL); // Approximately 60 FPS
        break;

    case WM_TIMER:
        if (wParam == 1)
        {
            DWORD current_time = GetTickCount();
            float delta_time = (current_time - last_time) / 1000.0f;
            if (delta_time > 0.1f) delta_time = 0.1f; // Cap delta_time
            last_time = current_time;

            // Update all balls
            for (auto& ball : balls)
            {
                ball.vy += gravity * delta_time;
                ball.x += ball.vx * delta_time;
                ball.y += ball.vy * delta_time;
            }

            // Ball-to-ball collisions
            for (size_t i = 0; i < balls.size(); ++i)
            {
                for (size_t j = i + 1; j < balls.size(); ++j)
                {
                    float dx = balls[j].x - balls[i].x;
                    float dy = balls[j].y - balls[i].y;
                    float distance = sqrtf(dx * dx + dy * dy);
                    if (distance < 2 * radius)
                    {
                        // Normalize collision direction
                        float nx = dx / distance;
                        float ny = dy / distance;
                        // Separate balls
                        float overlap = 2 * radius - distance;
                        balls[i].x -= nx * overlap / 2;
                        balls[i].y -= ny * overlap / 2;
                        balls[j].x += nx * overlap / 2;
                        balls[j].y += ny * overlap / 2;
                        // Update velocities (elastic collision, equal mass)
                        float v1_par = balls[i].vx * nx + balls[i].vy * ny;
                        float v2_par = balls[j].vx * nx + balls[j].vy * ny;
                        float v1_perp_x = balls[i].vx - v1_par * nx;
                        float v1_perp_y = balls[i].vy - v1_par * ny;
                        float v2_perp_x = balls[j].vx - v2_par * nx;
                        float v2_perp_y = balls[j].vy - v2_par * ny;
                        balls[i].vx = v1_perp_x + v2_par * nx;
                        balls[i].vy = v1_perp_y + v2_par * ny;
                        balls[j].vx = v2_perp_x + v1_par * nx;
                        balls[j].vy = v2_perp_y + v1_par * ny;
                    }
                }
            }

            // Ball-to-virtual ball collisions
            if (virtual_ball_active)
            {
                for (auto& ball : balls)
                {
                    float dx = ball.x - virtual_ball_x;
                    float dy = ball.y - virtual_ball_y;
                    float distance = sqrtf(dx * dx + dy * dy);
                    if (distance < 2 * radius)
                    {
                        // Normalize collision direction
                        float nx = dx / distance;
                        float ny = dy / distance;
                        // Separate ball from virtual ball
                        float overlap = 2 * radius - distance;
                        ball.x += nx * overlap;
                        ball.y += ny * overlap;
                        // Reflect velocity (virtual ball is immovable)
                        float v_dot_n = ball.vx * nx + ball.vy * ny;
                        ball.vx -= 2 * v_dot_n * nx;
                        ball.vy -= 2 * v_dot_n * ny;
                    }
                }
            }

            // Wall collisions
            for (auto& ball : balls)
            {
                if (ball.x - radius < 0)
                {
                    ball.x = radius;
                    ball.vx = -ball.vx * damping;
                }
                if (ball.x + radius > window_width)
                {
                    ball.x = window_width - radius;
                    ball.vx = -ball.vx * damping;
                }
                if (ball.y - radius < 0)
                {
                    ball.y = radius;
                    ball.vy = -ball.vy * damping;
                }
                if (ball.y + radius > window_height)
                {
                    ball.y = window_height - radius;
                    ball.vy = -ball.vy * damping;
                }
            }

            // Trigger repaint
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;

    case WM_SIZE:
        window_width = LOWORD(lParam);
        window_height = HIWORD(lParam);
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // Clear background
        RECT rect;
        GetClientRect(hWnd, &rect);
        HBRUSH hBgBrush = CreateSolidBrush(bg_color);
        FillRect(hdc, &rect, hBgBrush);
        DeleteObject(hBgBrush);
        // Draw all balls
        HBRUSH hBallBrush = CreateSolidBrush(ball_color);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBallBrush);
        for (const auto& ball : balls)
        {
            int left = (int)(ball.x - radius);
            int top = (int)(ball.y - radius);
            int right = (int)(ball.x + radius);
            int bottom = (int)(ball.y + radius);
            Ellipse(hdc, left, top, right, bottom);
        }
        SelectObject(hdc, hOldBrush);
        DeleteObject(hBallBrush);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_LBUTTONDOWN:
    {
        is_holding = true;
        start_x = (float)GET_X_LPARAM(lParam);
        start_y = (float)GET_Y_LPARAM(lParam);
        SetCapture(hWnd);
    }
    break;

    case WM_LBUTTONUP:
    {
        if (is_holding)
        {
            is_holding = false;
            ReleaseCapture();
            float end_x = (float)GET_X_LPARAM(lParam);
            float end_y = (float)GET_Y_LPARAM(lParam);
            float dx = end_x - start_x;
            float dy = end_y - start_y;
            float distance = sqrtf(dx * dx + dy * dy);
            if (distance < threshold)
            {
                // Drop ball with zero velocity
                balls.push_back({ start_x, start_y, 0.0f, 0.0f });
            }
            else
            {
                // Launch ball with velocity
                float vx = velocity_scale * dx;
                float vy = velocity_scale * dy;
                balls.push_back({ start_x, start_y, vx, vy });
            }
            InvalidateRect(hWnd, NULL, TRUE);
        }
    }
    break;

    case WM_MOUSEMOVE:
    {
        int mouse_x = GET_X_LPARAM(lParam);
        int mouse_y = GET_Y_LPARAM(lParam);
        if (wParam & MK_RBUTTON)
        {
            virtual_ball_active = true;
            virtual_ball_x = (float)mouse_x;
            virtual_ball_y = (float)mouse_y;
        }
        else
        {
            virtual_ball_active = false;
        }
    }
    break;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case ID_BG_RED:
            bg_color = RGB(255, 0, 0);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case ID_BG_GREEN:
            bg_color = RGB(0, 255, 0);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case ID_BG_BLUE:
            bg_color = RGB(0, 0, 255);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case ID_BALL_RED:
            ball_color = RGB(255, 0, 0);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case ID_BALL_GREEN:
            ball_color = RGB(0, 255, 0);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case ID_BALL_BLUE:
            ball_color = RGB(0, 0, 255);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;

    case WM_DESTROY:
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
