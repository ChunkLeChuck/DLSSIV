// dt_fake.h -- a fake IDirect3DDevice9 and its resource classes for dt_test.cpp.
//
// Every class derives from d3d9.h's own interface, so the compiler builds each
// vtable and checks every signature: the device thread's wrappers are called
// through these vtables by the test and call back into them, and a wrapper whose
// argument list differed from d3d9.h's would corrupt the stack here.
//
// THE LOG. Every call that must reach DXVK IN ORDER is appended to gFk with its
// slot, its arguments and a copy of any data they point at (a pointer argument
// itself is logged as 0, since the device thread hands DXVK its own copy). Left
// out, because their order is not promised: AddRef and Release of anything, the
// PASS calls (GetSwapChain and both GetBackBuffers among them), the fourteen Gets
// the shadow answers (the declaration, the FVF and the pixel float constants joined
// the eleven on 2026-09-19), NOOVERWRITE locks of DEFAULT-pool buffers and buffer
// unlocks. dt_test.cpp logs what it SENT by the same rules into gExp, and the two
// must be identical.
//
// THE FAKE IS DXVK 3.1 wherever the layer depends on DXVK (d3d9_device.cpp): a null
// stream zeroes its offset and stride; SetViewport lifts MaxZ above MinZ; a UP draw
// refuses a zero stride, and draws nothing for no primitives (or, indexed, no
// vertices), both before it unbinds; EndStateBlock(nullptr) is refused and the
// recording goes on; NOOVERWRITE counts only on a DEFAULT buffer; and a texture
// level's count is its texture's; THE FVF IS THE BOUND DECLARATION'S, fixed when the
// object is made, SetFVF binds the device's own table declaration for the value and
// SetFVF(0) does nothing (d3d9_device.cpp, SetFVF and GetFVF). Left out on purpose:
// DXVK's refusal of a UP draw with no declaration bound, which the layer's UP draw
// wrappers do not consult the shadow for. The test never makes one.
//
// LIFETIME. An object whose count reaches 0 is marked dead and never freed. Any
// method on a dead object, and any draw with a dead object bound, is an error.
// ONE EXCEPTION, ON PURPOSE: a dead DECLARATION's memory can be handed out again by
// CreateVertexDeclaration when the test asks (gFkReuseDecl), with a different FVF.
// That is a real heap reusing an address, which is the one way the layer's
// per-object FVF memory could answer for the wrong object.
// A STATE BLOCK HOLDS WHAT IT CAPTURED PRIVATELY (2026-09-28, C1), as DXVK 3.1's does (d3d9_state.h's Com<T, false>,
// TextureChangePrivate in d3d9_stateblock.cpp): an object is dead when its public count AND its private count are both 0.
// The device's own bindings stay non-owning here, on purpose: that is the stricter rule, and the lifetime checks above
// were built on it.
//
// THE STATE BLOCK IS DXVK 3.1's (2026-09-28, C1; d3d9_stateblock.h/.cpp, d3d9_device.cpp), for every entry FkState models:
// a capture set (m_captures) and values (m_state); BeginStateBlock makes a recorder that every recordable Set DXVK accepts
// goes into INSTEAD of the device; CreateStateBlock's masks are DXVK's three lists; Capture re-reads only the capture set
// and never a stream's offset; Apply skips a captured null declaration and goes through the device's own rules (a null
// stream zeroes offset and stride, MaxZ is lifted); both are refused while any block records, and so are Create and a
// second Begin; a Reset is refused while a block lives (after clearing the state, as DXVK does). Until this date the
// fake snapshotted the whole device for any block, which would have passed a wrong design. That fake is kept behind
// gFkSnapshot as the CONTROL the conformance cases in dt_test.cpp must fail, each by name.
#pragma once
#include <vector>
#include <atomic>
#include <unordered_set>

static std::atomic<int> gErr{ 0 };
static void Err(const char* fmt, ...)
{
    if (gErr++ < 40) { va_list a; va_start(a, fmt); printf("  ERROR: "); vprintf(fmt, a); printf("\n"); va_end(a); }
}

// THE THREE DEVICE CALLS WINDOWS' COMPATIBILITY LAYER HOOKS (1.0.8): QueryInterface, CreateAdditionalSwapChain, Reset. The layer
// installs over apphelp.dll's hooks on them because it makes them on the calling thread only (dt_install.h, DtForeignSlotOk): the
// fake counts each call and counts one made on the worker, which the run must never see.
static volatile LONG gFkCaller3[3] = { 0, 0, 0 }, gFkOnWorker3 = 0;
static volatile LONG gFkRelease = 0;   // every Release that reached the fake device: Windows' layer hooks that slot too, and the worker may make it
static DWORD FkWorkerTid();            // dt_test.cpp, after the layer's headers: the worker's thread id
static void FkCaller3(int k)
{
    InterlockedIncrement(&gFkCaller3[k]);
    const DWORD w = FkWorkerTid();
    if (w && GetCurrentThreadId() == w) InterlockedIncrement(&gFkOnWorker3);
}

struct FkRec { int slot; DWORD tid; std::vector<DWORD> a; std::vector<BYTE> data; };
static bool operator==(const FkRec& x, const FkRec& y) { return x.slot == y.slot && x.a == y.a && x.data == y.data; }
static CRITICAL_SECTION gFkLock;
static std::vector<FkRec> gFk, gExp;
static void LogTo(std::vector<FkRec>& v, int slot, std::initializer_list<DWORD> a,
                  const void* d1 = nullptr, size_t n1 = 0, const void* d2 = nullptr, size_t n2 = 0)
{
    FkRec r; r.slot = slot; r.tid = GetCurrentThreadId(); r.a = a;
    if (d1 && n1) r.data.insert(r.data.end(), (const BYTE*)d1, (const BYTE*)d1 + n1);
    if (d2 && n2) r.data.insert(r.data.end(), (const BYTE*)d2, (const BYTE*)d2 + n2);
    EnterCriticalSection(&gFkLock); v.push_back(std::move(r)); LeaveCriticalSection(&gFkLock);
}
#define FK(slot, ...) LogTo(gFk, slot, { __VA_ARGS__ })
static void (*gFkMidRead)() = nullptr;         // one shot, fired from inside GetRenderTarget(3): a hook INTO the validator's whole-state read
static void (*gFkAfterSet)() = nullptr;        // one shot, fired from inside SetRenderState once the state HAS changed and before the call returns
static volatile LONG gFkRefusedRt = 0;          // SetRenderTarget calls the fake refused: they arrive outside the ordered log
// Every object whose public count reached 0. Objects are never freed, so a
// pointer in here is exactly "this was already dead when the call arrived".
static std::unordered_set<const void*> gDead;
static void MarkDead(const void* p) { EnterCriticalSection(&gFkLock); gDead.insert(p); LeaveCriticalSection(&gFkLock); }
static bool IsDead(const void* p)
{
    if (!p) return false;
    EnterCriticalSection(&gFkLock); const bool d = gDead.count(p) != 0; LeaveCriticalSection(&gFkLock);
    return d;
}
static DWORD P(const void* p) { return (DWORD)(UINT_PTR)p; }
static DWORD F(float f) { DWORD d; memcpy(&d, &f, 4); return d; }
static unsigned long long FkVerts(D3DPRIMITIVETYPE t, UINT pc)
{
    const unsigned long long n = pc;
    switch (t) { case D3DPT_POINTLIST: return n; case D3DPT_LINELIST: return n * 2; case D3DPT_LINESTRIP: return n + 1;
                 case D3DPT_TRIANGLELIST: return n * 3; case D3DPT_TRIANGLESTRIP: case D3DPT_TRIANGLEFAN: return n + 2; default: return 0; }
}
// A UP draw's data size, or 0 for a nonsense one (a count and a stride whose product
// wraps): the test sends those only to prove the layer does not try to copy them, and
// the fake then logs the call without its data.
static const unsigned long long kFkUpMax = 1u << 20;
static size_t FkUpBytes(unsigned long long n, unsigned long long each)
{ return (n <= kFkUpMax && each <= kFkUpMax && n * each <= kFkUpMax) ? (size_t)(n * each) : 0; }

struct IDirect3DDevice9;
static IDirect3DDevice9* gDev = nullptr;
static int gFkSleepEvery = 0;           // > 0: the fake sleeps on one call in this many (varies the worker's pace)
static void FkMaybeSleep() { static std::atomic<unsigned> n{ 0 }; if (gFkSleepEvery && (++n % gFkSleepEvery) == 0) Sleep(0); }
static volatile LONG gFkPresentHr = 0;  // the next swapchain Present's answer, taken once (dt_test.cpp sets it)
static volatile LONG gFkDrawHr = 0;     // the next DrawPrimitive's answer, taken once: a recorded DEVICE call that fails on the worker
static void (*gFkOnReset)() = nullptr;  // runs inside the fake device's Reset, before it: the test asserts what must already be dead
static void (*gFkOnGetDecl)() = nullptr;      // runs inside GetVertexDeclaration, before it answers: another thread's call lands between the drain and DXVK's answer (C1FvfForeign)
static void (*gFkOnGetTexture)() = nullptr;   // runs inside GetTexture, before it answers: the post's fetch is the only direct caller that matters (NoteAcrossDisarm)

static bool gFkSnapshot = false;        // THE OLD FAKE'S STATE BLOCK, whole-device snapshots: the conformance cases' control only

// ---- resources ---------------------------------------------------------------------------
// priv: the references state blocks hold (see LIFETIME above). Either count reaching 0 with the other at 0 kills it.
#define FK_UNK(NAME)                                                                              \
    LONG ref = 1; LONG priv = 0; bool dead = false;                                               \
    STDMETHOD(QueryInterface)(REFIID, void** pp) override { *pp = this; AddRef(); return S_OK; }  \
    STDMETHOD_(ULONG, AddRef)() override { if (dead) Err("AddRef on a dead " NAME); return (ULONG)InterlockedIncrement(&ref); } \
    STDMETHOD_(ULONG, Release)() override { const LONG r = InterlockedDecrement(&ref); if (r < 0) Err("a " NAME " released below zero"); if (r == 0 && *(volatile LONG*)&priv == 0) { dead = true; MarkDead(this); } return (ULONG)r; } \
    void PrivAdd() { if (dead) Err("a state block took a dead " NAME); InterlockedIncrement(&priv); }                     \
    void PrivRel() { const LONG p = InterlockedDecrement(&priv); if (p < 0) Err("a " NAME "'s private count went below zero"); if (p == 0 && *(volatile LONG*)&ref == 0) { dead = true; MarkDead(this); } }
#define FK_ALIVE(NAME) if (dead) Err("a method on a dead " NAME)
#define FK_RES(TYPE)                                                                               \
    IDirect3DDevice9* fkDev = nullptr;          /* set for an object of another device */         \
    STDMETHOD(GetDevice)(IDirect3DDevice9** pp) override { *pp = fkDev ? fkDev : gDev; ((IUnknown*)*pp)->AddRef(); return S_OK; } \
    STDMETHOD(SetPrivateData)(REFGUID, const void*, DWORD, DWORD) override { return S_OK; }        \
    STDMETHOD(GetPrivateData)(REFGUID, void*, DWORD*) override { return D3DERR_NOTFOUND; }         \
    STDMETHOD(FreePrivateData)(REFGUID) override { return S_OK; }                                  \
    STDMETHOD_(DWORD, SetPriority)(DWORD) override { return 0; }                                   \
    STDMETHOD_(DWORD, GetPriority)() override { return 0; }                                        \
    STDMETHOD_(void, PreLoad)() override {}                                                        \
    STDMETHOD_(D3DRESOURCETYPE, GetType)() override { return TYPE; }
static BYTE gLockMem[1 << 16];

static bool gFkDcOk = false;                   // a surface's GetDC succeeds (it refuses, as it always did, without)
struct FakeSurf : IDirect3DSurface9
{
    UINT w, h; IUnknown* owner; DWORD usage;       // a texture level forwards its count, as DXVK's does
    FakeSurf(UINT W, UINT H, IUnknown* o, DWORD u = 0) : w(W), h(H), owner(o), usage(u) {}
    LONG ref = 1; bool dead = false;
    STDMETHOD(QueryInterface)(REFIID, void** pp) override { *pp = this; AddRef(); return S_OK; }
    STDMETHOD_(ULONG, AddRef)() override { if (owner) return owner->AddRef(); if (dead) Err("AddRef on a dead surface"); return (ULONG)InterlockedIncrement(&ref); }
    STDMETHOD_(ULONG, Release)() override { if (owner) return owner->Release(); const LONG r = InterlockedDecrement(&ref); if (r < 0) Err("a surface released below zero"); if (r == 0) { dead = true; MarkDead(this); } return (ULONG)r; }
    FK_RES(D3DRTYPE_SURFACE)
    // As DXVK's (d3d9_subresource.h): a level answers with its texture, anything else has none.
    STDMETHOD(GetContainer)(REFIID, void** pp) override { if (!owner) { *pp = nullptr; return E_NOINTERFACE; } *pp = owner; owner->AddRef(); return S_OK; }
    STDMETHOD(GetDesc)(D3DSURFACE_DESC* d) override { memset(d, 0, sizeof(*d)); d->Width = w; d->Height = h; d->Usage = usage; return S_OK; }
    STDMETHOD(LockRect)(D3DLOCKED_RECT* r, const RECT* rc, DWORD f) override { FK_ALIVE("surface"); FK(1513, P(this), rc ? 1u : 0u, f); r->pBits = gLockMem; r->Pitch = 64; return S_OK; }
    STDMETHOD(UnlockRect)() override { FK(1514, P(this)); return S_OK; }
    STDMETHOD(GetDC)(HDC* h) override { FK(1515, P(this)); if (gFkDcOk) { *h = (HDC)(UINT_PTR)1; return S_OK; } *h = nullptr; return D3DERR_INVALIDCALL; }   // gFkDcOk: dt_test's back buffer case needs one that succeeds
    STDMETHOD(ReleaseDC)(HDC) override { FK(1516, P(this)); return S_OK; }
};

template <class I, D3DRESOURCETYPE T, int BASE> struct FakeTexBase : I
{
    FK_UNK("texture")
    FK_RES(T)
    STDMETHOD_(DWORD, SetLOD)(DWORD l) override { FK(BASE + 11, P(this), l); return 0; }
    STDMETHOD_(DWORD, GetLOD)() override { return 0; }
    STDMETHOD_(DWORD, GetLevelCount)() override { return 1; }
    STDMETHOD(SetAutoGenFilterType)(D3DTEXTUREFILTERTYPE f) override { FK(BASE + 14, P(this), (DWORD)f); return S_OK; }
    STDMETHOD_(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)() override { return D3DTEXF_LINEAR; }
    STDMETHOD_(void, GenerateMipSubLevels)() override { FK(BASE + 16, P(this)); }
};
struct FakeTex : FakeTexBase<IDirect3DTexture9, D3DRTYPE_TEXTURE, 1200>
{
    FakeSurf lvl;
    FakeTex(UINT w, UINT h, DWORD u = 0) : lvl(w, h, this, u) {}
    STDMETHOD(GetLevelDesc)(UINT, D3DSURFACE_DESC* d) override { return lvl.GetDesc(d); }
    STDMETHOD(GetSurfaceLevel)(UINT, IDirect3DSurface9** pp) override { FK_ALIVE("texture"); *pp = &lvl; AddRef(); return S_OK; }
    STDMETHOD(LockRect)(UINT l, D3DLOCKED_RECT* r, const RECT* rc, DWORD f) override { FK_ALIVE("texture"); FK(1219, P(this), l, rc ? 1u : 0u, f); r->pBits = gLockMem; r->Pitch = 64; return S_OK; }
    STDMETHOD(UnlockRect)(UINT l) override { FK(1220, P(this), l); return S_OK; }
    STDMETHOD(AddDirtyRect)(const RECT*) override { FK(1221, P(this)); return S_OK; }
};
// A texture level is dead when its texture is; a standalone surface by itself.
static bool IsDeadSurf(const IDirect3DSurface9* s)
{
    if (!s) return false;
    const FakeSurf* f = (const FakeSurf*)s;
    return f->owner ? IsDead(f->owner) : IsDead(f);
}
struct FakeCube : FakeTexBase<IDirect3DCubeTexture9, D3DRTYPE_CUBETEXTURE, 1300>
{
    FakeSurf face{ 4, 4, this };
    STDMETHOD(GetLevelDesc)(UINT, D3DSURFACE_DESC* d) override { return face.GetDesc(d); }
    STDMETHOD(GetCubeMapSurface)(D3DCUBEMAP_FACES, UINT, IDirect3DSurface9** pp) override { *pp = &face; AddRef(); return S_OK; }
    STDMETHOD(LockRect)(D3DCUBEMAP_FACES fc, UINT l, D3DLOCKED_RECT* r, const RECT*, DWORD f) override { FK(1319, P(this), (DWORD)fc, l, f); r->pBits = gLockMem; r->Pitch = 64; return S_OK; }
    STDMETHOD(UnlockRect)(D3DCUBEMAP_FACES fc, UINT l) override { FK(1320, P(this), (DWORD)fc, l); return S_OK; }
    STDMETHOD(AddDirtyRect)(D3DCUBEMAP_FACES fc, const RECT*) override { FK(1321, P(this), (DWORD)fc); return S_OK; }
};
struct FakeVol : IDirect3DVolume9
{
    IUnknown* owner = nullptr;
    STDMETHOD(QueryInterface)(REFIID, void** pp) override { *pp = this; AddRef(); return S_OK; }
    STDMETHOD_(ULONG, AddRef)() override { return owner->AddRef(); }
    STDMETHOD_(ULONG, Release)() override { return owner->Release(); }
    STDMETHOD(GetDevice)(IDirect3DDevice9** pp) override { *pp = gDev; ((IUnknown*)gDev)->AddRef(); return S_OK; }
    STDMETHOD(SetPrivateData)(REFGUID, const void*, DWORD, DWORD) override { return S_OK; }
    STDMETHOD(GetPrivateData)(REFGUID, void*, DWORD*) override { return D3DERR_NOTFOUND; }
    STDMETHOD(FreePrivateData)(REFGUID) override { return S_OK; }
    STDMETHOD(GetContainer)(REFIID, void** pp) override { *pp = owner; owner->AddRef(); return S_OK; }
    STDMETHOD(GetDesc)(D3DVOLUME_DESC* d) override { memset(d, 0, sizeof(*d)); return S_OK; }
    STDMETHOD(LockBox)(D3DLOCKED_BOX* b, const D3DBOX*, DWORD f) override { FK(1609, P(this), f); b->pBits = gLockMem; b->RowPitch = 64; b->SlicePitch = 256; return S_OK; }
    STDMETHOD(UnlockBox)() override { FK(1610, P(this)); return S_OK; }
};
struct FakeVTex : FakeTexBase<IDirect3DVolumeTexture9, D3DRTYPE_VOLUMETEXTURE, 1400>
{
    FakeVol vol;
    FakeVTex() { vol.owner = this; }
    STDMETHOD(GetLevelDesc)(UINT, D3DVOLUME_DESC* d) override { return vol.GetDesc(d); }
    STDMETHOD(GetVolumeLevel)(UINT, IDirect3DVolume9** pp) override { *pp = &vol; AddRef(); return S_OK; }
    STDMETHOD(LockBox)(UINT l, D3DLOCKED_BOX* b, const D3DBOX*, DWORD f) override { FK(1419, P(this), l, f); b->pBits = gLockMem; b->RowPitch = 64; b->SlicePitch = 256; return S_OK; }
    STDMETHOD(UnlockBox)(UINT l) override { FK(1420, P(this), l); return S_OK; }
    STDMETHOD(AddDirtyBox)(const D3DBOX*) override { FK(1421, P(this)); return S_OK; }
};
template <class I, D3DRESOURCETYPE T, int BASE> struct FakeBuf : I
{
    FK_UNK("buffer")
    FK_RES(T)
    D3DPOOL pool = D3DPOOL_DEFAULT;
    // A NOOVERWRITE lock of a DEFAULT buffer (with DISCARD or without: DXVK drops the
    // DISCARD) and every unlock run out of order on purpose: not logged. Any other
    // pool loses NOOVERWRITE in DXVK, so there the lock is an ordered one.
    STDMETHOD(Lock)(UINT o, UINT n, void** pp, DWORD f) override
    {
        FK_ALIVE("buffer");
        if (!((f & D3DLOCK_NOOVERWRITE) && pool == D3DPOOL_DEFAULT)) FK(BASE + 11, P(this), o, n, f);
        *pp = gLockMem; return S_OK;
    }
    STDMETHOD(Unlock)() override { FK_ALIVE("buffer"); return S_OK; }
};
struct FakeVB : FakeBuf<IDirect3DVertexBuffer9, D3DRTYPE_VERTEXBUFFER, 1000>
{ STDMETHOD(GetDesc)(D3DVERTEXBUFFER_DESC* d) override { memset(d, 0, sizeof(*d)); d->Pool = pool; return S_OK; } };
struct FakeIB : FakeBuf<IDirect3DIndexBuffer9, D3DRTYPE_INDEXBUFFER, 1100>
{ STDMETHOD(GetDesc)(D3DINDEXBUFFER_DESC* d) override { memset(d, 0, sizeof(*d)); d->Pool = pool; return S_OK; } };
struct FakeVS : IDirect3DVertexShader9
{ FK_UNK("vertex shader") STDMETHOD(GetDevice)(IDirect3DDevice9** pp) override { *pp = gDev; ((IUnknown*)gDev)->AddRef(); return S_OK; }
  STDMETHOD(GetFunction)(void*, UINT* n) override { *n = 0; return S_OK; } };
struct FakePS : IDirect3DPixelShader9
{ FK_UNK("pixel shader") STDMETHOD(GetDevice)(IDirect3DDevice9** pp) override { *pp = gDev; ((IUnknown*)gDev)->AddRef(); return S_OK; }
  STDMETHOD(GetFunction)(void*, UINT* n) override { *n = 0; return S_OK; } };
struct FakeDecl : IDirect3DVertexDeclaration9
{ FK_UNK("declaration") DWORD fvf = 0;
  STDMETHOD(GetDevice)(IDirect3DDevice9** pp) override { *pp = gDev; ((IUnknown*)gDev)->AddRef(); return S_OK; }
  STDMETHOD(GetDeclaration)(D3DVERTEXELEMENT9*, UINT* n) override { *n = 0; return S_OK; } };
struct FakeQuery : IDirect3DQuery9
{
    FK_UNK("query")
    STDMETHOD(GetDevice)(IDirect3DDevice9** pp) override { *pp = gDev; ((IUnknown*)gDev)->AddRef(); return S_OK; }
    STDMETHOD_(D3DQUERYTYPE, GetType)() override { return D3DQUERYTYPE_EVENT; }
    STDMETHOD_(DWORD, GetDataSize)() override { return 4; }
    // GetData IS NOT IN THE ORDERED LOG (2026-09-19): the layer waits for this query's own Issue
    // and then asks beside the worker, so it may arrive ahead of calls recorded after that Issue.
    // What it may NEVER do is arrive ahead of the Issue itself, and the fake keeps the evidence:
    // how many Issues had reached the device when the last GetData did.
    volatile LONG issues = 0; LONG seenAtGet = -1;
    STDMETHOD(Issue)(DWORD f) override { FK_ALIVE("query"); FK(1706, P(this), f); InterlockedIncrement(&issues); return S_OK; }
    STDMETHOD(GetData)(void* p, DWORD n, DWORD) override { FK_ALIVE("query"); seenAtGet = issues; if (p && n >= 4) *(DWORD*)p = 1; return S_OK; }
};

// FOUR KINDS THE SHADOW NEVER ANSWERS (2026-09-28, C1 step 2): the vertex float constants, the texture stage states, the
// transforms and the stream frequencies. Their Gets are SYNC calls, so they read DXVK after a drain, and until today the fake
// answered them with zeros: a Get made after a queued state-block Apply could not have been checked. They are modelled now,
// by DXVK 3.1's rules (d3d9_device.cpp, d3d9_util.h): a stage state's stage is clamped to 7 and its type is stored at
// type - 1, clamped to 31 (RemapTextureStageStateType, SetStateTextureStageState); a transform lives at DXVK's index
// (GetTransformIndex), and the fake keeps VIEW, PROJECTION, TEXTURE0..7 and WORLD..WORLDMATRIX(3), the fourteen a game uses,
// answering zeros for the rest; a frequency is validated first, and a block's Apply goes through that same check.
static bool FkFreqOk(UINT i, UINT f)
{
    const bool inst = (f & D3DSTREAMSOURCE_INSTANCEDATA) != 0, idx = (f & D3DSTREAMSOURCE_INDEXEDDATA) != 0;
    return i < 16 && f && !(i == 0 && inst) && !(inst && idx);
}
static int FkXf(DWORD t)
{
    if (t == D3DTS_VIEW) return 0;
    if (t == D3DTS_PROJECTION) return 1;
    if (t >= D3DTS_TEXTURE0 && t <= D3DTS_TEXTURE7) return 2 + (int)(t - D3DTS_TEXTURE0);
    if (t >= 256 && t <= 259) return 10 + (int)(t - 256);
    return -1;
}
static DWORD FkTssIdx(DWORD t) { const DWORD k = t - 1; return k < 31 ? k : 31; }

// The state the shadow follows, with DXVK 3.1's side effects: a colour target at
// slot 0 resets the viewport and the scissor, a UP draw unbinds stream 0 (and the
// indices, for the indexed one), a null stream zeroes its offset and stride, and a
// viewport's MaxZ is lifted above its MinZ. dt_test.cpp keeps a second copy as the
// reference, updated in program order as it makes each call.
struct FkState
{
    DWORD rs[256]; DWORD samp[21][14]; IDirect3DBaseTexture9* tex[21]; IDirect3DSurface9* rt[4]; IDirect3DSurface9* ds;
    D3DVIEWPORT9 vp; RECT sc; IDirect3DVertexShader9* vs; IDirect3DPixelShader9* ps;
    IDirect3DVertexBuffer9* vb[16]; UINT off[16], str[16]; IDirect3DIndexBuffer9* ib;
    IDirect3DVertexDeclaration9* decl; float psc[224][4];
    float vsc[256][4]; DWORD tss[8][32]; UINT freq[16]; D3DMATRIX xf[14];   // the four kinds above
    DWORD Fvf() const { return decl ? ((const FakeDecl*)decl)->fvf : 0; }
    void Rt(DWORD i, IDirect3DSurface9* s)
    {
        rt[i] = s;
        if (i || !s) return;
        const FakeSurf* f = (const FakeSurf*)s;
        vp = { 0, 0, f->w, f->h, 0.0f, 1.0f }; sc = { 0, 0, (LONG)f->w, (LONG)f->h };
    }
    void Up(bool idx) { vb[0] = nullptr; off[0] = 0; str[0] = 0; if (idx) ib = nullptr; }
    void Stream(UINT i, IDirect3DVertexBuffer9* v, UINT o, UINT s) { vb[i] = v; off[i] = v ? o : 0; str[i] = v ? s : 0; }
    void Vp(const D3DVIEWPORT9& v) { vp = v; vp.MaxZ = v.MinZ < v.MaxZ ? v.MaxZ : v.MinZ + 0.001f; }
    void Freq(UINT i, UINT f) { if (FkFreqOk(i, f)) freq[i] = f; }
    // A device's state after creation or a Reset, as far as the fake models it: zeros, but for DXVK's stream frequency of 1
    // (d3d9_state.cpp), and the viewport and scissor the back buffer sets.
    void Reset(IDirect3DSurface9* bb) { memset(this, 0, sizeof(*this)); for (int i = 0; i < 16; i++) freq[i] = 1; Rt(0, bb); }
};
static int FkSamp(DWORD s) { return s < 16 ? (int)s : (s >= 256 && s <= 260) ? (int)(s - 240) : -1; }

// THE DEVICE'S TABLE OF FVF DECLARATIONS, DXVK's m_fvfTable: made on first use, kept for
// the life of the device, never emptied (not by a Reset either). Find-or-make under the
// log's lock, because the test asks for the object at the moment it RECORDS a SetFVF (its
// reference needs the pointer) and the worker asks when it REPLAYS it; either may be first
// and both must get the same object.
static std::vector<FakeDecl*> gFkFvfTab;
// ONE BROKEN DXVK RULE, for C1's probe's controls (dt_sbtab.h, DtSbProbeRun; 2026-09-29): 1 a Capture takes a stream's
// offset, 2 an Apply applies a null declaration, 3 an applied null stream keeps its offset and stride, 4 an applied viewport is
// not lifted, 5 an Apply is accepted while a block records, 6 SetFVF of one value makes a new declaration each time, 7 an Apply
// writes a render state outside its block, 8 a Capture is accepted while a block records. 0, always outside that one case: none.
static int gFkBreak = 0;
static FakeDecl* FkFvfDecl(DWORD f)
{
    if (gFkBreak == 6) { FakeDecl* x = new FakeDecl; x->fvf = f; return x; }
    EnterCriticalSection(&gFkLock);
    FakeDecl* d = nullptr;
    for (FakeDecl* x : gFkFvfTab) if (x->fvf == f) { d = x; break; }
    if (!d) { d = new FakeDecl; d->fvf = f; gFkFvfTab.push_back(d); }
    LeaveCriticalSection(&gFkLock);
    return d;
}
// The table's declaration for f if the device has made it, and nothing otherwise (C1's oracle must not make one).
static FakeDecl* FkFvfFind(DWORD f)
{
    EnterCriticalSection(&gFkLock);
    FakeDecl* d = nullptr;
    for (FakeDecl* x : gFkFvfTab) if (x->fvf == f) { d = x; break; }
    LeaveCriticalSection(&gFkLock);
    return d;
}
static FakeDecl* gFkReuseDecl = nullptr;   // the next CreateVertexDeclaration hands out THIS dead object's memory
static DWORD     gFkNextDeclFvf = 0;       // ...and every one made gets this FVF (the test varies it)

// A BLOCK'S CAPTURE SET, DXVK's D3D9StateCaptures for the entries FkState models: a bit per render state, per sampler
// state (by FkSamp's index, which is DXVK's own: d3d9_util.h), per texture, per stream, per pixel and vertex float
// register, per stage state, per stream frequency and per modelled transform, and a flag for each of the rest. The kinds
// FkState does not model (lights, the integer and bool constants, clip planes, the material) DXVK records as well; the
// test never sets them, and a Get of one is a drain that reads DXVK's answer.
struct FkCap
{
    DWORD rs[8]; WORD samp[21]; DWORD tex, vb; DWORD psc[7];
    bool decl, ib, vs, ps, vp, sc;
    DWORD vsc[8], tss[8]; WORD freq, xf;        // the four kinds above: a bit a register, a stage's types, a stream, a matrix
    bool Rs(DWORD s) const        { return (rs[s >> 5] >> (s & 31) & 1u) != 0; }
    bool Samp(int i, DWORD t) const { return (samp[i] >> t & 1u) != 0; }
    bool Tex(int i) const         { return (tex >> i & 1u) != 0; }
    bool Vb(UINT i) const         { return (vb >> i & 1u) != 0; }
    bool Psc(UINT r) const        { return (psc[r >> 5] >> (r & 31) & 1u) != 0; }
    bool Vsc(UINT r) const        { return (vsc[r >> 5] >> (r & 31) & 1u) != 0; }
    bool Tss(UINT g, DWORD k) const { return (tss[g] >> k & 1u) != 0; }
    bool Freq(UINT i) const       { return (freq >> i & 1u) != 0; }
    bool Xf(int k) const          { return (xf >> k & 1u) != 0; }
};
struct FakeSB : IDirect3DStateBlock9
{
    LONG ref = 1; bool dead = false;
    FkCap cap{}; FkState s{};                   // m_captures and m_state
    bool counted = false;                       // in its device's live count (DXVK's losable counter): made by Create or End
    BYTE made = 0;                              // 1 CreateStateBlock (typed), 2 BeginStateBlock (recorded): C1's oracle holds the record's kind to it
    bool snapAll = false, snapNoop = false;     // the SNAPSHOT fake's two kinds (gFkSnapshot): a whole-device copy, a recorded no-op
    IDirect3DDevice9* fkDev = nullptr;          // set for a block of another device
    STDMETHOD(QueryInterface)(REFIID, void** pp) override { *pp = this; AddRef(); return S_OK; }
    STDMETHOD_(ULONG, AddRef)() override { if (dead) Err("AddRef on a dead state block"); return (ULONG)InterlockedIncrement(&ref); }
    STDMETHOD_(ULONG, Release)() override;      // dt_fake_dev.h: at 0 it lets go of everything it holds
    STDMETHOD(GetDevice)(IDirect3DDevice9** pp) override { *pp = fkDev ? fkDev : gDev; ((IUnknown*)*pp)->AddRef(); return S_OK; }
    STDMETHOD(Capture)() override;
    STDMETHOD(Apply)() override;
};
// THE SAME ADDRESS, A NEW BLOCK (2026-09-28, C1's H4): the next CreateStateBlock or BeginStateBlock hands out THIS dead block's
// memory, as a real allocator would, so a record kept by address meets a block that is not the one it learned.
static FakeSB* gFkReuseSB = nullptr;
static FakeSB* FkNewSB()
{
    FakeSB* x = gFkReuseSB; gFkReuseSB = nullptr;
    if (!x) return new FakeSB;
    if (!IsDead(x)) Err("the test asked for a state block's memory back while it was alive");
    EnterCriticalSection(&gFkLock); gDead.erase(x); LeaveCriticalSection(&gFkLock);
    x->ref = 1; x->dead = false; x->cap = FkCap{}; x->s = FkState{}; x->counted = false; x->snapAll = x->snapNoop = false; x->fkDev = nullptr; x->made = 0;
    return x;
}
// A Set of a render state from this thread sleeps this long inside the fake before it lands: a call held IN FLIGHT (C1's H5 (d)).
static volatile DWORD gFkSlowSetTid = 0; static volatile LONG gFkSlowSetMs = 0, gFkSlowSetIn = 0;   // In: the Set is inside, asleep
static volatile LONG gFkApplyHr = 0;    // the next state block Apply's answer, taken once: a RECORDED Apply that fails on the worker (C1)
static volatile LONG gFkResetRefused = 0;   // Resets refused because a block lived: dt_test.cpp expects none outside its conformance cases
struct FakeSC : IDirect3DSwapChain9
{
    FK_UNK("swapchain")
    FakeSurf bb{ 1920, 1080, nullptr, D3DUSAGE_RENDERTARGET };
    STDMETHOD(Present)(const RECT* s, const RECT* t, HWND w, const RGNDATA*, DWORD f) override
    {
        FK_ALIVE("swapchain"); LogTo(gFk, 1803, { P(w), f }, s, s ? sizeof(RECT) : 0, t, t ? sizeof(RECT) : 0);
        return (HRESULT)_InterlockedExchange(&gFkPresentHr, 0);
    }
    STDMETHOD(GetFrontBufferData)(IDirect3DSurface9* t) override { FK(1804, P(t)); return S_OK; }
    STDMETHOD(GetBackBuffer)(UINT, D3DBACKBUFFER_TYPE, IDirect3DSurface9** pp) override { *pp = &bb; bb.AddRef(); return S_OK; }   // PASS: not logged
    STDMETHOD(GetRasterStatus)(D3DRASTER_STATUS* r) override { memset(r, 0, sizeof(*r)); return S_OK; }
    STDMETHOD(GetDisplayMode)(D3DDISPLAYMODE* m) override { memset(m, 0, sizeof(*m)); return S_OK; }
    STDMETHOD(GetDevice)(IDirect3DDevice9** pp) override { *pp = gDev; ((IUnknown*)gDev)->AddRef(); return S_OK; }
    STDMETHOD(GetPresentParameters)(D3DPRESENT_PARAMETERS* p) override { memset(p, 0, sizeof(*p)); return S_OK; }
};
