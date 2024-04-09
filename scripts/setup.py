from setuptools import setup, find_packages

setup(
    name='podtp',
    version='0.1',
    packages=find_packages(),
    install_requires=[
        # Any dependencies your package needs to work, e.g.,
        'numpy>=1.18.1',
        'Pillow>=8.0',
    ],
)