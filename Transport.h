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

#ifndef TransportH
#define TransportH
//---------------------------------------------------------------------------
#include <vcl.h>
#include <vector>
#include <algorithm>
#pragma hdrstop
//---------------------------------------------------------------------------

class TFax;
class IProcessor;
//---------------------------------------------------------------------------

enum TTransportType {
	ttStoneFax,
};

enum TTransportEvent {
	teTransportBegin,
	teFaxConvertBegin,
	teFaxConvertEnd,
	teFaxSendBegin,
	teFaxSendEnd,
	teTransportEnd,
};
//---------------------------------------------------------------------------

class ITransportNotify
{
public:
	virtual void TransportNotify(TTransportEvent aEvent, TFax *pFax, bool result, Exception *pError) = 0;
};
//---------------------------------------------------------------------------

class ITransportUnauthorized
{
public:
	virtual bool HandleUnauthorized() = 0;
};
//---------------------------------------------------------------------------

class ITransport
{
public:
	virtual ~ITransport() {}
	virtual void Subscribe(ITransportNotify *pNotify) = 0;
	virtual void Unsubscribe(ITransportNotify *pNotify) = 0;
	virtual void OnUnauthorized(ITransportUnauthorized *pUnauthorized) = 0;
	virtual void SetServer(const UnicodeString &aServer, bool aUseSSL, bool aSkipCertificateCheck) = 0;
	virtual void SetUsername(const UnicodeString &aUsername) = 0;
	virtual void SetPassword(const UnicodeString &aPassword) = 0;
	virtual void SetProcessor(IProcessor *pProcessor) = 0;
	virtual void Enqueue(TFax *pFax) = 0;
	virtual void SendAllAsync() = 0;
	virtual void Cancel() = 0;
};
//---------------------------------------------------------------------------

class TTransport : public ITransport
{
protected:
	IProcessor *FProcessor;
	UnicodeString FServer, FUsername, FPassword;
	bool FUseSSL, FSkipCertificateCheck;
	std::vector<ITransportNotify *> FNotifyTargets;
    ITransportUnauthorized *FUnauthorizedTarget;
	bool FCancelled;
	virtual void NotifyAll(TTransportEvent aEvent, TFax *pFax, bool result, Exception *pError);

public:
	TTransport()
		:
		FCancelled(false),
		FProcessor(NULL),
		FUseSSL(true),
		FSkipCertificateCheck(false),
		FUnauthorizedTarget(NULL)
	{}
	virtual void Subscribe(ITransportNotify *pNotify) { FNotifyTargets.push_back(pNotify); }
	virtual void Unsubscribe(ITransportNotify *pNotify) { FNotifyTargets.erase(std::remove(FNotifyTargets.begin(), FNotifyTargets.end(), pNotify), FNotifyTargets.end()); }
	virtual void OnUnauthorized(ITransportUnauthorized *pUnauthorized) { FUnauthorizedTarget = pUnauthorized; }
	virtual void SetServer(const UnicodeString &aServer, bool aUseSSL, bool aSkipCertificateCheck) { FServer = aServer; FUseSSL = aUseSSL; FSkipCertificateCheck = aSkipCertificateCheck; }
	virtual void SetUsername(const UnicodeString &aUsername) { FUsername = aUsername; }
	virtual void SetPassword(const UnicodeString &aPassword) { FPassword = aPassword; }
	virtual void SetProcessor(IProcessor *pProcessor) { FProcessor = pProcessor; }
	virtual void Cancel() { FCancelled = true; }
    __property bool Cancelled = { read = FCancelled, write = FCancelled };
};
//---------------------------------------------------------------------------
#endif
