"""Sega mapper cartridge SRAM tests using a compiled, ROM-free C probe.

The synthetic ROM only holds one different byte repeated per physical bank;
no proprietary ROM/game instructions are compiled into the probe.
"""
import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class SRAM(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not (ROOT/'engine/runner/glue.c').exists():
            raise unittest.SkipTest('bootstrap engine before runtime hardware tests')
        if not shutil.which('gcc'):
            raise unittest.SkipTest('GCC unavailable')
        cls.tmp = tempfile.TemporaryDirectory(prefix='ps-sram-')
        cls.addClassCleanup(cls.tmp.cleanup)
        cls.base=Path(cls.tmp.name)
        cls.exe=cls.base/'mapper'
        cls.rom=cls.base/'synthetic.sms'
        cls.rom.write_bytes(b''.join(bytes((i,))*16384 for i in range(4)))
        cmd=['gcc','-std=c11','-O1','-Wall','-Wextra',
             '-I'+str(ROOT/'engine/runner'),
             '-I'+str(ROOT/'engine/runner/include'),
             '-I'+str(ROOT/'engine/runner/video'),
             str(ROOT/'tests/sram_mapper_probe.c'),
             str(ROOT/'engine/runner/glue.c'),
             str(ROOT/'engine/runner/video/sms_vdp.c'),
             str(ROOT/'engine/runner/audio/sn76489.c'),
             str(ROOT/'engine/runner/external/superzazu/z80.c'),
             '-o',str(cls.exe)]
        result=subprocess.run(cmd,text=True,capture_output=True)
        if result.returncode:
            raise AssertionError('SRAM test probe compiler failed:\n'+result.stderr[:8000])

    def probe(self, save, *args, should_pass=True):
        result=subprocess.run([str(self.exe),str(self.rom),str(save),*args],
                              capture_output=True,text=True,cwd=self.base)
        if should_pass:
            self.assertEqual(result.returncode,0,result.stdout+'\n'+result.stderr)
        else:
            self.assertNotEqual(result.returncode,0,result.stdout+'\n'+result.stderr)
        return result

    def test_01_32k_bank_switch_rom_restore_and_persistence(self):
        save=self.base/'new_32k.sav'
        self.assertFalse(save.exists())
        self.probe(save)
        blob=save.read_bytes()
        self.assertEqual(len(blob),32768)
        self.assertEqual([(blob[k]) for k in (0,0x3fff,0x4000,0x7fff)],
                         [0x11,0x22,0x44,0x55])
        self.assertEqual(blob[1:2000],b'\xff'*1999)

    def test_02_reboot_retains_saved_banks(self):
        save=self.base/'persist.sav'
        self.probe(save)
        expected=save.read_bytes()
        self.probe(save,'reload')
        self.assertEqual(save.read_bytes(),expected)

    def test_03_small_cartridge_sram_mirrors_and_preserves_format(self):
        save=self.base/'small.sav'
        save.write_bytes(bytes([0xA0])+b'\xff'*8191)
        self.probe(save,'mirror')
        self.assertEqual(len(save.read_bytes()),8192)
        self.assertEqual(save.read_bytes()[0],0x5A)

    def test_04_invalid_save_size_is_rejected_without_mutation(self):
        save=self.base/'corrupt.sav'
        save.write_bytes(b'\0\1\2\3')
        result=self.probe(save,should_pass=False)
        self.assertIn('unsupported file size',result.stderr)
        self.assertEqual(save.read_bytes(),b'\0\1\2\3')

    def test_05_direct_cli_rejects_bad_save(self):
        exe=ROOT/'bin/PhantasyStarNative'
        if not exe.exists(): self.skipTest('build generated game first')
        save=self.base/'malformed.sav'
        save.write_bytes(b'short')
        result=subprocess.run([str(exe),str(self.rom),'--sram-file',str(save),
                               '--frames','1'],cwd=self.base,capture_output=True,text=True)
        self.assertNotEqual(result.returncode,0)
        self.assertEqual(save.read_bytes(),b'short')
        self.assertIn('unsupported file size',result.stderr)

    def test_06_sram_absent_by_default_does_not_create_files(self):
        exe=ROOT/'bin/PhantasyStarNative'
        if not exe.exists():self.skipTest('build generated game first')
        d=self.base/'isolated';d.mkdir()
        # Don't launch synthetic ROM into generated game code. Input validation
        # does not depend on ROM bytes and cannot create a .sav.
        result=subprocess.run([str(exe),'--check-input','--press','1:A'],
                              cwd=d,capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertEqual(list(d.glob('*.sav')),[])

    def test_07_patch_and_release_contract(self):
        self.assertTrue((ROOT/'patches/stage10-sram.patch').is_file())
        self.assertTrue((ROOT/'patches/stage10_patched_hashes.json').is_file())
        text=(ROOT/'engine/runner/glue.c').read_text()
        self.assertIn('glue_load_sram',text)
        self.assertIn('g_mapper_ctl & 0x08',text)
        self.assertIn('glue_flush_sram',text)
        self.assertIn('sms_slot_bank',text)


if __name__=='__main__': unittest.main()
