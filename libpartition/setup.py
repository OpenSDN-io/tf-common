#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

from setuptools import setup

setup(
    name='libpartition',
    version='0.1.dev0',
    packages=[
        'libpartition',
    ],
    package_data={'': ['*.html', '*.css', '*.xml']},
    zip_safe=False,
    long_description="Partition Library Implementation",
    install_requires=[
        'gevent',
        'kazoo',
    ]
)
