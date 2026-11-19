// Copyright AnnBuildTools. All Rights Reserved.
//
// UEYaml.h -- the only YAML entry point for UEAbilityKit, UEAssetLens and
// UnLuaNova. No rapidyaml types cross this boundary.
//
// Contract:
//   * FJsonObject is the in-memory DOM. Only the serialized text is YAML.
//   * UTF-8, no BOM. Scalars are always double-quoted, so UE paths
//     (/Script/...), GameplayTags, '::' enums, '#' and CJK text can never
//     change YAML's indentation or scalar semantics.
//   * Emission order follows FJsonObject::Values, which is stable for a given
//     insertion sequence within one process (round-trip assertions rely on it).
//   * Malformed input: rapidyaml's default error callback aborts. Producers
//     must emit well-formed YAML; ParseStream isolates documents so one bad
//     document does not corrupt the rest of a stream.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

namespace UEYaml
{
	/** Emit one YAML document body (no "---"), ending with a newline. */
	UEYAML_API FString ToYaml(const TSharedRef<FJsonObject>& Root);

	/** Parse one YAML document into an FJsonObject. Returns false on malformed/empty input. */
	UEYAML_API bool FromYaml(const FString& Text, TSharedPtr<FJsonObject>& Out);

	/** Aliases kept so callers can migrate mechanically from per-plugin helpers. */
	UEYAML_API bool EmitYaml(const TSharedRef<FJsonObject>& Dom, FString& OutYamlText);
	UEYAML_API bool ParseYaml(const FString& YamlText, TSharedPtr<FJsonObject>& OutDom);
	UEYAML_API FString EmitDocument(const TSharedRef<FJsonObject>& Obj);
	UEYAML_API TSharedPtr<FJsonObject> ParseDocument(const FString& YamlText);

	/** Multi-document YAML stream ("---"-separated), used by UEAssetLens staging. */
	UEYAML_API FString EmitStream(const TArray<FString>& Docs);
	UEYAML_API TArray<TSharedPtr<FJsonObject>> ParseStream(const FString& YamlText);

	/** UTF-8 without BOM file I/O. LoadFile tolerates a leading BOM. */
	UEYAML_API bool LoadFile(const FString& Path, TSharedPtr<FJsonObject>& Out);
	UEYAML_API bool SaveFile(const FString& Path, const TSharedRef<FJsonObject>& Root);
	UEYAML_API bool LoadFileStream(const FString& Path, TArray<TSharedPtr<FJsonObject>>& Out);
	UEYAML_API bool SaveFileStream(const FString& Path, const TArray<FString>& Docs);
}
