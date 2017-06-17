# -*- coding: utf-8 -*-
# Original Author: Konstantin Batura <https://github.com/rusty-dev>
#
# Portions Copyright (C) 2017 Aratelia Limited - Juan A. Rubio
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import unicode_literals

import hashlib


def get_md5(value):
    d = hashlib.md5()
    d.update(value.encode('latin-1'))
    return d.hexdigest()
