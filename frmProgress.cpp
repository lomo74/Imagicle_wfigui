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
#pragma hdrstop

#include "frmProgress.h"
#include "Transport.h"
#include "Fax.h"
#include "Config.h"
#include <gnugettext.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormProgress *FormProgress;
//---------------------------------------------------------------------------

#define STATUS_INPROGRESS 0
#define STATUS_SUCCEEDED 1
#define STATUS_FAILED 2
//---------------------------------------------------------------------------

__fastcall TFormProgress::TFormProgress(TComponent *Owner, ITransport *pTransport)
	: TForm(Owner),
	FTransport(pTransport),
    FCancel(true)
{
	FTransport->Subscribe(this);
    FTransport->OnUnauthorized(this);
}
//---------------------------------------------------------------------------

void TFormProgress::TransportNotify(TTransportEvent aEvent, TFax *pFax, bool result, Exception *pError)
{
	TListItem *pItem = NULL;

	switch (aEvent) {
	case teTransportBegin:
		break;

	case teFaxConvertBegin:
		SetFaxStatus(pFax, CONVERT, _(L"Converting file to PDF..."), STATUS_INPROGRESS);
		break;

	case teFaxConvertEnd:
		if (result) {
			SetFaxStatus(pFax, CONVERT, _(L"File conversion succeeded"), STATUS_SUCCEEDED);
		} else {
			if (pError) {
				SetFaxStatus(pFax, CONVERT, pError->Message, STATUS_FAILED);
			} else {
				SetFaxStatus(pFax, CONVERT, _(L"An unknown error has occurred. File has not been converted"), STATUS_FAILED);
			}
        }
		break;

	case teFaxSendBegin:
		SetFaxStatus(pFax, SEND, _(L"Sending fax..."), STATUS_INPROGRESS);
		break;

	case teFaxSendEnd:
		if (result) {
			SetFaxStatus(pFax, SEND, _(L"Fax correctly sent"), STATUS_SUCCEEDED);
		} else {
			if (pError) {
				SetFaxStatus(pFax, SEND, pError->Message, STATUS_FAILED);
			} else {
				SetFaxStatus(pFax, SEND, _(L"An unknown error has occurred. Fax has not been sent"), STATUS_FAILED);
			}
		}
		break;

	case teTransportEnd:
		FCancel = false;
		btnCancelClose->Caption = _(L"Close");
		if (result) {
			SetFaxStatus(NULL, FINAL, _(L"Job completed successfully"), STATUS_SUCCEEDED);
		} else {
			SetFaxStatus(NULL, FINAL, _(L"Job completed with errors"), STATUS_FAILED);
		}
		break;

	default:
		break;
	}
}
//---------------------------------------------------------------------------

void TFormProgress::SetFaxStatus(TFax *pFax, FAX_PHASE aPhase, const UnicodeString &aMessage, int nStatus)
{
	TListItem *pItem = NULL;

	if (pFax) {
		for (int i = 0; i < lvResults->Items->Count; i++) {
			TListItem *pCurrent = lvResults->Items->Item[i];

			TJobData *pData = static_cast<TJobData *>(pCurrent->Data);

			if (pData && pData->pFax == pFax && pData->phase == aPhase) {
				pItem = pCurrent;
				break;
			}
		}
	}

	if (pItem == NULL) {
		pItem = lvResults->Items->Add();

		TJobData *pData = new TJobData();
		pData->pFax = pFax;
		pData->phase = aPhase;

		pItem->Data = pData;

		if (pFax) {
			pItem->SubItems->Add(pFax->RecipientsAsString);
			pItem->SubItems->Add(pFax->Subject);
		} else {
			pItem->SubItems->Add(L"");
			pItem->SubItems->Add(L"");
		}

		pItem->SubItems->Add(aMessage);
	} else {
		pItem->SubItems->Strings[2] = aMessage;
	}

	pItem->StateIndex = nStatus;
}
//---------------------------------------------------------------------------

void __fastcall TFormProgress::FormCreate(TObject *Sender)
{
	TranslateComponent(this, L"wfigui");
	FTransport->SendAllAsync();
}
//---------------------------------------------------------------------------

void __fastcall TFormProgress::FormDestroy(TObject *Sender)
{
	FTransport->Unsubscribe(this);

	for (int i = 0; i < lvResults->Items->Count; i++) {
		TListItem *pCurrent = lvResults->Items->Item[i];

		TJobData *pData = static_cast<TJobData *>(pCurrent->Data);

		if (pData)
			delete pData;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormProgress::btnCancelCloseClick(TObject *Sender)
{
	if (FCancel) {
		FTransport->Cancel();
	} else {
		Close();
	}
}
//---------------------------------------------------------------------------

bool TFormProgress::HandleUnauthorized()
{
	TConfig *Config = TConfig::GetInstance();

	MessageDlg(_(L"Bad username/password. Please enter correct values in the configuration dialog."),
		mtError, TMsgDlgButtons() << mbOK, 0);

	if (Config->Configure()) {
		FTransport->SetUsername(Config->Username);
		FTransport->SetPassword(Config->Password);

		return true;
	}

    return false;
}
//---------------------------------------------------------------------------

