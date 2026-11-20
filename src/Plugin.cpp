// SPDX-License-Identifier: GPL-3.0-only
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "Gesture.h"

namespace {
using namespace goggles;
struct Nvse {
    unsigned version, runtime, editor, isEditor;
    void *registerCmd, *setOpcode;
    void *(__cdecl *query)(unsigned);
    unsigned (__cdecl *handle)();
};
struct Info { unsigned info; const char *name; unsigned version; };
struct Message { const char *sender; unsigned type, size; void *data; };
struct Messages { unsigned version; bool (__cdecl *listen)(unsigned, const char *, void (__cdecl *)(Message *)); };
struct Result { double value{}; unsigned char type{}; };
struct Scripts {
    bool (__cdecl *call)(void *, void *, void *, Result *, unsigned char, ...);
    void *params, *extract, *format, *callAlt;
    void *(__cdecl *compile)(const char *);
};
Scripts *scripts = nullptr;
void *menuQuery, *inventory, *pipLight;
bool inWorld = false, initialized = false, hasGoggles = false, installed = false;
DWORD nextInventory = 0;
BYTE *host = nullptr;
bool traceInput = false;
void log(const char *fmt, ...) {
    if (auto file = std::fopen("Data/NVSE/Plugins/TTWVRNightVision.log","ab")) {
        std::fprintf(file,"tick=%lu ", GetTickCount());
        va_list args;
        va_start(args, fmt);
        std::vfprintf(file, fmt, args);
        va_end(args);
        std::fputc('\n',file);
        std::fclose(file);
    }
}
// Signed results identify the requested equipment change. Fallout can commit it on a later tick.
// A read-only call returns the available variant. Prefer the worn one, then reinforced.
constexpr char gogglesScript[] = R"(int toggle
ref basic
ref reinforced
ref item
int kind
begin Function {toggle}
 let basic := GetFormFromMod "TaleOfTwoWastelands.esm" "079056"
 let reinforced := GetFormFromMod "TaleOfTwoWastelands.esm" "079057"
 if eval !basic || !reinforced
  return
 endif
 if player.GetEquipped basic
  let item := basic
  let kind := 1
 elseif player.GetEquipped reinforced
  let item := reinforced
  let kind := 2
 elseif player.GetItemCount reinforced
  let item := reinforced
  let kind := 2
 elseif player.GetItemCount basic
  let item := basic
  let kind := 1
 else
  return
 endif
 if toggle
  if player.GetEquipped item
   player.UnequipItemAlt item 0 1
   let kind := 0 - kind
  else
   player.EquipItemAlt item 0 1
  endif
 endif
 SetFunctionValue kind
end)";
// JIP's no-argument call reads the real light state. Supplying 0 or 1 sets it.
constexpr char pipLightScript[] = R"(begin Function {}
 if TogglePipBoyLight
  TogglePipBoyLight 0
  SetFunctionValue 0
 else
  TogglePipBoyLight 1
  SetFunctionValue 1
 endif
end)";
int run(void *script, bool withArg = false, int arg = 0) {
    Result out;
    if (!scripts->call(script, nullptr, nullptr, &out, withArg ? 1 : 0, arg) || out.type != 1)
        return 0x7fffffff;
    return int(out.value);
}

SRWLOCK inputLock = SRWLOCK_INIT;
Gesture gesture;
unsigned eligibleHands = 0;
unsigned pending = 0;
DWORD pendingAt = 0;
void reset() {
    AcquireSRWLockExclusive(&inputLock);
    gesture.interrupt();
    eligibleHands = 0;
    pending = 0;
    ReleaseSRWLockExclusive(&inputLock);
    nextInventory = 0;
    hasGoggles = false;
}

using Poll = bool (__thiscall *)(void *,Frame *);
template<int Backend> struct Input {
    static inline Poll original;
    static bool __fastcall poll(void *self, void *, Frame *f) {
        bool ok = original(self,f);
        AcquireSRWLockExclusive(&inputLock);
        if (ok && f) {
            if (traceInput) {
                static unsigned last;
                const unsigned buttons = (f->buttons[primary[0]] ? 1u : 0u) | (f->buttons[primary[1]] ? 2u : 0u);
                if (buttons & ~last) {
                    const auto &p = f->head.position;
                    const auto &l = f->hands[0].grip.position;
                    const auto &r = f->hands[1].grip.position;
                    log("buttons=%u eligible_hands=%u near=%d,%d head=%.2f,%.2f,%.2f left=%.2f,%.2f,%.2f right=%.2f,%.2f,%.2f",
                        buttons,eligibleHands,nearTemple(*f,0),nearTemple(*f,1),p.x,p.y,p.z,l.x,l.y,l.z,r.x,r.y,r.z);
                }
                last = buttons;
            }
            const unsigned edge = gesture.filter(*f,eligibleHands);
            if (edge) {
                pending |= edge;
                pendingAt = GetTickCount();
            }
        } else {
            gesture.interrupt();
        }
        ReleaseSRWLockExclusive(&inputLock);
        return ok;
    }
    static bool install(unsigned table) {
        void **slot = reinterpret_cast<void **>(host + table) + 6;
        // Other addons can already be filtering this method. Forward the current chain.
        void *previous = *slot;
        MEMORY_BASIC_INFORMATION region{};
        if (!VirtualQuery(previous,&region,sizeof(region)) || region.State != MEM_COMMIT ||
            !(region.Protect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))
            return false;
        DWORD old;
        if (!VirtualProtect(slot, sizeof(void *), PAGE_READWRITE, &old))
            return false;
        original = reinterpret_cast<Poll>(previous);
        bool success = InterlockedCompareExchangePointer(slot,reinterpret_cast<void *>(&poll),previous) == previous;
        DWORD ignored;
        VirtualProtect(slot,sizeof(void *),old,&ignored);
        return success;
    }
};
bool initialize() {
    char trace[8]{};
    traceInput = GetEnvironmentVariableA("TTWVR_NVG_TRACE",trace,sizeof(trace)) == 1 && trace[0] == '1';
    host = reinterpret_cast<BYTE *>(GetModuleHandleW(L"TTWVR.dll"));
    if (!host) return false;
    const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(host);
    const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS32 *>(host + dos->e_lfanew);
    // Both the author's and Linux renderer builds have this 1.1.7 input layout.
    const unsigned char xrCheck[]{0x88,0x47,0x25,0x8d,0x4c,0x24,0x28};
    const unsigned char vrCheck[]{0x88,0x41,0x20,0x8d,0x4c,0x24};
    if (nt->FileHeader.TimeDateStamp != 0x6afa9b4f || nt->OptionalHeader.SizeOfImage != 0x1d84000 ||
        std::memcmp(host+0x6e986d,xrCheck,sizeof xrCheck) || std::memcmp(host+0x6c8954,vrCheck,sizeof vrCheck))
        return false;
    menuQuery = scripts->compile("begin Function {}\nSetFunctionValue (GetActiveMenuMode)\nend");
    inventory = scripts->compile(gogglesScript);
    pipLight = scripts->compile(pipLightScript);
    if (!menuQuery || !inventory || !pipLight) {
        log("script compilation failed menu=%d goggles=%d light=%d",bool(menuQuery),bool(inventory),bool(pipLight));
        return false;
    }
    const bool xr = Input<0>::install(0xaf3568), vr = Input<1>::install(0xaf36d0);
    log("TTWVR 1.1.7 input OpenXR=%d OpenVR=%d",xr,vr);
    return xr || vr;
}

void __cdecl message(Message *m) {
    if (!m) return;
    switch (m->type) {
        case 1: case 2: case 6: case 7: // Exit or preload
            inWorld = false; reset(); return;
        case 8: case 14: // Successful load or new game
            inWorld = m->type == 14 || m->data != nullptr;
            reset(); log("world ready=%d",inWorld); return;
        case 20: break; // xNVSE main loop -- all inventory work belongs here
        default: return;
    }
    // Install after the first load, once startup input filters have registered.
    if (!inWorld) return;
    if (!initialized) {
        initialized = true;
        installed = initialize();
        if (!installed) log("inactive: supported TTWVR 1.1.7 and JIP LN required");
    }
    if (!installed) return;
    DWORD now = GetTickCount();
    const int menu = run(menuQuery);
    // WristBoy also has its own interface and hotkey wheel outside Fallout's menu stack.
    bool playing = (menu == 0 || menu == 1004) && !host[0x124a139] && !host[0x124a975];
    if (playing && (nextInventory == 0 || static_cast<LONG>(now-nextInventory) >= 0)) {
        int variant = run(inventory,true,0);
        const bool owned = variant == 1 || variant == 2;
        if (traceInput && owned != hasGoggles) log("inventory available=%d variant=%d",owned,variant);
        hasGoggles = owned;
        nextInventory = now + 250;
    }
    AcquireSRWLockExclusive(&inputLock);
    eligibleHands = playing ? (hasGoggles ? bothHands : leftHand) : 0;
    const unsigned hands = GetTickCount() - pendingAt < 350 ? (pending & eligibleHands) : 0;
    pending = 0;
    ReleaseSRWLockExclusive(&inputLock);
    if (hands & leftHand) {
        const int state = run(pipLight);
        log("gesture hand=left-X pip_light=%s", state == 1 ? "on" : state == 0 ? "off" : "unavailable");
    }
    if (hands & rightHand) {
        int result = run(inventory,true,1);
        log("gesture hand=right-A request=%s variant=%s",
            result == 1 || result == 2 ? "equip" : result == -1 || result == -2 ? "remove" : "unavailable",
            (result == -2 || result == 2) ? "reinforced" : (result == -1 || result == 1) ? "standard" : "none");
        nextInventory = 0;
    }
}
}
extern "C" __declspec(dllexport) bool NVSEPlugin_Query(const Nvse *n, Info *p) {
    if (!p || !n) return false;
    *p = {1,"TTWVR Night Vision Gesture",2};
    return !n->isEditor && n->runtime == 0x040020d0 && n->version >= 6;
}
extern "C" __declspec(dllexport) bool NVSEPlugin_Load(const Nvse *n) {
    scripts = static_cast<Scripts *>(n->query(6));
    auto *messages = static_cast<Messages *>(n->query(2));
    return scripts && scripts->compile && scripts->call && messages &&
           messages->listen(n->handle(),"NVSE",message);
}
