# Copyright (C) 2019-2022 The ESPResSo project
#
# This file is part of ESPResSo.
#
# ESPResSo is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ESPResSo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import unittest as ut
import importlib_wrapper
import os
import pathlib

os.chdir("@SAMPLES_DIR@/object_in_fluid")
sample, skipIfMissingFeatures = importlib_wrapper.configure_and_import(
    "@SAMPLES_DIR@/object_in_fluid/motivation.py", maxCycle=4, cmd_arguments=[0])


@skipIfMissingFeatures
class Sample(ut.TestCase):
    system = sample.system

    def test_file_generation(self):
        basenames = [
            "cylinderA.vtk",
            "cylinderB.vtk",
            "cylinderC.vtk",
            "wallTop.vtk",
            "wallBottom.vtk",
            "wallBack.vtk",
            "wallFront.vtk"]
        for i in [0, 1]:
            for j in range(sample.maxCycle):
                basenames.append(f"cell{i}_{j}.vtk")

        # test .vtk files exist
        path_vtk_root = pathlib.Path("output")
        for name in basenames:
            filepath = path_vtk_root / "sim0" / name
            self.assertTrue(filepath.is_file(), f"File {filepath} not created")


if __name__ == "__main__":
    ut.main()
