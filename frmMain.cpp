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
#include <JclDateTime.hpp>
#include <DateUtils.hpp>
#include <StrUtils.hpp>
#include <shlobj.h>
#include <gnugettext.hpp>
#include <stdio.h>
#include <Variants.hpp>
#include <Quick.Logger.Provider.Files.hpp>
#include <ShellApi.h>
#pragma hdrstop

#include "frmMain.h"
#include "frmRecipient.h"
#include "frmConfirm.h"
#include "Config.h"
#include "AddrBookCSV.h"
#include "AddrBookMAPI.h"
#include "AddrBookODBC.h"
#include "frmSelect.h"
#include "Utils.h"
#include "ipc.h"
#include "TransportStoneFax.h"
#include "ProcessorStoneFax.h"
#include "Fax.h"
#include "frmProgress.h"
#include "log.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "JvAppInst"
#pragma resource "*.dfm"

TMainForm *MainForm;
//---------------------------------------------------------------------------

int __cdecl IpcCallback(DWORD nPages, WCHAR *szFile, WCHAR *szTitle, LPVOID param)
{
	//data arrived through pipe
	TMainForm *form = static_cast<TMainForm *>(param);

	UnicodeString file = (LPCWSTR)szFile;
	UnicodeString title = (LPCWSTR)szTitle;

	LOG_DEBUG3(L"IpcCallback: data read from pipe: pages=%d file=%s title=%s", nPages, file, title);

	TThread::Synchronize(TThread::Current, [&]() {
		form->AddFaxFromIPC(title, file, nPages);
	});

	return 0;
}
//---------------------------------------------------------------------------

__fastcall TMainForm::TMainForm(TComponent *Owner)
	: TForm(Owner),
	FHasNumber(false),
	FHasManyNumbers(false),
	FImmediateSend(false),
	FAutoClose(false),
	FSuccess(false)
{
	//use this often, so translate only once
	SelectFromAB = _(L"Select from address book");

	//synchronize access to the documents list box
	InitializeCriticalSection(&CSUserInterface);

	//first translate the form
	LOG_DEBUG0(L"TMainForm::TMainForm: translating form");
	TP_Ignore(this, L"lblVersion.Caption"); // version label excluded
	TranslateComponent(this, L"wfigui");

	//create config ini
	TConfig *Config = TConfig::GetInstance();
	//attach event handlers
	Config->OnAddrBookTypeChanged = AddrBookTypeChanged;
	Config->OnAddrBookLocationChanged = AddrBookLocationChanged;
	Config->OnLanguageChanged = LanguageChanged;
	Config->OnLogLevelChanged = LogLevelChanged;

	//load config ini
	Config->Load();

	//disable WER so Ghostscript can eventually crash silently
	typedef UINT (WINAPI *PFNGETERRORMODE)(void);
	PFNGETERRORMODE pfnGetErrorMode = NULL;
	HMODULE hKernel = NULL;
	UINT oldMode = 0;

	hKernel = LoadLibraryW(L"KERNEL32.DLL");
	if (hKernel) {
		pfnGetErrorMode = (PFNGETERRORMODE)GetProcAddress(hKernel, "GetErrorMode");

		if (pfnGetErrorMode) {
			oldMode = pfnGetErrorMode();
		}

		FreeLibrary(hKernel);
	}

	SetErrorMode(oldMode | SEM_NOGPFAULTERRORBOX);
}
//---------------------------------------------------------------------------

__fastcall TMainForm::~TMainForm()
{
	//stop pipe
	StopIpc();

	//cleanup
	DeleteCriticalSection(&CSUserInterface);

	for (int i = 0; i < lbDocuments->Items->Count; i++) {
		TFileData *file = static_cast<TFileData *>(lbDocuments->Items->Objects[i]);

		//delete file only if it belongs to us
		if (file->FromSpooler) {
			LOG_INFO1(L"TMainForm::~TMainForm: deleting file %s", file->FileName);

			DeleteFile(file->FileName);
		}

		delete file;
	}

	TConfig::Destroy();

	if (AddressBook)
		delete AddressBook;
}
//---------------------------------------------------------------------------

void TMainForm::ParseCommandLine(TStrings *pArgs)
{
	int i;
	bool bToSaw = false;
	bool bAddRcptSaw = false;
	bool bRcptSaw = false;
	bool bNotifySaw = false;
	bool bMMSaw = false;
	bool bPagesSaw = false;
	bool bFromSpooler = false;
	bool bTitleSaw = false;
	UnicodeString filename, arg, rcpt, title;
	DWORD pages = 0;

	LOG_DEBUG1(L"TMainForm::ParseCommandLine: arguments=%s", pArgs->DelimitedText);

	LockUI();
	try {
		for (i = 0; i < pArgs->Count; i++) {
			arg = pArgs->Strings[i];

			if (!bMMSaw && SameText(arg, L"--"))
				bMMSaw = true;
			else if (bToSaw) {
				rbSendTogether->Checked = true;
				if (Trim(edtFAXNumber->Text).Length() > 0)
					edtFAXNumber->Text = edtFAXNumber->Text + L";";
				edtFAXNumber->Text = edtFAXNumber->Text + arg;
				bToSaw = false;
			} else if (bAddRcptSaw) {
				rbSendTogether->Checked = true;
				if (Trim(edtFAXNumber->Text).Length() > 0)
					edtFAXNumber->Text = edtFAXNumber->Text + L";";
				edtFAXNumber->Text = edtFAXNumber->Text + arg;
				bAddRcptSaw = false;
			} else if (bRcptSaw) {
				rbSendIndividually->Checked = true;
				rcpt = arg;
				bRcptSaw = false;
			} else if (bNotifySaw) {
				edtNotificationEmail->Text = arg;
				bNotifySaw = false;
			} else if (bPagesSaw) {
				pages = System::Sysutils::StrToUIntDef(arg, 0);
				bPagesSaw = false;
			} else if (bTitleSaw) {
				title.SetLength(arg.Length() / 4);
				HexToBin(arg.c_str(), title.c_str(), title.Length() * sizeof(WideChar));
				bTitleSaw = false;
			} else if (!bMMSaw && SameText(arg, L"-to"))
				bToSaw = true;
			else if (!bMMSaw && SameText(arg, L"-addrcpt"))
				bAddRcptSaw = true;
			else if (!bMMSaw && SameText(arg, L"-rcpt"))
				bRcptSaw = true;
			else if (!bMMSaw && SameText(arg, L"-notify"))
				bNotifySaw = true;
			else if (!bMMSaw && SameText(arg, L"-send"))
				FImmediateSend = true;
			else if (!bMMSaw && SameText(arg, L"-autoclose"))
				FAutoClose = true;
			else if (!bMMSaw && SameText(arg, L"-fromspooler"))
				bFromSpooler = true;
			else if (!bMMSaw && SameText(arg, L"-pages"))
				bPagesSaw = true;
			else if (!bMMSaw && SameText(arg, L"-title"))
				bTitleSaw = true;
			else if (Sysutils::FileExists(filename = ExpandUNCFileName(arg))) {
				AddFileToList(title, filename, rcpt, pages, bFromSpooler);
				rcpt = L"";
			}
		}
	}
	__finally {
		UnlockUI();
	}
}
//---------------------------------------------------------------------------

void TMainForm::AddFileToList(const UnicodeString &Title,
	const UnicodeString &FileName, DWORD Pages, bool FromSpooler)
{
	AddFileToList(Title, FileName, L"", Pages, FromSpooler);
}
//---------------------------------------------------------------------------

void TMainForm::AddFileToList(const UnicodeString &Title,
	const UnicodeString &FileName, const UnicodeString &Number, DWORD Pages,
	bool FromSpooler)
{
	TFileData *data = new TFileData();
	data->FileName = FileName;
	data->Pages = Pages;
	data->FromSpooler = FromSpooler;

	LockUI();
	try {
		UnicodeString tempnum = Number;

		if (tempnum.Length() > 0) {
			data->FaxNumber = tempnum;
			data->HasNumber = GetNumbersCount(tempnum) > 0;

			if (rbSendTogether->Checked) {
				UnicodeString curnum = edtFAXNumber->Text.Trim();
				if (curnum.Length() > 0 &&
				curnum.CompareIC(tempnum) != 0) {
					//current number is different from that of previously added
					//documents so change send mode to "individually"
					rbSendIndividually->Checked = true;
				} else {
					//same number, or first submitted document
					FSettingNumberEdit = true;
					edtFAXNumber->Text = tempnum;
					FSettingNumberEdit = false;
				}
			}
		} else
			data->HasNumber = false;

		lbDocuments->Items->BeginUpdate();
		try {
			lbDocuments->AddItem(Title, data);
			lbDocuments->Checked[lbDocuments->Items->Count - 1] = true;
		}
		__finally {
			lbDocuments->Items->EndUpdate();
		}
	}
	__finally
	{
		UnlockUI();
	}
}
//---------------------------------------------------------------------------

void TMainForm::BringFaxWndToFront()
{
	//restore window if it was minimized
	if (this->WindowState == wsMinimized)
		this->WindowState = wsNormal;
	Application->BringToFront();
	//trick to force window to appear
	//we bring it topmost...
	SetWindowPos(this->Handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	//...then we restore its non-topmost state; the window keeps its Z-order
	SetWindowPos(this->Handle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::AddFaxFromIPC(const UnicodeString &szTitle,
	const UnicodeString &szFileName, DWORD nPages)
{
	UnicodeString file = ExpandUNCFileName(szFileName);

	//check parameter sanity
	if (!Sysutils::FileExists(file)) {
		LOG_ERROR1(L"TMainForm::AddFaxFromIPC: file %s does not exist", file);
		return;
	}

	//ok add file to list of files to send
	AddFileToList(szTitle, file, nPages, true);

	BringFaxWndToFront();
}
//---------------------------------------------------------------------------

MESSAGE void __fastcall TMainForm::HandleWMImmediateSend(TMessage &Message)
{
	actSendExecute(NULL);
}
//---------------------------------------------------------------------------

MESSAGE void __fastcall TMainForm::HandleWMDropFiles(TWMDropFiles &Message)
{
	const DWORD dwChars = MAX_PATH + 1;
	wchar_t fileName[dwChars];
	HDROP hdrop = reinterpret_cast<HDROP>(Message.Drop);
	UINT nFiles = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);

	for (UINT n = 0; n < nFiles; n++) {
		if (DragQueryFile(hdrop, n, fileName, dwChars) == dwChars) {
			LOG_ERROR0(L"TMainForm::HandleWMDropFiles: a file dropped on the form has too long name");
			continue; //filename too long, MAX_PATH reached
		}

		AddFileToList(fileName, fileName, 0);
	}

	//release memory
	DragFinish(reinterpret_cast<HDROP>(Message.Drop));
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::AddrBookTypeChanged(TObject *Sender, TWFIAddressBookType AType)
{
	//already have one? delete it
	if (AddressBook) {
		AddressBook->Clear();
		delete AddressBook;
	}

	UnicodeString aTypeName;

	//create new address book
	switch (AType) {
	case abCSV:
		aTypeName = L"CSV";
		AddressBook = new TAddressBookCSV();
		break;
	case abMAPI:
		aTypeName = L"MAPI";
		AddressBook = new TAddressBookMAPI();
		break;
	case abODBC:
		aTypeName = L"ODBC";
		AddressBook = new TAddressBookODBC();
		break;
	default:
		aTypeName = L"none";
		AddressBook = NULL;
	}

	LOG_INFO1(L"TMainForm::AddrBookTypeChanged: address book type has changed to %s", aTypeName);

	if (AddressBook) {
		//attach event handlers
		AddressBook->OnAddressBookChanged = AddressBookChanged;
		AddressBook->OnAddressBookDuplicate = AddressBookDuplicate;
		//load address book
		try {
			AddressBook->Load();
		}
		catch (EAddressBookException &E) {
			Application->ShowException(&E);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::AddrBookLocationChanged(TObject *Sender)
{
	if (AddressBook) {
		LOG_INFO0(L"TMainForm::AddrBookLocationChanged: address book location has changed");

		AddressBook->Clear();
		//reload address book from new location
		try {
			AddressBook->Load();
		}
		catch (EAddressBookException &E) {
			Application->ShowException(&E);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::AddressBookChanged(TObject *Sender)
{
	//reload combo
	cbABNames->Items->Assign(AddressBook);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::AddressBookDuplicate(TObject *Sender, const UnicodeString &Name,
	bool &ChangeNumber)
{
	//ask user what to do with duplicate name
	UnicodeString Msg = Name +
		_(L" is already present in the address book.\nDo you want to modify the associated number?");
	ChangeNumber = MessageDlg(Msg, mtConfirmation,
	TMsgDlgButtons() << mbYes << mbNo, 0, mbNo) == mrYes;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::LanguageChanged(TObject *Sender)
{
	//reload
	SelectFromAB = _(L"Select from address book");

	//translate again
	RetranslateComponent(this, L"wfigui");
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::LogLevelChanged(TObject *Sender, int LogLevel)
{
	switch (LogLevel) {
	case LOGLEVEL_ERRORS:
		GlobalLogFileProvider->LogLevel = LOG_ONLYERRORS;
		GlobalLogFileProvider->Enabled = true;
		break;
	case LOGLEVEL_DEBUG:
		GlobalLogFileProvider->LogLevel = LOG_DEBUG;
		GlobalLogFileProvider->Enabled = true;
		break;
	default:
		GlobalLogFileProvider->Enabled = false;
		break;
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FormCreate(TObject *Sender)
{
	lblVersion->Caption = _("rel. ") + GetVersionDescription(NULL);

	int i, n;

	TConfig *Config = TConfig::GetInstance();

	edtNotificationEmail->Text = Config->NotificationEmail;
	chkFaxcover->Checked = true;

	//got parameters from the command line?
	if ((n = ParamCount()) > 0) {
		TStringList *pArgs = new TStringList();
		try {
			for (i = 1; i <= n; i++) {
				pArgs->Add(ParamStr(i));
			}

			ParseCommandLine(pArgs);
		}
		__finally {
			delete pArgs;
		}
	}

	//activate single instance only if this is not a "send and forget" process
	AppInst->Active = !FAutoClose;

	//only do window placement if we haven't successfully sent something yet
	if (!FSuccess) {
		if (FImmediateSend)
			PostMessage(Handle, WM_IMMEDIATESEND, 0, 0);

		if (!FAutoClose) {
			//put window on bottom-right of the screen
			MoveWindowToCorner();

			//start pipe
			StartIpc(IpcCallback, this);

			BringFaxWndToFront();
		} else {
#ifndef _DEBUG
			//trick to avoid the window blink
			//if autoclose is true, we put the window outside our desktop area
			MoveWindowAway();
#endif
		}
	}

	DragAcceptFiles(Handle, TRUE);
}
//---------------------------------------------------------------------------

void TMainForm::MoveWindowAway()
{
	RECT rect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

	Left = rect.right + 100;
	Top = rect.bottom + 100;
}
//---------------------------------------------------------------------------

void TMainForm::MoveWindowToCorner()
{
	RECT rect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

	Left = rect.right - Width - 8;
	Top = rect.bottom - Height - 8;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::cbABNamesChange(TObject *Sender)
{
	if (AddressBook) {
		edtFAXNumber->Text = AddressBook->Numbers[cbABNames->ItemIndex];
    }
}
//---------------------------------------------------------------------------

void TMainForm::EnableFields(bool Enable)
{
	lbDocuments->Enabled = Enable;
	edtFAXNumber->Enabled = Enable;
	cbABNames->Enabled = Enable;
	edtNotificationEmail->Enabled = Enable;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::actionsUpdate(TBasicAction *Action, bool &Handled)
{
	bool hasDoc = false;

	//do we have at least a checked document in the list?
	for (int i = 0; i < lbDocuments->Items->Count; i++)
		if (lbDocuments->Checked[i]) {
			hasDoc = true;
			break;
		}

	//if we're sending individually, checked documents have associated numbers?
	bool numbersOk = true;

	if (rbSendIndividually->Checked) {
		for (int i = 0; i < lbDocuments->Items->Count; i++) {
			if (!lbDocuments->Checked[i])
				continue;

			TFileData *data = static_cast<TFileData *>(lbDocuments->Items->Objects[i]);
			if (!data->HasNumber) {
				numbersOk = false;
				break;
			}
		}
	} else
		numbersOk = FHasNumber;

	TConfig *Config = TConfig::GetInstance();

	actSend->Enabled = 		!FSending &&
							hasDoc &&
							numbersOk &&
							Trim(Config->Server).Length() > 0;

	actUp->Enabled = 		!FSending &&
							lbDocuments->Items->Count > 1 &&
							lbDocuments->ItemIndex > 0;

	actDown->Enabled = 		!FSending &&
							lbDocuments->Items->Count > 1 &&
							lbDocuments->ItemIndex >= 0 &&
							lbDocuments->ItemIndex < lbDocuments->Items->Count - 1;

	actSave->Enabled = 		!FSending &&
							AddressBook &&
							AddressBook->OnLine &&
							!AddressBook->ReadOnly &&
							FHasNumber &&
							!FHasManyNumbers;

	actDelete->Enabled = 	!FSending &&
							AddressBook &&
							AddressBook->OnLine &&
							!AddressBook->ReadOnly &&
							Trim(cbABNames->Text).Length() > 0 &&
							cbABNames->Text != SelectFromAB;

	actClose->Enabled = 	!FSending;

	actConfigure->Enabled = !FSending;

	actSelect->Enabled = 	!FSending &&
							AddressBook &&
							AddressBook->OnLine;

    actHelp->Enabled =      true;

	Handled = true;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FormCloseQuery(TObject *Sender, bool &CanClose)
{
	if (!FAutoClose && lbDocuments->Items->Count > 0 &&
	MessageDlg(_(L"Unsent faxes will be lost. Are you sure you want to exit?"),
	mtConfirmation, TMsgDlgButtons() << mbYes << mbNo, 0, mbNo) == mrNo) {
		CanClose = false;
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::actUpExecute(TObject *Sender)
{
	int i = lbDocuments->ItemIndex;
	lbDocuments->Items->Move(i, i - 1);
	lbDocuments->ItemIndex = i - 1;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::actDownExecute(TObject *Sender)
{
	int i = lbDocuments->ItemIndex;
	lbDocuments->Items->Move(i, i + 1);
	lbDocuments->ItemIndex = i + 1;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::actSaveExecute(TObject *Sender)
{
	if (!AddressBook || !FHasNumber || FHasManyNumbers)
		return;

	UnicodeString Number = PurgeNumber(edtFAXNumber->Text);

	UnicodeString RecipientName;

	TRecipientName *FRecipientName;

	FRecipientName = new TRecipientName(this);
	try {
		FRecipientName->FAXNumber->Text = Number;
		if (FRecipientName->ShowModal() == mrOk)
			RecipientName = FRecipientName->RecipientName->Text.Trim();
	}
	__finally {
		delete FRecipientName;
	}

	if (RecipientName.Length() > 0) {
		try {
			AddressBook->SetRecipient(RecipientName, Number);
			cbABNames->Text = RecipientName;
		}
		catch (EAddressBookUnchanged &E)
		{
			//silently catch EAddressBookUnchanged
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::actDeleteExecute(TObject *Sender)
{
	if (!AddressBook || MessageDlg(_(L"Delete recipient?"), mtConfirmation,
	TMsgDlgButtons() << mbYes << mbNo, 0, mbNo) == mrNo) {
		return;
	}

	AddressBook->DeleteRecipient(cbABNames->Text);
	cbABNames->Text = _(L"Select from address book");
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::actConfigureExecute(TObject *Sender)
{
	TConfig *Config = TConfig::GetInstance();

	Config->Configure();
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::actCloseExecute(TObject *Sender)
{
	Close();
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FormActivate(TObject *Sender)
{
	if (edtFAXNumber->Enabled) {
		edtFAXNumber->SetFocus();
    }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FormConstrainedResize(TObject *Sender, int &MinWidth, int &MinHeight,
  		int &MaxWidth, int &MaxHeight)
{
	MinHeight = 564;
	MinWidth = 446;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::AppInstInstanceCreated(TObject *Sender, DWORD ProcessId)
{
	BringFaxWndToFront();
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::AppInstCmdLineReceived(TObject *Sender, TStrings *CmdLine)
{
	ParseCommandLine(CmdLine);

	//activate single instance only if this is not a "send and forget" process
	AppInst->Active = !FAutoClose;

	if (FImmediateSend) {
		actSendExecute(NULL);
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::lbDocumentsMouseDown(TObject *Sender, TMouseButton Button,
  		TShiftState Shift, int X, int Y)
{
	//drag'n'drop starts here
	DragPoint.x = X;
	DragPoint.y = Y;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::lbDocumentsDragOver(TObject *Sender, TObject *Source, int X,
  		int Y, TDragState State, bool &Accept)
{
	//only accept dragging from ourselves
	Accept = (Source == lbDocuments);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::lbDocumentsDragDrop(TObject *Sender, TObject *Source, int X,
  		int Y)
{
	//move elements
	TCheckListBox *lb = dynamic_cast<TCheckListBox *>(Sender);
	TPoint DropPoint;

	DropPoint.x = X;
	DropPoint.y = Y;
	int DropSource = lb->ItemAtPos(DragPoint, true);
	int DropTarget = lb->ItemAtPos(DropPoint, true);

	if (DropSource >= 0 && DropTarget >= 0 && DropSource != DropTarget) {
		lb->Items->Move(DropSource, DropTarget);
		lb->ItemIndex = DropTarget;
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::mnuAddfileClick(TObject *Sender)
{
	if (opendlg->Execute()) {
		AddFileToList(opendlg->FileName, opendlg->FileName, 0);
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::actSelectExecute(TObject *Sender)
{
	if (!AddressBook)
		return;

	TSelectRcpt *frm = new TSelectRcpt(this);

	try {
		frm->Numbers = edtFAXNumber->Text;

		if (frm->ShowModal() == mrOk) {
			edtFAXNumber->Text = frm->Numbers;
		}
	}
	__finally {
		delete frm;
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::mnuSelectAllClick(TObject *Sender)
{
	lbDocuments->CheckAll(cbChecked, false, false);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::mnuSelectNoneClick(TObject *Sender)
{
	lbDocuments->CheckAll(cbUnchecked, false, false);
}
//---------------------------------------------------------------------------

int TMainForm::GetNumbersCount(const UnicodeString &Numbers)
{
	int i, count = 0;
	bool bSemicolonSaw = false;
	bool bDigitSaw = false;
	bool bInQuotes = false;

	for (i = 1; i <= Numbers.Length(); i++) {
		if (Numbers[i] >= L'0' && Numbers[i] <= L'9') {
			if (bSemicolonSaw) {
				count++;
				break;
			}

			if (!bDigitSaw) {
				count++;
				bDigitSaw = true;
			}
		} else if (Numbers[i] == L';' && !bInQuotes) {
			bDigitSaw = false;
			bSemicolonSaw = true;
		} else if (Numbers[i] == L'"') {
			bInQuotes = !bInQuotes;
			if (bInQuotes) {
				bSemicolonSaw = false;
				bDigitSaw = false;
			}
		}
	}

	return count;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::edtFAXNumberChange(TObject *Sender)
{
	UnicodeString Number = edtFAXNumber->Text.Trim();
	int count = GetNumbersCount(Number);

	FHasNumber = count > 0;
	FHasManyNumbers = count > 1;

	if (!FSettingNumberEdit) {
		int idx = lbDocuments->ItemIndex;
		if (idx < 0)
			return;

		TFileData *data = static_cast<TFileData *>(lbDocuments->Items->Objects[idx]);
		data->FaxNumber = Number;
		data->HasNumber = FHasNumber;

		lbDocuments->Invalidate();
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::lbDocumentsClick(TObject *Sender)
{
	if (rbSendIndividually->Checked) {
		int idx = lbDocuments->ItemIndex;
		if (idx < 0)
			return;

		FSettingNumberEdit = true; // prevent infinite loop
		edtFAXNumber->Text = (static_cast<TFileData *>(lbDocuments->Items->Objects[idx]))->FaxNumber;
		FSettingNumberEdit = false;
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::lbDocumentsDrawItem(TWinControl *Control, int Index, TRect &Rect,
  		TOwnerDrawState State)
{
	TFileData *data = static_cast<TFileData *>(lbDocuments->Items->Objects[Index]);
	SIZE s;
	UnicodeString out;
	int w;
	const int PAGES_BOX_WIDTH = 30;

	if (rbSendIndividually->Checked)
		w = (Rect.Width() - PAGES_BOX_WIDTH) / 2;
	else
		w = Rect.Width() - PAGES_BOX_WIDTH;

	lbDocuments->Canvas->FillRect(Rect);

	if (lbDocuments->Checked[Index]) {
		if (State.Contains(odSelected))
			lbDocuments->Canvas->Font->Color = clBlack;
		else
			lbDocuments->Canvas->Font->Color = clNavy;
	} else
		lbDocuments->Canvas->Font->Color = clGray;

	UnicodeString text;

	//pages
	if (data->Pages) {
		text = UIntToStr((unsigned)data->Pages) + L"p";
		lbDocuments->Canvas->TextOutW(Rect.Left, Rect.Top, text);
	}

	lbDocuments->Canvas->MoveTo(Rect.Left + PAGES_BOX_WIDTH + 1, Rect.Top);
	lbDocuments->Canvas->LineTo(Rect.Left + PAGES_BOX_WIDTH + 1, Rect.Bottom);

	//title
	text = lbDocuments->Items->Strings[Index].Trim();
	s = lbDocuments->Canvas->TextExtent(text);
	if (s.cx > w) {
		int newlen = (text.Length() * w / s.cx) - 3;
		if (newlen < 0)
			newlen = 0;
		out = text.SubString(1, newlen) + L"...";
	} else
		out = text;

	lbDocuments->Canvas->TextOutW(Rect.Left + PAGES_BOX_WIDTH + 6, Rect.Top, out);

	//number
	if (rbSendIndividually->Checked) {
		lbDocuments->Canvas->MoveTo(Rect.Left + PAGES_BOX_WIDTH + w + 1, Rect.Top);
		lbDocuments->Canvas->LineTo(Rect.Left + PAGES_BOX_WIDTH + w + 1, Rect.Bottom);

		text = data->FaxNumber.Trim();
		s = lbDocuments->Canvas->TextExtent(text);
		if (s.cx > w) {
			int newlen = (text.Length() * w / s.cx) - 3;
			if (newlen < 0)
				newlen = 0;
			out = text.SubString(1, newlen) + L"...";
		} else
			out = text;

		lbDocuments->Canvas->TextOutW(Rect.Left + PAGES_BOX_WIDTH + w + 6, Rect.Top, out);
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FormResize(TObject *Sender)
{
	//gosh darn it, why the f@*! it does not repaint correctly,
	//unless I make this horrible call???
	lbDocuments->Invalidate();
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::rbSendModeClick(TObject *Sender)
{
	lbDocuments->Invalidate();

	if (Sender == rbSendIndividually) {
		FSettingNumberEdit = true;
		edtFAXNumber->Clear();
		FSettingNumberEdit = false;
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::actSendExecute(TObject *Sender)
{
	int i, nrdoc, nrdest, pos;
	bool hasDoc = false, numbersOk = true, needsNumbers = false;
	TStringList *Numbers;
	TFax *pFax = NULL;
	std::vector<TFax *> faxes;

	TConfig *Config = TConfig::GetInstance();

	LockUI();
	try {
		for (i = 0; i < lbDocuments->Items->Count; i++) {
			if (lbDocuments->Checked[i]) {
				hasDoc = true;
				break;
			}
		}

		if (rbSendIndividually->Checked) {
			for (int i = 0; i < lbDocuments->Items->Count; i++) {
				if (!lbDocuments->Checked[i]) {
					continue;
				}

				TFileData *data = static_cast<TFileData *>(lbDocuments->Items->Objects[i]);
				if (!data->HasNumber) {
					numbersOk = false;
					break;
				}
			}
		} else {
			numbersOk = FHasNumber;
		}

		if (FSending || !hasDoc || !numbersOk || Trim(Config->Server).Length() == 0) {
			return;
		}

		ITransport *pTransport = new TTransportStoneFax();
		IProcessor *pProcessor = new TProcessorStoneFax();

		Numbers = new TStringList();

		if (rbSendTogether->Checked) {
			LOG_INFO0(L"TMainForm::actSendExecute: creating a new fax job");
			pFax = new TFax();
			faxes.push_back(pFax);
			needsNumbers = true;
		}

		try {
			LOG_INFO1(L"TMainForm::actSendExecute: temp directory is %s", Config->WFITempDir);
			pProcessor->SetWorkingDirectory(Config->WFITempDir);

			LOG_DEBUG3(L"TMainForm::actSendExecute: server=%s ssl=%d check=%d",
				Config->Server, (int)Config->UseSSL, (int)Config->SkipCertificateCheck);
			pTransport->SetServer(Config->Server, Config->UseSSL, Config->SkipCertificateCheck);
			pTransport->SetUsername(Config->Username);
			pTransport->SetPassword(Config->Password);
			pTransport->SetProcessor(pProcessor);

			pTransport->Subscribe(this);

			Numbers->Delimiter = L';';
			Numbers->StrictDelimiter = true;

			EnableFields(false);
			FSending = true;

			try {
				for (nrdoc = 0; nrdoc < lbDocuments->Items->Count; nrdoc++) {
					if (!lbDocuments->Checked[nrdoc]) {
						continue;
                    }

					if (!rbSendTogether->Checked) {
						LOG_INFO0(L"TMainForm::actSendExecute: creating a new fax job");
						pFax = new TFax();
						faxes.push_back(pFax);
						needsNumbers = true;
					}

					TFileData *data = static_cast<TFileData *>(lbDocuments->Items->Objects[nrdoc]);

					LOG_DEBUG1(L"TMainForm::actSendExecute: adding file %s to fax job", data->FileName);
					pFax->AddFile(data->FileName);
					data->Fax = pFax;

					if (needsNumbers) {
						Numbers->DelimitedText = ReplaceText(
							rbSendTogether->Checked
								? edtFAXNumber->Text
								: data->FaxNumber,
							L"\"",
							L""
						);

						for (nrdest = 0; nrdest < Numbers->Count; nrdest++) {
							UnicodeString Number = Numbers->Strings[nrdest].Trim();

							//check number is not empty
							if (Number.Length() == 0) {
								continue;
							}

							//extract cover name and subaddress
							UnicodeString Recipient;
							if (edtRecipientName->Text.Trim().Length() == 0 ||
							edtRecipientName->Text == _(L"<from address book>")) {
								if ((pos = Number.Pos(L"@")) != 0) {
									Recipient = Number.SubString(1, pos - 1).Trim();
									Number = Number.SubString(pos + 1, Number.Length()).Trim();
								} else {
									if (AddressBook) {
										//try reverse lookup: number -> name
										int idx = AddressBook->IndexOfNumber(Number);
										Recipient = AddressBook->Names[idx];
									} else
										Recipient = Number;
								}
							} else {
								Recipient = edtRecipientName->Text;
							}

							UnicodeString SubAddress;
							if ((pos = Number.Pos(L"#")) != 0) {
								SubAddress = Number.SubString(pos + 1, Number.Length()).Trim();
								Number = Number.SubString(1, pos - 1).Trim();
							}

							//check again
							if (Number.Length() == 0) {
								continue;
							}

							pFax->AddRecipient(Recipient, L"", Number);
						}

                        needsNumbers = false;
					}

					if (chkFaxcover->Checked) {
						//TODO: gestire nome attuale della cover
						pFax->UseCover = true;
						pFax->CoverPageName = L"BusinessCover";
					}

					pFax->Subject = edtSubject->Text;
					pFax->Body = memComments->Text;
					UnicodeString aNotify = edtNotificationEmail->Text.Trim();
					if (aNotify.Length() > 0) {
						pFax->NotifyByEmail = true;
						pFax->NotificationEmailAddress = aNotify;
					}

					if (!rbSendTogether->Checked) {
						LOG_INFO0(L"TMainForm::actSendExecute: enqueuing fax job");
						pTransport->Enqueue(pFax);
					}
				}

				if (rbSendTogether->Checked) {
                    LOG_INFO0(L"TMainForm::actSendExecute: enqueuing fax job");
					pTransport->Enqueue(pFax);
				}

				TFormProgress *pProgress = new TFormProgress(this, pTransport);
				try {
					pProgress->ShowModal();
				}
				__finally {
					delete pProgress;
				}
			}
			__finally {
				FSending = false;
				EnableFields(true);
			}
		}
		__finally {
			delete pTransport;
			delete pProcessor;
			delete Numbers;

			std::vector<TFax *>::iterator it;
			for (it = faxes.begin(); it != faxes.end(); it++) {
				delete *it;
            }
		}
	}
	__finally {
		UnlockUI();
	}
}
//---------------------------------------------------------------------------

void TMainForm::TransportNotify(TTransportEvent aEvent, TFax *pFax, bool result, Exception *pError)
{
	int i;

	switch (aEvent) {
	case teTransportBegin:
		break;

	case teFaxConvertBegin:
		break;

	case teFaxConvertEnd:
		break;

	case teFaxSendBegin:
		break;

	case teFaxSendEnd:
		if (result) {
			LOG_INFO0(L"TMainForm::TransportNotify: teFaxSendEnd received, cleaning up files");

			for (i = lbDocuments->Items->Count - 1; i >= 0; i--) {
				TFileData *file = static_cast<TFileData *>(lbDocuments->Items->Objects[i]);

				if (file->Fax == pFax) {
					if (file->FromSpooler) {
						LOG_INFO1(L"TMainForm::TransportNotify: deleting file %s", file->FileName);

						DeleteFile(file->FileName);
					}

					delete file;

					lbDocuments->Items->Delete(i);
				}
			}
		}

	case teTransportEnd:
		break;

	default:
		break;
	}
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::lblLinkLinkClick(TObject *Sender, const UnicodeString Link,
          TSysLinkType LinkType)
{
	ShellExecuteW(Handle, L"open", Link.c_str(), NULL, NULL, SW_SHOW);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::actHelpExecute(TObject *Sender)
{
	ShellExecuteW(Handle, L"open", L"https://www.imagicle.com/knowledgebase", NULL, NULL, SW_SHOW);
}
//---------------------------------------------------------------------------

