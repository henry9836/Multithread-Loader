
#include <windows.h>
#include <vector>
#include <string>
#include "resource.h"
#include <thread>
#include <iostream>
#include <gdiplus.h>
#include <mutex>

using namespace std;
using namespace Gdiplus;

//lets us use gdiplus
#pragma comment (lib, "gdiplus.lib")
#pragma comment (lib, "winmm.lib")

#define WINDOW_CLASS_NAME L"MultiThreaded Loader Tool"
const unsigned int _kuiWINDOWWIDTH = 1200;
const unsigned int _kuiWINDOWHEIGHT = 1200;
#define MAX_FILES_TO_OPEN 50
#define MAX_CHARACTERS_IN_FILENAME 25

//Global Variables
std::vector<std::wstring> g_vecImageFileNames;
std::vector<std::wstring> g_vecSoundFileNames;
std::vector<std::thread> g_vecThreads; //Holds threads
//number of threads is equal to the amount the system can handle 
unsigned int _iNumThreads = std::thread::hardware_concurrency();

//position for placing the images
constexpr int diff = 145;
int x = 5;
int y = 5;

std::mutex imageMutex;
std::mutex soundMutex;

HDC _hWindowDC;

HINSTANCE g_hInstance;
bool g_bIsFileLoaded = false;


int PlayMySound() {
	while (true) {
		soundMutex.lock();
		if (g_vecSoundFileNames.empty()) { //Stop if we have already emptyed g_vecImageFileNames
			soundMutex.unlock();
			break;
		}
		std::wstring al = L"mysong";
		std::wstring m_cmd1 = L"open \"" + *g_vecSoundFileNames.begin() + L"\" type waveaudio alias mysong";
		std::wstring m_cmd2 = L"play mysong";
		g_vecSoundFileNames.erase(g_vecSoundFileNames.begin());
		soundMutex.unlock();
		MCIERROR e = mciSendString(m_cmd1.c_str(), NULL, 0, NULL);
		MCIERROR e2 = mciSendString(m_cmd2.c_str(), NULL, 0, NULL);
		// check error
		if (e != 0)
		{
			OutputDebugString(L"Error: ");
			std::vector<wchar_t> d(256);
			d[255] = '\0';
			mciGetErrorString(e, d.data(), d.size() - 1);
			OutputDebugString(d.data());
			OutputDebugString(L"\n");
		}
	}
	return 0;
}
void DisplayMyImage() {

	
	Graphics graphics(_hWindowDC);
	while (true) {

		imageMutex.lock();
		
		if (g_vecImageFileNames.empty()) {
			imageMutex.unlock();
			break;
		}

		std::wstring filename = *g_vecImageFileNames.begin();
		g_vecImageFileNames.erase(g_vecImageFileNames.begin());

		Gdiplus::Image* image = Gdiplus::Image::FromFile(filename.c_str());

		graphics.DrawImage(image, x, y);
		
		x += diff;
		if (x >= 1031) {
			x = 5;
			y += diff;
		}
	
		image = 0;
		delete image;
		imageMutex.unlock();

	}
}


bool ChooseImageFilesToLoad(HWND _hwnd)
{
	OPENFILENAME ofn;
	SecureZeroMemory(&ofn, sizeof(OPENFILENAME)); // Better to use than ZeroMemory
	wchar_t wsFileNames[MAX_FILES_TO_OPEN * MAX_CHARACTERS_IN_FILENAME + MAX_PATH]; //The string to store all the filenames selected in one buffer togther with the complete path name.
	wchar_t _wsPathName[MAX_PATH + 1];
	wchar_t _wstempFile[MAX_PATH + MAX_CHARACTERS_IN_FILENAME]; //Assuming that the filename is not more than 20 characters
	wchar_t _wsFileToOpen[MAX_PATH + MAX_CHARACTERS_IN_FILENAME];
	ZeroMemory(wsFileNames, sizeof(wsFileNames));
	ZeroMemory(_wsPathName, sizeof(_wsPathName));
	ZeroMemory(_wstempFile, sizeof(_wstempFile));

	//Fill out the fields of the structure
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = _hwnd;
	ofn.lpstrFile = wsFileNames;
	ofn.nMaxFile = MAX_FILES_TO_OPEN * 20 + MAX_PATH;  //The size, in charactesr of the buffer pointed to by lpstrFile. The buffer must be atleast 256(MAX_PATH) characters long; otherwise GetOpenFileName and 
													   //GetSaveFileName functions return False
													   // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
													   // use the contents of wsFileNames to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrFilter = L"Bitmap Images(.bmp)\0*.bmp\0"; //Filter for bitmap images
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

	//If the user makes a selection from the  open dialog box, the API call returns a non-zero value
	if (GetOpenFileName(&ofn) != 0) //user made a selection and pressed the OK button
	{
		//Extract the path name from the wide string -  two ways of doing it
		//First way: just work with wide char arrays
		wcsncpy_s(_wsPathName, wsFileNames, ofn.nFileOffset);
		int i = ofn.nFileOffset;
		int j = 0;

		while (true)
		{
			if (*(wsFileNames + i) == '\0')
			{
				_wstempFile[j] = *(wsFileNames + i);
				wcscpy_s(_wsFileToOpen, _wsPathName);
				wcscat_s(_wsFileToOpen, L"\\");
				wcscat_s(_wsFileToOpen, _wstempFile);
				g_vecImageFileNames.push_back(_wsFileToOpen);
				j = 0;
			}
			else
			{
				_wstempFile[j] = *(wsFileNames + i);
				j++;
			}
			if (*(wsFileNames + i) == '\0' && *(wsFileNames + i + 1) == '\0')
			{
				break;
			}
			else
			{
				i++;
			}

		}

		g_bIsFileLoaded = true;
		return true;
	}
	else // user pressed the cancel button or closed the dialog box or an error occured
	{
		return false;
	}

}

bool ChooseSoundFilesToLoad(HWND _hwnd)
{
	OPENFILENAME ofn;
	SecureZeroMemory(&ofn, sizeof(OPENFILENAME)); // Better to use than ZeroMemory
	wchar_t wsFileNames[MAX_FILES_TO_OPEN * MAX_CHARACTERS_IN_FILENAME + MAX_PATH]; //The string to store all the filenames selected in one buffer togther with the complete path name.
	wchar_t _wsPathName[MAX_PATH + 1];
	wchar_t _wstempFile[MAX_PATH + MAX_CHARACTERS_IN_FILENAME]; //Assuming that the filename is not more than 20 characters
	static wchar_t _wsFileToOpen[MAX_PATH + MAX_CHARACTERS_IN_FILENAME];
	ZeroMemory(wsFileNames, sizeof(wsFileNames));
	ZeroMemory(_wsPathName, sizeof(_wsPathName));
	ZeroMemory(_wstempFile, sizeof(_wstempFile));

	//Fill out the fields of the structure
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = _hwnd;
	ofn.lpstrFile = wsFileNames;
	ofn.nMaxFile = MAX_FILES_TO_OPEN * 20 + MAX_PATH;  //The size, in charactesr of the buffer pointed to by lpstrFile. The buffer must be atleast 256(MAX_PATH) characters long; otherwise GetOpenFileName and 
													   //GetSaveFileName functions return False
													   // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
													   // use the contents of wsFileNames to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrFilter = L"Wave Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0"; //Filter for wav files
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

	//If the user makes a selection from the  open dialog box, the API call returns a non-zero value
	if (GetOpenFileName(&ofn) != 0) //user made a selection and pressed the OK button
	{
		//Extract the path name from the wide string -  two ways of doing it
		//Second way: work with wide strings and a char pointer 

		std::wstring _wstrPathName = ofn.lpstrFile;

		_wstrPathName.resize(ofn.nFileOffset, '\\');

		wchar_t *_pwcharNextFile = &ofn.lpstrFile[ofn.nFileOffset];

		while (*_pwcharNextFile)
		{
			std::wstring _wstrFileName = _wstrPathName + _pwcharNextFile;

			g_vecSoundFileNames.push_back(_wstrFileName);

			_pwcharNextFile += lstrlenW(_pwcharNextFile) + 1;
		}

		g_bIsFileLoaded = true;
		return true;
	}
	else // user pressed the cancel button or closed the dialog box or an error occured
	{
		return false;
	}

}

LRESULT CALLBACK WindowProc(HWND _hwnd, UINT _uiMsg, WPARAM _wparam, LPARAM _lparam)
{


	PAINTSTRUCT ps;



	//RECT rect;
	switch (_uiMsg)
	{
	case WM_KEYDOWN:
	{
		switch (_wparam)
		{
		case VK_ESCAPE:
		{
			SendMessage(_hwnd, WM_CLOSE, 0, 0);
			return(0);
		}
		break;
		default:
			break;
		}
	}
	break;
	case WM_PAINT:
	{

		_hWindowDC = BeginPaint(_hwnd, &ps);
		//Do all our painting here

		EndPaint(_hwnd, &ps);
		return (0);
	}
	break;
	case WM_COMMAND:
	{
		switch (LOWORD(_wparam))
		{


		case ID_FILE_LOADIMAGE:
		{

			Gdiplus::GdiplusStartupInput gdiplusStartupInput;
			ULONG_PTR gdiplusToken;
			Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);


			//std::thread::hardware_concurrency();


			// used to load multiple images to the canvas in parallel

			if (ChooseImageFilesToLoad(_hwnd))
			{

				for (unsigned int i = 0; i < _iNumThreads; i++)
				{
					//push threads into a vector based on the number of threads
					g_vecThreads.push_back(std::thread(DisplayMyImage));
				}
				for (unsigned int i = 0; i < g_vecThreads.size(); i++)
				{
					//join threads equal to the size of g_vecThreads
					g_vecThreads[i].join();
				}

				g_vecThreads.clear();


			}
			//if there are too many images or no images selected call this
			else
			{
				MessageBox(_hwnd, L"No Image File selected", L"Error Message", MB_ICONWARNING);
			}

			return (0);
		}
		break;
		case ID_FILE_LOADSOUND:
		{
			if (ChooseSoundFilesToLoad(_hwnd))
			{

				for (unsigned int i = 0; i < _iNumThreads; i++)
				{
					//push threads into a vector based on the number of threads
					g_vecThreads.push_back(std::thread(PlayMySound));
				}
				for (unsigned int i = 0; i < g_vecThreads.size(); i++)
				{
					//join threads equal to the size of g_vecThreads
					g_vecThreads[i].join();
				}

				g_vecThreads.clear();
				//Write code here to create multiple threads to load sound files in parallel
			}
			else
			{
				MessageBox(_hwnd, L"No Sound File selected", L"Error Message", MB_ICONWARNING);
			}
			return (0);
		}
		break;
		case ID_EXIT:
		{
			SendMessage(_hwnd, WM_CLOSE, 0, 0);
			return (0);
		}
		break;
		default:
			break;
		}
	}
	break;
	case WM_CLOSE:
	{
		PostQuitMessage(0);
	}
	break;
	default:
		break;
	}
	return (DefWindowProc(_hwnd, _uiMsg, _wparam, _lparam));
}


HWND CreateAndRegisterWindow(HINSTANCE _hInstance)
{
	WNDCLASSEX winclass; // This will hold the class we create.
	HWND hwnd;           // Generic window handle.

						 // First fill in the window class structure.
	winclass.cbSize = sizeof(WNDCLASSEX);
	winclass.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	winclass.lpfnWndProc = WindowProc;
	winclass.cbClsExtra = 0;
	winclass.cbWndExtra = 0;
	winclass.hInstance = _hInstance;
	winclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground =
		static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	winclass.lpszMenuName = NULL;
	winclass.lpszClassName = WINDOW_CLASS_NAME;
	winclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// register the window class
	if (!RegisterClassEx(&winclass))
	{
		return (0);
	}

	HMENU _hMenu = LoadMenu(_hInstance, MAKEINTRESOURCE(IDR_MENU1));

	// create the window
	hwnd = CreateWindowEx(NULL, // Extended style.
		WINDOW_CLASS_NAME,      // Class.
		L"MultiThreaded Loader Tool",   // Title.
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		10, 10,                    // Initial x,y.
		_kuiWINDOWWIDTH, _kuiWINDOWHEIGHT,                // Initial width, height.
		NULL,                   // Handle to parent.
		_hMenu,                   // Handle to menu.
		_hInstance,             // Instance of this application.
		NULL);                  // Extra creation parameters.

	return hwnd;
}



int WINAPI WinMain(HINSTANCE _hInstance,
	HINSTANCE _hPrevInstance,
	LPSTR _lpCmdLine,
	int _nCmdShow)
{
	MSG msg;  //Generic Message

	HWND _hwnd = CreateAndRegisterWindow(_hInstance);

	if (!(_hwnd))
	{
		return (0);
	}


	// Enter main event loop
	while (true)
	{
		// Test if there is a message in queue, if so get it.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// Test if this is a quit.
			if (msg.message == WM_QUIT)
			{
				break;
			}

			// Translate any accelerator keys.
			TranslateMessage(&msg);
			// Send the message to the window proc.
			DispatchMessage(&msg);
		}

	}

	// Return to Windows like this...
	return (static_cast<int>(msg.wParam));
}


