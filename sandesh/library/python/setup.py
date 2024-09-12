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
    name='sandesh',
    version='0.1.dev0',
    packages=[
        'pysandesh',
        'pysandesh.protocol',
        'pysandesh.transport',
        'pysandesh.gen_py',
        'pysandesh.gen_py.sandesh',
        'pysandesh.gen_py.sandesh_alarm',
        'pysandesh.gen_py.sandesh_ctrl',
        'pysandesh.gen_py.sandesh_uve',
        'pysandesh.gen_py.sandesh_uve.io',
        'pysandesh.gen_py.sandesh_uve.derived_stats_results',
        'pysandesh.gen_py.sandesh_trace',
        'pysandesh.gen_py.process_info',
    ],
    package_data={
        'pysandesh': ['webs/*.xsl', 'webs/js/*.js', 'webs/css/*.css',
                      'webs/css/images/*.png'],
        'pysandesh.gen_py.sandesh_uve': ['*.html', '*.css', '*.xml'],
        'pysandesh.gen_py.sandesh_alarm': ['*.html', '*.css', '*.xml'],
        'pysandesh.gen_py.sandesh_uve.io': ['*.html', '*.css', '*.xml'],
        'pysandesh.gen_py.sandesh_uve.derived_stats_results': [
                                    '*.html', '*.css', '*.xml'],
        'pysandesh.gen_py.sandesh_ctrl': ['*.html', '*.css', '*.xml'],
        'pysandesh.gen_py.sandesh_trace': ['*.html', '*.css', '*.xml'],
        'pysandesh.gen_py.process_info': ['*.html', '*.css', '*.xml'],
    },
    long_description="Sandesh python Implementation",

    install_requires=requirements('requirements.txt'),
    tests_require=requirements('test-requirements.txt'),
)
