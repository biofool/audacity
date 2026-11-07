"""Regression checks for #27. Run with python3; requires PyYAML."""
import os
from pathlib import Path
import re
import subprocess
import unittest

import yaml


WORKFLOWS = Path(__file__).resolve().parents[2] / ".github/workflows"
FILES = ("au4_update_learn_playlists.yml", "translate_tx_push.yml", "au4_build_windows.yml")


def steps(filename):
    workflow = yaml.safe_load((WORKFLOWS / filename).read_text())
    return [step for job in workflow["jobs"].values() for step in job["steps"]]


def step(filename, name):
    return next(item for item in steps(filename) if item["name"] == name)


def render(script):
    values = {
        "github.event_name": "pull_request",
        "github.event.schedule": "",
        "env.CURRENT_RELEASE_BRANCH": "master",
        "github.event.pull_request.number": "123",
        "matrix.arch": "x86_64",
    }
    return re.sub(r"\$\{\{\s*(.*?)\s*\}\}", lambda match: values[match[1]], script)


def bash(script, **env):
    return subprocess.run(["bash", "-e", "-o", "pipefail", "-c", script],
                          env={**os.environ, **env}, capture_output=True, text=True)


class WorkflowInputTests(unittest.TestCase):
    def test_no_input_or_secret_interpolation_in_scripts(self):
        for filename in FILES:
            for item in steps(filename):
                script = item.get("run", "")
                self.assertNotRegex(script, r"\$\{\{[^}]*(?:inputs\.|secrets\.|pull_request\.title)")
                if script and item.get("shell", "bash") == "bash":
                    result = subprocess.run(["bash", "-n"], input=render(script),
                                            text=True, capture_output=True)
                    self.assertEqual(result.returncode, 0, result.stderr)

    def test_branch_and_token_are_literal_data(self):
        script = step("translate_tx_push.yml", "Check preconditions")["run"]
        hostile = '$(exit 42)"; exit 43; #'
        result = bash(script, GITHUB_REF="refs/heads/" + hostile,
                      RETYPED_BRANCH_NAME=hostile, TRANSIFEX_API_TOKEN=hostile)
        self.assertEqual(result.returncode, 0, result.stderr)
        result = bash(script, GITHUB_REF="refs/heads/master",
                      RETYPED_BRANCH_NAME=hostile, TRANSIFEX_API_TOKEN="token")
        self.assertEqual(result.returncode, 1)
        self.assertNotIn(hostile, result.stdout)
        result = bash(script, GITHUB_REF="refs/heads/master",
                      RETYPED_BRANCH_NAME="master", TRANSIFEX_API_TOKEN="")
        self.assertEqual(result.returncode, 1)

    def test_playlist_modes_and_quoted_secrets(self):
        script = step("au4_update_learn_playlists.yml",
                      "Update info about the get started playlist in s3")["run"]
        # Replace privileged upload commands with an argv recorder; no network effects.
        stub = 'sudo() { printf "%s\\n" "$@"; };\n'
        hostile = 'key with spaces $(exit 42)"; exit 43; #'
        for mode, suffix in (("stable", "started_playlist.json"),
                             ("testing", "started_playlist.test.json")):
            result = bash(stub + script, MODE=mode, YOUTUBE_API_KEY=hostile,
                          S3_KEY=hostile, S3_SECRET=hostile)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout.splitlines().count(hostile), 3)
            self.assertIn("/" + suffix + "\n", result.stdout)
        result = bash(stub + script, MODE='testing; exit 42',
                      YOUTUBE_API_KEY="", S3_KEY="", S3_SECRET="")
        self.assertEqual(result.returncode, 1)
        self.assertNotIn("--s3_key", result.stdout)

    def test_windows_title_and_input_validation(self):
        script = step("au4_build_windows.yml", "Configure workflow")["run"]
        # Exercise the real validation before cmake; building/uploading is out of scope.
        prefix = render(script.split("cmake -DEVENT", 1)[0])
        prefix += '\nprintf "%s" "$ADD_INFO"\n'
        env = dict(GITHUB_REF="refs/pull/123/merge", SENTRY_PROJECT="",
                   pull_request_title='title\nINJECTED=value\r$(exit 42);' + 'x' * 200)
        for mode in ("", "devel_build", "nightly_build", "testing_build", "stable_build"):
            result = bash(prefix, BUILD_MODE=mode, **env)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertRegex(result.stdout, r"^_123_[a-zA-Z0-9 _.-]{0,80}$")
            self.assertNotIn("\n", result.stdout)
        for mode in ('$(exit 42)', 'stable_build\nINJECTED=value'):
            self.assertEqual(bash(prefix, BUILD_MODE=mode, **env).returncode, 1)
        env["SENTRY_PROJECT"] = 'au4\nINJECTED=value'
        self.assertEqual(bash(prefix, BUILD_MODE="stable_build", **env).returncode, 1)


if __name__ == "__main__":
    unittest.main()
