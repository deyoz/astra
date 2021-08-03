import subprocess


def GetFlagsFromMakefile(varname):
    return subprocess.check_output(["make", "-s", f"print-{varname}"]).decode().split()


def FlagsForFile(filename):
    return {'flags': GetFlagsFromMakefile('CXXFLAGS'), 'do_cache': True}
