#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

import re
from setuptools import setup


def requirements(filename):
    with open(filename) as f:
        lines = f.read().splitlines()
    c = re.compile(r'\s*#.*')
    return list(filter(bool, map(lambda y: c.sub('', y).strip(), lines)))


setup(
    name='libpartition',
    version='0.1.dev0',
    packages=['libpartition'],
    package_data={'': ['*.html', '*.css', '*.xml']},
    zip_safe=False,
    long_description="Partition Library Implementation",
    install_requires=requirements('requirements.txt'),
)
