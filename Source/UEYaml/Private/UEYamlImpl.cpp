// Copyright AnnBuildTools. All Rights Reserved.
//
// The ONE translation unit in this project that instantiates rapidyaml's
// implementation. Every other TU includes ryml_all.hpp without this macro
// (declarations only). UEYaml.Build.cs sets bUseUnity=false so unity build
// cannot merge this file with another TU and leak the macro.

// Isolate UE's DEFAULTS macro for the same reason as UEYaml.cpp: it collides
// with c4::yml::ParserOptions::DEFAULTS in the implementation section too.
#pragma push_macro("DEFAULTS")
#undef DEFAULTS
#define RYML_SINGLE_HDR_DEFINE_NOW
#include "ryml_all.hpp"
#undef RYML_SINGLE_HDR_DEFINE_NOW
#pragma pop_macro("DEFAULTS")
#undef C4CORE_SINGLE_HDR_DEFINE_NOW
