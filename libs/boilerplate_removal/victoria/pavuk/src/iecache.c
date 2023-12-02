/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#if defined(__CYGWIN__) || defined(WIN32) || defined(__WIN32)

#include "tools.h"

typedef struct _CACHE_ENTRY_INFO
{
  DWORD dwStructSize;
  LPSTR lpszSourceUrlName;
  LPSTR lpszLocalFileName;
  DWORD CacheEntryType;
  DWORD dwUseCount;
  DWORD dwHitRate;
  DWORD dwSizeLow;
  DWORD dwSizeHigh;
  FILETIME LastModifiedTime;
  FILETIME ExpireTime;
  FILETIME LastAccessTime;
  FILETIME LastSyncTime;
  LPBYTE lpHeaderInfo;
  DWORD dwHeaderInfoSize;
  LPSTR lpszFileExtension;
  union
  {
    DWORD dwReserved;
    DWORD dwExemptDelta;
  } R;
} INTERNET_CACHE_ENTRY_INFO, *LPINTERNET_CACHE_ENTRY_INFO;

char *ie_cache_find_localname(char *urlstr)
{
  LPINTERNET_CACHE_ENTRY_INFO info;
  static bool_t exist = TRUE;
  static HMODULE hModule = NULL;
  static BOOL(WINAPI * RetrieveUrlCacheEntryFile) (LPCSTR, LPINTERNET_CACHE_ENTRY_INFO, LPDWORD, DWORD);
  static char *buf = NULL;
  static int buf_size = 0;
  DWORD s = 0;
  DWORD err;
  char *rv = NULL;

  if(!hModule && exist)
  {
    hModule = LoadLibrary("wininet.dll");

    if(!hModule)
    {
      exist = FALSE;
      return NULL;
    }

    RetrieveUrlCacheEntryFile =
      (BOOL(WINAPI *) (LPCSTR, LPINTERNET_CACHE_ENTRY_INFO, LPDWORD, DWORD)) GetProcAddress(hModule, "RetrieveUrlCacheEntryFileA");
    if(!RetrieveUrlCacheEntryFile)
    {
      exist = FALSE;
      FreeLibrary(hModule);
      return NULL;
    }
  }

  if(!buf)
  {
    buf_size = TL_MAX(PATH_MAX, 256);
    buf = _malloc(buf_size);
  }
  memset(buf, '\0', s);

  do
  {
    s = buf_size;

    info = (void *) buf;
    if(RetrieveUrlCacheEntryFile(urlstr, info, &s, 0) && info)
    {
      rv = cvt_win32_to_unix_path(info->lpszLocalFileName, FALSE);
      break;
    }
    else
    {
      err = GetLastError();
      if(err == ERROR_INSUFFICIENT_BUFFER)
      {
        buf_size = s;
        buf = _realloc(buf, buf_size);
      }
    }

  }
  while(err == ERROR_INSUFFICIENT_BUFFER);

  return rv;
}

#endif
