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
#include <JclFileUtils.hpp>
#include <JclMiscel.hpp>
#include <JclSysInfo.hpp>
#include "magic.h"
#include <gnugettext.hpp>
#pragma hdrstop

#include "log.h"

#pragma link "libmagic.lib"

#include "ProcessorStoneFax.h"

#define BUFSIZE 1024*1024
//---------------------------------------------------------------------------

TProcessorStoneFax::TProcessorStoneFax()
{
	FWorkingDirectory = IncludeTrailingPathDelimiter(PathGetTempPath());

	FProcessedFiles = new TStringList();
	FBuffer = new char[BUFSIZE];
	FMagicData = new TMemoryStream();

	FMagic = magic_open(MAGIC_MIME);

	if (FMagic == NULL) {
		throw EProcessorError(_(L"Cannot initialize libmagic"));
	}

	UnicodeString mgc = ExtractFilePath(ParamStr(0)) + L"magic.mgc";

	LOG_INFO1(L"TProcessorStoneFax::TProcessorStoneFax: reading magic database from %s", mgc);

	TFileStream *pStream = new TFileStream(mgc, fmOpenRead | fmShareDenyNone);
	try {
		FMagicData->CopyFrom(pStream);
		void *data = FMagicData->Memory;
		size_t size = FMagicData->Size;
		if (magic_load_buffers(FMagic, &data, &size, 1) != 0) {
			throw EProcessorError(_(L"Cannot load magic.mgc"));
		}
	}
	__finally {
		delete pStream;
	}
}
//---------------------------------------------------------------------------

TProcessorStoneFax::~TProcessorStoneFax()
{
	delete FProcessedFiles;
	delete[] FBuffer;

	if (FMagic) magic_close(FMagic);

	delete FMagicData;
}
//---------------------------------------------------------------------------

void TProcessorStoneFax::SetWorkingDirectory(const UnicodeString &aWorkDir)
{
	FWorkingDirectory = IncludeTrailingPathDelimiter(aWorkDir);
	System::Sysutils::ForceDirectories(FWorkingDirectory);
}
//---------------------------------------------------------------------------

UnicodeString TProcessorStoneFax::ProcessFile(const UnicodeString &aFile)
{
	int nRead = 0;

	TFileStream *pFileStream = new TFileStream(aFile, fmOpenRead | fmShareDenyNone);
	try {
		nRead = pFileStream->Read(FBuffer, BUFSIZE);
	}
	__finally {
		delete pFileStream;
	}

	const char *cmime = magic_buffer(FMagic, FBuffer, nRead);

	if (cmime == NULL) {
		UnicodeString err;
		UnicodeString fmt = _(L"Call to magic_buffer failed: message=%hs");
		err.printf(fmt.c_str(), magic_error(FMagic));
		throw EProcessorError(err);
	}

    AnsiString mime = cmime;

	int pos;
	if ((pos = mime.Pos(";")) != 0) {
		AnsiString type = mime.SubString(1, pos - 1);
		if (type.AnsiCompareIC("application/postscript") == 0) {
			UnicodeString temp = FWorkingDirectory + ExtractFileName(aFile) + L".pdf";

#ifdef _DEBUG
			UnicodeString ghostscript = ExtractFilePath(ParamStr(0)) + L"..\\..\\gs\\";
#else
			UnicodeString ghostscript = ExtractFilePath(ParamStr(0)) + L"..\\gs\\";
#endif
			if (IsWindows64()) {
				ghostscript += L"x64\\bin\\gswin64c.exe";
			} else {
				ghostscript += L"x86\\bin\\gswin32c.exe";
			}
			ghostscript = ExpandFileName(ghostscript);

			UnicodeString cmd;

			TVarRec vars[] = {
				ghostscript,
				temp,
				aFile
			};
			cmd = System::Sysutils::Format(L"\"%s\" -sDEVICE=pdfwrite -dPDFA -dBATCH -dNOPAUSE -dSAFER -dQUIET -dPDFSETTINGS=/prepress -dAutoRotatePages=/PageByPage -r200 -sColorConversionStrategy=RGB -sProcessColorModel=DeviceRGB -sOutputFile=\"%s\" \"%s\"", vars, 3);

			LOG_DEBUG1(L"TProcessorStoneFax::ProcessFile: executing Ghostscript. Command line=%s", cmd);

			UnicodeString outp;
			DWORD dwRet;
			if ((dwRet = WinExec32AndRedirectOutput(cmd, outp)) != 0) {
				LOG_CRITICAL1(L"TProcessorStoneFax::ProcessFile: Ghostscript failed (%d)", dwRet);
				LOG_CRITICAL1(L"TProcessorStoneFax::ProcessFile: Ghostscript output: %s", outp);

				throw EProcessorError(_(L"Ghostscript failed"));
			}

			LOG_INFO1(L"TProcessorStoneFax::ProcessFile: file %s created", temp);
			FProcessedFiles->Add(temp);
			return temp;
		}
	}

	return aFile;
}
//---------------------------------------------------------------------------

void TProcessorStoneFax::Cleanup()
{
	for (int i = 0; i < FProcessedFiles->Count; i++) {
		LOG_INFO1(L"TProcessorStoneFax::Cleanup: deleting file %s", FProcessedFiles->Strings[i]);

		DeleteFile(FProcessedFiles->Strings[i]);
	}

    FProcessedFiles->Clear();
}
//---------------------------------------------------------------------------

#pragma package(smart_init)
