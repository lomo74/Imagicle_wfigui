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

#ifndef frmMainH
#define frmMainH

#include <ActnList.hpp>
#include <Buttons.hpp>
#include <CheckLst.hpp>
#include <Classes.hpp>
#include <Controls.hpp>
#include <ExtCtrls.hpp>
#include <jpeg.hpp>
#include <StdCtrls.hpp>
#include "JvAppInst.hpp"
#include <Dialogs.hpp>
#include <Menus.hpp>
#include <ComCtrls.hpp>
#include <System.Actions.hpp>
#include <Vcl.Samples.Spin.hpp>

#include "Transport.h"
#include "Config.h"
//---------------------------------------------------------------------------

extern int __cdecl IpcCallback(DWORD nPages, LPCSTR szFile, LPVOID param);

#define WM_IMMEDIATESEND WM_USER + 2001
//---------------------------------------------------------------------------

class TFax;

class TFileData : public TObject
{
public:
	UnicodeString FileName;
	UnicodeString FaxNumber;
	bool HasNumber;
	DWORD Pages;
	bool FromSpooler;
	TFax *Fax;
};
//---------------------------------------------------------------------------

class TMainForm : public TForm, public ITransportNotify
{
__published:	// IDE-managed Components
	TPanel *Panel1;
	TImage *Image1;
	TPanel *Panel2;
	TPanel *Panel3;
	TButton *Cancel;
	TButton *Send;
	TLabel *Label4;
	TButton *Configure;
	TActionList *actions;
	TAction *actSend;
	TAction *actUp;
	TAction *actDown;
	TAction *actSave;
	TAction *actDelete;
	TAction *actConfigure;
	TAction *actClose;
	TLabel *lblVersion;
	TJvAppInstances *AppInst;
	TPopupMenu *popup;
	TMenuItem *mnuAddfile;
	TOpenDialog *opendlg;
	TAction *actSelect;
	TMenuItem *mnuSelectAll;
	TMenuItem *mnuSelectNone;
	TPageControl *PageControl1;
	TTabSheet *tsFax;
	TRadioButton *rbSendTogether;
	TRadioButton *rbSendIndividually;
	TLabel *Label1;
	TEdit *edtFAXNumber;
	TComboBox *cbABNames;
	TLabel *Label3;
	TEdit *edtNotificationEmail;
	TButton *btnDelete;
	TButton *btnSave;
	TButton *btnSelect;
	TPanel *Panel4;
	TLabel *Label5;
	TCheckListBox *lbDocuments;
	TBitBtn *btnUp;
	TBitBtn *btnDown;
	TLabel *Label7;
	TTabSheet *tsFaxcover;
	TCheckBox *chkFaxcover;
	TEdit *edtSubject;
	TStaticText *StaticText1;
	TMemo *memComments;
	TStaticText *StaticText2;
	TStaticText *StaticText3;
	TEdit *edtRecipientName;
	TLinkLabel *lblLink;
	TButton *Button1;
	TAction *actHelp;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall cbABNamesChange(TObject *Sender);
	void __fastcall actionsUpdate(TBasicAction *Action, bool &Handled);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall actUpExecute(TObject *Sender);
	void __fastcall actDownExecute(TObject *Sender);
	void __fastcall actSaveExecute(TObject *Sender);
	void __fastcall actDeleteExecute(TObject *Sender);
	void __fastcall actConfigureExecute(TObject *Sender);
	void __fastcall actCloseExecute(TObject *Sender);
	void __fastcall FormActivate(TObject *Sender);
	void __fastcall FormConstrainedResize(TObject *Sender, int &MinWidth, int &MinHeight,
  		int &MaxWidth, int &MaxHeight);
	void __fastcall AppInstInstanceCreated(TObject *Sender, DWORD ProcessId);
	void __fastcall AppInstCmdLineReceived(TObject *Sender, TStrings *CmdLine);
	void __fastcall lbDocumentsMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
  		int X, int Y);
	void __fastcall lbDocumentsDragOver(TObject *Sender, TObject *Source, int X, int Y,
  		TDragState State, bool &Accept);
	void __fastcall lbDocumentsDragDrop(TObject *Sender, TObject *Source, int X, int Y);
	void __fastcall mnuAddfileClick(TObject *Sender);
	void __fastcall actSelectExecute(TObject *Sender);
	void __fastcall mnuSelectAllClick(TObject *Sender);
	void __fastcall mnuSelectNoneClick(TObject *Sender);
	void __fastcall edtFAXNumberChange(TObject *Sender);
	void __fastcall lbDocumentsClick(TObject *Sender);
	void __fastcall lbDocumentsDrawItem(TWinControl *Control, int Index, TRect &Rect,
  		TOwnerDrawState State);
	void __fastcall FormResize(TObject *Sender);
	void __fastcall rbSendModeClick(TObject *Sender);
	void __fastcall actSendExecute(TObject *Sender);
	void __fastcall lblLinkLinkClick(TObject *Sender, const UnicodeString Link, TSysLinkType LinkType);
	void __fastcall actHelpExecute(TObject *Sender);


private:	// User declarations
	CRITICAL_SECTION CSUserInterface;
	UnicodeString SelectFromAB;
	TPoint DragPoint;
	UnicodeString FFileBeingSent;
	bool FSending, FSettingNumberEdit, FSuccess;
	bool FHasNumber, FHasManyNumbers;
	bool FImmediateSend, FAutoClose;
	void __fastcall AddressBookChanged(TObject *Sender);
	void __fastcall AddressBookDuplicate(TObject *Sender, const UnicodeString &Name,
		bool &ChangeNumber);
	void __fastcall AddrBookTypeChanged(TObject *Sender, TWFIAddressBookType AType);
	void __fastcall AddrBookLocationChanged(TObject *Sender);
	void __fastcall LanguageChanged(TObject *Sender);
	void __fastcall LogLevelChanged(TObject *Sender, int LogLevel);
	void BringFaxWndToFront();
	void EnableFields(bool Enable);
	int GetNumbersCount(const UnicodeString &Numbers);
	bool FindMatches(const UnicodeString &Line, UnicodeString &Numbers);
	__inline void LockUI() { EnterCriticalSection(&CSUserInterface); }
	__inline void UnlockUI() { LeaveCriticalSection(&CSUserInterface); }
	void ParseCommandLine(TStrings *pArgs);
	void AddFileToList(const UnicodeString &Title,
		const UnicodeString &FileName, const UnicodeString &Number, DWORD Pages,
		bool FromSpooler = false);
	void AddFileToList(const UnicodeString &Title,
		const UnicodeString &FileName, DWORD Pages, bool FromSpooler = false);
	void MoveWindowAway();
	void MoveWindowToCorner();
	MESSAGE void __fastcall HandleWMAddFax(TMessage &Message);
	MESSAGE void __fastcall HandleWMImmediateSend(TMessage &Message);
	MESSAGE void __fastcall HandleWMDropFiles(TWMDropFiles &Message);
	BEGIN_MESSAGE_MAP
	MESSAGE_HANDLER(WM_IMMEDIATESEND, TMessage, HandleWMImmediateSend);
	MESSAGE_HANDLER(WM_DROPFILES, TWMDropFiles, HandleWMDropFiles);
	END_MESSAGE_MAP(TForm);

public:		// User declarations
	__fastcall TMainForm(TComponent *Owner);
	virtual __fastcall ~TMainForm();
	void __fastcall AddFaxFromIPC(const UnicodeString &szTitle,
		const UnicodeString &szFileName, DWORD nPages);

public: // ITransportNotify
	virtual void TransportNotify(TTransportEvent aEvent, TFax *pFax, bool result, Exception *pError);
};
//---------------------------------------------------------------------------
extern PACKAGE TMainForm *MainForm;
//---------------------------------------------------------------------------
#endif
