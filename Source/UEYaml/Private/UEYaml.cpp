// Copyright AnnBuildTools. All Rights Reserved.
//
// Implementation of the shared YAML <-> FJsonObject adapter.
//
// ryml_all.hpp is included here in DECLARATION-ONLY form. The single TU that
// instantiates rapidyaml's implementation is UEYamlImpl.cpp. Dependents never
// see this header, so there is exactly one compiled copy of rapidyaml in the
// whole project and no cross-DLL symbol problem.

#include "UEYaml.h"

#include "ryml_all.hpp"

#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"

#include <string>

namespace UEYaml
{
	namespace
	{
		using c4::yml::id_type;
		using c4::yml::Tree;

		FString Indent(int32 Spaces)
		{
			FString Out;
			Out.Reserve(Spaces);
			for (int32 Index = 0; Index < Spaces; ++Index)
			{
				Out.AppendChar(TEXT(' '));
			}
			return Out;
		}

		// Quote EVERY string. YAML plain scalars treat ': ', '#', '/', '(', '['
		// and leading punctuation as syntax; double quotes remove that whole
		// class of bug for UE paths, gameplay tags and CJK tutorial text.
		void AppendQuoted(const FString& In, FString& Out)
		{
			static const TCHAR Hex[] = TEXT("0123456789abcdef");
			Out.AppendChar(TEXT('"'));
			for (int32 Index = 0; Index < In.Len(); ++Index)
			{
				const TCHAR Ch = In[Index];
				switch (Ch)
				{
				case TEXT('\\'): Out += TEXT("\\\\"); break;
				case TEXT('"'):  Out += TEXT("\\\""); break;
				case TEXT('\n'): Out += TEXT("\\n"); break;
				case TEXT('\r'): Out += TEXT("\\r"); break;
				case TEXT('\t'): Out += TEXT("\\t"); break;
				default:
					if (Ch < 0x20)
					{
						Out += TEXT("\\u00");
						Out.AppendChar(Hex[(Ch >> 4) & 0xF]);
						Out.AppendChar(Hex[Ch & 0xF]);
					}
					else
					{
						Out.AppendChar(Ch);
					}
					break;
				}
			}
			Out.AppendChar(TEXT('"'));
		}

		bool IsContainer(const TSharedPtr<FJsonValue>& Value)
		{
			return Value.IsValid()
				&& (Value->Type == EJson::Object || Value->Type == EJson::Array);
		}

		bool IsEmptyContainer(const TSharedPtr<FJsonValue>& Value)
		{
			if (!Value.IsValid())
			{
				return true;
			}
			if (Value->Type == EJson::Object)
			{
				return !Value->AsObject().IsValid() || Value->AsObject()->Values.Num() == 0;
			}
			if (Value->Type == EJson::Array)
			{
				return Value->AsArray().Num() == 0;
			}
			return false;
		}

		void EmitScalar(const TSharedPtr<FJsonValue>& Value, FString& Out)
		{
			if (!Value.IsValid() || Value->Type == EJson::None || Value->Type == EJson::Null)
			{
				Out += TEXT("null");
				return;
			}
			switch (Value->Type)
			{
			case EJson::String:  AppendQuoted(Value->AsString(), Out); break;
			case EJson::Number:  Out += FString::Printf(TEXT("%.15g"), Value->AsNumber()); break;
			case EJson::Boolean: Out += Value->AsBool() ? TEXT("true") : TEXT("false"); break;
			default:             Out += TEXT("null"); break;
			}
		}

		void EmitValue(const TSharedPtr<FJsonValue>& Value, int32 Level, FString& Out);

		void EmitObject(const FJsonObject& Obj, int32 Level, FString& Out)
		{
			const FString Pad = Indent(Level);
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Obj.Values)
			{
				Out += Pad;
				AppendQuoted(Pair.Key, Out);
				Out += TEXT(":");
				const TSharedPtr<FJsonValue>& Child = Pair.Value;
				if (IsContainer(Child) && !IsEmptyContainer(Child))
				{
					Out += TEXT("\n");
					EmitValue(Child, Level + 2, Out);
				}
				else
				{
					Out += TEXT(" ");
					if (IsEmptyContainer(Child))
					{
						Out += (Child.IsValid() && Child->Type == EJson::Array)
							? TEXT("[]") : TEXT("{}");
					}
					else
					{
						EmitScalar(Child, Out);
					}
					Out += TEXT("\n");
				}
			}
		}

		void EmitArray(const TArray<TSharedPtr<FJsonValue>>& Array, int32 Level, FString& Out)
		{
			const FString Pad = Indent(Level);
			for (const TSharedPtr<FJsonValue>& Item : Array)
			{
				Out += Pad;
				Out += TEXT("-");
				if (IsContainer(Item) && !IsEmptyContainer(Item))
				{
					Out += TEXT("\n");
					EmitValue(Item, Level + 2, Out);
				}
				else
				{
					Out += TEXT(" ");
					if (IsEmptyContainer(Item))
					{
						Out += (Item.IsValid() && Item->Type == EJson::Array)
							? TEXT("[]") : TEXT("{}");
					}
					else
					{
						EmitScalar(Item, Out);
					}
					Out += TEXT("\n");
				}
			}
		}

		void EmitValue(const TSharedPtr<FJsonValue>& Value, int32 Level, FString& Out)
		{
			if (!Value.IsValid())
			{
				Out += Indent(Level);
				Out += TEXT("null\n");
				return;
			}
			if (Value->Type == EJson::Object)
			{
				const TSharedPtr<FJsonObject> Obj = Value->AsObject();
				if (Obj.IsValid() && Obj->Values.Num() > 0)
				{
					EmitObject(*Obj, Level, Out);
					return;
				}
				Out += Indent(Level);
				Out += TEXT("{}\n");
				return;
			}
			if (Value->Type == EJson::Array)
			{
				if (Value->AsArray().Num() > 0)
				{
					EmitArray(Value->AsArray(), Level, Out);
					return;
				}
				Out += Indent(Level);
				Out += TEXT("[]\n");
				return;
			}
			Out += Indent(Level);
			EmitScalar(Value, Out);
			Out += TEXT("\n");
		}

		FString ToFString(c4::csubstr Scalar)
		{
			if (Scalar.len == 0 || Scalar.str == nullptr)
			{
				return FString();
			}
			const std::string Bytes(Scalar.str, Scalar.len);
			return FString(UTF8_TO_TCHAR(Bytes.c_str()));
		}

		std::string ToUtf8(const FString& Text)
		{
			const FTCHARToUTF8 Conv(*Text);
			return std::string(reinterpret_cast<const char*>(Conv.Get()),
				static_cast<size_t>(Conv.Length()));
		}

		// YAML 1.2 core-schema-ish resolution. ryml returns raw scalar views;
		// a quoted scalar must stay a string even when it looks numeric.
		TSharedPtr<FJsonValue> ScalarValue(const Tree& TreeRef, id_type Id)
		{
			if (!TreeRef.has_val(Id))
			{
				return MakeShared<FJsonValueNull>();
			}
			const c4::csubstr Raw = TreeRef.val(Id);
			if (TreeRef.is_val_quoted(Id))
			{
				return MakeShared<FJsonValueString>(ToFString(Raw));
			}
			const std::string Bytes(Raw.str, Raw.len);
			if (Bytes.empty() || Bytes == "null" || Bytes == "~"
				|| Bytes == "Null" || Bytes == "NULL")
			{
				return MakeShared<FJsonValueNull>();
			}
			if (Bytes == "true" || Bytes == "True" || Bytes == "TRUE")
			{
				return MakeShared<FJsonValueBoolean>(true);
			}
			if (Bytes == "false" || Bytes == "False" || Bytes == "FALSE")
			{
				return MakeShared<FJsonValueBoolean>(false);
			}
			double Number = 0.0;
			if (c4::from_chars(Raw, &Number))
			{
				return MakeShared<FJsonValueNumber>(Number);
			}
			return MakeShared<FJsonValueString>(FString(UTF8_TO_TCHAR(Bytes.c_str())));
		}

		TSharedPtr<FJsonValue> NodeValue(const Tree& TreeRef, id_type Id)
		{
			if (TreeRef.is_map(Id))
			{
				TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
				for (id_type Child = TreeRef.first_child(Id);
					Child != c4::yml::NONE;
					Child = TreeRef.next_sibling(Child))
				{
					if (!TreeRef.has_key(Child))
					{
						continue;
					}
					Obj->SetField(ToFString(TreeRef.key(Child)), NodeValue(TreeRef, Child));
				}
				return MakeShared<FJsonValueObject>(Obj);
			}
			if (TreeRef.is_seq(Id))
			{
				TArray<TSharedPtr<FJsonValue>> Array;
				for (id_type Child = TreeRef.first_child(Id);
					Child != c4::yml::NONE;
					Child = TreeRef.next_sibling(Child))
				{
					Array.Add(NodeValue(TreeRef, Child));
				}
				return MakeShared<FJsonValueArray>(Array);
			}
			return ScalarValue(TreeRef, Id);
		}

		id_type ContentRoot(const Tree& TreeRef)
		{
			id_type Id = TreeRef.root_id();
			for (int32 Guard = 0; Guard < 4; ++Guard)
			{
				if (!TreeRef.is_stream(Id) && !TreeRef.is_doc(Id))
				{
					break;
				}
				if (!TreeRef.has_children(Id))
				{
					break;
				}
				Id = TreeRef.first_child(Id);
			}
			return Id;
		}

		// Deliberately NO try/catch: UE builds with exceptions disabled and
		// rapidyaml is exception-agnostic. See the malformed-input note in
		// Public/UEYaml.h.
		TSharedPtr<FJsonObject> ParseOne(const FString& Text)
		{
			if (Text.IsEmpty())
			{
				return nullptr;
			}
			std::string Buffer = ToUtf8(Text);
			if (Buffer.empty())
			{
				return nullptr;
			}
			Tree Parsed = c4::yml::parse_in_place(c4::to_substr(Buffer));
			if (Parsed.empty())
			{
				return nullptr;
			}
			const id_type Root = ContentRoot(Parsed);
			if (!Parsed.is_map(Root))
			{
				return nullptr;
			}
			const TSharedPtr<FJsonValue> Value = NodeValue(Parsed, Root);
			return (Value.IsValid() && Value->Type == EJson::Object) ? Value->AsObject() : nullptr;
		}

		FString StripBom(FString Text)
		{
			if (Text.Len() > 0 && Text[0] == static_cast<TCHAR>(0xFEFF))
			{
				Text.RemoveAt(0);
			}
			return Text;
		}
	}

	FString ToYaml(const TSharedRef<FJsonObject>& Root)
	{
		FString Out;
		if (Root->Values.Num() == 0)
		{
			return TEXT("{}\n");
		}
		EmitObject(*Root, 0, Out);
		return Out;
	}

	bool FromYaml(const FString& Text, TSharedPtr<FJsonObject>& Out)
	{
		Out.Reset();
		Out = ParseOne(StripBom(Text));
		return Out.IsValid();
	}

	bool EmitYaml(const TSharedRef<FJsonObject>& Dom, FString& OutYamlText)
	{
		OutYamlText = ToYaml(Dom);
		return true;
	}

	bool ParseYaml(const FString& YamlText, TSharedPtr<FJsonObject>& OutDom)
	{
		return FromYaml(YamlText, OutDom);
	}

	FString EmitDocument(const TSharedRef<FJsonObject>& Obj)
	{
		return ToYaml(Obj);
	}

	TSharedPtr<FJsonObject> ParseDocument(const FString& YamlText)
	{
		return ParseOne(StripBom(YamlText));
	}

	FString EmitStream(const TArray<FString>& Docs)
	{
		FString Out;
		for (const FString& Doc : Docs)
		{
			Out += TEXT("---\n");
			Out += Doc;
			if (!Doc.EndsWith(TEXT("\n")))
			{
				Out += TEXT("\n");
			}
		}
		return Out;
	}

	TArray<TSharedPtr<FJsonObject>> ParseStream(const FString& YamlText)
	{
		TArray<TSharedPtr<FJsonObject>> Out;
		TArray<FString> Lines;
		YamlText.ParseIntoArrayLines(Lines);

		FString Current;
		auto Flush = [&]()
		{
			if (!Current.TrimStartAndEnd().IsEmpty())
			{
				const TSharedPtr<FJsonObject> Doc = ParseOne(Current);
				if (Doc.IsValid())
				{
					Out.Add(Doc);
				}
			}
			Current.Reset();
		};

		for (const FString& Line : Lines)
		{
			const bool bSeparator = Line.StartsWith(TEXT("---"))
				&& (Line.Len() == 3 || Line[3] == TEXT(' ') || Line[3] == TEXT('\t')
					|| Line[3] == TEXT('\r') || Line[3] == TEXT('#'));
			if (bSeparator)
			{
				Flush();
				continue;
			}
			Current += Line;
			Current += TEXT("\n");
		}
		Flush();
		return Out;
	}

	bool LoadFile(const FString& Path, TSharedPtr<FJsonObject>& Out)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			return false;
		}
		return FromYaml(StripBom(Text), Out);
	}

	bool SaveFile(const FString& Path, const TSharedRef<FJsonObject>& Root)
	{
		return FFileHelper::SaveStringToFile(ToYaml(Root), *Path,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	bool LoadFileStream(const FString& Path, TArray<TSharedPtr<FJsonObject>>& Out)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			return false;
		}
		Out = ParseStream(StripBom(Text));
		return Out.Num() > 0;
	}

	bool SaveFileStream(const FString& Path, const TArray<FString>& Docs)
	{
		return FFileHelper::SaveStringToFile(EmitStream(Docs), *Path,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
}
