// Copyright AnnBuildTools. All Rights Reserved.
//
// The ONE translation unit in this project that instantiates rapidyaml's
// implementation. Every other TU includes ryml_all.hpp without this macro
// (declarations only). UEYaml.Build.cs sets bUseUnity=false so unity build
// cannot merge this file with another TU and leak the macro.

#define RYML_SINGLE_HDR_DEFINE_NOW
#include "ryml_all.hpp"
#undef RYML_SINGLE_HDR_DEFINE_NOW
#undef C4CORE_SINGLE_HDR_DEFINE_NOW
