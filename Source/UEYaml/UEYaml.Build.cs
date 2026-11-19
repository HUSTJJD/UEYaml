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

		// Unreal builds with C++ exceptions disabled. rapidyaml is
		// exception-agnostic; keep its default error callbacks on the abort
		// path rather than the throwing path (which would not compile here).
		PublicDefinitions.Add("RYML_DEFAULT_CALLBACK_USES_EXCEPTIONS=0");
		PublicDefinitions.Add("C4CORE_DEFAULT_CALLBACK_USES_EXCEPTIONS=0");
	}
}
