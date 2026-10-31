// dt_fake_dev.h -- the fake device for dt_test.cpp. See dt_fake.h for the rules.
#pragma once

#define ALIVE(p)  do { if (IsDead(p)) Err("slot %d was handed a dead object %p", SLOT, (void*)(p)); } while (0)
#define ALIVES(p) do { if (IsDeadSurf(p)) Err("slot %d was handed a dead surface %p", SLOT, (void*)(p)); } while (0)
#define M(ret, name) STDMETHOD_(ret, name)

// ---- a state block's private references and its recorder (DXVK 3.1's D3D9StateBlock, 2026-09-28) ----------------------
// Every pointer a block's m_state holds is a private reference (Com<T, false>, TextureChangePrivate), taken as it is stored
// and let go of as it is replaced or the block dies. The block calls no method of the object that the layer hooks.
static void FkPrivTex(IDirect3DBaseTexture9* t, bool add)
{
    if (!t) return;
    switch (t->GetType())
    {
    case D3DRTYPE_TEXTURE:       if (add) ((FakeTex*)t)->PrivAdd();  else ((FakeTex*)t)->PrivRel();  break;
    case D3DRTYPE_CUBETEXTURE:   if (add) ((FakeCube*)t)->PrivAdd(); else ((FakeCube*)t)->PrivRel(); break;
    case D3DRTYPE_VOLUMETEXTURE: if (add) ((FakeVTex*)t)->PrivAdd(); else ((FakeVTex*)t)->PrivRel(); break;
    default: Err("a state block was handed a texture of type %d", (int)t->GetType());
    }
}
static void FkPriv(IDirect3DBaseTexture9* o, bool a)        { FkPrivTex(o, a); }
static void FkPriv(IDirect3DVertexBuffer9* o, bool a)       { if (o) { if (a) ((FakeVB*)o)->PrivAdd();   else ((FakeVB*)o)->PrivRel(); } }
static void FkPriv(IDirect3DIndexBuffer9* o, bool a)        { if (o) { if (a) ((FakeIB*)o)->PrivAdd();   else ((FakeIB*)o)->PrivRel(); } }
static void FkPriv(IDirect3DVertexShader9* o, bool a)       { if (o) { if (a) ((FakeVS*)o)->PrivAdd();   else ((FakeVS*)o)->PrivRel(); } }
static void FkPriv(IDirect3DPixelShader9* o, bool a)        { if (o) { if (a) ((FakePS*)o)->PrivAdd();   else ((FakePS*)o)->PrivRel(); } }
static void FkPriv(IDirect3DVertexDeclaration9* o, bool a)  { if (o) { if (a) ((FakeDecl*)o)->PrivAdd(); else ((FakeDecl*)o)->PrivRel(); } }
template <class T> static void FkHold(T*& slot, T* v) { if (slot == v) return; FkPriv(v, true); FkPriv(slot, false); slot = v; }

// THE RECORDER: what DXVK stores in a block for each Set it diverts there, RAW (the device's rules are applied at Apply).
// A stream keeps a null buffer's offset and stride here; the viewport is not lifted. The snapshot fake records nothing.
static void FkRecRs(FakeSB* b, DWORD s, DWORD v)            { if (b->snapNoop) return; b->cap.rs[s >> 5] |= 1u << (s & 31); b->s.rs[s] = v; }
static void FkRecSamp(FakeSB* b, int i, DWORD t, DWORD v)   { if (b->snapNoop) return; b->cap.samp[i] |= (WORD)(1u << t); b->s.samp[i][t] = v; }
static void FkRecTex(FakeSB* b, int i, IDirect3DBaseTexture9* t) { if (b->snapNoop) return; b->cap.tex |= 1u << i; FkHold(b->s.tex[i], t); }
static void FkRecVb(FakeSB* b, UINT i, IDirect3DVertexBuffer9* v, UINT o, UINT str)
{ if (b->snapNoop) return; b->cap.vb |= 1u << i; FkHold(b->s.vb[i], v); b->s.off[i] = o; b->s.str[i] = str; }
static void FkRecIb(FakeSB* b, IDirect3DIndexBuffer9* x)    { if (b->snapNoop) return; b->cap.ib = true; FkHold(b->s.ib, x); }
static void FkRecVs(FakeSB* b, IDirect3DVertexShader9* x)   { if (b->snapNoop) return; b->cap.vs = true; FkHold(b->s.vs, x); }
static void FkRecPs(FakeSB* b, IDirect3DPixelShader9* x)    { if (b->snapNoop) return; b->cap.ps = true; FkHold(b->s.ps, x); }
static void FkRecDecl(FakeSB* b, IDirect3DVertexDeclaration9* x) { if (b->snapNoop) return; b->cap.decl = true; FkHold(b->s.decl, x); }
static void FkRecVp(FakeSB* b, const D3DVIEWPORT9& v)       { if (b->snapNoop) return; b->cap.vp = true; b->s.vp = v; }
static void FkRecSc(FakeSB* b, const RECT& r)               { if (b->snapNoop) return; b->cap.sc = true; b->s.sc = r; }
static void FkRecPsc(FakeSB* b, UINT s, const float* d, UINT n)
{ if (b->snapNoop) return; for (UINT r = s; r < s + n; r++) b->cap.psc[r >> 5] |= 1u << (r & 31); memcpy(b->s.psc[s], d, n * 16); }
static void FkRecVsc(FakeSB* b, UINT s, const float* d, UINT n)
{ if (b->snapNoop) return; for (UINT r = s; r < s + n; r++) b->cap.vsc[r >> 5] |= 1u << (r & 31); memcpy(b->s.vsc[s], d, n * 16); }
static void FkRecTss(FakeSB* b, UINT g, DWORD k, DWORD v) { if (b->snapNoop) return; b->cap.tss[g] |= 1u << k; b->s.tss[g][k] = v; }
static void FkRecFreq(FakeSB* b, UINT i, UINT f)          { if (b->snapNoop) return; b->cap.freq |= (WORD)(1u << i); b->s.freq[i] = f; }
static void FkRecXf(FakeSB* b, int k, const D3DMATRIX& m) { if (b->snapNoop) return; b->cap.xf |= (WORD)(1u << k); b->s.xf[k] = m; }

// CAPTURE: each captured entry takes the device's current value, and nothing outside the capture set is touched
// (stateblock.cpp:48-61). A declaration is taken even when it is null (:55-56). A STREAM'S OFFSET IS TAKEN ONLY AT A TYPED
// BLOCK'S CREATION (<Capture, false>, :580-585); Capture() takes the buffer and the stride and leaves the offset as it was
// (<Capture, true>, stateblock.h:218-230).
static void FkCaptureFrom(FakeSB* b, const FkState& d, bool offsets)
{
    const FkCap& c = b->cap;
    if (c.decl) FkHold(b->s.decl, d.decl);
    for (DWORD s = 0; s < 256; s++) if (c.Rs(s)) b->s.rs[s] = d.rs[s];
    for (int i = 0; i < 21; i++) for (DWORD t = 1; t <= 13; t++) if (c.Samp(i, t)) b->s.samp[i][t] = d.samp[i][t];
    for (UINT i = 0; i < 16; i++) if (c.Vb(i)) { FkHold(b->s.vb[i], d.vb[i]); b->s.str[i] = d.str[i]; if (offsets) b->s.off[i] = d.off[i]; }
    for (int i = 0; i < 21; i++) if (c.Tex(i)) FkHold(b->s.tex[i], d.tex[i]);
    if (c.vs) FkHold(b->s.vs, d.vs);
    if (c.ps) FkHold(b->s.ps, d.ps);
    if (c.vp) b->s.vp = d.vp;
    if (c.sc) b->s.sc = d.sc;
    if (c.ib) FkHold(b->s.ib, d.ib);
    for (UINT r = 0; r < 224; r++) if (c.Psc(r)) memcpy(b->s.psc[r], d.psc[r], 16);
    for (UINT r = 0; r < 256; r++) if (c.Vsc(r)) memcpy(b->s.vsc[r], d.vsc[r], 16);
    for (UINT g = 0; g < 8; g++) for (DWORD k = 0; k < 32; k++) if (c.Tss(g, k)) b->s.tss[g][k] = d.tss[g][k];
    for (UINT i = 0; i < 16; i++) if (c.Freq(i)) b->s.freq[i] = d.freq[i];
    for (int k = 0; k < 14; k++) if (c.Xf(k)) b->s.xf[k] = d.xf[k];
}
// APPLY: through the device's own Set rules, which is what DXVK's direct calls into its Set implementations amount to
// (stateblock.cpp:64-77, stateblock.h ApplyOrCapture). A captured null declaration is never restored; a null stream zeroes
// its offset and stride; MaxZ is lifted; a frequency the device would refuse (a fresh fake's 0) is not applied, because
// DXVK applies one through the public SetStreamSourceFreq. Never a render target or the depth stencil: a block has no
// member for them.
static void FkApplyTo(const FakeSB* b, FkState& d)
{
    const FkCap& c = b->cap;
    if (c.decl && (b->s.decl || gFkBreak == 2)) d.decl = b->s.decl;
    for (DWORD s = 0; s < 256; s++) if (c.Rs(s)) d.rs[s] = b->s.rs[s];
    if (gFkBreak == 7) d.rs[D3DRS_TEXTUREFACTOR] = b->s.rs[D3DRS_TEXTUREFACTOR];
    for (int i = 0; i < 21; i++) for (DWORD t = 1; t <= 13; t++) if (c.Samp(i, t)) d.samp[i][t] = b->s.samp[i][t];
    for (UINT i = 0; i < 16; i++)
        if (c.Vb(i)) { if (gFkBreak == 3) { d.vb[i] = b->s.vb[i]; d.off[i] = b->s.off[i]; d.str[i] = b->s.str[i]; } else d.Stream(i, b->s.vb[i], b->s.off[i], b->s.str[i]); }
    for (int i = 0; i < 21; i++) if (c.Tex(i)) d.tex[i] = b->s.tex[i];
    if (c.vs) d.vs = b->s.vs;
    if (c.ps) d.ps = b->s.ps;
    if (c.vp) { if (gFkBreak == 4) d.vp = b->s.vp; else d.Vp(b->s.vp); }
    if (c.sc) d.sc = b->s.sc;
    if (c.ib) d.ib = b->s.ib;
    for (UINT r = 0; r < 224; r++) if (c.Psc(r)) memcpy(d.psc[r], b->s.psc[r], 16);
    for (UINT r = 0; r < 256; r++) if (c.Vsc(r)) memcpy(d.vsc[r], b->s.vsc[r], 16);
    for (UINT g = 0; g < 8; g++) for (DWORD k = 0; k < 32; k++) if (c.Tss(g, k)) d.tss[g][k] = b->s.tss[g][k];
    for (UINT i = 0; i < 16; i++) if (c.Freq(i)) d.Freq(i, b->s.freq[i]);
    for (int k = 0; k < 14; k++) if (c.Xf(k)) d.xf[k] = b->s.xf[k];
}
// THE THREE TYPED LISTS (stateblock.cpp:364-586), for the kinds FkState models. PIXEL: 61 render states, samplers 0..16
// (the DMAP slot included) with states 1..12, the pixel shader and all its constants, every stage state of every stage.
// VERTEX: 45 render states, samplers 17..20 with DMAPOFFSET only, the vertex shader and all 256 of its float constants,
// the declaration, every stream frequency. ALL: both, and every texture and stream, the indices, the viewport, the
// scissor and every transform. The names are the SDK's, as DXVK writes them.
static const DWORD kFkPixelRs[] = {
    D3DRS_ZENABLE, D3DRS_FILLMODE, D3DRS_SHADEMODE, D3DRS_ZWRITEENABLE, D3DRS_ALPHATESTENABLE, D3DRS_LASTPIXEL,
    D3DRS_SRCBLEND, D3DRS_DESTBLEND, D3DRS_ZFUNC, D3DRS_ALPHAREF, D3DRS_ALPHAFUNC, D3DRS_DITHERENABLE, D3DRS_FOGSTART,
    D3DRS_FOGEND, D3DRS_FOGDENSITY, D3DRS_ALPHABLENDENABLE, D3DRS_DEPTHBIAS, D3DRS_STENCILENABLE, D3DRS_STENCILFAIL,
    D3DRS_STENCILZFAIL, D3DRS_STENCILPASS, D3DRS_STENCILFUNC, D3DRS_STENCILREF, D3DRS_STENCILMASK, D3DRS_STENCILWRITEMASK,
    D3DRS_TEXTUREFACTOR, D3DRS_WRAP0, D3DRS_WRAP1, D3DRS_WRAP2, D3DRS_WRAP3, D3DRS_WRAP4, D3DRS_WRAP5, D3DRS_WRAP6,
    D3DRS_WRAP7, D3DRS_WRAP8, D3DRS_WRAP9, D3DRS_WRAP10, D3DRS_WRAP11, D3DRS_WRAP12, D3DRS_WRAP13, D3DRS_WRAP14,
    D3DRS_WRAP15, D3DRS_COLORWRITEENABLE, D3DRS_BLENDOP, D3DRS_SCISSORTESTENABLE, D3DRS_SLOPESCALEDEPTHBIAS,
    D3DRS_ANTIALIASEDLINEENABLE, D3DRS_TWOSIDEDSTENCILMODE, D3DRS_CCW_STENCILFAIL, D3DRS_CCW_STENCILZFAIL,
    D3DRS_CCW_STENCILPASS, D3DRS_CCW_STENCILFUNC, D3DRS_COLORWRITEENABLE1, D3DRS_COLORWRITEENABLE2,
    D3DRS_COLORWRITEENABLE3, D3DRS_BLENDFACTOR, D3DRS_SRGBWRITEENABLE, D3DRS_SEPARATEALPHABLENDENABLE,
    D3DRS_SRCBLENDALPHA, D3DRS_DESTBLENDALPHA, D3DRS_BLENDOPALPHA };
static const DWORD kFkVertexRs[] = {
    D3DRS_CULLMODE, D3DRS_FOGENABLE, D3DRS_FOGCOLOR, D3DRS_FOGTABLEMODE, D3DRS_FOGSTART, D3DRS_FOGEND, D3DRS_FOGDENSITY,
    D3DRS_RANGEFOGENABLE, D3DRS_AMBIENT, D3DRS_COLORVERTEX, D3DRS_FOGVERTEXMODE, D3DRS_CLIPPING, D3DRS_LIGHTING,
    D3DRS_LOCALVIEWER, D3DRS_EMISSIVEMATERIALSOURCE, D3DRS_AMBIENTMATERIALSOURCE, D3DRS_DIFFUSEMATERIALSOURCE,
    D3DRS_SPECULARMATERIALSOURCE, D3DRS_VERTEXBLEND, D3DRS_CLIPPLANEENABLE, D3DRS_POINTSIZE, D3DRS_POINTSIZE_MIN,
    D3DRS_POINTSPRITEENABLE, D3DRS_POINTSCALEENABLE, D3DRS_POINTSCALE_A, D3DRS_POINTSCALE_B, D3DRS_POINTSCALE_C,
    D3DRS_MULTISAMPLEANTIALIAS, D3DRS_MULTISAMPLEMASK, D3DRS_PATCHEDGESTYLE, D3DRS_POINTSIZE_MAX,
    D3DRS_INDEXEDVERTEXBLENDENABLE, D3DRS_TWEENFACTOR, D3DRS_POSITIONDEGREE, D3DRS_NORMALDEGREE,
    D3DRS_MINTESSELLATIONLEVEL, D3DRS_MAXTESSELLATIONLEVEL, D3DRS_ADAPTIVETESS_X, D3DRS_ADAPTIVETESS_Y,
    D3DRS_ADAPTIVETESS_Z, D3DRS_ADAPTIVETESS_W, D3DRS_ENABLEADAPTIVETESSELLATION, D3DRS_NORMALIZENORMALS,
    D3DRS_SPECULARENABLE, D3DRS_SHADEMODE };
static_assert(sizeof(kFkPixelRs) / 4 == 61 && sizeof(kFkVertexRs) / 4 == 45, "DXVK's lists are 61 and 45 long");
static void FkTypedCaps(FkCap& c, D3DSTATEBLOCKTYPE t)
{
    const bool px = t == D3DSBT_PIXELSTATE || t == D3DSBT_ALL, vx = t == D3DSBT_VERTEXSTATE || t == D3DSBT_ALL;
    if (px)
    {
        for (DWORD s : kFkPixelRs) c.rs[s >> 5] |= 1u << (s & 31);
        for (int i = 0; i < 17; i++) c.samp[i] |= 0x1FFEu;                     // states 1..12, not DMAPOFFSET
        c.ps = true; memset(c.psc, 0xFF, sizeof(c.psc));
        memset(c.tss, 0xFF, sizeof(c.tss));
    }
    if (vx)
    {
        for (DWORD s : kFkVertexRs) c.rs[s >> 5] |= 1u << (s & 31);
        for (int i = 17; i < 21; i++) c.samp[i] |= (WORD)(1u << D3DSAMP_DMAPOFFSET);
        c.vs = true; c.decl = true;
        memset(c.vsc, 0xFF, sizeof(c.vsc)); c.freq = 0xFFFFu;
    }
    if (t == D3DSBT_ALL) { c.tex = 0x1FFFFFu; c.vb = 0xFFFFu; c.ib = c.vp = c.sc = true; c.xf = 0x3FFFu; }
}

struct FakeDev : IDirect3DDevice9
{
    // rec: DXVK's m_recorder, the block BeginStateBlock made; while it is set, every recordable Set goes into it instead.
    // blocksLive: blocks made by Create or End and not yet dead, which is what makes DXVK refuse a Reset.
    LONG ref = 1; FkState st{}; FakeSB* rec = nullptr; volatile LONG blocksLive = 0; FakeSC* sc = nullptr;
    void Defaults() { st.Reset(&sc->bb); }

    STDMETHOD(QueryInterface)(REFIID, void** pp) override { FkCaller3(0); *pp = this; AddRef(); return S_OK; }
    M(ULONG, AddRef)() override { return (ULONG)InterlockedIncrement(&ref); }
    M(ULONG, Release)() override { InterlockedIncrement(&gFkRelease); return (ULONG)InterlockedDecrement(&ref); }
    M(HRESULT, TestCooperativeLevel)() override { FK(3); return S_OK; }
    M(UINT, GetAvailableTextureMem)() override { FK(4); return 512u << 20; }
    M(HRESULT, EvictManagedResources)() override { FK(5); return S_OK; }
    M(HRESULT, GetDirect3D)(IDirect3D9** pp) override { *pp = nullptr; return D3DERR_INVALIDCALL; }
    M(HRESULT, GetDeviceCaps)(D3DCAPS9* c) override { memset(c, 0, sizeof(*c)); return S_OK; }
    M(HRESULT, GetDisplayMode)(UINT, D3DDISPLAYMODE* m) override { memset(m, 0, sizeof(*m)); return S_OK; }
    // GTA IV's own flags, 0x44: MULTITHREADED with hardware vertex processing, which the layer needs to arm.
    M(HRESULT, GetCreationParameters)(D3DDEVICE_CREATION_PARAMETERS* p) override
    { memset(p, 0, sizeof(*p)); p->BehaviorFlags = D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING; return S_OK; }
#define SLOT 10
    M(HRESULT, SetCursorProperties)(UINT x, UINT y, IDirect3DSurface9* s) override { ALIVES(s); FK(10, x, y, P(s)); return S_OK; }
#undef SLOT
    M(void, SetCursorPosition)(int x, int y, DWORD f) override { FK(11, (DWORD)x, (DWORD)y, f); }
    M(BOOL, ShowCursor)(BOOL b) override { FK(12, (DWORD)b); return FALSE; }
    M(HRESULT, CreateAdditionalSwapChain)(D3DPRESENT_PARAMETERS*, IDirect3DSwapChain9** pp) override { FkCaller3(1); FK(13); *pp = new FakeSC; return S_OK; }
    M(HRESULT, GetSwapChain)(UINT, IDirect3DSwapChain9** pp) override { *pp = sc; sc->AddRef(); return S_OK; }   // PASS: not logged
    M(UINT, GetNumberOfSwapChains)() override { return 1; }
    // DXVK clears the state FIRST and then refuses a Reset while a losable resource lives (d3d9_device.cpp:465-517). Only
    // state blocks are counted here: the phases keep DEFAULT-pool objects across their Resets, which is a stated departure.
    M(HRESULT, Reset)(D3DPRESENT_PARAMETERS*) override
    {
        FkCaller3(2);
        if (gFkOnReset) gFkOnReset();
        FK(16); Defaults();
        if (!gFkSnapshot && blocksLive) { InterlockedIncrement(&gFkResetRefused); return D3DERR_INVALIDCALL; }
        return S_OK;
    }
    // DXVK's device Present calls the swapchain's through its vtable, so the fake does too.
    M(HRESULT, Present)(const RECT* s, const RECT* t, HWND w, const RGNDATA* r) override
    { FK(17, s ? 1u : 0u, t ? 1u : 0u, P(w), r ? 1u : 0u); return ((IDirect3DSwapChain9*)sc)->Present(s, t, w, r, 0); }
    M(HRESULT, GetBackBuffer)(UINT, UINT, D3DBACKBUFFER_TYPE, IDirect3DSurface9** pp) override { *pp = &sc->bb; sc->bb.AddRef(); return S_OK; }   // PASS: not logged
    M(HRESULT, GetRasterStatus)(UINT, D3DRASTER_STATUS* r) override { memset(r, 0, sizeof(*r)); return S_OK; }
    M(HRESULT, SetDialogBoxMode)(BOOL b) override { FK(20, (DWORD)b); return S_OK; }
    M(void, SetGammaRamp)(UINT s, DWORD f, const D3DGAMMARAMP* r) override { LogTo(gFk, 21, { s, f, 1u }, r, sizeof(*r)); }
    M(void, GetGammaRamp)(UINT s, D3DGAMMARAMP* r) override { FK(22, s); memset(r, 0, sizeof(*r)); }
    M(HRESULT, CreateTexture)(UINT w, UINT h, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DTexture9** pp, HANDLE*) override
    { FK(23, w, h, l, u, (DWORD)f, (DWORD)p); *pp = new FakeTex(w, h, u); return S_OK; }
    M(HRESULT, CreateVolumeTexture)(UINT w, UINT h, UINT d, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DVolumeTexture9** pp, HANDLE*) override
    { FK(24, w, h, d, l, u, (DWORD)f, (DWORD)p); *pp = new FakeVTex; return S_OK; }
    M(HRESULT, CreateCubeTexture)(UINT e, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DCubeTexture9** pp, HANDLE*) override
    { FK(25, e, l, u, (DWORD)f, (DWORD)p); *pp = new FakeCube; return S_OK; }
    M(HRESULT, CreateVertexBuffer)(UINT n, DWORD u, DWORD fvf, D3DPOOL p, IDirect3DVertexBuffer9** pp, HANDLE*) override
    { FK(26, n, u, fvf, (DWORD)p); FakeVB* v = new FakeVB; v->pool = p; *pp = v; return S_OK; }
    M(HRESULT, CreateIndexBuffer)(UINT n, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DIndexBuffer9** pp, HANDLE*) override
    { FK(27, n, u, (DWORD)f, (DWORD)p); FakeIB* b = new FakeIB; b->pool = p; *pp = b; return S_OK; }
    M(HRESULT, CreateRenderTarget)(UINT w, UINT h, D3DFORMAT f, D3DMULTISAMPLE_TYPE m, DWORD q, BOOL lk, IDirect3DSurface9** pp, HANDLE*) override
    { FK(28, w, h, (DWORD)f, (DWORD)m, q, (DWORD)lk); *pp = new FakeSurf(w, h, nullptr, D3DUSAGE_RENDERTARGET); return S_OK; }
    M(HRESULT, CreateDepthStencilSurface)(UINT w, UINT h, D3DFORMAT f, D3DMULTISAMPLE_TYPE m, DWORD q, BOOL dc, IDirect3DSurface9** pp, HANDLE*) override
    { FK(29, w, h, (DWORD)f, (DWORD)m, q, (DWORD)dc); *pp = new FakeSurf(w, h, nullptr, D3DUSAGE_DEPTHSTENCIL); return S_OK; }
#define SLOT 30
    M(HRESULT, UpdateSurface)(IDirect3DSurface9* s, const RECT* sr, IDirect3DSurface9* t, const POINT* tp) override
    { ALIVES(s); ALIVES(t); LogTo(gFk, 30, { P(s), sr ? 1u : 0u, P(t), tp ? 1u : 0u }, sr, sr ? sizeof(RECT) : 0, tp, tp ? sizeof(POINT) : 0); return S_OK; }
#undef SLOT
#define SLOT 31
    M(HRESULT, UpdateTexture)(IDirect3DBaseTexture9* s, IDirect3DBaseTexture9* t) override { ALIVE(s); ALIVE(t); FK(31, P(s), P(t)); return S_OK; }
#undef SLOT
    M(HRESULT, GetRenderTargetData)(IDirect3DSurface9* s, IDirect3DSurface9* t) override { FK(32, P(s), P(t)); return S_OK; }
    M(HRESULT, GetFrontBufferData)(UINT s, IDirect3DSurface9* t) override { FK(33, s, P(t)); return S_OK; }
#define SLOT 34
    M(HRESULT, StretchRect)(IDirect3DSurface9* s, const RECT* sr, IDirect3DSurface9* t, const RECT* tr, D3DTEXTUREFILTERTYPE f) override
    { ALIVES(s); ALIVES(t); LogTo(gFk, 34, { P(s), sr ? 1u : 0u, P(t), tr ? 1u : 0u, (DWORD)f }, sr, sr ? sizeof(RECT) : 0, tr, tr ? sizeof(RECT) : 0); return S_OK; }
#undef SLOT
#define SLOT 35
    M(HRESULT, ColorFill)(IDirect3DSurface9* s, const RECT* r, D3DCOLOR c) override { ALIVES(s); LogTo(gFk, 35, { P(s), r ? 1u : 0u, c }, r, r ? sizeof(RECT) : 0); return S_OK; }
#undef SLOT
    M(HRESULT, CreateOffscreenPlainSurface)(UINT w, UINT h, D3DFORMAT f, D3DPOOL p, IDirect3DSurface9** pp, HANDLE*) override
    { FK(36, w, h, (DWORD)f, (DWORD)p); *pp = new FakeSurf(w, h, nullptr); return S_OK; }
#define SLOT 37
    // As DXVK: refused (and nothing changes) past slot 3, null at 0, or not a
    // render target; and applied even while a state block records.
    // A REFUSED ONE IS NOT IN THE ORDERED LOG (2026-09-19): the layer hands it to DXVK at once,
    // beside the worker, because DXVK decides it from the arguments alone and changes nothing.
    // So it arrives out of order by design, like the Gets that are not logged either. Counted,
    // so the test can prove every one it made did reach the device.
    M(HRESULT, SetRenderTarget)(DWORD i, IDirect3DSurface9* s) override
    {
        ALIVES(s);
        if (i >= 4 || (!i && !s) || (s && !(((FakeSurf*)s)->usage & D3DUSAGE_RENDERTARGET))) { InterlockedIncrement(&gFkRefusedRt); return D3DERR_INVALIDCALL; }
        FK(37, i, P(s));
        st.Rt(i, s); return S_OK;
    }
#undef SLOT
    M(HRESULT, GetRenderTarget)(DWORD i, IDirect3DSurface9** pp) override
    { if (i == 3 && gFkMidRead) { void (*f)() = gFkMidRead; gFkMidRead = nullptr; f(); }
      if (i >= 4) return D3DERR_INVALIDCALL; *pp = st.rt[i]; if (!*pp) return D3DERR_NOTFOUND; (*pp)->AddRef(); return S_OK; }
#define SLOT 39
    M(HRESULT, SetDepthStencilSurface)(IDirect3DSurface9* s) override
    { ALIVES(s); FK(39, P(s)); if (s && !(((FakeSurf*)s)->usage & D3DUSAGE_DEPTHSTENCIL)) return D3DERR_INVALIDCALL; st.ds = s; return S_OK; }
#undef SLOT
    M(HRESULT, GetDepthStencilSurface)(IDirect3DSurface9** pp) override { *pp = st.ds; if (!*pp) return D3DERR_NOTFOUND; (*pp)->AddRef(); return S_OK; }
    M(HRESULT, BeginScene)() override { FK(41); return S_OK; }
    M(HRESULT, EndScene)() override { FK(42); return S_OK; }
    // A list is logged as a list (1) even when it is empty: DXVK answers Clear(0, list)
    // D3D_OK and clears nothing, while Clear(0, NULL) clears the viewport.
    // As DXVK 3.1 (d3d9_device.cpp:1910): its ONE refusal is depth or stencil asked for with no depth-stencil bound.
    M(HRESULT, Clear)(DWORD n, const D3DRECT* r, DWORD f, D3DCOLOR c, float z, DWORD s) override
    { LogTo(gFk, 43, { n, r ? 1u : 0u, f, c, F(z), s }, r, r ? n * sizeof(D3DRECT) : 0);
      return (!st.ds && (f & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL))) ? D3DERR_INVALIDCALL : S_OK; }
    M(HRESULT, SetTransform)(D3DTRANSFORMSTATETYPE s, const D3DMATRIX* m) override
    { LogTo(gFk, 44, { (DWORD)s, 1u }, m, 64); const int k = FkXf((DWORD)s); if (k >= 0) { if (rec) FkRecXf(rec, k, *m); else st.xf[k] = *m; } return S_OK; }
    M(HRESULT, GetTransform)(D3DTRANSFORMSTATETYPE s, D3DMATRIX* m) override
    { FK(45, (DWORD)s); const int k = FkXf((DWORD)s); if (k >= 0) *m = st.xf[k]; else memset(m, 0, 64); return S_OK; }
    // NOT MODELLED: the test never multiplies a transform, and DXVK's recorder multiplies against the block's own copy.
    M(HRESULT, MultiplyTransform)(D3DTRANSFORMSTATETYPE s, const D3DMATRIX* m) override { LogTo(gFk, 46, { (DWORD)s, 1u }, m, 64); return S_OK; }
    M(HRESULT, SetViewport)(const D3DVIEWPORT9* v) override
    { if (!v) { FK(47, 0u); return D3DERR_INVALIDCALL; } LogTo(gFk, 47, { 1u }, v, sizeof(*v)); if (rec) FkRecVp(rec, *v); else st.Vp(*v); return S_OK; }
    M(HRESULT, GetViewport)(D3DVIEWPORT9* v) override { *v = st.vp; return S_OK; }
    M(HRESULT, SetMaterial)(const D3DMATERIAL9* m) override { LogTo(gFk, 49, { 1u }, m, sizeof(*m)); return S_OK; }
    M(HRESULT, GetMaterial)(D3DMATERIAL9* m) override { FK(50); memset(m, 0, sizeof(*m)); return S_OK; }
    M(HRESULT, SetLight)(DWORD i, const D3DLIGHT9* l) override { LogTo(gFk, 51, { i, 1u }, l, sizeof(*l)); return S_OK; }
    M(HRESULT, GetLight)(DWORD i, D3DLIGHT9* l) override { FK(52, i); memset(l, 0, sizeof(*l)); return S_OK; }
    M(HRESULT, LightEnable)(DWORD i, BOOL b) override { FK(53, i, (DWORD)b); return S_OK; }
    M(HRESULT, GetLightEnable)(DWORD i, BOOL* b) override { FK(54, i); *b = FALSE; return S_OK; }
    M(HRESULT, SetClipPlane)(DWORD i, const float* p) override { LogTo(gFk, 55, { i, 1u }, p, 16); return S_OK; }
    M(HRESULT, GetClipPlane)(DWORD i, float* p) override { FK(56, i); memset(p, 0, 16); return S_OK; }
    // DXVK answers D3D_OK and does nothing for a state past 255 or from 1 to 6 (d3d9_device.cpp:2318), BEFORE a recording sees it.
    M(HRESULT, SetRenderState)(D3DRENDERSTATETYPE s, DWORD v) override
    {
        FK(57, (DWORD)s, v);
        if (gFkSlowSetTid && GetCurrentThreadId() == gFkSlowSetTid) { _InterlockedExchange(&gFkSlowSetIn, 1); Sleep((DWORD)gFkSlowSetMs); _InterlockedExchange(&gFkSlowSetIn, 0); }
        if ((DWORD)s <= 255 && ((DWORD)s >= 7 || s == 0)) { if (rec) FkRecRs(rec, (DWORD)s, v); else st.rs[s] = v; }
        if (gFkAfterSet) { void (*f)() = gFkAfterSet; gFkAfterSet = nullptr; f(); }
        FkMaybeSleep(); return S_OK;
    }
    M(HRESULT, GetRenderState)(D3DRENDERSTATETYPE s, DWORD* v) override { if ((DWORD)s < 7 || (DWORD)s > 209) return D3DERR_INVALIDCALL; *v = st.rs[s]; return S_OK; }
    // DXVK 3.1 (d3d9_device.cpp:2690-2756): Create is refused while a block records, and for a type that is not one of the
    // three; a typed block takes its values at once, stream offsets included. Begin is refused while one records; End
    // hands the recorder out, or is refused (a null pointer, or nothing recording) and the recording goes on.
    M(HRESULT, CreateStateBlock)(D3DSTATEBLOCKTYPE t, IDirect3DStateBlock9** pp) override
    {
        FK(59, (DWORD)t);
        // A block of a device other than gDev names it, in both fakes: the conformance cases run on devices of their own.
        FakeSB* b = nullptr;
        if (gFkSnapshot) { b = new FakeSB; b->snapAll = true; b->s = st; if ((IDirect3DDevice9*)this != gDev) b->fkDev = this; *pp = b; return S_OK; }
        if (rec || !pp) return D3DERR_INVALIDCALL;
        *pp = nullptr;
        if (t != D3DSBT_ALL && t != D3DSBT_PIXELSTATE && t != D3DSBT_VERTEXSTATE) return D3DERR_INVALIDCALL;
        b = FkNewSB();
        b->made = 1;
        if ((IDirect3DDevice9*)this != gDev) b->fkDev = this;
        FkTypedCaps(b->cap, t);
        FkCaptureFrom(b, st, true);
        b->counted = true; InterlockedIncrement(&blocksLive);
        *pp = b; return S_OK;
    }
    M(HRESULT, BeginStateBlock)() override
    {
        FK(60);
        if (rec) return gFkSnapshot ? S_OK : D3DERR_INVALIDCALL;
        rec = FkNewSB();
        rec->made = 2;
        if ((IDirect3DDevice9*)this != gDev) rec->fkDev = this;
        rec->snapNoop = gFkSnapshot;
        return S_OK;
    }
    M(HRESULT, EndStateBlock)(IDirect3DStateBlock9** pp) override
    {
        FK(61);
        if (!pp || !rec) return D3DERR_INVALIDCALL;
        *pp = rec;
        if (!gFkSnapshot) { rec->counted = true; InterlockedIncrement(&blocksLive); }
        rec = nullptr;
        return S_OK;
    }
    M(HRESULT, SetClipStatus)(const D3DCLIPSTATUS9* c) override { LogTo(gFk, 62, { 1u }, c, sizeof(*c)); return S_OK; }
    M(HRESULT, GetClipStatus)(D3DCLIPSTATUS9* c) override { FK(63); memset(c, 0, sizeof(*c)); return S_OK; }
    M(HRESULT, GetTexture)(DWORD s, IDirect3DBaseTexture9** pp) override
    { if (gFkOnGetTexture) gFkOnGetTexture(); const int i = FkSamp(s); if (i < 0) return D3DERR_INVALIDCALL; *pp = st.tex[i]; if (*pp) (*pp)->AddRef(); return S_OK; }
#define SLOT 65
    // An invalid sampler is D3D_OK and nothing (d3d9_device.cpp:2803).
    M(HRESULT, SetTexture)(DWORD s, IDirect3DBaseTexture9* t) override
    { ALIVE(t); FK(65, s, P(t)); const int i = FkSamp(s); if (i < 0) return S_OK; if (rec) FkRecTex(rec, i, t); else st.tex[i] = t; return S_OK; }
#undef SLOT
    M(HRESULT, GetTextureStageState)(DWORD s, D3DTEXTURESTAGESTATETYPE t, DWORD* v) override
    { FK(66, s, (DWORD)t); *v = st.tss[s < 7 ? s : 7][FkTssIdx((DWORD)t)]; return S_OK; }
    // Clamped, never refused (SetStateTextureStageState: "Matches tests + Dawn of Magic 2 relies on it").
    M(HRESULT, SetTextureStageState)(DWORD s, D3DTEXTURESTAGESTATETYPE t, DWORD v) override
    {
        FK(67, s, (DWORD)t, v);
        const UINT g = s < 7 ? s : 7; const DWORD k = FkTssIdx((DWORD)t);
        if (rec) FkRecTss(rec, g, k, v); else st.tss[g][k] = v;
        return S_OK;
    }
    M(HRESULT, GetSamplerState)(DWORD s, D3DSAMPLERSTATETYPE t, DWORD* v) override
    { const int i = FkSamp(s); if (i < 0 || (DWORD)t < 1 || (DWORD)t > 13) return D3DERR_INVALIDCALL; *v = st.samp[i][t]; return S_OK; }
    // An invalid sampler is D3D_OK and nothing (d3d9_device.cpp:2864). DXVK does not check the type at all (a type past 13
    // writes past its array); the fake refuses it, and the layer sends it to DXVK as a SYNC for DXVK's own answer.
    M(HRESULT, SetSamplerState)(DWORD s, D3DSAMPLERSTATETYPE t, DWORD v) override
    {
        FK(69, s, (DWORD)t, v); const int i = FkSamp(s);
        if (i < 0) return S_OK;
        if ((DWORD)t < 1 || (DWORD)t > 13) return D3DERR_INVALIDCALL;
        if (rec) FkRecSamp(rec, i, (DWORD)t, v); else st.samp[i][t] = v;
        return S_OK;
    }
    M(HRESULT, ValidateDevice)(DWORD* n) override { FK(70); *n = 1; return S_OK; }
    M(HRESULT, SetPaletteEntries)(UINT n, const PALETTEENTRY* e) override { LogTo(gFk, 71, { n, 1u }, e, 256 * sizeof(PALETTEENTRY)); return S_OK; }
    M(HRESULT, GetPaletteEntries)(UINT n, PALETTEENTRY*) override { FK(72, n); return S_OK; }
    M(HRESULT, SetCurrentTexturePalette)(UINT n) override { FK(73, n); return S_OK; }
    M(HRESULT, GetCurrentTexturePalette)(UINT* n) override { FK(74); *n = 0; return S_OK; }
    M(HRESULT, SetScissorRect)(const RECT* r) override
    { if (!r) { FK(75, 0u); return D3DERR_INVALIDCALL; } LogTo(gFk, 75, { 1u }, r, sizeof(*r)); if (rec) FkRecSc(rec, *r); else st.sc = *r; return S_OK; }
    M(HRESULT, GetScissorRect)(RECT* r) override { *r = st.sc; return S_OK; }
    M(HRESULT, SetSoftwareVertexProcessing)(BOOL b) override { FK(77, (DWORD)b); return S_OK; }
    M(BOOL, GetSoftwareVertexProcessing)() override { FK(78); return FALSE; }
    M(HRESULT, SetNPatchMode)(float f) override { FK(79, F(f)); return S_OK; }
    M(float, GetNPatchMode)() override { FK(80); return 0.0f; }
    M(HRESULT, DrawPrimitive)(D3DPRIMITIVETYPE t, UINT sv, UINT pc) override { FK(81, (DWORD)t, sv, pc); FkMaybeSleep(); return (HRESULT)_InterlockedExchange(&gFkDrawHr, 0); }
    M(HRESULT, DrawIndexedPrimitive)(D3DPRIMITIVETYPE t, INT bv, UINT mi, UINT nv, UINT si, UINT pc) override
    { FK(82, (DWORD)t, (DWORD)bv, mi, nv, si, pc); FkMaybeSleep(); return S_OK; }
    // 3.1's order: a zero stride is refused, then no primitives (or, indexed, no
    // vertices) is S_OK, both before anything is read or unbound.
    M(HRESULT, DrawPrimitiveUP)(D3DPRIMITIVETYPE t, UINT pc, const void* v, UINT str) override
    {
        LogTo(gFk, 83, { (DWORD)t, pc, 0u, str }, v, (str && pc) ? FkUpBytes(FkVerts(t, pc), str) : 0);
        if (!str) return D3DERR_INVALIDCALL;
        if (pc) st.Up(false);
        return S_OK;
    }
    M(HRESULT, DrawIndexedPrimitiveUP)(D3DPRIMITIVETYPE t, UINT mi, UINT nv, UINT pc, const void* ix, D3DFORMAT f, const void* v, UINT str) override
    {
        const size_t ib = FkUpBytes(FkVerts(t, pc), f == D3DFMT_INDEX32 ? 4u : 2u), vb = FkUpBytes((unsigned long long)mi + nv, str);
        const bool data = str && pc && nv && ib && vb;
        LogTo(gFk, 84, { (DWORD)t, mi, nv, pc, 0u, (DWORD)f, 0u, str }, ix, data ? ib : 0, v, data ? vb : 0);
        if (!str) return D3DERR_INVALIDCALL;
        if (pc && nv) st.Up(true);
        return S_OK;
    }
    M(HRESULT, ProcessVertices)(UINT a, UINT b, UINT c, IDirect3DVertexBuffer9* vb, IDirect3DVertexDeclaration9* vd, DWORD f) override
    { FK(85, a, b, c, P(vb), P(vd), f); return S_OK; }
    M(HRESULT, CreateVertexDeclaration)(const D3DVERTEXELEMENT9*, IDirect3DVertexDeclaration9** pp) override
    {
        FK(86);
        FakeDecl* x = gFkReuseDecl; gFkReuseDecl = nullptr;
        if (x)
        {
            // THE SAME ADDRESS, A NEW OBJECT. It must be dead and bound nowhere, as a freed one is.
            if (!x->dead || st.decl == x) Err("the test asked for a declaration's memory back while it was alive or bound");
            EnterCriticalSection(&gFkLock); gDead.erase(x); LeaveCriticalSection(&gFkLock);
            x->dead = false; x->ref = 1;
        }
        else x = new FakeDecl;
        x->fvf = gFkNextDeclFvf;
        *pp = x; return S_OK;
    }
#define SLOT 87
    M(HRESULT, SetVertexDeclaration)(IDirect3DVertexDeclaration9* d) override { ALIVE(d); FK(87, P(d)); if (rec) FkRecDecl(rec, d); else st.decl = d; return S_OK; }
#undef SLOT
    // The shadow answers these two and the pixel constants now: not logged, like the other eleven.
    M(HRESULT, GetVertexDeclaration)(IDirect3DVertexDeclaration9** pp) override { if (gFkOnGetDecl) gFkOnGetDecl(); *pp = st.decl; if (*pp) (*pp)->AddRef(); return S_OK; }
    M(HRESULT, SetFVF)(DWORD f) override { FK(89, f); if (!f) return S_OK; FakeDecl* d = FkFvfDecl(f); if (rec) FkRecDecl(rec, d); else st.decl = d; return S_OK; }
    M(HRESULT, GetFVF)(DWORD* f) override { *f = st.Fvf(); return S_OK; }
    M(HRESULT, CreateVertexShader)(const DWORD*, IDirect3DVertexShader9** pp) override { FK(91); *pp = new FakeVS; return S_OK; }
#define SLOT 92
    M(HRESULT, SetVertexShader)(IDirect3DVertexShader9* s) override { ALIVE(s); FK(92, P(s)); if (rec) FkRecVs(rec, s); else st.vs = s; return S_OK; }
#undef SLOT
    M(HRESULT, GetVertexShader)(IDirect3DVertexShader9** pp) override { *pp = st.vs; if (*pp) (*pp)->AddRef(); return S_OK; }
    // DXVK 3.1 (d3d9_device.h, SetShaderConstants and GetShaderConstants): refused past the software count, 8192, then
    // clamped to the hardware count, 256, then none is D3D_OK, then the pointer. The pixel constants' two counts are one, 224.
    M(HRESULT, SetVertexShaderConstantF)(UINT s, const float* d, UINT n) override
    {
        LogTo(gFk, 94, { s, 0u, n }, d, (s < 256 && n <= 256 - s) ? n * 16 : 0); FkMaybeSleep();
        if (s > 8192 || n > 8192 - s) return D3DERR_INVALIDCALL;
        n = s >= 256 ? 0 : (n > 256 - s ? 256 - s : n);
        if (!n) return S_OK;
        if (!d) return D3DERR_INVALIDCALL;
        if (rec) FkRecVsc(rec, s, d, n); else memcpy(st.vsc[s], d, n * 16);
        return S_OK;
    }
    M(HRESULT, GetVertexShaderConstantF)(UINT s, float* d, UINT n) override
    { FK(95, s, n); if (s > 8192 || n > 8192 - s) return D3DERR_INVALIDCALL; n = s >= 256 ? 0 : (n > 256 - s ? 256 - s : n);
      if (!n) return S_OK; if (!d) return D3DERR_INVALIDCALL; memcpy(d, st.vsc[s], n * 16); return S_OK; }
    M(HRESULT, SetVertexShaderConstantI)(UINT s, const int* d, UINT n) override { LogTo(gFk, 96, { s, 0u, n }, d, n * 16); return S_OK; }
    M(HRESULT, GetVertexShaderConstantI)(UINT s, int* d, UINT n) override { FK(97, s, n); memset(d, 0, n * 16); return S_OK; }
    M(HRESULT, SetVertexShaderConstantB)(UINT s, const BOOL* d, UINT n) override { LogTo(gFk, 98, { s, 0u, n }, d, n * 4); return S_OK; }
    M(HRESULT, GetVertexShaderConstantB)(UINT s, BOOL* d, UINT n) override { FK(99, s, n); memset(d, 0, n * 4); return S_OK; }
#define SLOT 100
    M(HRESULT, SetStreamSource)(UINT i, IDirect3DVertexBuffer9* vb, UINT o, UINT s) override
    { ALIVE(vb); FK(100, i, P(vb), o, s); if (i >= 16) return D3DERR_INVALIDCALL; if (rec) FkRecVb(rec, i, vb, o, s); else st.Stream(i, vb, o, s); return S_OK; }
#undef SLOT
    M(HRESULT, GetStreamSource)(UINT i, IDirect3DVertexBuffer9** pp, UINT* o, UINT* s) override
    { if (i >= 16) return D3DERR_INVALIDCALL; *pp = st.vb[i]; *o = st.off[i]; *s = st.str[i]; if (*pp) (*pp)->AddRef(); return S_OK; }
    // Validated BEFORE the recorder sees it (d3d9_device.cpp:3789-3812).
    M(HRESULT, SetStreamSourceFreq)(UINT i, UINT f) override
    {
        FK(102, i, f);
        if (!FkFreqOk(i, f)) return D3DERR_INVALIDCALL;
        if (rec) FkRecFreq(rec, i, f); else st.freq[i] = f;
        return S_OK;
    }
    M(HRESULT, GetStreamSourceFreq)(UINT i, UINT* f) override { FK(103, i); if (i >= 16) return D3DERR_INVALIDCALL; *f = st.freq[i]; return S_OK; }
#define SLOT 104
    M(HRESULT, SetIndices)(IDirect3DIndexBuffer9* ib) override { ALIVE(ib); FK(104, P(ib)); if (rec) FkRecIb(rec, ib); else st.ib = ib; return S_OK; }
#undef SLOT
    M(HRESULT, GetIndices)(IDirect3DIndexBuffer9** pp) override { *pp = st.ib; if (*pp) (*pp)->AddRef(); return S_OK; }
    M(HRESULT, CreatePixelShader)(const DWORD*, IDirect3DPixelShader9** pp) override { FK(106); *pp = new FakePS; return S_OK; }
#define SLOT 107
    M(HRESULT, SetPixelShader)(IDirect3DPixelShader9* s) override { ALIVE(s); FK(107, P(s)); if (rec) FkRecPs(rec, s); else st.ps = s; return S_OK; }
#undef SLOT
    M(HRESULT, GetPixelShader)(IDirect3DPixelShader9** pp) override { *pp = st.ps; if (*pp) (*pp)->AddRef(); return S_OK; }
    M(HRESULT, SetPixelShaderConstantF)(UINT s, const float* d, UINT n) override
    { LogTo(gFk, 109, { s, 0u, n }, d, n * 16); if (s > 224 || n > 224 - s) return D3DERR_INVALIDCALL; if (!n) return S_OK; if (!d) return D3DERR_INVALIDCALL;
      if (rec) FkRecPsc(rec, s, d, n); else memcpy(st.psc[s], d, n * 16); return S_OK; }
    // 3.1's order (d3d9_device.h, GetShaderConstants): past the end is refused, none is S_OK, then the pointer.
    M(HRESULT, GetPixelShaderConstantF)(UINT s, float* d, UINT n) override
    { if (s + n > 224) return D3DERR_INVALIDCALL; if (!n) return S_OK; if (!d) return D3DERR_INVALIDCALL; memcpy(d, st.psc[s], n * 16); return S_OK; }
    M(HRESULT, SetPixelShaderConstantI)(UINT s, const int* d, UINT n) override { LogTo(gFk, 111, { s, 0u, n }, d, n * 16); return S_OK; }
    M(HRESULT, GetPixelShaderConstantI)(UINT s, int* d, UINT n) override { FK(112, s, n); memset(d, 0, n * 16); return S_OK; }
    M(HRESULT, SetPixelShaderConstantB)(UINT s, const BOOL* d, UINT n) override { LogTo(gFk, 113, { s, 0u, n }, d, n * 4); return S_OK; }
    M(HRESULT, GetPixelShaderConstantB)(UINT s, BOOL* d, UINT n) override { FK(114, s, n); memset(d, 0, n * 4); return S_OK; }
    M(HRESULT, DrawRectPatch)(UINT h, const float*, const D3DRECTPATCH_INFO*) override { FK(115, h); return S_OK; }
    M(HRESULT, DrawTriPatch)(UINT h, const float*, const D3DTRIPATCH_INFO*) override { FK(116, h); return S_OK; }
    M(HRESULT, DeletePatch)(UINT h) override { FK(117, h); return S_OK; }
    M(HRESULT, CreateQuery)(D3DQUERYTYPE t, IDirect3DQuery9** pp) override { FK(118, (DWORD)t); *pp = new FakeQuery; return S_OK; }
};
#undef M

static FakeDev* FkDevOf(const FakeSB* b) { return (FakeDev*)(b->fkDev ? b->fkDev : gDev); }
// Refused while any block records (stateblock.cpp:51-53, 67-69). Logged either way: the call reached the device in order.
HRESULT FakeSB::Capture()
{
    FK_ALIVE("state block"); FK(1904, P(this));
    FakeDev* const dv = FkDevOf(this);
    if (gFkSnapshot) { if (!snapNoop) s = dv->st; return S_OK; }
    if (dv->rec && gFkBreak != 8) return D3DERR_INVALIDCALL;
    FkCaptureFrom(this, dv->st, gFkBreak == 1);
    return S_OK;
}
// A DXVK 3.1 BLOCK HOLDS NO RENDER TARGET AND NO DEPTH STENCIL (d3d9_stateblock.h, D3D9CapturedStateFlag). The snapshot
// fake restores everything else it copied, as the fake did until 2026-09-28 (the conformance control).
HRESULT FakeSB::Apply()
{
    FK_ALIVE("state block"); FK(1905, P(this));
    FakeDev* const dv = FkDevOf(this);
    if (gFkSnapshot)
    {
        if (snapNoop) return S_OK;
        FkState& st = dv->st;
        IDirect3DSurface9* rt[4]; memcpy(rt, st.rt, sizeof(rt)); IDirect3DSurface9* const ds = st.ds;
        st = s;
        memcpy(st.rt, rt, sizeof(rt)); st.ds = ds;
        return S_OK;
    }
    if (dv->rec && gFkBreak != 5) return D3DERR_INVALIDCALL;
    // A one-shot failure the test asks for (C1's H10): DXVK fails an Apply only while a block records, which the layer keeps
    // from happening behind its own back; this stands in for a caller that went around it.
    const HRESULT hr = (HRESULT)_InterlockedExchange(&gFkApplyHr, 0);
    if (FAILED(hr)) return hr;
    FkApplyTo(this, dv->st);
    return S_OK;
}
// At 0 the block dies and lets go of everything it held privately; a Create or End block leaves its device's live count.
ULONG FakeSB::Release()
{
    const LONG r = InterlockedDecrement(&ref);
    if (r < 0) Err("a state block released below zero");
    if (r != 0) return (ULONG)r;
    dead = true; MarkDead(this);
    if (!snapAll && !snapNoop)
    {
        for (int i = 0; i < 21; i++) FkPriv(s.tex[i], false);
        for (int i = 0; i < 16; i++) FkPriv(s.vb[i], false);
        FkPriv(s.ib, false); FkPriv(s.vs, false); FkPriv(s.ps, false); FkPriv(s.decl, false);
    }
    if (counted) InterlockedDecrement(&FkDevOf(this)->blocksLive);
    return 0;
}
