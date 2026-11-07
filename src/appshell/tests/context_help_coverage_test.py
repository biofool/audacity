"""Check the F1 mapping tables without requiring the Qt/Muse build dependencies.

Run with: python3 src/appshell/tests/context_help_coverage_test.py
"""
import re
from pathlib import Path
import unittest


SRC = Path(__file__).resolve().parents[2]
CONTROLLER = (SRC / "appshell/internal/applicationactioncontroller.cpp").read_text()


def table(name):
    body = CONTROLLER.split(name + " {", 1)[1].split("\n    };", 1)[0]
    return re.findall(r'\{ "([^"]+)", "([^"]+)" \}', body)


EXACT = dict(table("ACTION_TO_HANDBOOK_PAGE"))
PREFIXES = table("ACTION_PREFIX_TO_HANDBOOK_PAGE")


def page(action):
    action = action.rstrip("0123456789")
    return EXACT.get(action) or next(
        (target for prefix, target in PREFIXES if action.startswith(prefix)), None
    )


class ContextHelpCoverageTest(unittest.TestCase):
    maxDiff = None

    def test_registered_actions_have_help(self):
        actions = set()
        for source in SRC.glob("*/internal/*uiactions.cpp"):
            text = source.read_text()
            actions.update(re.findall(r'UiAction\("([^"]+)"', text))
            actions.update(re.findall(r'ActionQuery \w+\("([^"]+)"', text))
        self.assertGreater(len(actions), 250)
        # Navigation commands and developer diagnostics are not help-bearing UI items.
        excluded = {
            "action://cancel", "action://context-menu", "action://enter",
            "action://shift-enter", "action://trigger", "restart",
            "frame-statistics", "menu-tree", "raise-segfault",
            "throw-exception", "violate-assertion",
        }
        self.assertTrue(excluded <= actions)
        self.assertEqual(sorted(action for action in actions - excluded if not page(action)), [])

    def test_toolbar_record_alias(self):
        toolbar = (SRC / "projectscene/qml/Audacity/ProjectScene/toolbars/PlaybackToolBar.qml").read_text()
        self.assertIn('itemData.id === "action://record/start" ? "record"', toolbar)
        self.assertEqual(page("record"), "man/recording.html")

    def test_specific_entries_precede_family_fallbacks(self):
        self.assertEqual(page("action://record/lead-in-recording"),
                         "man/recording.html#Continuing_recording_in_a_new_track")
        self.assertEqual(page("action://trackedit/copy"),
                         "man/edit_menu.html#Copy_.C2.A0Ctrl_.2B_C")
        self.assertIsNone(page("unknown-control"))

    def test_menu_indices_and_dynamic_parameters(self):
        self.assertEqual(page("action://playback/change-api?api=ALSA12"), "man/preferences.html")
        self.assertEqual(page("record7"), "man/recording.html")
        self.assertEqual(page("favourite-effect-112"), "man/effect_menu.html")
        self.assertEqual(page("clip-export9"), "man/file_export_dialog.html")

    def test_no_duplicate_exact_entries(self):
        self.assertEqual(len(EXACT), len(table("ACTION_TO_HANDBOOK_PAGE")))


if __name__ == "__main__":
    unittest.main()
