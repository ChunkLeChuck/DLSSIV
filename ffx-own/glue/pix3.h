// OUR OWN FILE (not Microsoft's, not AMD's): a stand-in for WinPixEventRuntime's pix3.h, which is not in the Windows
// SDK and is not downloaded.
//
// AMD's backend/dx12/ffx_dx12.cpp includes <pix3.h> and takes one thing from it, the PIX_COLOR macro, for one constant.
// The PIX functions themselves are looked up at run time with GetProcAddress, and only when a caller hands the backend
// the path of a PIX library (ffxLoadPixDll), which the mod never does. The value is the one PIX's own header gives:
// opaque alpha, then red, green, blue.
#pragma once
#ifndef PIX_COLOR
#define PIX_COLOR(r, g, b) ((UINT)(0xff000000u | ((UINT)(r) << 16) | ((UINT)(g) << 8) | (UINT)(b)))
#endif
