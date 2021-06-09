unit indybkport;

interface

uses Windows, SysUtils, Classes;

procedure ParseContentType(const AValue: string; var AContentType, ACharSet: string);
function TextStartsWith(const S, SubS: string): Boolean;

implementation

{$DEFINE USE_INLINE}

type
  TIdHeaderQuotingType = (QuotePlain, QuoteRFC822, QuoteMIME, QuoteHTTP);
  TPosProc = function(const substr, str: String): LongInt;

var
  IndyPos: TPosProc = nil;

const
  QuoteSpecials: array[TIdHeaderQuotingType] of String = (
    {Plain } '',                    {do not localize}
    {RFC822} '()<>@,;:\"./',        {do not localize}
    {MIME  } '()<>@,;:\"/[]?=',     {do not localize}
    {HTTP  } '()<>@,;:\"/[]?={} '#9 {do not localize}
    );
  IdFetchDelimDefault = ' ';    {Do not Localize}
  IdFetchDeleteDefault = True;
  IdFetchCaseSensitiveDefault = True;
  CR = #13;

function InternalAnsiPos(const Substr, S: string): LongInt;
{$IFDEF USE_INLINE}inline;{$ENDIF}
begin
  Result := SysUtils.AnsiPos(Substr, S);
end;

function SBPos(const Substr, S: string): LongInt; inline;
begin
  // Necessary because of "Compiler magic"
  Result := Pos(Substr, S);
end;

function CharRange(const AMin, AMax : Char): String;
var
  i : Char;
begin
  SetLength(Result, Ord(AMax) - Ord(AMin) + 1);
  for i := AMin to AMax do begin
    Result[Ord(i) - Ord(AMin) + 1] := i;
  end;
end;

function CharPosInSet(const AString: string; const ACharPos: Integer; const ASet: String): Integer;
{$IFDEF USE_INLINE}inline;{$ENDIF}
var
  LChar: Char;
  I: Integer;
begin
  Result := 0;
  if ACharPos < 1 then begin
    raise Exception.Create('Invalid ACharPos');{ do not localize }
  end;
  if ACharPos <= Length(AString) then begin
    // RLebeau 5/8/08: Calling Pos() with a Char as input creates a temporary
    // String.  Normally this is fine, but profiling reveils this to be a big
    // bottleneck for code that makes a lot of calls to CharIsInSet(), so need
    // to scan through ASet looking for the character without a conversion...
    //
    // Result := IndyPos(AString[ACharPos], ASet);
    //
    LChar := AString[ACharPos];
    for I := 1 to Length(ASet) do begin
      if ASet[I] = LChar then begin
        Result := I;
        Exit;
      end;
    end;
  end;
end;

function CharIsInSet(const AString: string; const ACharPos: Integer; const ASet: String): Boolean;
{$IFDEF USE_INLINE}inline;{$ENDIF}
begin
  Result := CharPosInSet(AString, ACharPos, ASet) > 0;
end;

function TextIsSame(const A1, A2: string): Boolean;
{$IFDEF USE_INLINE}inline;{$ENDIF}
begin
  Result := AnsiCompareText(A1, A2) = 0;
end;

{This searches an array of string for an occurance of SearchStr}
function PosInStrArray(const SearchStr: string; const Contents: array of string; const CaseSensitive: Boolean = True): Integer;
begin
  for Result := Low(Contents) to High(Contents) do begin
    if CaseSensitive then begin
      if SearchStr = Contents[Result] then begin
        Exit;
      end;
    end else begin
      if TextIsSame(SearchStr, Contents[Result]) then begin
        Exit;
      end;
    end;
  end;
  Result := -1;
end;

function FetchCaseInsensitive(var AInput: string; const ADelim: string;
  const ADelete: Boolean): string;
{$IFDEF USE_INLINE}inline;{$ENDIF}
var
  LPos: Integer;
begin
  if ADelim = #0 then begin
    // AnsiPos does not work with #0
    LPos := Pos(ADelim, AInput);
  end else begin
    //? may be AnsiUpperCase?
    LPos := IndyPos(UpperCase(ADelim), UpperCase(AInput));
  end;
  if LPos = 0 then begin
    Result := AInput;
    if ADelete then begin
      AInput := '';    {Do not Localize}
    end;
  end else begin
    Result := Copy(AInput, 1, LPos - 1);
    if ADelete then begin
      //faster than Delete(AInput, 1, LPos + Length(ADelim) - 1); because the
      //remaining part is larger than the deleted
      AInput := Copy(AInput, LPos + Length(ADelim), MaxInt);
    end;
  end;
end;

function Fetch(var AInput: string; const ADelim: string = IdFetchDelimDefault;
  const ADelete: Boolean = IdFetchDeleteDefault;
  const ACaseSensitive: Boolean = IdFetchCaseSensitiveDefault): string;
{$IFDEF USE_INLINE}inline;{$ENDIF}
var
  LPos: Integer;
begin
  if ACaseSensitive then begin
    if ADelim = #0 then begin
      // AnsiPos does not work with #0
      LPos := Pos(ADelim, AInput);
    end else begin
      LPos := IndyPos(ADelim, AInput);
    end;
    if LPos = 0 then begin
      Result := AInput;
      if ADelete then begin
        AInput := '';    {Do not Localize}
      end;
    end
    else begin
      Result := Copy(AInput, 1, LPos - 1);
      if ADelete then begin
        //slower Delete(AInput, 1, LPos + Length(ADelim) - 1); because the
        //remaining part is larger than the deleted
        AInput := Copy(AInput, LPos + Length(ADelim), MaxInt);
      end;
    end;
  end else begin
    Result := FetchCaseInsensitive(AInput, ADelim, ADelete);
  end;
end;

function TextStartsWith(const S, SubS: string): Boolean;
var
  LLen: Integer;
  P1, P2: PChar;
begin
  LLen := Length(SubS);
  Result := LLen <= Length(S);
  if Result then
  begin
    P1 := PChar(S);
    P2 := PChar(SubS);
    Result := CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, P1, LLen, P2, LLen) = 2;
  end;
end;

function TextEndsWith(const S, SubS: string): Boolean;
var
  LLen: Integer;
  P1, P2: PChar;
begin
  LLen := Length(SubS);
  Result := LLen <= Length(S);
  if Result then
  begin
    P1 := PChar(S);
    P2 := PChar(SubS);
    Inc(P1, Length(S)-LLen);
    Result := CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, P1, LLen, P2, LLen) = 2;
  end;
end;

function IndyMin(const AValueOne, AValueTwo: LongInt): LongInt;
{$IFDEF USE_INLINE}inline;{$ENDIF}
begin
  if AValueOne > AValueTwo then begin
    Result := AValueTwo;
  end else begin
    Result := AValueOne;
  end;
end;

function IndyMax(const AValueOne, AValueTwo: LongInt): LongInt;
{$IFDEF USE_INLINE}inline;{$ENDIF}
begin
  if AValueOne < AValueTwo then begin
    Result := AValueTwo;
  end else begin
    Result := AValueOne;
  end;
end;

function IndyLength(const ABuffer: String; const ALength: Integer = -1; const AIndex: Integer = 1): Integer;
{$IFDEF USE_INLINE}inline;{$ENDIF}
var
  LAvailable: Integer;
begin
  Assert(AIndex >= 1);
  LAvailable := IndyMax(Length(ABuffer)-AIndex+1, 0);
  if ALength < 0 then begin
    Result := LAvailable;
  end else begin
    Result := IndyMin(LAvailable, ALength);
  end;
end;

function FindFirstOf(const AFind, AText: string; const ALength: Integer = -1;
  const AStartPos: Integer = 1): Integer;
var
  I, LLength, LPos: Integer;
begin
  Result := 0;
  if Length(AFind) > 0 then begin
    LLength := IndyLength(AText, ALength, AStartPos);
    if LLength > 0 then begin
      for I := 0 to LLength-1 do begin
        LPos := AStartPos + I;
        if IndyPos(AText[LPos], AFind) <> 0 then begin
          Result := LPos;
          Exit;
        end;
      end;
    end;
  end;
end;

procedure SplitHeaderSubItems(AHeaderLine: String; AItems: TStrings;
  AQuoteType: TIdHeaderQuotingType);
var
  LName, LValue, LSep: String;
  I: Integer;

  function FetchQuotedString(var VHeaderLine: string): string;
  begin
    Result := '';
    Delete(VHeaderLine, 1, 1);
    I := 1;
    while I <= Length(VHeaderLine) do begin
      if VHeaderLine[I] = '\' then begin
        // TODO: disable this logic for HTTP 1.0
        if I < Length(VHeaderLine) then begin
          Delete(VHeaderLine, I, 1);
        end;
      end
      else if VHeaderLine[I] = '"' then begin
        Result := Copy(VHeaderLine, 1, I-1);
        VHeaderLine := Copy(VHeaderLine, I+1, MaxInt);
        Break;
      end;
      Inc(I);
    end;
    Fetch(VHeaderLine, ';');
  end;

begin
  Fetch(AHeaderLine, ';'); {do not localize}
  LSep := CharRange(#0, #32) + QuoteSpecials[AQuoteType] + #127;
  while AHeaderLine <> '' do
  begin
    AHeaderLine := TrimLeft(AHeaderLine);
    if AHeaderLine = '' then begin
      Exit;
    end;
    LName := Trim(Fetch(AHeaderLine, '=')); {do not localize}
    AHeaderLine := TrimLeft(AHeaderLine);
    if TextStartsWith(AHeaderLine, '"') then {do not localize}
    begin
      LValue := FetchQuotedString(AHeaderLine);
    end else begin
      I := FindFirstOf(LSep, AHeaderLine);
      if I <> 0 then
      begin
        LValue := Copy(AHeaderLine, 1, I-1);
        if AHeaderLine[I] = ';' then begin {do not localize}
          Inc(I);
        end;
        Delete(AHeaderLine, 1, I-1);
      end else begin
        LValue := AHeaderLine;
        AHeaderLine := '';
      end;
    end;
    if (LName <> '') and (LValue <> '') then begin
      AItems.Add(LName + '=' + LValue);
    end;
  end;
end;

function ExtractHeaderItem(const AHeaderLine: String): String;
var
  s: string;
begin
  // Store in s and not Result because of Fetch semantics
  s := AHeaderLine;
  Result := Trim(Fetch(s, ';')); {do not localize}
end;

function ReplaceHeaderSubItem(const AHeaderLine, ASubItem, AValue: String;
  var VOld: String; AQuoteType: TIdHeaderQuotingType): String;
var
  LItems: TStringList;
  I: Integer;
  LValue: string;

  function QuoteString(const S: String): String;
  var
    I: Integer;
    LAddQuotes: Boolean;
    LNeedQuotes, LNeedEscape: String;
  begin
    Result := '';
    if Length(S) = 0 then begin
      Exit;
    end;
    LAddQuotes := False;
    LNeedQuotes := CharRange(#0, #32) + QuoteSpecials[AQuoteType] + #127;
    // TODO: disable this logic for HTTP 1.0
    LNeedEscape := '"\'; {Do not Localize}
    if AQuoteType in [QuoteRFC822, QuoteMIME] then begin
      LNeedEscape := LNeedEscape + CR; {Do not Localize}
    end;
    for I := 1 to Length(S) do begin
      if CharIsInSet(S, I, LNeedEscape) then begin
        LAddQuotes := True;
        Result := Result + '\'; {do not localize}
      end
      else if CharIsInSet(S, I, LNeedQuotes) then begin
        LAddQuotes := True;
      end;
      Result := Result + S[I];
    end;
    if LAddQuotes then begin
      Result := '"' + Result + '"';
    end;
  end;

begin
  Result := '';
  LItems := TStringList.Create;
  try
    SplitHeaderSubItems(AHeaderLine, LItems, AQuoteType);
    LItems.CaseSensitive := False;
    I := LItems.IndexOfName(ASubItem);
    if I >= 0 then begin
      VOld := LItems.Strings[I];
      Fetch(VOld, '=');
    end else begin
      VOld := '';
    end;
    LValue := Trim(AValue);
    if LValue <> '' then begin
      if I < 0 then begin
        I := LItems.Add('');
      end;
      LItems.Strings[I] := ASubItem + '=' + LValue; {do not localize}
    end
    else if I >= 0 then begin
      LItems.Delete(I);
    end;
    Result := ExtractHeaderItem(AHeaderLine);
    if Result <> '' then begin
      for I := 0 to LItems.Count-1 do begin
        Result := Result + '; ' + LItems.Names[I] + '=' + QuoteString(LItems.ValueFromIndex[I]); {do not localize}
      end;
    end;
  finally
    LItems.Free;
  end;
end;

function RemoveHeaderEntry(const AHeader, AEntry: string; var VOld: String;
  AQuoteType: TIdHeaderQuotingType): string;
{$IFDEF USE_INLINE}inline;{$ENDIF}
begin
  Result := ReplaceHeaderSubItem(AHeader, AEntry, '', VOld, AQuoteType);
end;

function MediaTypeMatches(const AValue, AMediaType: String): Boolean;
begin
  if Pos('/', AMediaType) > 0 then begin {do not localize}
    Result := TextIsSame(AValue, AMediaType);
  end else begin
    Result := TextStartsWith(AValue, AMediaType + '/'); {do not localize}
  end;
end;

function IsHeaderMediaType(const AHeaderLine, AMediaType: String): Boolean;
begin
  Result := MediaTypeMatches(ExtractHeaderItem(AHeaderLine), AMediaType);
end;

function ExtractHeaderMediaSubType(const AHeaderLine: String): String;
var
  S: String;
  I: Integer;
begin
  S := ExtractHeaderItem(AHeaderLine);
  I := Pos('/', S);
  if I > 0 then begin
    Result := Copy(S, I+1, Length(S));
  end else begin
    Result := '';
  end;
end;

procedure ParseContentType(const AValue: string; var AContentType, ACharSet: string);
var
  S, LCharSet: string;
begin
  if AValue <> '' then begin
    AContentType := RemoveHeaderEntry(AValue, 'charset', LCharSet, QuoteHTTP); {do not localize}

    // RLebeau: per RFC 2616 Section 3.7.1:
    //
    // The "charset" parameter is used with some media types to define the
    // character set (section 3.4) of the data. When no explicit charset
    // parameter is provided by the sender, media subtypes of the "text"
    // type are defined to have a default charset value of "ISO-8859-1" when
    // received via HTTP. Data in character sets other than "ISO-8859-1" or
    // its subsets MUST be labeled with an appropriate charset value. See
    // section 3.4.1 for compatibility problems.

    // RLebeau: per RFC 3023 Sections 3.1, 3.3, 3.6, and 8.5:
    //
    // Conformant with [RFC2046], if a text/xml entity is received with
    // the charset parameter omitted, MIME processors and XML processors
    // MUST use the default charset value of "us-ascii"[ASCII].  In cases
    // where the XML MIME entity is transmitted via HTTP, the default
    // charset value is still "us-ascii".  (Note: There is an
    // inconsistency between this specification and HTTP/1.1, which uses
    // ISO-8859-1[ISO8859] as the default for a historical reason.  Since
    // XML is a new format, a new default should be chosen for better
    // I18N.  US-ASCII was chosen, since it is the intersection of UTF-8
    // and ISO-8859-1 and since it is already used by MIME.)
    //
    // ...
    //
    // The charset parameter of text/xml-external-parsed-entity is
    // handled the same as that of text/xml as described in Section 3.1
    //
    // ...
    //
    // The following list applies to text/xml, text/xml-external-parsed-
    // entity, and XML-based media types under the top-level type "text"
    // that define the charset parameter according to this specification:
    //
    // - If the charset parameter is not specified, the default is "us-
    //   ascii".  The default of "iso-8859-1" in HTTP is explicitly
    //   overridden.
    //
    // ...
    //
    // Omitting the charset parameter is NOT RECOMMENDED for text/xml.  For
    // example, even if the contents of the XML MIME entity are UTF-16 or
    // UTF-8, or the XML MIME entity has an explicit encoding declaration,
    // XML and MIME processors MUST assume the charset is "us-ascii".

    if (LCharSet = '') and IsHeaderMediaType(AContentType, 'text') then begin {do not localize}
      S := ExtractHeaderMediaSubType(AContentType);
      if (PosInStrArray(S, ['xml', 'xml-external-parsed-entity'], False) >= 0) or TextEndsWith(S, '+xml') then begin {do not localize}
        LCharSet := 'us-ascii'; {do not localize}
      end else begin
        LCharSet := 'ISO-8859-1'; {do not localize}
      end;
    end;

    {RLebeau: override the current CharSet only if the header specifies a new value}
    if LCharSet <> '' then begin
      ACharSet := LCharSet;
    end;
  end else begin
    AContentType := '';
    ACharSet := '';
  end;
end;

initialization
  // AnsiPos does not handle strings with #0 and is also very slow compared to Pos
  if LeadBytes = [] then begin
    IndyPos := SBPos;
  end else begin
    IndyPos := InternalAnsiPos;
  end;

end.
