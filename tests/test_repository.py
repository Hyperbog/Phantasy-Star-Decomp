"""Tests that deliberately require no ROM or external engine checkout."""
import hashlib
import json
from pathlib import Path
import tomllib
import unittest
import subprocess

ROOT=Path(__file__).resolve().parents[1]
CONFIG=ROOT/'game.toml'
ROM_SHA='9e324e92bd833b45126861b6f1d2dcbb51251bdf3df6d563ab3a3ff784d8cf88'

class RepositoryChecks(unittest.TestCase):
    def test_config(self):
        t=tomllib.loads(CONFIG.read_text())
        self.assertEqual(t['game']['output_prefix'],'ps1en')
        self.assertEqual(t['game']['platform'],'sms')
        self.assertEqual(t['mapper']['kind'],'sega')
        self.assertEqual(t['game']['crc32'],0x00BEF1D7)
        self.assertEqual(t['ram_layout']['game_mode'],0xC202)

    def test_bank_aware_tables(self):
        t=tomllib.loads(CONFIG.read_text())
        self.assertEqual(t['jump_tables']['addr'],[0x00BE,0x018F,0x82E5,0x8530])
        self.assertEqual(t['jump_tables']['bank'],[-1,-1,12,12])
        self.assertEqual(t['jump_tables']['count'],[20,12,5,18])

    def test_patch_provenance(self):
        d=json.loads((ROOT/'patches/upstream_hashes.json').read_text())
        self.assertEqual(set(d),{'recompiler/src/code_generator.c','recompiler/src/function_finder.c','runner/glue.c','runner/main.c'})
        for p,hashes in d.items():
            self.assertRegex(hashes['upstream_sha256'],r'^[0-9a-f]{64}$')
            self.assertRegex(hashes['patched_sha256'],r'^[0-9a-f]{64}$')
            self.assertNotEqual(hashes['upstream_sha256'],hashes['patched_sha256'])
        s=(ROOT/'patches/stage9-runtime-and-generator.patch').read_text()
        for p in d:self.assertIn('+++ b/'+p,s)


    def test_exact_offline_dependency_snapshots(self):
        d=json.loads((ROOT/'patches/source_archives.json').read_text())
        self.assertEqual(set(d),{'smsggrecomp-main.zip','z80-recomp-core-main.zip'})
        for sha in d.values():self.assertRegex(sha,r'^[0-9a-f]{64}$')

    def test_build_scripts_hash_check(self):
        self.assertIn(ROM_SHA,(ROOT/'regenerate.sh').read_text())
        self.assertIn(ROM_SHA,(ROOT/'run.sh').read_text())
        self.assertIn(ROM_SHA,(ROOT/'tools/run_scenario.py').read_text())

    def test_source_only_separation(self):
        tracked=subprocess.run(['git','ls-files','--cached'],cwd=ROOT,
                               capture_output=True,text=True,check=False)
        if tracked.returncode != 0: return # Filesystem-only source ZIP before git init.
        for name in tracked.stdout.splitlines():
            self.assertFalse(name.startswith(('engine/','bin/','generated/',
                                               'evidence/','runs/','evidence_stage9/')),name)
            self.assertFalse(name.lower().endswith(('.sms','.nes','.gg','.rom','.png','.wav','.csv','.ram','.exe')),name)

    def test_gitignore_guards(self):
        g=(ROOT/'.gitignore').read_text()
        for s in ('*.sms','*.nes','/engine/','/generated/','/bin/','/runs/'):
            self.assertIn(s,g)

    def test_replay_scenarios_preserved(self):
        import sys
        sys.path.insert(0,str(ROOT/'tools'))
        from run_scenario import scenarios
        d=scenarios()
        self.assertEqual(d['citysouth14500'][0],14500)
        self.assertEqual(d['city9700'][0],9700)
        self.assertEqual(len(d['citysouth14500'][1]),44)

    def test_checkpoint_explicit_limitations(self):
        s=(ROOT/'STATUS.md').read_text()
        for n in ('Pending','PCM','battle','independent emulator'):
            self.assertIn(n.lower(),s.lower())

if __name__=='__main__':unittest.main()
