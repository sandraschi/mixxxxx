#pragma once

// Mixxxxx includes NDI MIT headers for dynamic runtime load only — never link NDI libs.
// PROCESSINGNDILIB_STATIC yields plain C declarations (no dllimport) while we still load
// the runtime DLL manually via LoadLibrary / NDIlib_v5_load().
#define PROCESSINGNDILIB_STATIC
#include "Processing.NDI.Lib.h"
#undef PROCESSINGNDILIB_STATIC
