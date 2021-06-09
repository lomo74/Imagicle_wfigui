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

#ifndef JsonH
#define JsonH
//---------------------------------------------------------------------------

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/encodings.h>
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>
//---------------------------------------------------------------------------

typedef rapidjson::GenericDocument<rapidjson::UTF16<>> JsonDocument;
typedef rapidjson::GenericValue<rapidjson::UTF16<>> JsonValue;
typedef rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF16<>, rapidjson::ASCII<>> JsonASCIIWriter;
typedef rapidjson::GenericPointer<JsonValue> JsonPointer;
//---------------------------------------------------------------------------

class IJsonSerializable
{
public:
	virtual void Deserialize(const JsonValue *val) = 0;
};

#endif
