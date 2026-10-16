"""Dependency bootstrap must reject unsafe ZIP content before any extraction."""
from pathlib import Path
import importlib.util
import tempfile
import unittest
import zipfile

ROOT=Path(__file__).resolve().parents[1]
SPEC=importlib.util.spec_from_file_location('ps_bootstrap', ROOT/'tools/bootstrap_engine.py')
bootstrap=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(bootstrap)

class BootstrapSecurity(unittest.TestCase):
    def assert_rejected(self, names):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/'src.zip'
            with zipfile.ZipFile(p,'w') as z:
                for k,v in names.items():z.writestr(k,v)
            with self.assertRaises(ValueError):
                bootstrap.unpack(p,Path(t)/'out',('some/source.c',))
            self.assertFalse((Path(t)/'pwned').exists())

    def test_path_escape_rejected(self):
        self.assert_rejected({'root/some/source.c':'ok','root/../../pwned':'bad'})

    def test_absolute_path_rejected(self):
        self.assert_rejected({'root/some/source.c':'ok','/tmp/evil.c':'bad'})

    def test_second_root_rejected(self):
        self.assert_rejected({'root/some/source.c':'ok','elsewhere/source.c':'bad'})

    def test_missing_required_file_rejected(self):
        self.assert_rejected({'root/incorrect.c':'oops'})

    def test_genuine_small_archive_can_unzip(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/'src.zip'
            with zipfile.ZipFile(p,'w') as z:
                z.writestr('root/','')
                z.writestr('root/some/source.c','verified text')
            extracted=bootstrap.unpack(p,Path(t)/'out',('some/source.c',))
            self.assertEqual((extracted/'some/source.c').read_text(),'verified text')

if __name__=='__main__':unittest.main()
