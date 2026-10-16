"""Source-only Stage 10 publication and reproducibility invariants; CI-safe."""
import json
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

class Stage10Contract(unittest.TestCase):
    def test_patch_contains_exact_three_upstream_files(self):
        patch=(ROOT/'patches/stage10-sram.patch').read_text()
        manifest=json.loads((ROOT/'patches/stage10_patched_hashes.json').read_text())
        self.assertEqual(set(manifest),{'runner/glue.c','runner/glue.h','runner/main.c'})
        for rel,entry in manifest.items():
            self.assertIn('+++ b/'+rel,patch)
            for key in ('stage9_sha256','stage10_sha256'):
                self.assertRegex(entry[key],r'^[0-9a-f]{64}$')
            self.assertNotEqual(entry['stage9_sha256'],entry['stage10_sha256'])

    def test_parent_hash_matches_stage9_manifest(self):
        stage9=json.loads((ROOT/'patches/upstream_hashes.json').read_text())
        stage10=json.loads((ROOT/'patches/stage10_patched_hashes.json').read_text())
        for name in ('runner/glue.c','runner/main.c'):
            self.assertEqual(stage10[name]['stage9_sha256'],stage9[name]['patched_sha256'])

    def test_bootstrap_applies_both_patches(self):
        src=(ROOT/'tools/bootstrap_engine.py').read_text()
        self.assertIn('stage9-runtime-and-generator.patch',src)
        self.assertIn('stage10-sram.patch',src)
        self.assertIn("'stage9_sha256'",src)
        self.assertIn("'stage10_sha256'",src)

    def test_sram_parity_fail_closed(self):
        src=(ROOT/'tools/run_scenario.py').read_text()
        for marker in ('--with-sram','refusing to overwrite','cartridge SRAM differs',
                       'backup RAM header missing','expected cartridge SRAM image not written'):
            self.assertIn(marker,src)

    def test_documentation_discloses_open_save_and_audio_gaps(self):
        docs=(ROOT/'docs/STAGE10_VALIDATION.md').read_text().lower()
        for marker in ('save/continue','remain unverified', 'pcm', 'independent', 'battle'):
            self.assertIn(marker,docs)

if __name__=='__main__':unittest.main()
