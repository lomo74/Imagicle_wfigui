//---------------------------------------------------------------------------

#ifndef HttpClientH
#define HttpClientH

#include <vcl.h>
#include <curl/curl.h>
#include <curl/easy.h>
#pragma hdrstop
//---------------------------------------------------------------------------

class EUnauthorizedError : public Exception
{
public:
	EUnauthorizedError();
};
//---------------------------------------------------------------------------

class EClientError : public Exception
{
public:
	EClientError(int nCode);
};
//---------------------------------------------------------------------------

class ECurlException : public Exception
{
public:
	ECurlException(const UnicodeString Msg) : Exception(Msg) {}
};
//---------------------------------------------------------------------------

UnicodeString GetResponse(CURL *curl, UnicodeString &aContentType, long &aCode);
//---------------------------------------------------------------------------

#endif
