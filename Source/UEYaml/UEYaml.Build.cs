// Copyright AnnBuildTools. All Rights Reserved.
//
// UEYaml is the single owner of rapidyaml in this project. Dependents only
// include Public/UEYaml.h and never see ryml_all.hpp, so the amalgamated
// header is compiled exactly once, in Private/UEYamlImpl.cpp.

using System.IO;
using UnrealBuildTool;

public class UEYaml : ModuleRules
{
	public UEYaml(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;
		Type = ModuleType.CPlusPlus;

		// Only UEYamlImpl.cpp may define RYML_SINGLE_HDR_DEFINE_NOW. Unity build
		// would merge that TU with others and leak the macro, so keep it isolated.
		bUseUnity = false;

		// FJsonObject is the in-memory DOM on purpose: DAP, LSP, MCP and the
		// engine's own tool schemas all speak FJsonObject. Only the on-disk
		// text is YAML.
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Json",
			"JsonUtilities",
		});

		// PRIVATE on purpose: dependents must not include ryml_all.hpp directly.
		// ModuleDirectory is <Plugin>/Source/UEYaml, so ../.. reaches <Plugin>.
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "RapidYaml"));

		// Third-party rapidyaml: UE turns upstream diagnostics into errors
		// (C4668 "__GNUC__/__CPPUNWIND not defined in #if", C4996 aligned_storage
		// under C++23, shadowed names). Follow Luau.Build.cs from the same
		// engine tree and switch these categories Off rather than Warning.
#if UE_5_6_OR_LATER
		CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off;
		CppCompileWarningSettings.ShadowVariableWarningLevel = WarningLevel.Off;
#elif UE_5_5_OR_LATER
		UndefinedIdentifierWarningLevel = WarningLevel.Off;
		ShadowVariableWarningLevel = WarningLevel.Off;
#else
		bEnableUndefinedIdentifierWarnings = false;
		bEnableShadowVariableWarnings = false;
#endif
		PrivateDefinitions.Add("_SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING=1");

		// Keep rapidyaml on its non-throwing default error path so this plugin
		// does not depend on the target's /EH setting. Note that
		// RYML_DEFAULT_CALLBACK_USES_EXCEPTIONS is tested with #ifdef (not by
		// value): defining it, even as "=0", would switch the default error
		// handler to throwing std::runtime_error. Leave it UNDEFINED.
		// See the malformed-input note in Public/UEYaml.h; ParseStream isolates
		// documents to limit the blast radius of a bad document.
	}
}
