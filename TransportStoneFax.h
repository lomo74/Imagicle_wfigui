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

#ifndef TransportStoneFaxH
#define TransportStoneFaxH
//---------------------------------------------------------------------------
#include <vcl.h>
#include <curl/curl.h>
#include <vector>
#include <gnugettext.hpp>
#pragma hdrstop

#include "Transport.h"
#include "Json.h"
//---------------------------------------------------------------------------

class TFax;
//---------------------------------------------------------------------------

class EStoneFaxError : public Exception
{
private:
	int FCode;
	void Deserialize(const JsonValue *val);
public:
	EStoneFaxError(int nCode, const JsonValue *val) : Exception(L""), FCode(nCode) { Deserialize(val); }
};
//---------------------------------------------------------------------------

class TFaxesOutboundPOSTResponse : public IJsonSerializable
{
private:
	TStringList *FIds;
public:
	TFaxesOutboundPOSTResponse() { FIds = new TStringList(); }
	virtual ~TFaxesOutboundPOSTResponse() { delete FIds; }
	virtual void Deserialize(const JsonValue *val);
};
//---------------------------------------------------------------------------

class TTransportStoneFax : public TTransport
{
private:
	std::vector<TFax *> FFaxes;
    DWORD FLastActivity;

private:
	static cdecl size_t readfunc(char *buffer, size_t size, size_t nitems, void *arg);
	static cdecl int seekfunc(void *arg, curl_off_t offset, int origin);
	static cdecl void freefunc(void *arg);

	static cdecl int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
		curl_off_t ultotal, curl_off_t ulnow);

	void Send(TFax *pFax);
	void DeserializeResponse(CURL *curl, IJsonSerializable *response);
	void ResetCurlCountdown();
	bool CurlCountdownExpired();

public:
	TTransportStoneFax();
	virtual ~TTransportStoneFax();

	virtual void Enqueue(TFax *pFax);
	virtual void SendAllAsync();
};
//---------------------------------------------------------------------------
#endif
