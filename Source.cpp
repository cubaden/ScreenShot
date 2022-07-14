/*
* 
* 
* This is a speed test, and you can copy code from anywhere you want, use any API you want, just do it fast - in 60 minutes.
*
* Tasks:
*
* Part 1 - write a program, no UI, that takes a screenshot and writes it to .bmp.
*
* Part 2 - modify the program. Take a screenshot, sleep 10 seconds, take another screenshot. 
* Pixels that are NOT the same - make them green, and write the new bmp.
* 
* 
* Output:
* 
* Outputting to a current directory
* 
* First screenshot - ("firstScreenShot.bmp")
* Second screenshot - ("secondScreenShot.bmp")
* Result screenshot - ("resultScreenShot.bmp") - Green pixels as a difference between First and Second screenshots
* 
*/
#include <windows.h>
#include <vector>
#include <string>
#include <functional>
#include <atlbase.h>
#include <ole2.h>
#include <olectl.h>
#include "wtypes.h"

using namespace std;

const std::string name1("firstScreenShot.bmp");
const std::string name2("secondScreenShot.bmp");
const std::string nameResult("resultScreenShot.bmp");

int WIDTH = 0;
int HEIGHT = 0;

COLORREF GREEN_COLORREF = 0x00ff00;

// Get the horizontal and vertical screen sizes in pixel
void GetDesktopResolution(int& horizontal, int& vertical)
{
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    // The top left corner will have coordinates (0,0)
    // and the bottom right corner will have coordinates
    // (horizontal, vertical)
    horizontal = desktop.right;
    vertical = desktop.bottom;
}

// Save bitmap to a file
bool saveBitmap(LPCSTR filename, HBITMAP bmp, HPALETTE pal);

// get bitmap of screen capture
HBITMAP getHBitmapScreenCapture(int x, int y, int w, int h, function<bool(int, int, COLORREF)> f, bool make_green = false)
{
    HDC hdcSource = GetDC(NULL);
    HDC hdcMemory = CreateCompatibleDC(hdcSource);

    int capX = GetDeviceCaps(hdcSource, HORZRES);
    int capY = GetDeviceCaps(hdcSource, VERTRES);

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcSource, w, h);
    HBITMAP hBitmapOld = (HBITMAP) SelectObject(hdcMemory, hBitmap);

    BitBlt(hdcMemory, 0, 0, w, h, hdcSource, x, y, SRCCOPY);

    for (int w = 0; w < WIDTH; ++w)
        for (int h = 0; h < HEIGHT; ++h)
            if (f(w, h, GetPixel(hdcMemory, w, h)) && make_green)
                SetPixel(hdcMemory, w, h, GREEN_COLORREF);

    hBitmap = (HBITMAP) SelectObject(hdcMemory, hBitmapOld);

    DeleteDC(hdcSource);
    DeleteDC(hdcMemory);

    return hBitmap;
}

vector<vector<COLORREF>> arrFirstBmpColors;
vector<vector<COLORREF>> arrSecondBmpColors;

// saving colors for first screenshot
bool functor1(int w, int h, COLORREF c)
{ 
    arrFirstBmpColors[w][h] = c;
    return false;
};

// saving colors for second screenshot
bool functor2(int w, int h, COLORREF c)
{
    arrSecondBmpColors[w][h] = c;
    return false;
};

// checking if pixels are different
bool functorMakePixelGreen(int w, int h, COLORREF c)
{
    return (arrFirstBmpColors[w][h] != arrSecondBmpColors[w][h]);
};

// capturing rect (x,y,w,h) and writing first, second and result bmp
bool screenCapturePart(int x, int y, int w, int h, LPCSTR fname1, LPCSTR fname2, LPCSTR fnameResult) 
{
    HBITMAP hBitmap1 = getHBitmapScreenCapture(x, y, w, h, functor1);
    Sleep(1000);
    HBITMAP hBitmap2 = getHBitmapScreenCapture(x, y, w, h, functor2);
    HBITMAP hBitmapRes = getHBitmapScreenCapture(x, y, w, h, functorMakePixelGreen, true);

    if (saveBitmap(fname1, hBitmap1, NULL) 
        && saveBitmap(fname2, hBitmap2, NULL)
        && saveBitmap(fnameResult, hBitmapRes, NULL))
        return true;

    return false;
}

// save HBITMAP to a file in the current directory
bool saveBitmap(LPCSTR filename, HBITMAP bmp, HPALETTE pal)
{
    USES_CONVERSION;

    bool result = false;
    PICTDESC pd;

    pd.cbSizeofstruct = sizeof(PICTDESC);
    pd.picType = PICTYPE_BITMAP;
    pd.bmp.hbitmap = bmp;
    pd.bmp.hpal = pal;

    LPPICTURE picture;
    HRESULT res = OleCreatePictureIndirect(&pd, IID_IPicture, false,
        reinterpret_cast<void**>(&picture));

    if (!SUCCEEDED(res))
        return false;

    LPSTREAM stream;
    res = CreateStreamOnHGlobal(0, true, &stream);

    if (!SUCCEEDED(res))
    {
        picture->Release();
        return false;
    }

    LONG bytes_streamed;
    res = picture->SaveAsFile(stream, true, &bytes_streamed);

    LPCWSTR w = A2W(filename);
    HANDLE file = CreateFile(w, GENERIC_WRITE, FILE_SHARE_READ, 0,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (!SUCCEEDED(res) || !file)
    {
        stream->Release();
        picture->Release();
        return false;
    }

    HGLOBAL mem = 0;
    GetHGlobalFromStream(stream, &mem);
    LPVOID data = GlobalLock(mem);

    DWORD bytes_written;

    result = !!WriteFile(file, data, bytes_streamed, &bytes_written, 0);
    result &= (bytes_written == static_cast<DWORD>(bytes_streamed));

    GlobalUnlock(mem);
    CloseHandle(file);

    stream->Release();
    picture->Release();

    return result;
}

int main(int argc, char* argv[]) 
{
    USES_CONVERSION;

    int width = 0, height = 0;
    GetDesktopResolution(width, height);

    WIDTH = width;
    HEIGHT = height;

    arrFirstBmpColors.resize(WIDTH, vector<COLORREF>(HEIGHT));
    arrSecondBmpColors.resize(WIDTH, vector<COLORREF>(HEIGHT));

    screenCapturePart(0, 0, WIDTH, HEIGHT, name1.c_str(), name2.c_str(), nameResult.c_str());

    return 0;
}