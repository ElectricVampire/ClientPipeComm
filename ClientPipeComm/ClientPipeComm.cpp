// ClientPipeComm.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <fstream>
#include <iostream>
using namespace std;

#define PIPE_NAME TEXT("\\\\.\\pipe\\myPipe.Comm")



HANDLE hPipeHandleReadWrite = INVALID_HANDLE_VALUE;
int MAX_WAIT_FOR_PIPE = 30000;
bool WAIT_FOREVER_FOR_PIPE = true;
BOOL bOpened = false;
BOOL bComplete = false;
DWORD WINAPI VdmStatusChanged(LPVOID);
DWORD Initialize();
DWORD WriteMessage(char * stMsg, int iMsgLen);
HANDLE hVdmStatusThread = INVALID_HANDLE_VALUE;
HANDLE hVdmSemaphore = INVALID_HANDLE_VALUE;
HANDLE glohFunctionMutex = NULL;

int main()
{
	// The read operation will block until there is data to read
	Initialize();

	// Mutex to synchronise the VDM thread and main thread access to the pipe
	if (NULL == hVdmSemaphore)
		glohFunctionMutex = CreateMutex(NULL,                // Security Attributes
			FALSE,                                           // Non-Owned initially
			(LPCWSTR)"EmvKernelVdmSyncMutex"                     // named mutex.
		);
	printf("Creating VDM watcher thread");
	hVdmStatusThread = CreateThread(NULL, 0, VdmStatusChanged, NULL, 0, NULL);
	char* stBuffer1 = "Hello1";
	char* stBuffer2 = "Hello2";
	char* stBuffer3 = "Hello3";
	char* stBuffer4 = "Hello4";
	WriteMessage(stBuffer1, 6);
	WriteMessage(stBuffer2, 6);
	WriteMessage(stBuffer3, 6);
	WriteMessage(stBuffer4, 6);


	// Close our pipe handle
	//CloseHandle(hPipeHandleReadWrite);

	wcout << "Done." << endl;



	system("pause");
	return 0;
}

DWORD Initialize()
{
	DWORD dwRetCode = 0;
	// Skip opening the pipe if we have already established a connection
	if (!bOpened || hPipeHandleReadWrite == INVALID_HANDLE_VALUE)
	{
		printf("\nRead/Write Pipe not already connected");

		int iWaitCount = 0;
		hPipeHandleReadWrite = CreateFile(PIPE_NAME, (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_WRITE | FILE_SHARE_READ), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		while (hPipeHandleReadWrite == INVALID_HANDLE_VALUE)
		{
			// If we are not waiting forever and the pipe connect wait count is exceeded exit the loop
			if (iWaitCount > MAX_WAIT_FOR_PIPE && !WAIT_FOREVER_FOR_PIPE)
			{
				printf("\nFailed to create pipe connection in the given time: %d", GetLastError());
				dwRetCode = 1;
				break;
			}

			// Otherwise sleep a short while and 
			iWaitCount++;
			Sleep(10);

			hPipeHandleReadWrite = CreateFile((LPCWSTR)PIPE_NAME, (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_WRITE | FILE_SHARE_READ), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			printf("Error:%d\n", GetLastError());
		}

		if (hPipeHandleReadWrite != INVALID_HANDLE_VALUE)
		{
			printf("\nPipe cocnnection established successfully");
			char* stBuffer = "Connected";
			dwRetCode = WriteMessage(stBuffer, 9);
			bOpened = true;
		}
		else
		{
			dwRetCode = 1;
		}
	}

	return dwRetCode;
}

DWORD WINAPI VdmStatusChanged(LPVOID)
{
	HANDLE hPipe;
	char buffer[1024];
	DWORD dwRead;


	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\PipeLis"),
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
		1,
		1024 * 16,
		1024 * 16,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);

	while (hPipe != INVALID_HANDLE_VALUE)
	{
		if (ConnectNamedPipe(hPipe, NULL) != FALSE)   // wait for someone to connect to the pipe
		{
			while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
			{
				/* add terminating zero */
				buffer[dwRead] = '\0';

				/* do something with data in buffer */
				printf("Message received from Client %s \n", buffer);
			}
		}

		DisconnectNamedPipe(hPipe);
	}

	return 0;
//	DWORD dwKrnlAPIRet = 1;
//	char stBuffer[128] = { 0x00 };
//	DWORD dwNbrOfBytesRead = 0;
//	BOOL bWriteFileRet = false;
//
//
//	wchar_t buffer[128];
//	DWORD numBytesRead = 0;
//	while (true)
//	{
//		WaitForSingleObject(glohFunctionMutex, INFINITE);
//		printf("Waiting/n");
//		BOOL result = ReadFile(
//			hPipeHandleReadWrite,
//			buffer, // the data from the pipe will be put here
//			127 * sizeof(wchar_t), // number of bytes allocated
//			&numBytesRead, // this will store number of bytes actually read
//			NULL // not using overlapped IO
//		);
//		printf("Released/n");
//		ReleaseMutex(glohFunctionMutex);
//
//		if (result) {
//			buffer[numBytesRead / sizeof(wchar_t)] = '\0'; // null terminate the string
//			cout << "Number of bytes read: " << numBytesRead << endl;
//		}
//
//		//printf( __FUNCTION__);
//
//		//if (hPipeHandleReadWrite != INVALID_HANDLE_VALUE)
//		//{
//		//	WaitForSingleObject(hVdmSemaphore, INFINITE);
//		//	bWriteFileRet = ReadFile(hPipeHandleReadWrite, stBuffer, sizeof(stBuffer), &dwNbrOfBytesRead, NULL);
//		//	ReleaseMutex(hVdmSemaphore);
//
//		//	if (!bWriteFileRet)
//		//	{
//		//		DWORD dwLastError = 0;
//		//		dwLastError = GetLastError();
//
//		//		if (dwLastError != ERROR_IO_PENDING || dwLastError != ERROR_SUCCESS)
//		//		{
//		//			if (dwLastError == ERROR_BROKEN_PIPE)
//		//			{
//		//				dwKrnlAPIRet = 1;
//		//				printf("\nFailed to write to pipe: ERROR_BROKEN_PIPE - EMVConfigurationManager may have terminated");
//		//			}
//		//			else
//		//			{
//		//				printf("\nFailed to write to pipe, LastErrorCode: %d", dwLastError);
//		//			}
//		//		}
//		//	}
//		//	else
//		//	{
//		//		if (strcmp("vdm_active", stBuffer) == 0)
//		//		{
//		//			printf("\nEnter VDM recieved");
//		//			
//		//		}
//		//		else if (strcmp("vdm_not_active", stBuffer) == 0)
//		//		{
//		//			printf("\nExit VDM recieved");
//		//			
//		//		}
//		//		else
//		//		{
//		//			printf("\nInvalid VDM status recieved");
//		//		}
//
//		//		// Clear the buffer
//		//		memset(stBuffer, 0x00, sizeof(stBuffer));
//		//	}
//		//}
//		//else
//		//{
//		//	printf("\nPipe is an INVALID_HANDLE_VALUE, cannot write to pipe.");
//		//	dwKrnlAPIRet = 1;
//		//}
//
//		//printf( __FUNCTION__);
//	}
//
//	return dwKrnlAPIRet;
}

DWORD WriteMessage(char * stMsg, int iMsgLen)
{
	DWORD dwRetCode = 0;
	printf("\nWrite Starts");

	if (hPipeHandleReadWrite != INVALID_HANDLE_VALUE)
	{
		DWORD dwBytes = 0;
		BOOL bdone = false;
		bdone = WriteFile(hPipeHandleReadWrite, stMsg, iMsgLen, &dwBytes, NULL);

		if (bdone == false)
		{
			DWORD dwError = 0;
			dwError = GetLastError();

			if (dwError != ERROR_IO_PENDING || dwError != ERROR_SUCCESS)
			{
				if (dwError == ERROR_BROKEN_PIPE)
				{
					dwRetCode = 1;
					printf("\nFailed to write to pipe: ERROR_BROKEN_PIPE");
				}
				else
				{
					printf("\nFailed to write to pipe, LastErrorCode: %d", dwRetCode);
				}
			}
		}
	}
	else
	{
		printf("\nPipe is an INVALID_HANDLE_VALUE, cannot write to pipe.");
		dwRetCode = 1;
	}

	printf("\nWrite Exit");
	return dwRetCode;
}
