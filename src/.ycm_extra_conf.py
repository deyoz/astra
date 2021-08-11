import subprocess
import os.path


def GetFlagsFromMakefile(filename, varname):
    subdir = os.path.split(filename)[0]
    return subprocess.check_output(['make', '-C', subdir, '-s', f'print-{varname}']).decode().split()


def FlagsForFile(filename):
    return {'flags': GetFlagsFromMakefile(filename, 'CXXFLAGS'), 'do_cache': True}
