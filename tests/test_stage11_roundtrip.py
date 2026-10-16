"""Source-only and optional local-generation contracts for the Stage 11 flow."""
import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from run_save_roundtrip import (CONTINUE_FRAMES, SAVE_FRAMES, continue_events,
                                run_roundtrip, save_events, transitions)


class Stage11Contract(unittest.TestCase):
    def test_patch_sources_and_parent_hashes_are_pinned(self):
        manifest=json.loads((ROOT/'patches/stage11_patched_hashes.json').read_text())
        self.assertEqual(set(manifest),{'runner/glue.c','recompiler/src/code_generator.c'})
        self.assertEqual(manifest['runner/glue.c']['stage10_sha256'],
                         json.loads((ROOT/'patches/stage10_patched_hashes.json').read_text())['runner/glue.c']['stage10_sha256'])
        for key, hashes in manifest.items():
            self.assertEqual(len(hashes['stage10_sha256']),64)
            self.assertEqual(len(hashes['stage11_sha256']),64)
            target=ROOT/'engine'/key
            if target.is_file():
                self.assertEqual(hashlib.sha256(target.read_bytes()).hexdigest(),
                                 hashes['stage11_sha256'])

    def test_cross_slot_trace_is_emitted_not_silently_dropped(self):
        patch=(ROOT/'patches/stage11-control-flow.patch').read_text()
        self.assertIn('cross-slot fall-through',patch)
        self.assertIn('call_by_address(0x%04X); return;',patch)
        self.assertIn('(a >> 14) != (next_pc >> 14)',patch)
        # No compiled or derived ROM bytes need be shipped for CI.
        generated=ROOT/'generated/ps1en_full.c'
        if generated.is_file():
            text=generated.read_text()
            self.assertIn('cross-slot fall-through 3FFF -> 4002',text)
            self.assertIn('call_by_address(0x4002); return;',text)

    def test_fallback_respects_extra_pop_unwind(self):
        patch=(ROOT/'patches/stage11-control-flow.patch').read_text()
        self.assertIn('if ((int16_t)(sp0 - entry_sp) >= 0) break;',patch)
        self.assertNotIn('+            if (sp0 == entry_sp) break;',patch)

    def test_save_menu_events_and_name_entry(self):
        for slot in (1,2):
            events=save_events(slot)
            self.assertEqual(events,sorted(events,key=lambda x:x[0]))
            self.assertEqual([(f,k)for f,k in events if k=='D' and f<6400],
                             [(6230,'D'),(6280,'D'),(6330,'D'),(6380,'D')])
            self.assertIn((6530,'B'),events)
            self.assertIn((7700,'B'),events)
            self.assertIn((8450,'B'),events)
            self.assertLessEqual(len(events),512)
        self.assertIn((6860,'D'),save_events(2))
        self.assertNotIn((6860,'D'),save_events(1))

    def test_continue_flow_selects_expected_slot(self):
        self.assertIn((310,'D'),continue_events(1))
        self.assertIn((1400,'B'),continue_events(1))
        self.assertIn((1660,'B'),continue_events(1))
        self.assertNotIn((1580,'D'),continue_events(1))
        self.assertIn((1580,'D'),continue_events(2))
        self.assertEqual(SAVE_FRAMES,9150)
        self.assertEqual(CONTINUE_FRAMES,2300)

    def test_altered_rom_rejected_without_running_or_creating_evidence(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);invalid=root/'bad.sms';invalid.write_bytes(b'not the ROM')
            destination=root/'output'
            with self.assertRaisesRegex(ValueError,'exact English Rev. 3'):
                run_roundtrip(invalid,destination,1)
            self.assertFalse(destination.exists())

    def test_prior_evidence_never_overwritten(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);destination=root/'output';destination.mkdir()
            marker=destination/'private.sav';marker.write_bytes(b'previous')
            with self.assertRaisesRegex(FileExistsError,'refusing to reuse'):
                run_roundtrip(root/'rom',destination,1)
            self.assertEqual(marker.read_bytes(),b'previous')

    def test_invalid_slot_not_accepted(self):
        for slot in (0,3,6):
            with self.assertRaises(ValueError):save_events(slot)
            with self.assertRaises(ValueError):continue_events(slot)
            with self.assertRaises(ValueError):run_roundtrip(Path('missing'),Path('new'),slot)

    def test_reported_transitions_only_on_mode_change(self):
        rows=[{'mode_c202':x}for x in ('00','00','02','02','09','09','0D')]
        self.assertEqual(transitions(rows),[[1,'00'],[3,'02'],[5,'09'],[7,'0D']])

    def test_documentation_exposes_accuracy_limits(self):
        text=(ROOT/'docs/STAGE11_VALIDATION.md').read_text()
        for phrase in ('PSG', 'stack', 'Mesen', 'empty slot', 'slot 2'):
            self.assertIn(phrase,text)


if __name__=='__main__': unittest.main()
