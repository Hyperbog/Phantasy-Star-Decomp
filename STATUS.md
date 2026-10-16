# Current checkpoint — Stage 9

- Source-only game-specific Git repository prepared from validated Stage 9 package.
- ROM: English Rev.3 512 KiB SHA256 `9e324e92bd833b45126861b6f1d2dcbb51251bdf3df6d563ab3a3ff784d8cf88`.
- User-provided engine ZIP + Z80 core ZIP are required for local regeneration; both are excluded from history.
- Native-recompiled execution previously tested to frame 14,500 with paired same-runtime interpreter; 31/31 Stage 9 release tests reported passed. These are **previous private-package results**, not CI of the trimmed source-only repo.
- Good: title, intro, home/dialogue, city map, tracked-mode parity, final video and RAM matching, PSG command-value matching.
- Pending: exact PSG timestamps/PCM, transient VRAM discrepancies, independent emulator comparison, battle and save paths, reduction of remaining hybrid fallback, graphical Windows build.
- Do not interpret static function count or native-cycle coverage as proportion of whole game semantics complete.
- Current version-control state: private-ready source-only repository, initial commits on `main`. A GitHub remote needs to be created and associated separately.
