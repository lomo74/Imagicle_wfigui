//---------------------------------------------------------------------------
#include <vcl.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <gnugettext.hpp>
#pragma hdrstop

#include "indybkport.hpp"
#include "HttpClient.h"
#include "Utils.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

static struct __curl_init
{
	__curl_init()
	{
		curl_global_init(CURL_GLOBAL_WIN32);
	}
} __curl_init_instance;
//---------------------------------------------------------------------------

EClientError::EClientError(int nCode)
	: Exception(L"")
{
	TVarRec args[] = {
		nCode,
	};
	Message = Format(_(L"The server responded with a status code of %d"), args, 1);
}
//---------------------------------------------------------------------------

EUnauthorizedError::EUnauthorizedError()
	: Exception(_(L"Bad username/password"))
{
}
//---------------------------------------------------------------------------

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	TMemoryStream *stream = static_cast<TMemoryStream *>(userdata);
	assert(stream);

	stream->Write(ptr, size * nmemb);

	return size * nmemb;
}
//---------------------------------------------------------------------------

UnicodeString GetResponse(CURL *curl, UnicodeString &aContentType, long &aCode)
{
	TMemoryStream *stream = new TMemoryStream();
	char curlerr[CURL_ERROR_SIZE] = { 0 };

	try {
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, stream);

		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerr);

		CURLcode res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			UnicodeString err;
			UnicodeString fmt = _(L"Call to curl_easy_perform failed: code=%i message=%hs");
			err.printf(fmt.c_str(), (int)res, (*curlerr ? curlerr : curl_easy_strerror(res)));
			throw ECurlException(err);
		}

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &aCode);

		char *content_type;
		curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);

		UnicodeString charset;
		ParseContentType(content_type, aContentType, charset);

		if (charset.Length() == 0)
			charset = L"us-ascii";

		UnicodeString data = ConvertToUcs2LE(
			charset,
			static_cast<const char *>(stream->Memory),
			stream->Size
		);

		return data;
	}
	__finally {
		delete stream;
	}
}
//---------------------------------------------------------------------------
