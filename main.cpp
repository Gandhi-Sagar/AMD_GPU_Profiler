#include <windows.h>
#include <iostream>
#include <fstream>
#include <GL/gl.h>
#include <string>

#include "wglext.h"		// for WGL_AMD_OGL_ASSOCIATION function declarations, def are in ogl32.dll
#include "adl_sdk.h"	// for ADL SDK from AMD, def are in atiadlxy.dll(for 32 bit) and atiadlxx(for 64 bit)
#include "glext.h"

#define GL_GPU_MEM_INFO_NVIDIA 0x9048

std::ofstream		GPUInfo("GPUInfo.txt");

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM  lParam);

void*	__stdcall	ADL_Main_Memory_Alloc(int size)
{
	void*	pvBuf = malloc(size);
	return  pvBuf;
}

void	__stdcall	ADL_Main_Memory_Free(void **ppvBuf)
{
	if(*ppvBuf)
	{
		free(*ppvBuf);
		*ppvBuf = NULL;
	}
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
        MSG            msg;            // Windows message structure
        WNDCLASS       wc;             // Windows class structure
        HWND           hWnd;           // Storage for window handle

        // Register Window style
        wc.style               = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc         = (WNDPROC) WndProc;
        wc.cbClsExtra          = 0;
        wc.cbWndExtra          = 0;
        wc.hInstance           = hInstance;
        wc.hIcon               = NULL;
        wc.hCursor             = LoadCursor(NULL, IDC_ARROW);

        // No need for background brush for OpenGL window
        wc.hbrBackground       = NULL;          

        wc.lpszMenuName        = NULL;
        wc.lpszClassName       = TEXT("JustForContext");

        // Register the window class
        if(RegisterClass(&wc) == 0)
                return FALSE;

        // Create the main application window
        hWnd = CreateWindowEx(
						NULL,
                       TEXT("JustForContext"),
                       TEXT("JustForContext"),

                       // OpenGL requires WS_CLIPCHILDREN and
                       WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,

                       // Window position and size
                       100, 100,
                       250, 250,

                       NULL,
                       NULL,
                       hInstance,
                       NULL);
        // If window was not created, quit
        if(hWnd == NULL)
                return FALSE;

        // Display the window
        //ShowWindow(hWnd,SW_SHOW);
        //UpdateWindow(hWnd);

        // Process application messages until the application closes
        while( GetMessage(&msg, NULL, 0, 0))
                {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                }

        return msg.wParam;
        }


void SetDCPixelFormat(HDC hDC)
{
        int nPixelFormat;

        static PIXELFORMATDESCRIPTOR pfd = {
                sizeof(PIXELFORMATDESCRIPTOR),  // Size of this structure
                1,                              // Version of this   structure

                PFD_DRAW_TO_WINDOW |            // Draw to window  (not bitmap)

                PFD_SUPPORT_OPENGL |            // Support OpenGL calls
                PFD_DOUBLEBUFFER,               // Double-buffered mode
                PFD_TYPE_RGBA,                  // RGBA Color mode
                24,                             // Want 24bit color
                0,0,0,0,0,0,                    // Not used to select mode
                0,0,                            // Not used to select mode
                0,0,0,0,0,                      // Not used to select mode
                32,                             // Size of depth buffer
                0,                              // Not used to select mode
                0,                              // Not used to select mode
                PFD_MAIN_PLANE,                 // Draw in main plane
                0,                              // Not used to select mode
                0,0,0 };                   // Not used to select mode

         // Choose a pixel format that best matches that described in pfd
         nPixelFormat = ChoosePixelFormat(hDC, &pfd);

         // Set the pixel format for the device context
         SetPixelFormat(hDC, nPixelFormat, &pfd);
         }


void GPUInfoUsing_WGL_OGL_Association(HWND hWnd)
{
	wchar_t			buff[4096] = {0};
	UINT			maxCount;
	UINT			*ID;
	size_t			memTotal = 0;
	
	PFNWGLGETGPUIDSAMDPROC	wglGetGPUIDsAMD = (PFNWGLGETGPUIDSAMDPROC) wglGetProcAddress("wglGetGPUIDsAMD");
	
	if(wglGetGPUIDsAMD)
	{
		maxCount = wglGetGPUIDsAMD(0, 0);
		ID = new UINT[maxCount];
		wglGetGPUIDsAMD(maxCount, ID);
		PFNWGLGETGPUINFOAMDPROC		wglGetGPUInfoAMD = (PFNWGLGETGPUINFOAMDPROC) wglGetProcAddress("wglGetGPUInfoAMD");
		if(wglGetGPUInfoAMD)
		{
			int retVal = wglGetGPUInfoAMD(ID[0], WGL_GPU_RAM_AMD, GL_UNSIGNED_INT, sizeof(UINT), &memTotal);
			GPUInfo << "Total GPU RAM is " << memTotal << std::endl;
			
			/*
			swprintf(buff, L"GPU RAM is %uMB", memTotal);
			MessageBox(hWnd, buff, TEXT("GPU mem info"), MB_OK); */
		}
		else
			MessageBox(hWnd, TEXT("No GPU info function found"), TEXT("GPU Mem err"), MB_OK); 
		delete []ID;
	}
	else
		MessageBox(hWnd, TEXT("No GPU ID function found"), TEXT("GPU ID err"), MB_OK); 
}

void GPUInfoUsing_ADL_SDK(HWND hWnd)
{
	HINSTANCE		hDll_ADL = NULL;
	int				numOfAdapters = -1;
	int				adapterID = -1;
	int				isOverdriveSupported = -1;
	int				isOverdriveEnabled = -1;
	int				overdriveVersion = -1;

	hDll_ADL = LoadLibrary(TEXT("atiadlxy.dll"));
	if(hDll_ADL == NULL)
	{
		MessageBox(hWnd, TEXT("LoadLibrary Failed"), TEXT("Err"), MB_OK | MB_ICONERROR);
		return;
	}

	typedef int (*ADL_MAIN_CONTROL_CREATE) (ADL_MAIN_MALLOC_CALLBACK, int);
	typedef int (*ADL_ADAPTER_NUMBEROFADAPTERS_GET)(int*);
	typedef int (*ADL_ADAPTER_ADAPTERINFO_GET)(LPAdapterInfo, int);
	typedef int (*ADL_ADAPTER_ACTIVE_GET)(int, int*);
	typedef int (*ADL_OVERDRIVE_CAPS)(int, int*, int*, int*);

	ADL_MAIN_CONTROL_CREATE					ADL_Main_Control_Create = NULL;
	ADL_ADAPTER_NUMBEROFADAPTERS_GET		ADL_Adapter_NumberOfAdapters_Get = NULL;
	LPAdapterInfo							structAdapterInfo = NULL;
	ADL_ADAPTER_ADAPTERINFO_GET				ADL_Adapter_AdapterInfo_Get = NULL;
	ADL_ADAPTER_ACTIVE_GET					ADL_Adapter_Active_Get = NULL;
	ADL_OVERDRIVE_CAPS						ADL_Overdrive_Caps = NULL;

	ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE) GetProcAddress(hDll_ADL, "ADL_Main_Control_Create");
	ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET) GetProcAddress(hDll_ADL, "ADL_Adapter_NumberOfAdapters_Get");
	ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET)GetProcAddress(hDll_ADL, "ADL_Adapter_AdapterInfo_Get");
	ADL_Adapter_Active_Get = (ADL_ADAPTER_ACTIVE_GET)GetProcAddress(hDll_ADL, "ADL_Adapter_Active_Get");
	ADL_Overdrive_Caps = (ADL_OVERDRIVE_CAPS)GetProcAddress(hDll_ADL, "ADL_Overdrive_Caps");

	if(	ADL_Main_Control_Create == NULL					||
		ADL_Adapter_NumberOfAdapters_Get == NULL		||
		ADL_Adapter_AdapterInfo_Get == NULL				||
		ADL_Adapter_Active_Get == NULL					||
		ADL_Overdrive_Caps == NULL
	  )
	{
		MessageBox(hWnd, TEXT("Could not find some of the API, guess dll is missing your required functions."), TEXT("Err"), MB_OK | MB_ICONERROR);
		return;
	}

	int ret = ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1);
	if(ret != ADL_OK)
	{
		MessageBox(hWnd, TEXT("ADL Initialization Failed."), TEXT("Err"), MB_OK | MB_ICONERROR);
		return;
	}

	ret = ADL_Adapter_NumberOfAdapters_Get(&numOfAdapters);
	if(ret != ADL_OK)
	{
		MessageBox(hWnd, TEXT("Could not get number of adapters"), TEXT("Err"), MB_OK | MB_ICONERROR);
		return;
	}

	if(numOfAdapters > 0)
	{
		structAdapterInfo = (LPAdapterInfo)malloc(sizeof(AdapterInfo) * numOfAdapters);
		memset(structAdapterInfo, '\0', sizeof(AdapterInfo)*numOfAdapters );

		ADL_Adapter_AdapterInfo_Get(structAdapterInfo, sizeof(AdapterInfo)*numOfAdapters);
	}

	for(int i = 0 ; i < numOfAdapters ; i++)
	{
		int activeAdapter = 0;
		AdapterInfo	 thisAdapterInfo = structAdapterInfo[i];
		ADL_Adapter_Active_Get(thisAdapterInfo.iAdapterIndex, &activeAdapter);
		if(activeAdapter && thisAdapterInfo.iVendorID == 1002)//1002 is AMD vendor ID
		{
			adapterID = thisAdapterInfo.iAdapterIndex;
			//MessageBox(hWnd, TEXT("good so far"), TEXT("Info"), MB_OK );
			break;
		}
	}

	ret = ADL_Overdrive_Caps(adapterID, &isOverdriveSupported, &isOverdriveEnabled, &overdriveVersion);
/*	if(ret != ADL_OK)
	{
		MessageBox(hWnd, TEXT("Could not get overdrive capabilities"), TEXT("Err"), MB_OK | MB_ICONERROR);
		return;
	}

	if(overdriveVersion == 5)
		MessageBox(hWnd, TEXT("version is 5"), TEXT("Err"), MB_OK | MB_ICONERROR);
	else if(overdriveVersion == 6)
		MessageBox(hWnd, TEXT("version is 6"), TEXT("Err"), MB_OK | MB_ICONERROR);
	else
		MessageBox(hWnd, TEXT("version is not that you can support"), TEXT("Err"), MB_OK | MB_ICONERROR);
*/
	if(hDll_ADL)
		FreeLibrary(hDll_ADL);
}


void	glInfoAll(HWND hWnd)
{
	bool			ATI_GPU;

	std::string		gl_helpString;
	std::wstring	sTemp;
	LPCWSTR			MsgboxStr;

	wchar_t			buff[4096] = {0};
	int				maxBuffers = 0;
	int				maxTextureSize = 0;
	int				maxTextureBufferSize = 0;
	int				maxTextureUnits = 0;
	int				totalGPU_RAM;

	gl_helpString = (const char*)glGetString(GL_VENDOR);
	if(gl_helpString.find("ATI") != std::string::npos)
		ATI_GPU = true;
	else if(gl_helpString.find("NVIDIA") != std::string::npos)
		ATI_GPU = false;
	else 
	{	//no gpu support.... exit
		GPUInfo << "Neither ATI or AMD GPU" << std::endl;
		return;
	}
	GPUInfo << gl_helpString << std::endl;
	
	/*sTemp = std::wstring(gl_helpString.begin(), gl_helpString.end());
	MsgboxStr  = sTemp.c_str();
	MessageBox(hWnd, MsgboxStr, TEXT("Vendor"), MB_OK); */
	
	gl_helpString = (const char*)glGetString(GL_VERSION);
	GPUInfo << gl_helpString << std::endl;
	/*
	sTemp = std::wstring(gl_helpString.begin(), gl_helpString.end());
	MsgboxStr  = sTemp.c_str();
	MessageBox(hWnd, MsgboxStr, TEXT("Version"), MB_OK); */

	gl_helpString = (const char*)glGetString(GL_RENDERER);
	GPUInfo << gl_helpString << std::endl;
	/*
	sTemp = std::wstring(gl_helpString.begin(), gl_helpString.end());
	MsgboxStr  = sTemp.c_str();
	MessageBox(hWnd, MsgboxStr, TEXT("Renderer"), MB_OK); */

	gl_helpString = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	GPUInfo <<"Shading Language Version Support" << gl_helpString << std::endl;
	/*
	sTemp = std::wstring(gl_helpString.begin(), gl_helpString.end());
	MsgboxStr  = sTemp.c_str();
	MessageBox(hWnd, MsgboxStr, TEXT("Shading Language Version"), MB_OK); 
*/
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxBuffers);
	GPUInfo << "Max no of simultaneous o/p buffers are " << maxBuffers << std::endl;
	
	/*
	swprintf(buff, L"Max no of simultaneous o/p buffers are %d", maxBuffers);
	MessageBox(hWnd, buff, TEXT("Mem Info - Buffers"), MB_OK); */

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	GPUInfo << "Max texture size is " << maxTextureSize << std::endl;

	/*swprintf(buff, L"Max texture size is %d", maxTextureSize);
	MessageBox(hWnd, buff, TEXT("Mem Info - Max Texture"), MB_OK); */

	glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxTextureBufferSize);
	GPUInfo << "Max texture buffer size is " << maxTextureBufferSize << std::endl;

	/*swprintf(buff, L"Max texture buffer size is %d", maxTextureBufferSize);
	MessageBox(hWnd, buff, TEXT("Mem Info - Max Texture Buffer"), MB_OK); */

	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &maxTextureUnits);
	GPUInfo << "Max texture Units are  " << maxTextureUnits << std::endl;
	/*swprintf(buff, L"Max texture Units are %d", maxTextureSize);
	MessageBox(hWnd, buff, TEXT("Mem Info - Max Texture Units"), MB_OK); */

	if(ATI_GPU)
		GPUInfoUsing_WGL_OGL_Association(hWnd);
	else
	{
		glGetIntegerv(GL_GPU_MEM_INFO_NVIDIA, &totalGPU_RAM);
		GPUInfo << "GPU RAM is  " << totalGPU_RAM << std::endl;
	/*	swprintf(buff, L"GPU RAM is  %d", totalGPU_RAM);
		MessageBox(hWnd, buff, TEXT("GPU RAM"), MB_OK); */
	}

	GPUInfoUsing_ADL_SDK(hWnd);

	PostQuitMessage(0);
	//destroySelf = true;
}


// Window procedure, handles all messages for this program
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM  lParam)
{
        static	HGLRC hRC;              // Permanent Rendering context
        static	HDC hDC;                // Private GDI Device context
		//const	GLubyte*		vendor = NULL;
		std::string		gl_helpString;
		
        switch (message)
        {
			// Window creation, setup for OpenGL
			case WM_CREATE:
							{                 // Store the device context
								hDC = GetDC(hWnd);          

								// Select the pixel format
								SetDCPixelFormat(hDC);          

								// Create the rendering context
								// and make it current

								hRC = wglCreateContext(hDC);
								wglMakeCurrent(hDC, hRC);
						  
								glInfoAll(hWnd);
							//						  free((void*)vendor);

								// Create a timer that fires every millisecond
							//         SetTimer(hWnd,101,1,NULL);
								break;
							}
			// Window is being destroyed, cleanup

			case WM_DESTROY:
							// Kill the timer that we created
							//KillTimer(hWnd,101);

							// Deselect the current rendering
							//context and delete it

							wglMakeCurrent(hDC,NULL);
							wglDeleteContext(hRC);

							// Tell the application to terminate
							//after the window

							// is gone.
							PostQuitMessage(0);
							break;

			// Window is resized.
			case WM_SIZE:
							// Call our function which modifies the clipping
							// volume and viewport
							// ChangeSize(LOWORD(lParam), HIWORD(lParam));
							break;

							// Timer, moves and bounces the rectangle, simply calls
							// our previous OnIdle function, then invalidates the
							// window so it will be redrawn.
			case WM_TIMER:
							{
							// IdleFunction();

							InvalidateRect(hWnd,NULL,FALSE);
							}
							break;

			// The painting function. This message sent by Windows
			// whenever the screen needs updating.
			case WM_PAINT:
							{
							// Call OpenGL drawing code
							//  RenderScene();

							// Call function to swap the buffers
							SwapBuffers(hDC);

							// Validate the newly painted client area
							ValidateRect(hWnd,NULL);
							}
							break;

			default:   // Passes it on if unproccessed
							return (DefWindowProc(hWnd, message, wParam, lParam));

       }
return (0L);
}

