"""ROM-free, fail-closed checks for Stage 13 encounter reconnaissance."""
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from scan_encounters import (
    CONTROL_ACTIONS, CONTROL_ENTRY_FRAME, DEFAULT_ROUTES, classify_transitions,
    endpoint_frame, ensure_control_route, parse_routes, run,
    verify_corridor_progress,
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

    def test_every_custom_scan_has_verified_corridor_control(self):
        custom = parse_routes(('unknown_south=DDDD',))
        selected = ensure_control_route(custom)
        self.assertEqual(selected[0], ('__stage12_control', CONTROL_ACTIONS))
        self.assertEqual(selected[1:], custom)
        original = parse_routes(DEFAULT_ROUTES)
        self.assertEqual(ensure_control_route(original), original)
        self.assertEqual(CONTROL_ENTRY_FRAME, 3120)

    @staticmethod
    def dungeon(position):
        return {'game_mode':0x0B, 'direction':3, 'position':position,
                'dungeon_index':2, 'enemy_id':0}

    def test_real_movement_positive_control(self):
        self.assertEqual(verify_corridor_progress(
            self.dungeon(0x1E), self.dungeon(0x17)), [])

    def test_identical_stalled_states_rejected(self):
        stalled = self.dungeon(0x1E)
        failures = verify_corridor_progress(stalled, stalled.copy())
        self.assertTrue(any('did not change' in item for item in failures))
        self.assertTrue(any('exit: position' in item for item in failures))

    def test_wrong_corridor_state_rejected(self):
        for key, value in (('game_mode', 0x08), ('direction', 0),
                           ('dungeon_index', 3), ('position', 0x1E)):
            with self.subTest(key=key):
                endpoint = self.dungeon(0x17)
                endpoint[key] = value
                self.assertTrue(verify_corridor_progress(
                    self.dungeon(0x1E), endpoint))

    def test_malformed_or_nonchronological_history_rejected(self):
        baseline = list(EXPECTED_MODE_TRANSITIONS)
        for extra in ([(2434, '0F')], [(2500, '0B')], [(2500, 'zz')],
                      [(2500, '0F'), (2499, '0A')],
                      [(2500, '0F'), (2501, '0F')], [(0, '0F')]):
            with self.subTest(extra=extra), self.assertRaises(ValueError):
                classify_transitions(baseline + extra)

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
