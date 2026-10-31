#!/usr/bin/env python3
"""Source-contract regression tests for ImageProcessor's mapping dispatch.

Run with: python3 test/test_image_processor_mapping.py
These check name/ID/algorithm consistency in both template overloads, not
pixel arithmetic or a running Hyperion instance. No Qt installation is needed.
"""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]


class ImageProcessorMappingTest(unittest.TestCase):
	@classmethod
	def setUpClass(cls):
		implementation = (ROOT / "libsrc/hyperion/ImageProcessor.cpp").read_text(encoding="utf-8")
		header = (ROOT / "include/hyperion/ImageProcessor.h").read_text(encoding="utf-8")
		cls.mappingIds = dict(
			(name, int(number)) for name, number in re.findall(
				r'if\s*\(mappingType\s*==\s*"([a-z_]+)"\s*\)\s*\{\s*return\s+(\d+)\s*;',
				implementation,
			)
		)
		cls.mappingNames = dict(
			(int(number), name) for number, name in re.findall(
				r'case\s+(\d+)\s*:\s*typeText\s*=\s*"([a-z_]+)"\s*;',
				implementation,
			)
		)
		cls.dispatch = re.findall(
			r'(?:case\s+(\d+)|default)\s*:\s*(?:colors\s*=\s*)?'
			r'_imageToLedColors->(\w+)\(image(?:,\s*ledColors)?\);',
			header,
		)

	def testMappingNamesAndIdsRemainStable(self):
		expected = {
			"multicolor_mean": 0,
			"multicolor_mean_squared": 1,
			"unicolor_mean": 2,
			"dominant_color": 3,
			"unicolor_dominant": 4,
			"dominant_color_advanced": 5,
			"unicolor_dominant_advanced": 6,
		}
		self.assertEqual(self.mappingIds, expected)
		self.assertEqual(self.mappingNames, {number: name for name, number in expected.items()})

	def testBothProcessOverloadsDispatchEachNamedMode(self):
		expected = {
			"multicolor_mean": "getMeanLedColor",
			"multicolor_mean_squared": "getMeanSqrtLedColor",
			"unicolor_mean": "getUniLedColor",
			"dominant_color": "getDominantLedColor",
			"unicolor_dominant": "getDominantUniLedColor",
			"dominant_color_advanced": "getDominantAdvLedColor",
			"unicolor_dominant_advanced": "getDominantAdvUniLedColor",
		}
		self.assertEqual(len(self.dispatch), 2 * len(expected))
		for name, algorithm in expected.items():
			with self.subTest(mapping=name):
				number = self.mappingIds[name]
				# Mean (ID 0) is handled by the default branch in each overload.
				case = str(number) if number else ""
				actual = [method for label, method in self.dispatch if label == case]
				self.assertEqual(actual, [algorithm, algorithm])


if __name__ == "__main__":
	unittest.main()
