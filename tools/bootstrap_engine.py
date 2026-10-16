#!/usr/bin/env python3
"""Recreate the privately validated Stage 9 engine from user-provided upstream ZIPs.

No copyrighted game data is downloaded, embedded or copied by this script.
Do not edit generated game C; fix the generator/runtime and regenerate instead.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import stat
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SMSGG_EXPECT = ('recompiler/src/code_generator.c','recompiler/src/function_finder.c',
                'runner/glue.c','runner/main.c')
CORE_EXPECT = ('include/sms_runtime.h','include/z80_ops.h','LICENSE')


def sha256(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def unpack(zip_path: Path, dest: Path, expected: tuple[str, ...]) -> Path:
    """Unpack one root directory without allowing absolute paths, traversals or links."""
    with zipfile.ZipFile(zip_path) as z:
        members = z.infolist()
        roots = set()
        total = 0
        for info in members:
            name=info.filename
            path=PurePosixPath(name)
            if (not name or name.startswith('/') or '\\' in name or
                any(c in ('.','..') for c in path.parts) or
                (len(path.parts)<2 and not info.is_dir())):
                raise ValueError('unsafe archive name: '+name)
            if stat.S_IFMT(info.external_attr >> 16) == stat.S_IFLNK:
                raise ValueError('links disallowed in source ZIP: '+name)
            total += info.file_size
            if total > 40 * 1024 * 1024:
                raise ValueError('ZIP uncompressed size exceeds 40 MiB')
            roots.add(path.parts[0])
        if len(roots)!=1:
            raise ValueError('ZIP must have exactly one root directory')
        root=next(iter(roots))
        if not set(expected).issubset({ '/'.join(PurePosixPath(i.filename).parts[1:])
                                        for i in members if not i.is_dir() }):
            raise ValueError('unexpected ZIP layout or missing required upstream files')
        target=dest/root
        for info in members:
            p=dest.joinpath(*PurePosixPath(info.filename).parts)
            if info.is_dir(): p.mkdir(parents=True,exist_ok=True); continue
            p.parent.mkdir(parents=True,exist_ok=True)
            with z.open(info) as src,p.open('wb') as dst: shutil.copyfileobj(src,dst)
    return target


def build(engine_zip: Path, core_zip: Path, destination: Path = ROOT/'engine') -> None:
    if destination.exists():
        raise ValueError(f'{destination} already exists; refusing to overwrite it')
    manifest=json.loads((ROOT/'patches/upstream_hashes.json').read_text())
    archives=json.loads((ROOT/'patches/source_archives.json').read_text())
    for archive, label in ((engine_zip,'smsggrecomp-main.zip'),
                           (core_zip,'z80-recomp-core-main.zip')):
        if sha256(archive) != archives[label]:
            raise ValueError(f'upstream ZIP checksum mismatch: {label}')
    with tempfile.TemporaryDirectory(prefix='ps-engine-') as t:
        staging=Path(t)
        engine=unpack(engine_zip,staging/'sms',SMSGG_EXPECT)
        core=unpack(core_zip,staging/'z80',CORE_EXPECT)
        for rel,values in manifest.items():
            actual=sha256(engine/rel)
            if actual!=values['upstream_sha256']:
                raise ValueError('upstream revision mismatch: '+rel+'; no patch applied')
        # ZIP snapshots can contain CRLF, but the Stage 9 source patch and
        # the validated modified glue.c use LF. Convert after raw hash checks.
        for rel in manifest:
            file=engine/rel
            raw=file.read_bytes()
            if b'\r\n' in raw:
                file.write_bytes(raw.replace(b'\r\n',b'\n'))
        # Git applies the patch without requiring a repository or network access.
        patch=ROOT/'patches/stage9-runtime-and-generator.patch'
        for check in (['git','apply','--check',str(patch)],['git','apply',str(patch)]):
            subprocess.run(check,cwd=engine,check=True,capture_output=True,text=True)
        for rel,values in manifest.items():
            if sha256(engine/rel)!=values['patched_sha256']:
                raise ValueError('patched source checksum mismatch: '+rel)
        # Stage 10 extends the audited Stage 9 runtime only after checking the
        # exact Stage 9 parent-byte hashes. This prevents patch drift.
        sram_manifest=json.loads((ROOT/'patches/stage10_patched_hashes.json').read_text())
        for rel, values in sram_manifest.items():
            if sha256(engine/rel)!=values['stage9_sha256']:
                raise ValueError('Stage 10 parent source mismatch: '+rel)
        patch10=ROOT/'patches/stage10-sram.patch'
        for check in (['git','apply','--check',str(patch10)],['git','apply',str(patch10)]):
            subprocess.run(check,cwd=engine,check=True,capture_output=True,text=True)
        for rel, values in sram_manifest.items():
            if sha256(engine/rel)!=values['stage10_sha256']:
                raise ValueError('Stage 10 source checksum mismatch: '+rel)
        ext=engine/'external/z80-recomp-core'
        ext.mkdir(parents=True,exist_ok=True)
        for p in core.iterdir():
            dest=ext/p.name
            if dest.exists():
                # Existing .gitignore is supplied by smsggrecomp ZIP; the
                # core copy's .gitignore is identical in supplied sources.
                if p.is_file() and dest.is_file() and sha256(p)==sha256(dest):
                    continue
                raise ValueError('conflicting external dependency file: '+str(dest))
            if p.is_dir():shutil.copytree(p,dest)
            else:shutil.copy2(p,dest)
        destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.move(str(engine),str(destination))
    print(f'Validated and patched upstream engine: {destination}')


def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--engine-zip',type=Path,required=True,help='original smsggrecomp-main.zip')
    ap.add_argument('--core-zip',type=Path,required=True,help='original z80-recomp-core-main.zip')
    ap.add_argument('--dest',type=Path,default=ROOT/'engine')
    args=ap.parse_args()
    try: build(args.engine_zip.resolve(),args.core_zip.resolve(),args.dest.resolve())
    except (OSError,ValueError,subprocess.CalledProcessError,zipfile.BadZipFile) as e:
        ap.exit(1, f'BOOTSTRAP FAILED: {e}\n')
    return 0


if __name__=='__main__': raise SystemExit(main())
