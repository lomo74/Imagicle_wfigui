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

#include <windows.h>
#include <bcrypt.h>
#include <vcl.h>
#include <tchar.h>
#include <StrUtils.hpp>
#include <unicode/utypes.h>
#include <unicode/ucnv.h>
#pragma hdrstop

#include "Utils.h"
#include "log.h"

#pragma link "bcrypt.lib"

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)

#pragma package(smart_init)
//---------------------------------------------------------------------------

typedef struct _TVersion {
	union {
		struct {
			DWORD verMS;
			DWORD verLS;
		} v1;
		struct {
			WORD verW2;
			WORD verW1;
			WORD verW4;
			WORD verW3;
		} v2;
	};
} TVersion;
//---------------------------------------------------------------------------

UnicodeString GetVersionDescription(HMODULE AModule)
{
	UnicodeString Result, attr;
	try {
		HRSRC rsrc = FindResource(AModule, MAKEINTRESOURCE(1), RT_VERSION);
		if (rsrc) {
			HGLOBAL glob = LoadResource(AModule, rsrc);
			if (glob) {
				try {
					LPVOID ptr = LockResource(glob);
					if (ptr) {
						TMemoryStream *mem = new TMemoryStream();
						try {
							VS_FIXEDFILEINFO *ffi;
							UINT len;
							TVersion ver;

							mem->Position = 0;
							mem->WriteBuffer(ptr, SizeofResource(AModule, rsrc));
							if (VerQueryValue(mem->Memory, _T("\\"), reinterpret_cast<LPVOID *>(&ffi), &len)) {
								ver.v1.verMS = ffi->dwFileVersionMS;
								ver.v1.verLS = ffi->dwFileVersionLS;
								TVarRec args[] = {
									ver.v2.verW1,
									ver.v2.verW2,
									ver.v2.verW3,
									ver.v2.verW4
								};
								Result = Format(L"%d.%d.%d.%d", args, 4);
								if ((ffi->dwFileFlagsMask & VS_FF_DEBUG) &&
								(ffi->dwFileFlags & VS_FF_DEBUG))
									attr = L",debug";
								if ((ffi->dwFileFlagsMask & VS_FF_PRERELEASE) &&
								(ffi->dwFileFlags & VS_FF_PRERELEASE))
									attr += L",prerelease";
								if ((ffi->dwFileFlagsMask & VS_FF_PATCHED) &&
								(ffi->dwFileFlags & VS_FF_PATCHED))
									attr += L",patched";
								if ((ffi->dwFileFlagsMask & VS_FF_SPECIALBUILD) &&
								(ffi->dwFileFlags & VS_FF_SPECIALBUILD))
									attr += L",special build";
								if ((ffi->dwFileFlagsMask & VS_FF_PRIVATEBUILD) &&
								(ffi->dwFileFlags & VS_FF_PRIVATEBUILD))
									attr += L",private build";
								if (attr.Length() > 0) {
									Result += L" ";
									Result += attr.SubString(2, MaxInt);
								}
							}
						}
						__finally {
							delete mem;
						}
					}
				}
				__finally {
					FreeResource(glob);
				}
			}
		} else
			return L"<no version info available>";
	} catch (...) {
		return L"";
	}

	return Result;
}
//---------------------------------------------------------------------------

UnicodeString PurgeNumber(UnicodeString Number)
{
	//remove trailing and leading spaces and semicolons
	int i, j;

	i = 1;
	while (i <= Number.Length() &&
	(Number[i] == L';' || Number[i] == L' ' || Number[i] == L'\t' ||
	Number[i] == L'\r' || Number[i] == L'\n'))
		i++;

	j = Number.Length();
	while (j >= 1 &&
	(Number[j] == L';' || Number[j] == L' ' || Number[j] == L'\t' ||
	Number[j] == L'\r' || Number[j] == L'\n'))
		j--;

	if (i >= j)
		return L"";

	Number = Number.SubString(i, j - i + 1);

	//remove surrounding quotes, if any
	if (Number[1] == L'"') {
		if (Number.Length() < 3 || Number[Number.Length()] != L'"')
			((void)0);

		Number = Number.SubString(2, Number.Length() - 2);
	} else if (Number.Length() == 0)
		((void)0);

	//remove inner double quotes, we don't handle them well for the moment
	Number = ReplaceText(Number, L"\"", L"");

	//add surrounding double quotes back if needed
	if (Number.Pos(L";") != 0)
		Number = L"\"" + Number + L"\"";

	return Number;
}
//---------------------------------------------------------------------------

static const BYTE rgbAES128Key[] =
{
	0x99, 0x6c, 0xf4, 0xd2, 0x6c, 0x29, 0xba, 0xad,
	0x8b, 0x66, 0x5a, 0x77, 0x94, 0x18, 0xc0, 0x67
};
//---------------------------------------------------------------------------

UnicodeString EncryptString(const UnicodeString &aString)
{
	BCRYPT_ALG_HANDLE hAesAlg = NULL;
	BCRYPT_KEY_HANDLE hKey = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DWORD cbCipherText = 0,
          cbPlainText = 0,
		  cbData = 0,
		  cbKeyObject = 0,
		  cbBlockLen = 0;
	PBYTE pbCipherText = NULL,
		  pbKeyObject = NULL,
		  pbIV = NULL;
	UnicodeString err, iv, data;

	try {
		try {
			if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(
				&hAesAlg,
				BCRYPT_AES_ALGORITHM,
				NULL,
				0)))
			{
				err.printf(L"BCryptOpenAlgorithmProvider failed (0x%08X)", status);
				throw Exception(err);
			}

			if (!NT_SUCCESS(status = BCryptGetProperty(
				hAesAlg,
				BCRYPT_OBJECT_LENGTH,
				(PBYTE)&cbKeyObject,
				sizeof(DWORD),
				&cbData,
				0)))
			{
				err.printf(L"BCryptGetProperty failed (0x%08X)", status);
				throw Exception(err);
			}

			pbKeyObject = new BYTE[cbKeyObject];

			if (!NT_SUCCESS(status = BCryptGetProperty(
				hAesAlg,
				BCRYPT_BLOCK_LENGTH,
				(PBYTE)&cbBlockLen,
				sizeof(DWORD),
				&cbData,
				0)))
			{
				err.printf(L"BCryptGetProperty failed (0x%08X)", status);
				throw Exception(err);
			}

			if (cbBlockLen > 16)
			{
				throw Exception(L"block length is too big");
			}

			pbIV = new BYTE[16];

			for (DWORD n = 0; n < 16; n++)
				pbIV[n] = rand() % 256;

			iv.SetLength(32);
			BinToHex(pbIV, iv.c_str(), 16);

			if (!NT_SUCCESS(status = BCryptSetProperty(
				hAesAlg,
				BCRYPT_CHAINING_MODE,
				(PBYTE)BCRYPT_CHAIN_MODE_CBC,
				sizeof(BCRYPT_CHAIN_MODE_CBC),
				0)))
			{
				err.printf(L"BCryptSetProperty failed (0x%08X)", status);
				throw Exception(err);
			}

			if (!NT_SUCCESS(status = BCryptGenerateSymmetricKey(
				hAesAlg,
				&hKey,
				pbKeyObject,
				cbKeyObject,
				(PBYTE)rgbAES128Key,
				sizeof(rgbAES128Key),
				0)))
			{
				err.printf(L"BCryptGenerateSymmetricKey failed (0x%08X)", status);
				throw Exception(err);
			}

			cbPlainText = aString.Length() * sizeof(WideChar);

			if (!NT_SUCCESS(status = BCryptEncrypt(
				hKey,
				(PBYTE)aString.c_str(),
				cbPlainText,
				NULL,
				pbIV,
				cbBlockLen,
				NULL,
				0,
				&cbCipherText,
				BCRYPT_BLOCK_PADDING)))
			{
				err.printf(L"BCryptEncrypt failed (0x%08X)", status);
				throw Exception(err);
			}

			pbCipherText = new BYTE[cbCipherText];

			if (!NT_SUCCESS(status = BCryptEncrypt(
				hKey,
				(PBYTE)aString.c_str(),
				cbPlainText,
				NULL,
				pbIV,
				cbBlockLen,
				pbCipherText,
				cbCipherText,
				&cbData,
				BCRYPT_BLOCK_PADDING)))
			{
				err.printf(L"BCryptEncrypt failed (0x%08X)", status);
				throw Exception(err);
			}

			if (!NT_SUCCESS(status = BCryptDestroyKey(hKey)))
			{
				err.printf(L"BCryptDestroyKey failed (0x%08X)", status);
				throw Exception(err);
			}

			data.SetLength(cbCipherText * 2);
			BinToHex(pbCipherText, data.c_str(), cbCipherText);

			return iv + data;
		}
		catch (Exception &e) {
            //TODO: log error
			return L"";
        }
	}
	__finally {
		if (hAesAlg) BCryptCloseAlgorithmProvider(hAesAlg, 0);
		if (hKey) BCryptDestroyKey(hKey);
		if (pbCipherText) delete[] pbCipherText;
		if (pbKeyObject) delete[] pbKeyObject;
		if (pbIV) delete[] pbIV;
	}
}
//---------------------------------------------------------------------------

UnicodeString DecryptString(const UnicodeString &aString)
{
	BCRYPT_ALG_HANDLE hAesAlg = NULL;
	BCRYPT_KEY_HANDLE hKey = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DWORD cbCipherText = 0,
		  cbPlainText = 0,
		  cbData = 0,
		  cbKeyObject = 0,
		  cbBlockLen = 0,
		  cbBlob = 0;
	PBYTE pbCipherText = NULL,
		  pbKeyObject = NULL,
		  pbBlob = NULL;
	UnicodeString err, data;

	try {
		try {
			cbBlob = aString.Length() / 2;

			if (cbBlob <= 16)
				return L"";

			pbBlob = new BYTE[cbBlob];

			HexToBin(aString.c_str(), pbBlob, cbBlob);

			if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(
				&hAesAlg,
				BCRYPT_AES_ALGORITHM,
				NULL,
				0)))
			{
				err.printf(L"BCryptOpenAlgorithmProvider failed (0x%08X)", status);
				throw Exception(err);
			}

			if (!NT_SUCCESS(status = BCryptGetProperty(
				hAesAlg,
				BCRYPT_OBJECT_LENGTH,
				(PBYTE)&cbKeyObject,
				sizeof(DWORD),
				&cbData,
				0)))
			{
				err.printf(L"BCryptGetProperty failed (0x%08X)", status);
				throw Exception(err);
			}

			pbKeyObject = new BYTE[cbKeyObject];

			if (!NT_SUCCESS(status = BCryptGetProperty(
				hAesAlg,
				BCRYPT_BLOCK_LENGTH,
				(PBYTE)&cbBlockLen,
				sizeof(DWORD),
				&cbData,
				0)))
			{
				err.printf(L"BCryptGetProperty failed (0x%08X)", status);
				throw Exception(err);
			}

			if (cbBlockLen > 16)
			{
				throw Exception(L"block length is too big");
			}

			if (!NT_SUCCESS(status = BCryptSetProperty(
				hAesAlg,
				BCRYPT_CHAINING_MODE,
				(PBYTE)BCRYPT_CHAIN_MODE_CBC,
				sizeof(BCRYPT_CHAIN_MODE_CBC),
				0)))
			{
				err.printf(L"BCryptSetProperty failed (0x%08X)", status);
				throw Exception(err);
			}

			if (!NT_SUCCESS(status = BCryptGenerateSymmetricKey(
				hAesAlg,
				&hKey,
				pbKeyObject,
				cbKeyObject,
				(PBYTE)rgbAES128Key,
				sizeof(rgbAES128Key),
				0)))
			{
				err.printf(L"BCryptGenerateSymmetricKey failed (0x%08X)", status);
				throw Exception(err);
			}

			pbCipherText = pbBlob + 16;
			cbCipherText = cbBlob - 16;

			if (!NT_SUCCESS(status = BCryptDecrypt(
				hKey,
				pbCipherText,
				cbCipherText,
				NULL,
				pbBlob,
				cbBlockLen,
				NULL,
				0,
				&cbPlainText,
				BCRYPT_BLOCK_PADDING)))
			{
				err.printf(L"BCryptDecrypt failed (0x%08X)", status);
				throw Exception(err);
			}

			data.SetLength(cbPlainText / sizeof(WideChar));

			if (!NT_SUCCESS(status = BCryptDecrypt(
				hKey,
				pbCipherText,
				cbCipherText,
				NULL,
				pbBlob,
				cbBlockLen,
				(PBYTE)data.c_str(),
				cbPlainText,
				&cbPlainText,
				BCRYPT_BLOCK_PADDING)))
			{
				err.printf(L"BCryptDecrypt failed (0x%08X)", status);
				throw Exception(err);
			}

			data.SetLength(cbPlainText / sizeof(WideChar));

			if (!NT_SUCCESS(status = BCryptDestroyKey(hKey)))
			{
				err.printf(L"BCryptDestroyKey failed (0x%08X)", status);
				throw Exception(err);
			}

			return data;
		}
		catch (Exception &e) {
			//TODO: log error
			return L"";
		}
	}
	__finally {
		if (hAesAlg) BCryptCloseAlgorithmProvider(hAesAlg, 0);
		if (hKey) BCryptDestroyKey(hKey);
		if (pbKeyObject) delete[] pbKeyObject;
		if (pbBlob) delete[] pbBlob;
	}
}
//---------------------------------------------------------------------------

UnicodeString ConvertToUcs2LE(const AnsiString &charset, const char *inbuf, int32_t insize)
{
	UErrorCode status = U_ZERO_ERROR;
	UConverter *cnv = NULL;
	int32_t len;
	UnicodeString res;

	cnv = ucnv_open(charset.c_str(), &status);

	if (!U_SUCCESS(status)) {
		LOG_CRITICAL1(L"ConvertToUcs2LE: ucnv_open failed (%d)", status);

		return L"";
	}

	res.SetLength(insize / ucnv_getMinCharSize(cnv));

	len = ucnv_toUChars(
		cnv,
		reinterpret_cast<UChar *>(res.c_str()),
		res.Length() * sizeof(wchar_t),
		inbuf,
		insize,
		&status
	);

	ucnv_close(cnv);

	if (!U_SUCCESS(status)) {
		LOG_CRITICAL1(L"ConvertToUcs2LE: ucnv_toUChars failed (%d)", status);

		return L"";
	}

	res.SetLength(len);

	return res;
}
//---------------------------------------------------------------------------

