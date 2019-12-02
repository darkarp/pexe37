#!/usr/bin/python3,3
# -*- coding: utf-8 -*-
"""pexe37 package
"""
__version__ = '0.9.6.2'

from .patch_distutils import patch_distutils

patch_distutils()
