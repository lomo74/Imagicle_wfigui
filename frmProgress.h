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

#ifndef frmProgressH
#define frmProgressH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>

#include "Transport.h"
#include <System.ImageList.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.ImgList.hpp>
//---------------------------------------------------------------------------

enum FAX_PHASE { CONVERT, SEND, FINAL };
//---------------------------------------------------------------------------

class TJobData : public TObject
{
public:
	TFax *pFax;
	FAX_PHASE phase;
};
//---------------------------------------------------------------------------

class TFormProgress : public TForm, public ITransportNotify, public ITransportUnauthorized
{
__published:	// IDE-managed Components
	TButton *btnCancelClose;
	TImageList *ilIcons;
	TListView *lvResults;
	TLabel *Label1;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall btnCancelCloseClick(TObject *Sender);
private:	// User declarations
	ITransport *FTransport;
    bool FCancel;
	void __fastcall SetFaxStatus(TFax *pFax, FAX_PHASE aPhase, const UnicodeString &aMessage, int nStatus);
public:		// User declarations
	__fastcall TFormProgress(TComponent *Owner, ITransport *pTransport);
	virtual __fastcall ~TFormProgress() { FTransport->Unsubscribe(this); }
public: // ITransportNotify
	virtual void TransportNotify(TTransportEvent aEvent, TFax *pFax, bool result, Exception *pError);
public: // ITransportUnauthorized
	virtual bool HandleUnauthorized();
};
//---------------------------------------------------------------------------
extern PACKAGE TFormProgress *FormProgress;
//---------------------------------------------------------------------------
#endif
