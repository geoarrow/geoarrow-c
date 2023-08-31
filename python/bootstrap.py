# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

import os
import glob
import shutil

if __name__ == '__main__':
    this_dir = os.path.abspath(os.path.dirname(__file__))
    vendor_dir = os.path.join(this_dir, 'src', 'geoarrow', 'geoarrow')
    vendor_source_dir = os.path.join(this_dir, '..', 'src', 'geoarrow')

    if os.path.exists(vendor_dir):
        shutil.rmtree(vendor_dir)

    vendor_source_files = glob.glob(os.path.join(vendor_source_dir, '*.c'))
    vendor_source_files += glob.glob(os.path.join(vendor_source_dir, '*.h'))
    vendor_source_files += glob.glob(os.path.join(vendor_source_dir, 'geoarrow.hpp'))

    os.mkdir(vendor_dir)
    for source_file in vendor_source_files:
        shutil.copyfile(
            source_file,
            os.path.join(vendor_dir, os.path.basename(source_file))
        )
