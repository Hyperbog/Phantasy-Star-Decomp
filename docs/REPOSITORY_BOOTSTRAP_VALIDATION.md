# Local repository bootstrap validation — 2026-10-16

Source-only Git setup reproduced the Stage 9 build in a *new* project directory from the original uploaded engine and core ZIPs and the exact English Rev. 3 ROM.

- Upstream ZIP SHA-256s recorded in `patches/source_archives.json`.
- Upstream engine file SHA-256s verified before patch application; all four patched files verified after applying `patches/stage9-runtime-and-generator.patch`.
- Upstream source `runner/glue.c` is CRLF in the source ZIP; the bootstrap normalizes the line endings **only after verifying the raw original source hashes**, then applies the LF-format patch. That preserves the exact patched source hashes from Stage 9.
- The same `game.toml`, `build.sh` and `regenerate.sh` successfully run the actual `SmsRecomp` generator and compile a native Linux executable.
- Generated source SHA-256 values exactly matched the previously validated Stage 9 set:

| File | SHA-256 |
|---|---|
| `ps1en_full.c` | `72cafa3187db75745891dcd5f3497b0e335b0b6f1aceeeb243eb58055c3a91ca` |
| `ps1en_dispatch.c` | `94622d4fa89d1f26e8e644e0f3e80676279bcce3217da1837f42eef58082439c` |
| `ps1en_layout.c` | `baca9d1906b1fb1a5656418065c1c3414ec8fadbc15bbf64883bc5f0e4676ecb` |
| `ps1en_funcs.h` | `a62b385b18b9c5e704e0a9b944e0354e34af3efaecb321fe871d30724cd02abc` |

- Fresh 600-frame and 9,700-frame native-versus-bundled-interpreter replays passed: tracked control-state equality and PNG equality; the 9,700-frame PSG trace had 34,944 matching command values. Its per-write cycle timestamps are **not** identical.
- No independent Mesen or actual SMS hardware run was performed.

The source-only Git history explicitly omits generated native C, captured screenshots, commercial ROM, memory dumps, audio recordings, compiled binaries, and unlicensed external engine source. This report contains no game byte listings.
