#!/usr/bin/env python

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
import sys
import subprocess
from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

# Run bootstrap.py to run cmake generating a fresh bundle based on this
# checkout or copy from ../dist if the caller doesn't have cmake available.
# Note that bootstrap.py won't exist if building from sdist.
this_dir = os.path.dirname(__file__)
bootstrap_py = os.path.join(this_dir, 'bootstrap.py')
if os.path.exists(bootstrap_py):
    subprocess.run([sys.executable, bootstrap_py])

vendor_dir = os.path.join(this_dir, 'geoarrow', 'geoarrow')
vendored_files = os.listdir(vendor_dir)
sources = [f'geoarrow/geoarrow/{f}' for f in vendored_files if f.endswith('.c')]

# Workdaround because setuptools has no easy way to mix C and C++ sources
# if extra flags are required (e.g., -std=c++11 like we need here).
class build_ext_subclass(build_ext):
    def build_extensions(self):
        original__compile = self.compiler._compile
        def new__compile(obj, src, ext, cc_args, extra_postargs, pp_opts):
            if src.endswith('.c'):
                extra_postargs = [s for s in extra_postargs if s != "-std=c++11"]
            return original__compile(obj, src, ext, cc_args, extra_postargs, pp_opts)
        self.compiler._compile = new__compile
        try:
            build_ext.build_extensions(self)
        finally:
            del self.compiler._compile

setup(
    ext_modules=[
        Extension(
            name='geoarrow._lib',
            include_dirs=['geoarrow/geoarrow'],
            language='c++',
            sources=['geoarrow/_lib.pyx'] + sources,
            extra_compile_args = ['-std=c++11'],
        )
    ],
    cmdclass = {"build_ext": build_ext_subclass}
)
