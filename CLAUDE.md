# wrathic — Windows KPS macro

Win32 desktop app (C++17, no framework, no package manager). Custom-drawn GDI+ UI,
licensed per-HWID against a Supabase table that the companion Discord bot writes to.

- Repo: https://github.com/vernoh/wrathic
- Companion bot: `../Macro bot` (separate repo, shared Supabase project) — see its CLAUDE.md
- Current version: `APP_VERSION` in [main.cpp:51](main.cpp:51) — **v3.0.1**. Latest
  published GitHub release is **v3.0.0**; v3.0.1 is staged as a draft release with the
  clean (publishable-key) binary, awaiting publish.

## Build

```bash
cl /EHsc /std:c++17 /W4 /O2 main.cpp resource.res
```

Run from an **x64 Native Tools for VS** prompt. All libs are `#pragma comment(lib, ...)`
near the top of [main.cpp](main.cpp) — do not add them to the command line.

Raw `cl` defaults to `/MT`, so the CRT is linked statically and the .exe has no
VC++ redistributable dependency. Don't add `/MD`.

Regenerate `resource.res` only when the icon changes:

```bash
rc resource.rc
```

`upx.exe` in the repo root is used to compress the final binary before release.

## Layout

Everything is one file: **[main.cpp](main.cpp) — ~16.8k lines, 1.2 MB.**

This is deliberate ("single executable entry point", per the original workspace rules).
Do not split it into modules unless explicitly asked. Navigate by the `// ===== NAME =====`
banner comments rather than reading the file top to bottom — it will not fit in context.

Key sections (line numbers drift; grep the banner text):

| Section | Line | What lives there |
|---|---|---|
| ENCRYPTED STRINGS | 47 | XOR-obfuscated Supabase URL + keys |
| ANTI-DEBUG | 185 | Debugger/VM detection |
| THEMES / VOID STARS | 242 / 281 | Palette + animated background |
| REGISTRY STORAGE | 12833 | Settings persistence (registry, not `settings.dat`) |
| TRIAL MODE | 13031 | Local trial gating |
| AUTO-UPDATE | 13077 | Polls GitHub releases API |
| HWID | 13394 | Machine fingerprint |
| HTTP | 13429 | `httpCall()` → Supabase REST, `githubGet()` → releases |
| LICENSE | 13461 | `validateLicense()` — the core auth path |
| CLICK ENGINE | 13562 | The actual macro loop |
| MEMORY CLEANER | 14143 | Working-set trimming |
| SPLASH / LAYOUT / PAINT | 14353+ | All custom GDI+ drawing |
| OVERLAY SYSTEM | 15818 | In-game CPU/GPU/disk overlay |
| MAIN WNDPROC | 16059 | Message pump; most UI wiring |

Other files: `hwid.h` (fingerprint helpers), `logo_icon.h` (embedded icon bytes),
`resource.rc` → `resource.res`.

## Licensing flow

The macro talks to **Supabase directly** — never to the bot. The bot is the writer,
the macro is the reader.

1. Bot issues a key → inserts a row into `public.licenses` (`hwid` NULL).
2. Macro `validateLicense(key, hwid)` GETs `/rest/v1/licenses?license_key=eq.<key>`
   using the **publishable** key (`sb_publishable_...`, stored XOR-obfuscated in
   `ENC_ANON_KEY`). It replaced the legacy `anon` JWT, which behaved identically —
   both are the public client role. Verified: GET returns 200, a request with no key
   returns 401.
3. If `hwid IS NULL`, macro PATCHes the row to claim it, same key. The
   `Allow hwid activation` RLS policy (`USING (hwid IS NULL)` /
   `WITH CHECK (hwid IS NOT NULL)`) is what permits it. Verified against the live DB:
   the claim succeeds, while updating or deleting an already-claimed row returns 0 rows.
   The service_role / `sb_secret` key must never be embedded in this binary.
4. Return codes: `-1` network error (falls back to cached license), `0` invalid,
   `1` valid + HWID match, `2` just activated, `3` HWID mismatch, `4` trial expired.

Local state lives in the registry (`LicenseKey`), not in `settings.dat`.

## Conventions

- No dependencies beyond the C++ stdlib and Win32. Do not introduce a package manager.
- Wide strings (`std::wstring`, `L"..."`) for anything user-facing or Win32-bound;
  narrow `std::string` only for HTTP bodies and JSON.
- JSON is parsed with `find()` / `substr()` on the raw response, not a library.
  Match that style when extending — do not add a JSON dependency.
- Style is dense: multiple statements per line, minimal whitespace. Match it.
- Logging: `LOG_OK(L"...")` / `LOG_ERR(L"...")` → `logs.txt`.

## Gitignored (present locally, never commit)

`license.dat`, `logs.txt`, `settings.dat`, `*.dat`, `resource.res`, `keygen.exe`,
`keygen.cpp`.

## Toolchain fixes at the top of main.cpp — leave them in place

The build broke on VS 18 / MSVC 14.51 (the committed v2.5.4 tree failed identically, so
it was toolchain drift, not a code regression). Three fixes are now applied at the top
of [main.cpp](main.cpp); removing any of them re-breaks the build:

1. `#define NOMINMAX` before `<windows.h>` — otherwise the `min`/`max` macros break
   every `std::min`/`std::max` call (~40 × C2589). Because `<gdiplus.h>` still needs
   those names, `namespace Gdiplus { using std::min; using std::max; }` is injected
   just before it is included.
2. `#undef small` after the includes — `rpcndr.h` does `#define small char`, which
   breaks the `HICON small` local in `getSmallIcon()`.
3. Four added lib pragmas: **advapi32** (registry + `OpenProcessToken`), **pdh**
   (`Pdh*` counters in `resourceThread`), **srclient** (`SRSetRestorePointW`),
   **comdlg32** (`GetOpenFileNameW`/`GetSaveFileNameW`).

Current state: builds clean, 0 errors, 27 `/W4` warnings (mostly `wcscpy`/`wcscat`
deprecation notices).
