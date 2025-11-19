#pragma once

// MSVC warns every time you use fopen() because they want you to use their
// MSVC-exclusive fopen_s() for some reason. We're disabling that here.
#pragma warning(disable : 4996)

// MSVC complains about C99 Flexible Array Members in C headers included in C++,
// even when marked with extern "C" {}. The warning it gives just describes the
// expected behaviour of a FAM, which is not a real problem.
#pragma warning(disable : 4200)
