#ifdef _WIN32
#include <windows.h>

void sleep(unsigned milliseconds)
{
    Sleep(milliseconds);
}
#else
#include <unistd.h>

void sleep(unsigned milliseconds)
{
    usleep(milliseconds * 1000); // takes microseconds
}
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <atlbase.h>
#include <ole2.h>
#include <olectl.h>
#include "wtypes.h"

#define COLORREF2RGB(Color) (Color & 0xff00) | ((Color >> 16) & 0xff) \
                                 | ((Color << 16) & 0xff0000)

using namespace std;

int WIDTH = 0;
int HEIGHT = 0;
int maxDIM = 2000;

COLORREF arr1[2000][2000];
COLORREF arr2[2000][2000];

COLORREF cGREEN = 0x00ff00;

// Get the horizontal and vertical screen sizes in pixel
void GetDesktopResolution(int& horizontal, int& vertical)
{
    RECT desktop;
    // Get a handle to the desktop window
    const HWND hDesktop = GetDesktopWindow();
    // Get the size of screen to the variable desktop
    GetWindowRect(hDesktop, &desktop);
    // The top left corner will have coordinates (0,0)
    // and the bottom right corner will have coordinates
    // (horizontal, vertical)
    horizontal = desktop.right;
    vertical = desktop.bottom;
}

bool saveBitmap(LPCSTR filename, HBITMAP bmp, HPALETTE pal);

bool screenCapturePart(int x, int y, int w, int h, LPCSTR fname1, LPCSTR fname2) {

    /* debug need
    ofstream myfile, myfile2;
    myfile.open("temp.txt");
    myfile2.open("temp2.txt");

    myfile << "Writing this to a file 1.\n";
    myfile2 << "Writing this to a file 2.\n";
    */

    HBITMAP hBitmap1 = NULL;
    HBITMAP hBitmap2 = NULL;
    HBITMAP hBitmap3 = NULL;

    {
        HDC hdcSource = GetDC(NULL);
        HDC hdcMemory = CreateCompatibleDC(hdcSource);

        int capX = GetDeviceCaps(hdcSource, HORZRES);
        int capY = GetDeviceCaps(hdcSource, VERTRES);

        hBitmap1 = CreateCompatibleBitmap(hdcSource, w, h);
        HBITMAP hBitmapOld1 = (HBITMAP)SelectObject(hdcMemory, hBitmap1);

        BitBlt(hdcMemory, 0, 0, w, h, hdcSource, x, y, SRCCOPY);

        for (int w = 0; w < WIDTH; ++w)
            for (int h = 0; h < HEIGHT; ++h)
            {
                arr1[w][h] = GetPixel(hdcMemory, w, h);
                //myfile << w << " " << h << " " << arr1[w][h] << "\n";
            }

        hBitmap1 = (HBITMAP)SelectObject(hdcMemory, hBitmapOld1);

        DeleteDC(hdcSource);
        DeleteDC(hdcMemory);
    }

    sleep(1000);

    {
        HDC hdcSource = GetDC(NULL);
        HDC hdcMemory = CreateCompatibleDC(hdcSource);

        int capX = GetDeviceCaps(hdcSource, HORZRES);
        int capY = GetDeviceCaps(hdcSource, VERTRES);

        hBitmap2 = CreateCompatibleBitmap(hdcSource, w, h);
        HBITMAP hBitmapOld2 = (HBITMAP)SelectObject(hdcMemory, hBitmap2);

        BitBlt(hdcMemory, 0, 0, w, h, hdcSource, x, y, SRCCOPY);

        for (int w = 0; w < WIDTH; ++w)
            for (int h = 0; h < HEIGHT; ++h)
            {
                arr2[w][h] = GetPixel(hdcMemory, w, h);
                //myfile2 << w << " " << h << " " << arr2[w][h] << "\n";
            }

        hBitmap2 = (HBITMAP)SelectObject(hdcMemory, hBitmapOld2);

        DeleteDC(hdcSource);
        DeleteDC(hdcMemory);
    }


    //myfile.close();
    //myfile2.close();

    {
        HDC hdcSource = GetDC(NULL);
        HDC hdcMemory = CreateCompatibleDC(hdcSource);

        int capX = GetDeviceCaps(hdcSource, HORZRES);
        int capY = GetDeviceCaps(hdcSource, VERTRES);

        hBitmap3 = CreateCompatibleBitmap(hdcSource, w, h);
        HBITMAP hBitmapOld3 = (HBITMAP)SelectObject(hdcMemory, hBitmap3);

        BitBlt(hdcMemory, 0, 0, w, h, hdcSource, x, y, SRCCOPY);

        for (int w = 0; w < WIDTH; ++w)
            for (int h = 0; h < HEIGHT; ++h)
            {
                if (arr1[w][h] != arr2[w][h])
                    SetPixel(hdcMemory, w, h, cGREEN);
            }

        hBitmap3 = (HBITMAP)SelectObject(hdcMemory, hBitmapOld3);

        DeleteDC(hdcSource);
        DeleteDC(hdcMemory);

    }

    if (saveBitmap(fname1, hBitmap1, NULL) && saveBitmap(fname2, hBitmap2, NULL)
        && saveBitmap("result.bmp", hBitmap3, NULL))
        return true;

    return false;
}

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

int main(int argc, char* argv[]) {
    USES_CONVERSION;
    int width = 0, height = 0;
    GetDesktopResolution(width, height);

    WIDTH = width;
    HEIGHT = height;

    screenCapturePart(0, 0, WIDTH, HEIGHT, "first.bmp", "second.bmp");

    return 0;

}