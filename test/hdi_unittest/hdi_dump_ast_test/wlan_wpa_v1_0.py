#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Copyright (c) 2024 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os
from base import Test


class wlan_wpa_v1_0(Test):
    def get_file_name(self):
        return __file__

    def set_idl_attr(self):
        work_dir = self.get_working_dir()
        package_path = os.path.join(work_dir, "foo")
        idl_dir = package_path + "/wlan/wpa/v1_0"
        self.set_dump_ast_param(package_path, idl_dir)

    def run(self):
        self.set_idl_attr()
        return self.run_choose(True)


if __name__ == "__main__":
    wlan_wpa_v1_0().test()

