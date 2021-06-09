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

//---------------------------------------------------------------------------
#include <vcl.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <gnugettext.hpp>
#pragma hdrstop

#include "Json.h"
#include "Fax.h"
#include "log.h"
#include "Utils.h"
#include "HttpClient.h"

#pragma link "libcurl.lib"
#pragma link "crypt32.lib"
#pragma link "normaliz.lib"
#pragma link "zlib.lib"

#include "TransportStoneFax.h"
#include "Processor.h"

#define TIMEOUT 10000

#pragma package(smart_init)
//---------------------------------------------------------------------------

void TFaxesOutboundPOSTResponse::Deserialize(const JsonValue *val)
{
	assert(val);
	const JsonValue *ids = JsonPointer(L"/ids").Get(*val);

	if (ids && ids->IsArray()) {
		JsonValue::ConstValueIterator it;

		for (it = ids->Begin(); it != ids->End(); it++) {
			const JsonValue &id = *it;

			if (id.IsString()) {
				FIds->Add(id.GetString());
			}
		}
	}
}
//---------------------------------------------------------------------------

void EStoneFaxError::Deserialize(const JsonValue *val)
{
	assert(val);
	const JsonValue *reason = JsonPointer(L"/reason").Get(*val);

	if (reason && reason->IsString()) {
		TVarRec args[] = {
			FCode,
			reason->GetString(),
		};
		Message = Format(_(L"The server responded with a status code of %d and provided the following error message: %s"), args, 2);
	}
}
//---------------------------------------------------------------------------

TTransportStoneFax::TTransportStoneFax()
{
}
//---------------------------------------------------------------------------

TTransportStoneFax::~TTransportStoneFax()
{
}
//---------------------------------------------------------------------------

int TTransportStoneFax::progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
	curl_off_t ultotal, curl_off_t ulnow)
{
	TTransportStoneFax *pTransport = static_cast<TTransportStoneFax *>(clientp);

	if (pTransport->Cancelled)
		return 1;

	if (dlnow != 0 || ulnow != 0) {
		pTransport->ResetCurlCountdown();
	} else if (pTransport->CurlCountdownExpired()) {
		LOG_WARNING1(L"libcurl has not been transferring data for %dms, aborting transfer", TIMEOUT);
        return 1;
	}

    return CURL_PROGRESSFUNC_CONTINUE;
}
//---------------------------------------------------------------------------

void TTransportStoneFax::ResetCurlCountdown()
{
	FLastActivity = GetTickCount();
}
//---------------------------------------------------------------------------

bool TTransportStoneFax::CurlCountdownExpired()
{
	DWORD elapsed;
	DWORD now = GetTickCount();

	if (now < FLastActivity)
		elapsed = (0xFFFFFFFF - FLastActivity) + now;
	else
		elapsed = now - FLastActivity;

	return elapsed > TIMEOUT;
}
//---------------------------------------------------------------------------

void TTransportStoneFax::DeserializeResponse(CURL *curl, IJsonSerializable *response)
{
	UnicodeString aContentType, resp;
	long aCode = 0;

	ResetCurlCountdown();
	resp = GetResponse(curl, aContentType, aCode);

	if (aCode == 401) {
		throw EUnauthorizedError();
	}

	if (aContentType.CompareIC(L"application/json") != 0) {
		if (aCode >= 400 && aCode <= 499) {
			throw EClientError(aCode);
		}

		throw ECurlException(_(L"Server response is not JSON"));
	}

	JsonDocument dom;
	dom.Parse(resp.c_str());

	if (dom.HasParseError())
		throw ECurlException(_(L"Error parsing JSON response"));

	if (aCode == 200 || aCode == 201 || aCode == 202) {
		response->Deserialize(&dom);
	} else {
		throw EStoneFaxError(aCode, &dom);
	}
}
//---------------------------------------------------------------------------

size_t TTransportStoneFax::readfunc(char *buffer, size_t size, size_t nitems, void *arg)
{
	TFileStream *pStream = static_cast<TFileStream *>(arg);
	assert(pStream);

	int toRead = size * nitems;
	int nLeft = pStream->Size - pStream->Position;

	if (nLeft < toRead)
		toRead = nLeft;

	int nRead = pStream->Read(buffer, toRead);

	if (nRead < toRead) {
		LOG_CRITICAL3(L"TTransportStoneFax::readfunc: a I/O error occurred while reading file %s (toRead=%d nRead=%d)",
			pStream->FileName, toRead, nRead);

		return CURL_READFUNC_ABORT;
	}

	return nRead;
}
//---------------------------------------------------------------------------

int TTransportStoneFax::seekfunc(void *arg, curl_off_t offset, int origin)
{
	TFileStream *pStream = static_cast<TFileStream *>(arg);
	assert(pStream);

	switch (origin) {
	case SEEK_SET:
		if (pStream->Seek(offset, soBeginning) == offset) {
			return CURL_SEEKFUNC_OK;
		} else {
			LOG_CRITICAL2(L"TTransportStoneFax::seekfunc: a I/O error occurred while seeking file %s (offset=%d)",
				pStream->FileName, offset);

			return CURL_SEEKFUNC_FAIL;
		}
	default:
		LOG_CRITICAL1(L"TTransportStoneFax::seekfunc: origin parameter has an unexpected value (%d)", origin);

		return CURL_SEEKFUNC_FAIL;
	}
}
//---------------------------------------------------------------------------

void TTransportStoneFax::freefunc(void *arg)
{
	TFileStream *pStream = static_cast<TFileStream *>(arg);
	assert(pStream);

	delete pStream;
}
//---------------------------------------------------------------------------

void TTransportStoneFax::Enqueue(TFax *pFax)
{
	FFaxes.push_back(pFax);
}
//---------------------------------------------------------------------------

void TTransportStoneFax::Send(TFax *pFax)
{
	try {
		int i;
		rapidjson::StringBuffer buf;
		JsonASCIIWriter writer(buf);
		JsonDocument dom;

		dom.SetObject();

		dom.AddMember(L"subject", rapidjson::StringRef(pFax->Subject.c_str()), dom.GetAllocator());
		dom.AddMember(L"body", rapidjson::StringRef(pFax->Body.c_str()), dom.GetAllocator());

		JsonValue aRecipients;
		aRecipients.SetArray();

		for (i = 0; i < pFax->RecipientCount; i++) {
			const TRecipient &aRecipient = pFax->Recipients[i];

			JsonValue aRec;
			aRec.SetObject();

			aRec.AddMember(L"firstName", rapidjson::StringRef(aRecipient.FirstName.c_str()), dom.GetAllocator());
			aRec.AddMember(L"lastName", rapidjson::StringRef(aRecipient.LastName.c_str()), dom.GetAllocator());
			aRec.AddMember(L"faxNumber", rapidjson::StringRef(aRecipient.FaxNumber.c_str()), dom.GetAllocator());

			aRecipients.PushBack(aRec, dom.GetAllocator());
		}

		dom.AddMember(L"recipients", aRecipients, dom.GetAllocator());

		if (pFax->NotifyByEmail) {
			dom.AddMember(L"notifyByEmail", true, dom.GetAllocator());
			dom.AddMember(L"notificationEmailAddress", rapidjson::StringRef(pFax->NotificationEmailAddress.c_str()), dom.GetAllocator());
		}

		if (pFax->UseCover) {
			dom.AddMember(L"coverPageName", rapidjson::StringRef(pFax->CoverPageName.c_str()), dom.GetAllocator());
		}

		dom.Accept(writer);

		CURL *curl = NULL;
		curl_mime *form = NULL;
		curl_mimepart *field = NULL;
		struct curl_slist *headerlist = NULL;

		try {
			if ((curl = curl_easy_init()) == NULL)
				throw ECurlException(_(L"Call to curl_easy_init failed"));

			if ((form = curl_mime_init(curl)) == NULL)
				throw ECurlException(_(L"Call to curl_mime_init failed"));

			for (i = 0; i < pFax->FileCount; i++) {
				UnicodeString aFile;
				if (FProcessor) {
					try {
						LOG_INFO1(L"TTransportStoneFax::Send: processing file %s", pFax->Files[i]);
						NotifyAll(teFaxConvertBegin, pFax, true, NULL);
						aFile = FProcessor->ProcessFile(pFax->Files[i]);
						LOG_DONE1(L"TTransportStoneFax::Send: file %s processed successfully", pFax->Files[i]);
						NotifyAll(teFaxConvertEnd, pFax, true, NULL);
					}
					catch (Exception &e) {
						NotifyAll(teFaxConvertEnd, pFax, false, &e);
						throw e;
					}
				} else {
					aFile = pFax->Files[i];
				}

				AnsiString attn = "attachment" + IntToStr(i);

				TFileStream *pStream = new TFileStream(aFile, fmOpenRead | fmShareDenyNone);

				AnsiString aAnsiFile = ExtractFileName(aFile);

				field = curl_mime_addpart(form);
				curl_mime_name(field, attn.c_str());
				curl_mime_data_cb(field, pStream->Size, readfunc, seekfunc, freefunc, pStream);
				curl_mime_filename(field, aAnsiFile.c_str());

				LOG_DEBUG2(L"TTransportStoneFax::Send: adding attachment %s=%s", attn, aAnsiFile);
			}

			field = curl_mime_addpart(form);
			curl_mime_name(field, "jsondata");
			curl_mime_data(field, buf.GetString(), CURL_ZERO_TERMINATED);

			LOG_DEBUG1(L"TTransportStoneFax::Send: setting jsondata=%s", buf.GetString());

			curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

			AnsiString aURL;
			aURL.printf("%s://%ls/fw/Apps/StoneFax/WebAPI/Faxes/Outbound", (FUseSSL ? "https" : "http"), FServer.c_str());
			curl_easy_setopt(curl, CURLOPT_URL, aURL.c_str());

			LOG_INFO1(L"TTransportStoneFax::Send: endpoint is %s", aURL);

			if (FUseSSL && FSkipCertificateCheck) {
				curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
				curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
			}

			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
			curl_easy_setopt(curl, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);

			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
			curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);

			curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);

			LOG_INFO0(L"TTransportStoneFax::Send: sending fax");
			NotifyAll(teFaxSendBegin, pFax, true, NULL);

			TFaxesOutboundPOSTResponse response;

LRetry:
			curl_easy_setopt(curl, CURLOPT_USERNAME, AnsiString(FUsername).c_str());
			curl_easy_setopt(curl, CURLOPT_PASSWORD, AnsiString(FPassword).c_str());
			try {
				DeserializeResponse(curl, &response);
			}
			catch (EUnauthorizedError &e) {
				bool bRetry = false;

				if (FUnauthorizedTarget) {
					TThread::Synchronize(TThread::Current, [&]() {
						try {
							bRetry = FUnauthorizedTarget->HandleUnauthorized();
						}
						catch (...) {
							//TODO: loggare errori?
						}
					});
				}

				if (bRetry) {
					goto LRetry;
				}

				throw;
			}

			LOG_DONE0(L"TTransportStoneFax::Send: fax sent correctly");
			NotifyAll(teFaxSendEnd, pFax, true, NULL);
		}
		__finally {
			if (curl) curl_easy_cleanup(curl);
			if (form) curl_mime_free(form);
			if (FProcessor) FProcessor->Cleanup();
		}
	}
	catch (Exception &e) {
		LOG_ERROR2(L"TTransportStoneFax::Send: %s error: %s", e.ClassName(), e.Message);
		NotifyAll(teFaxSendEnd, pFax, false, &e);
		throw;
	}
}
//---------------------------------------------------------------------------

void TTransportStoneFax::SendAllAsync()
{
	TThread *task = TThread::CreateAnonymousThread([&]() {
		bool result = true;

		NotifyAll(teTransportBegin, NULL, true, NULL);

		try {
			std::vector<TFax *>::iterator it;
			for (it = FFaxes.begin(); it != FFaxes.end(); it++) {
				try {
					Send(*it);
				}
				catch (...) {
					result = false;
				}

				if (Cancelled) {
					LOG_INFO0(L"TTransportStoneFax::SendAllAsync: user abort");
					result = false;
					break;
				}
			}
		}
		__finally {
			NotifyAll(teTransportEnd, NULL, result, NULL);
		}
	});

	task->Start();
}
//---------------------------------------------------------------------------

