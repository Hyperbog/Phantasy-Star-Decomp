"""ROM-free fail-closed checks for the Stage 12 dungeon input/replay judge."""
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from run_dungeon_route import (
    CHECKPOINTS, EXPECTED_MODE_TRANSITIONS, ALTERNATE_ROUTES,
    DUNGEON_DIRECTION, DUNGEON_INDEX, route_events, read_dungeon_state,
    verify_progress, compare_memory, run,
)


class Stage12DungeonTests(unittest.TestCase):
    def setUp(self):
        self.valid=[{'game_mode':0x0b,'position':pos,'direction':3,
                     'dungeon_index':2,'enemy_id':0} for frame,pos in CHECKPOINTS]

    def test_verified_seven_tile_progress(self):
        self.assertEqual(verify_progress(self.valid),[])
        self.assertEqual([p for _,p in CHECKPOINTS],
                         [0x1e,0x1d,0x1c,0x1b,0x1a,0x19,0x18,0x17,0x17])

    def test_stationary_output_cannot_pass(self):
        stalled=[dict(x,position=0x1e) for x in self.valid]
        self.assertTrue(verify_progress(stalled))

    def test_wrong_dungeon_position_fails(self):
        samples=[dict(s) for s in self.valid]
        samples[3]['position']=0x60
        self.assertIn('position', ' '.join(verify_progress(samples)))

    def test_wrong_direction_fails(self):
        samples=[dict(s) for s in self.valid]
        samples[4]['direction']=1
        self.assertIn('direction', ' '.join(verify_progress(samples)))

    def test_wrong_mode_fails(self):
        samples=[dict(s) for s in self.valid]
        samples[1]['game_mode']=0x0d
        self.assertIn('game_mode', ' '.join(verify_progress(samples)))

    def test_wrong_dungeon_index_fails(self):
        samples=[dict(s) for s in self.valid]
        samples[-1]['dungeon_index']=3
        self.assertIn('dungeon_index', ' '.join(verify_progress(samples)))

    def test_missing_checkpoint_fails(self):
        self.assertTrue(verify_progress(self.valid[:-1]))

    def test_both_save_slots_have_input_routes(self):
        for slot in (1,2):
            with self.subTest(slot=slot):
                ev=route_events(slot)
                self.assertTrue(all(a<b for (a,_),(b,_) in zip(ev,ev[1:])))
                self.assertEqual(ev[-1],(3918,''))
                for frame,key in [(3000,'L'),(3130,'U'),(3910,'U')]:
                    self.assertIn((frame,key),ev)
                self.assertEqual(sum(key=='U' and frame>=3000 for frame,key in ev),7)

    def test_bad_routes_rejected(self):
        for route in ('','A','X','luu','U'*33):
            with self.subTest(route=route), self.assertRaises(ValueError):
                route_events(1,route)

    def test_alternate_routes_independent_directions(self):
        self.assertEqual({r[0] for r in ALTERNATE_ROUTES},
                         {'west_return','south_three','south_return'})
        self.assertEqual({r[3] for r in ALTERNATE_ROUTES}, {0x1e,0x4e})
        for label,actions,frame,pos,direction in ALTERNATE_ROUTES:
            with self.subTest(label=label):
                self.assertIn((3000,actions[0]),route_events(1,actions))
                self.assertGreater(frame,3000+len(actions)*130)

    def test_ram_contract_has_real_mapper_offsets(self):
        ram=bytearray(0x2000)
        ram[0x202]=0xb;ram[0x30a]=3;ram[0x30c]=0x17;ram[0x30d]=2;ram[0x2e6]=0
        self.assertEqual(read_dungeon_state(ram),
                         {'game_mode':11,'direction':3,'position':23,
                          'dungeon_index':2,'enemy_id':0})
        with self.assertRaises(ValueError):read_dungeon_state(ram[:-1])

    def test_raw_video_mismatch_fails_closed(self):
        with tempfile.TemporaryDirectory() as d:
            folder=Path(d)
            for ext in ('vram','cram','regs','ram'):
                raw=b'\0'*(8192 if ext=='ram' else 16)
                (folder/f'native_100.png.{ext}').write_bytes(raw)
                (folder/f'interpreter_100.png.{ext}').write_bytes(raw)
            errors=[]
            self.assertFalse(errors)
            self.assertTrue(compare_memory(folder,100,errors)['ram']['stack_only'])
            self.assertEqual(errors,[])
            (folder/'interpreter_100.png.vram').write_bytes(b'\1'+b'\0'*15)
            errors=[]
            compare_memory(folder,100,errors)
            self.assertTrue(any('vram' in s for s in errors))

    def test_nonstack_ram_mismatch_fails_closed(self):
        with tempfile.TemporaryDirectory() as d:
            folder=Path(d)
            for ext in ('vram','cram','regs','ram'):
                raw=b'\0'*0x2000 if ext=='ram' else b'\0'*16
                (folder/f'native_100.png.{ext}').write_bytes(raw)
                (folder/f'interpreter_100.png.{ext}').write_bytes(raw)
            b=bytearray((folder/'interpreter_100.png.ram').read_bytes())
            b[0xACE]=1
            (folder/'interpreter_100.png.ram').write_bytes(b)
            errors=[]
            self.assertTrue(compare_memory(folder,100,errors)['ram']['stack_only'])
            self.assertEqual(errors,[])
            b[0x20C]=2
            (folder/'interpreter_100.png.ram').write_bytes(b)
            errors=[]
            self.assertFalse(compare_memory(folder,100,errors)['ram']['stack_only'])
            self.assertTrue(any('non-stack RAM' in s for s in errors))

    def test_bad_rom_rejected_before_output_created(self):
        with tempfile.TemporaryDirectory() as d:
            tmp=Path(d); rom=tmp/'mutant.sms';out=tmp/'output'
            rom.write_bytes(b'not original ROM')
            with self.assertRaisesRegex(ValueError,'exact English Rev 3'):
                run(rom,out)
            self.assertFalse(out.exists())

    def test_existing_evidence_dir_never_reused(self):
        with tempfile.TemporaryDirectory() as d:
            tmp=Path(d);out=tmp/'result';out.mkdir()
            with self.assertRaises(FileExistsError):run(tmp/'nonexistent.sms',out)


if __name__=='__main__':unittest.main()
