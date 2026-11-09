"""Exercise Guition ES8389's real TDM/AEC channel mapping on the host."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


@unittest.skipUnless(shutil.which("c++"), "requires a host C++ compiler")
class GuitionTdmAudioTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.tmp.cleanup)
        cls.binary = str(pathlib.Path(cls.tmp.name) / "guition-tdm-test")
        source = pathlib.Path(cls.tmp.name) / "guition_tdm_test.cc"
        source.write_text(r'''
#include "main/boards/guition/common/es8389_tdm.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 4) return 1;
    const bool capture = std::string(argv[1]) == "capture";
    const size_t frames = std::stoul(argv[2]);
    const size_t chunk = std::stoul(argv[3]);
    const size_t in_stride = capture ? 4 : 1;
    const size_t out_stride = capture ? 2 : 4;
    if (chunk == 0 || static_cast<size_t>(argc - 4) != frames * in_stride) return 2;
    std::vector<int16_t> input;
    for (int i = 4; i < argc; ++i) input.push_back(std::stoi(argv[i]));
    std::vector<int16_t> output(frames * out_stride + 2, 12345);
    for (size_t done = 0; done < frames; done += chunk) {
        size_t count = std::min(chunk, frames - done);
        if (capture) {
            guition_es8389::ExtractMicReference(input.data() + done * in_stride,
                                               output.data() + 1 + done * out_stride, count);
        } else {
            guition_es8389::PackPlayback(input.data() + done * in_stride,
                                         output.data() + 1 + done * out_stride, count);
        }
    }
    // Also exercise zero-frame calls against the last-frame boundary.
    guition_es8389::ExtractMicReference(nullptr, output.data() + output.size() - 1, 0);
    guition_es8389::PackPlayback(nullptr, output.data() + output.size() - 1, 0);
    for (int16_t sample : output) std::cout << sample << ' ';
    return 0;
}
''')
        subprocess.run(
            ["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT),
             str(source), "-o", cls.binary], check=True)

    def run_mapping(self, mode, frames, values, chunk=240):
        result = subprocess.run(
            [self.binary, mode, str(frames), str(chunk), *map(str, values)],
            check=True, capture_output=True, text=True)
        output = list(map(int, result.stdout.split()))
        self.assertEqual((output[0], output[-1]), (12345, 12345))
        return output[1:-1]

    def test_capture_selects_td0_td1_without_swapping_or_using_other_pair(self):
        self.assertEqual(
            self.run_mapping("capture", 3, [101, -202, 303, -404,
                                            505, -606, 707, -808,
                                            -32768, 32767, 909, -1010]),
            [101, -202, 505, -606, -32768, 32767])

    def test_playback_duplicates_only_td0_td2_and_zeroes_padding(self):
        self.assertEqual(
            self.run_mapping("playback", 3, [-32768, 17, 32767]),
            [-32768, 0, -32768, 0, 17, 0, 17, 0, 32767, 0, 32767, 0])

    def test_capture_handles_full_chunk_and_partial_last_chunk(self):
        frames = [[i, -i - 1000, i + 2000, -i - 3000] for i in range(243)]
        captured = self.run_mapping("capture", len(frames),
                                    [value for frame in frames for value in frame])
        self.assertEqual(captured, [value for frame in frames for value in frame[:2]])
        self.assertEqual(captured[-6:], [240, -1240, 241, -1241, 242, -1242])

    def test_playback_handles_full_chunk_and_partial_last_chunk(self):
        mono = list(range(-121, 122))
        played = self.run_mapping("playback", len(mono), mono)
        self.assertEqual(played, [value for sample in mono for value in (sample, 0, sample, 0)])
        self.assertEqual(played[-4:], [121, 0, 121, 0])

    def test_single_and_empty_frames_preserve_boundaries(self):
        self.assertEqual(self.run_mapping("capture", 1, [3, -4, 5, -6]), [3, -4])
        self.assertEqual(self.run_mapping("playback", 1, [-7]), [-7, 0, -7, 0])
        self.assertEqual(self.run_mapping("capture", 0, []), [])
        self.assertEqual(self.run_mapping("playback", 0, []), [])


if __name__ == "__main__":
    unittest.main()
