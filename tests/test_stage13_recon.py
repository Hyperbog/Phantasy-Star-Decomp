"""ROM-free, fail-closed checks for Stage 13 encounter reconnaissance."""
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from scan_encounters import (
    DEFAULT_ROUTES, classify_transitions, endpoint_frame, parse_routes, run,
)
from run_dungeon_route import EXPECTED_MODE_TRANSITIONS


class EncounterReconTests(unittest.TestCase):
    def test_default_routes_have_unique_valid_names(self):
        parsed = parse_routes(DEFAULT_ROUTES)
        self.assertEqual(len(parsed), 4)
        self.assertEqual(len({name for name, _ in parsed}), 4)

    def test_endpoints_are_after_input_release(self):
        for _, actions in parse_routes(DEFAULT_ROUTES):
            self.assertEqual(endpoint_frame(actions) -
                             (3000 + 130*(len(actions)-1)), 120)

    def test_reject_invalid_or_duplicate_routes(self):
        invalid = ((), ('dup=U','dup=D'), ('bad=X',), ('NoSpace=U',),
                   ('bad name=U',), ('test=',), ('=UD',), ('one=U'*40,))
        for specs in invalid:
            with self.subTest(specs=specs), self.assertRaises(ValueError):
                parse_routes(specs)
        with self.assertRaises(ValueError):
            parse_routes(tuple(f'path{i}=U' for i in range(17)))

    def test_baseline_is_not_battle(self):
        observation = classify_transitions(list(EXPECTED_MODE_TRANSITIONS))
        self.assertFalse(observation['unclassified_mode_change'])
        self.assertFalse(observation['battle_verified'])
        self.assertEqual(observation['additional_modes'], [])

    def test_unknown_mode_not_mislabeled_battle(self):
        transitions = list(EXPECTED_MODE_TRANSITIONS) + [(4500, '0F')]
        observation = classify_transitions(transitions)
        self.assertTrue(observation['unclassified_mode_change'])
        self.assertFalse(observation['battle_verified'])
        self.assertEqual(observation['additional_modes'], [(4500, '0F')])

    def test_dungeon_prerequisites_are_mandatory(self):
        for transitions in ([], [(1, '00')],
                            list(EXPECTED_MODE_TRANSITIONS)[:-1],
                            [(1,'FF')]+list(EXPECTED_MODE_TRANSITIONS)[1:]):
            with self.subTest(transitions=transitions), self.assertRaises(ValueError):
                classify_transitions(transitions)

    def test_reject_bad_rom_without_creating_evidence(self):
        with tempfile.TemporaryDirectory() as d:
            rom = Path(d) / 'mutant.sms'
            rom.write_bytes(b'unverified')
            output = Path(d) / 'result'
            with self.assertRaisesRegex(ValueError, 'exact English Rev 3'):
                run(rom, output)
            self.assertFalse(output.exists())

    def test_refuse_to_overwrite_results(self):
        with tempfile.TemporaryDirectory() as d:
            output = Path(d) / 'prior'
            output.mkdir()
            with self.assertRaises(FileExistsError):
                run(Path(d) / 'missing.sms', output)


if __name__ == '__main__':
    unittest.main()
