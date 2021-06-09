/*
Imagicle print2fax
Copyright (C) 2021 Lorenzo Monti

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <vcl.h>
#include <windows.h>
#include <wtsapi32.h>
#pragma hdrstop

#include "ipc.h"
#include "log.h"

#pragma link "wtsapi32.lib"

#define BUFSIZE 4096

static LPCWSTR szPipeTemplate = L"\\\\.\\pipe\\wfi_sessid%0.8X";
static HANDLE hThread = NULL;
static HANDLE hStop = NULL;
static HANDLE hPipe = NULL;
static OVERLAPPED ov;
static WCHAR *szFile = NULL;
static DWORD fileSize = 0;
static WCHAR *szTitle = NULL;
static DWORD titleSize = 0;
static BOOL bStarted = FALSE;

typedef struct {
	IPCCALLBACK callback;
	LPVOID param;
} THREADDATA, *LPTHREADDATA;

/*
  Alloca memoria per il buffer di ricezione
*/
static BOOL ReAllocBuf(WCHAR **buf, DWORD dwLen = 0)
{
	//libera la memoria se era già allocata
	if (*buf)
	{
		delete[] *buf;
		*buf = NULL;
	}

	//se dwBytes == 0 abbiamo finito
	if (dwLen == 0)
		return TRUE;

	//tenta di allocare la memoria
	if ((*buf = new WCHAR[dwLen]) == NULL)
		return FALSE;

	return TRUE;
}

/*
  Tenta di leggere un certo quantitativo di bytes entro un certo timeout
*/
static BOOL ReadFromPipe(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, DWORD dwMilliseconds)
{
	DWORD nLeft = nNumberOfBytesToRead;
	DWORD dwMsLeft = 0, dwStart = 0;
	DWORD cbRead = 0;
    DWORD dwErr;
	LPVOID lpCurrent = lpBuffer;
	HANDLE hEvents[2];
	BOOL bWaitingRead = FALSE;

	hEvents[0] = hStop;		// evento 0 = segnale di uscita
	hEvents[1] = ov.hEvent;	// evento 1 = operazione overlapped completata

	//momento di inizio
	dwStart = GetTickCount();

	//cicla finché non sono arrivati tutti i byte che aspettavamo
	while (nLeft > 0)
	{
		//se non c'è una read in attesa di completamento,
		//ne iniziamo una nuova
		if (!bWaitingRead)
		{
			if (!ReadFile(hPipe, lpCurrent, nLeft, NULL, &ov))
			{
				if ((dwErr = GetLastError()) != ERROR_IO_PENDING) {
					LOG_CRITICAL1(L"ReadFromPipe: ReadFile failed (%d)", dwErr);
					return FALSE;
				}
				bWaitingRead = TRUE;
			}
		}

		//se l'operazione è in stato di ERROR_IO_PENDING...
		if (bWaitingRead)
		{
			//quanti millisecondi abbiamo ancora?
			dwMsLeft = (dwMilliseconds == INFINITE)
				? INFINITE
				: dwStart + dwMilliseconds - GetTickCount();

			//test se tempo scaduto
			if (dwMsLeft != INFINITE && dwMsLeft <= 0) {
				LOG_CRITICAL1(L"ReadFromPipe: timeout elapsed (%dms)", dwMilliseconds);
				return FALSE;
			}

			//restiamo in attesa di un evento
			switch (WaitForMultipleObjects(2, hEvents, FALSE, dwMsLeft))
			{
			case WAIT_OBJECT_0 + 0:	// uscita immediata
				LOG_DEBUG0(L"ReadFromPipe: hStop signaled, immediate exit");
				return FALSE;
				//break;
			case WAIT_OBJECT_0 + 1:	// operazione completata
				LOG_DEBUG0(L"ReadFromPipe: operation completed");
				break;
			case WAIT_TIMEOUT:		// timeout, facciamo un altro giro
				continue;
				//break;
			default:				// non dovremmo mai arrivare qui
				return FALSE;
				//break;
			}
		}

		//controlliamo l'esito dell'operazione asincrona
		if (!GetOverlappedResult(hPipe, &ov, &cbRead, FALSE)) {
			DWORD dwErr = GetLastError();
			LOG_CRITICAL1(L"ReadFromPipe: GetOverlappedResult failed (%d)", dwErr);
			return FALSE;
		}

		//decremento numero di bytes rimasti e avanzo il puntatore al buffer
		nLeft -= cbRead;
		lpCurrent = static_cast<char *>(lpCurrent) + cbRead;
	}

	return TRUE;
}

/*
  Una volta connessa la pipe, legge i dati in ingresso
  per prima cosa leggiamo 4 byte (lunghezza della stringa che seguirà)
  poi leggiamo la stringa che rappresenta il nome del file
*/
void IpcChat(IPCCALLBACK pfnCallback, LPVOID param)
{
	DWORD len = 0;
	DWORD pages = 0;

	LOG_DEBUG0(L"IpcChat: reading pages (4 bytes)");
	//leggiamo pages (4 byte) entro 1 secondo
	if (!ReadFromPipe(&pages, sizeof(pages), 1000))
		return;

	/*  FILE  */
	LOG_DEBUG0(L"IpcChat: reading file length (4 bytes)");
	//leggiamo len (4 byte) entro 1 secondo
	if (!ReadFromPipe(&len, sizeof(len), 1000))
		return;

	if (len == 0)
		return;

	//se serve più spazio, allarghiamo il buffer
	//vengono allocati blocchi multipli di 1024 caratteri
	if (!szFile || len > fileSize) {
		fileSize = ((fileSize & -1024) | 1023) + 1;

		if (!ReAllocBuf(&szFile, fileSize)) {
			return;
		}
	}

	//leggiamo il nome del file
	LOG_DEBUG1(L"IpcChat: reading file (%d bytes)", len);
	if (!ReadFromPipe(szFile, len * sizeof(WCHAR), 1000))
		return;

	//terminiamo la stringa
	szFile[len] = L'\0';

	/*  TITLE  */
	//leggiamo len (4 byte) entro 1 secondo
	LOG_DEBUG0(L"IpcChat: reading title length (4 bytes)");
	if (!ReadFromPipe(&len, sizeof(len), 1000))
		return;

	if (len == 0)
		return;

	//se serve più spazio, allarghiamo il buffer
	//vengono allocati blocchi multipli di 1024 caratteri
	if (!szTitle || len > titleSize) {
		titleSize = ((titleSize & -1024) | 1023) + 1;

		if (!ReAllocBuf(&szTitle, titleSize)) {
			return;
		}
	}

	//infine, leggiamo il titolo
	LOG_DEBUG1(L"IpcChat: reading title (%d bytes)", len);
	if (!ReadFromPipe(szTitle, len * sizeof(WCHAR), 1000))
		return;

	//terminiamo la stringa
	szTitle[len] = L'\0';

	//e chiamiamo la callback
	pfnCallback(pages, szFile, szTitle, param);
}

/*
  Entry point del thread di gestione della pipe
*/
static int __fastcall IpcRoutine(LPVOID lpvParam)
{
	DWORD dwRet = 0, dwLe = 0, cbRet = 0;
	BOOL bDone = FALSE;
	HANDLE hEvents[2];
	LPTHREADDATA ptd = (LPTHREADDATA)lpvParam;
	WCHAR szPipeName[32];
	LPWSTR pBuffer = NULL;
	DWORD cbReturned;
	DWORD dwSessionId = 0;

	if (WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION,
	WTSSessionId, &pBuffer, &cbReturned)) {
		dwSessionId = *(reinterpret_cast<DWORD *>(pBuffer));
		WTSFreeMemory(pBuffer);
	} else {
		if ((dwRet = GetLastError()) != ERROR_APP_WRONG_OS) {
			LOG_CRITICAL1(L"IpcRoutine: WTSQuerySessionInformationW failed (%d)", dwRet);
			goto cleanup;
		}
	}

	wsprintfW(szPipeName, szPipeTemplate, dwSessionId);

	//creiamo un evento a reset manuale per la notifica dell'I/O asincrono
	ZeroMemory(&ov, sizeof(ov));
	ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

	if (ov.hEvent == NULL)
	{
		dwRet = GetLastError();
		LOG_CRITICAL1(L"IpcRoutine: CreateEventW failed (%d)", dwRet);
		goto cleanup;
	}

	hEvents[0] = hStop;		// evento 0 = segnale di uscita
	hEvents[1] = ov.hEvent;	// evento 1 = operazione overlapped completata

	//creiamo la pipe
	hPipe = CreateNamedPipeW(
		szPipeName,
		PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
		PIPE_WAIT | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
		1,
		BUFSIZE,
		BUFSIZE,
		0,
		NULL);

	if (hPipe == INVALID_HANDLE_VALUE)
	{
		dwRet = GetLastError();
		LOG_CRITICAL1(L"IpcRoutine: CreateNamedPipeW failed (%d)", dwRet);
		goto cleanup;
	}

	LOG_DEBUG1(L"IpcRoutine: listening on pipe %s", szPipeName);

	//ciclo finché non ricevo il segnale di stop
	while (!bDone)
	{
		//con FILE_FLAG_OVERLAPPED, il ritorno deve essere 0 altrimenti significa errore
		if (ConnectNamedPipe(hPipe, &ov) != 0)
		{
			dwRet = GetLastError();
			LOG_CRITICAL1(L"IpcRoutine: ConnectNamedPipe failed (%d)", dwRet);
			goto cleanup;
		}

		//vediamo cosa sta succedendo
		switch (dwLe = GetLastError())
		{
		case ERROR_PIPE_CONNECTED:	// il client si è connesso tra CreateNamedPipeA e ConnectNamedPipe: connessione buona
			break;
		case ERROR_IO_PENDING:		// l'operazione è in attesa di completamento
			switch (WaitForMultipleObjects(2, hEvents, FALSE, INFINITE))
			{
			case WAIT_OBJECT_0 + 0:	// uscita immediata
				LOG_DEBUG0(L"IpcRoutine: hStop signaled, immediate exit");
				bDone = TRUE;
				continue;
				//break;
			case WAIT_OBJECT_0 + 1:	// segnale di operazione completata
				//la connessione è andata bene?
				if (!GetOverlappedResult(hPipe, &ov, &cbRet, FALSE))
				{
					dwRet = GetLastError();
					LOG_CRITICAL1(L"IpcRoutine: GetOverlappedResult failed (%d)", dwRet);
					goto cleanup;
				}
				//let's chat!!
				IpcChat(ptd->callback, ptd->param);
				//chiudiamo la pipe
				DisconnectNamedPipe(hPipe);
				break;
			default:				// non dovremmo mai arivare qui
				dwRet = GetLastError();
				LOG_CRITICAL1(L"IpcRoutine: unexpected error (%d)", dwRet);
				goto cleanup;
				//break;
			}
			break;
		default:	// si è verificato qualche errore...
			dwRet = dwLe;
			LOG_CRITICAL1(L"IpcRoutine: ConnectNamedPipe failed (%d)", dwRet);
			goto cleanup;
		}
	}

cleanup:
	free(lpvParam);
	if (hPipe)
		CloseHandle(hPipe);
	if (ov.hEvent)
		CloseHandle(ov.hEvent);
	return dwRet;
}

/*
  Avvia il therad di gestione della pipe
*/
int StartIpc(IPCCALLBACK pfnCallback, LPVOID param)
{
	unsigned dwThreadId = 0;
	DWORD dwRet = 0;
	LPTHREADDATA ptd;

	//creiamo l'evento di segnalazione di stop
	hStop = CreateEventW(NULL, FALSE, FALSE, NULL);

	if (hStop == NULL)
	{
		dwRet = GetLastError();
		LOG_CRITICAL1(L"StartIpc: CreateEventW failed (%d)", dwRet);
		goto cleanup;
	}

	//creiamo il thread, passando il puntatore alla funzione callback
	ptd = (LPTHREADDATA)malloc(sizeof(THREADDATA));
	ptd->callback = pfnCallback;
	ptd->param = param;
	hThread = (HANDLE)BeginThread(
		NULL,
		0,
		IpcRoutine,
		(LPVOID)ptd,
		0,
		dwThreadId);

	if (hThread == NULL)
	{
		dwRet = GetLastError();
		LOG_CRITICAL1(L"StartIpc: BeginThread failed (%d)", dwRet);
		goto cleanup;
	}

	bStarted = TRUE;

	LOG_INFO0(L"StartIpc: IPC started");

	//tutto ok
	return 0;

cleanup:
	if (hStop)
		CloseHandle(hStop);
	if (hThread)
		CloseHandle(hThread);
	return dwRet;
}

/*
  Ferma il thread di gestione della pipe
*/
void StopIpc()
{
	if (!bStarted)
		return;

	//segnala stop
	SetEvent(hStop);

	//attende chiusura del thread
	WaitForSingleObject(hThread, INFINITE);

	//chiusura handle e memoria allocata
	CloseHandle(hThread);
	CloseHandle(hStop);
	ReAllocBuf(&szFile);
	ReAllocBuf(&szTitle);

	bStarted = FALSE;

	LOG_INFO0(L"StopIpc: IPC stopped");
}
