# -*- coding: utf-8 -*-

"""
pkg-config support

The PkgConfig class is useful by itself.  It allows reading and writing
of pkg-config files.

This file, when executed, will read a .pc file and print the result of
processing.  The result will be functionally equivalent, but not identical.
Re-running on its own output *should* produce identical results.
"""
from __future__ import print_function
from string import Template
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

class PkgConfig:
  """
  Contents of a .pc file for pkg-config.

  This can be initialized with or without a .pc file.
  Such a file consists of any combination of lines, where each is one of:
    - comment (starting with #)
    - variable definition (var = value)
    - field definition (field: value)
  (Allowable fields are listed in PgkConfig.fields.)
  All fields and variables become attributes of this object.
  Attributes can be altered, added, or removed at will.
  Attributes are interpolated when accessed as dictionary keys.

  When this is converted to a string, comments in the original file are
  re-printed in their original order (but all at the top), then any
  non-field attributes (as variables), then fields, and finally,
  unrecognized attributes from the original file, if any.

  @note Variables may not begin with '_'.
  """
  fields = ['Name', 'Description', 'Version', 'Requires', 'Conflicts', 'Libs', 'Cflags']
  own_comments = [
    "# Unrecognized fields",
    ]
  def __init__(self, FILE=None):
    self.__field_map = dict()
    self.__unrecognized_field_map = dict()
    self.__var_map = []
    self.__comments = []
    self.__parse(FILE)

  def __str__(self):
    OUT = StringIO()
    for comment in self.__comments:
      print(comment, file=OUT)
    print('', file=OUT)
    for i in self.__var_map:
      print("%s=%s" % (i[0], i[1]), file=OUT)
    print('', file=OUT)
    for key in PkgConfig.fields:
      if key not in self.__field_map: continue
      print("%s: %s" % (key, self.__field_map[key]), file=OUT)
    if self.__unrecognized_field_map:
      print('', file=OUT)
      print(PkgConfig.own_comments[0], file=OUT)
      for key,val in self.__unrecognized_field_map.items():
        print("%s: %s" % (key, val), file=OUT)
    assert set(self.__field_map).issubset(PkgConfig.fields)
    return OUT.getvalue()

  def __interpolated(self, raw):
    prev = None; current = raw
    while prev != current:
      # Interpolated repeatedly, until nothing changes.
      #print "prev, current: %s, %s" %(prev, current)
      prev = current
      current = Template(prev).substitute(self.__var_map)
    return current

  def __getitem__(self, name):
    """
    Return interpolated keys or variables.
    >>> pc = PkgConfig()
    >>> pc.WHO = 'me'
    >>> pc.WHERE = 'ho${WHO}'
    >>> pc.Required = '${WHO} and you'
    >>> pc['WHO']
    'me'
    >>> pc['WHERE']
    'home'
    >>> pc['Required']
    'me and you'
    """
    if hasattr(self, name):
      return self.__interpolated(getattr(self, name))
    else:
      raise IndexError(name)

  def __getattr__(self, name):
    if name in PkgConfig.fields:
      return self.__field_map.get(name, '')
    elif name in self.__var_map:
      return self.__var_map[name]
    else:
      raise AttributeError(name)

  def __setattr__(self, name, val):
    if name in PkgConfig.fields:
        if name in self.__field_map and self.__field_map[name] != 'unknown':
            self.__field_map[name] += " " + val;
        else:
            self.__field_map[name] = val
    elif not name.startswith('_'):
      self.__var_map.append((name, val))
    else:
      self.__dict__[name] = val

  def __delattr__(self, name):
    if name in self.__field_map:
      del self.__field_map[name]
    if name in self.__unrecognized_field_map:
      del self.__unrecognized_field_map[name]
    if name in self.__var_map:
      del self.__var_map[name]
    if name in self.__dict__:
      del self.__dict__[name]

  def __parse(self, PC):
    if not PC: return
    for line in PC:
      line = line.strip()
      if line.startswith('#'):
        if line not in PkgConfig.own_comments:
          self.__comments.append(line)
      elif ':' in line: # exported variable
        name, val = line.split(':')
        val = val.strip()
        if name not in PkgConfig.fields:
          self.__unrecognized_field_map[name] = val
        else:
          self.__field_map[name] = val
      elif '=' in line: # local variable
        name, val = line.split('=')
        self.__var_map[name] = val


def create_pc(name, prefix, libs, incdir = "/include", libdir = "/lib", **kwargs):
    a = PkgConfig()
    a.Name = name
    a.Description = "unknown"
    a.Version = "unknown"
    a.prefix = prefix
    a.exec_prefix = "${prefix}"
    a.libdir = "${exec_prefix}%s" % libdir
    a.includedir = "${exec_prefix}%s" % incdir
    a.Libs = "-L${libdir} %s" % libs
    a.Cflags = "-I${includedir}"
    for key in kwargs:
        a.__setattr__(key, kwargs[key])
    import os
    with open("pkgconfig/%s.pc" % name, 'wt') as f:
        f.write(str(a))
        return f.name


def create_oracle_pc(env):
    oracle_ic = env.get('ORACLE_INSTANT', None)
    oracle = env.get('ORACLE_HOME', None)
    if oracle_ic:
        if '/12.' in oracle_ic and oracle_ic.startswith('/usr/lib/'):
            create_pc("oracle", oracle_ic, "-lclntsh",
                      incdir='', libdir='/lib',
                      Cflags='-I'+oracle_ic.replace('/usr/lib/','/usr/include/'))
        else:
            create_pc("oracle", oracle_ic, "-lclntsh", "/sdk/include", "")
    elif oracle:
        create_pc("oracle", oracle, "-lclntsh", "/rdbms/public")


if __name__ == '__main__':
    import os
    import shutil
    shutil.rmtree('pkgconfig', ignore_errors=True)
    os.mkdir('pkgconfig')

    default_dirs = ["/usr", "/usr/local"]
    libroot = os.environ.get('LIBROOT', None)
    if libroot:
        default_dirs.append(libroot)
    print("Search externals libs in ", default_dirs)

    externallibs = os.path.abspath(os.environ.get('EXTERNALLIBS_DIR','externallibs'))

    create_oracle_pc(os.environ)

    for i in default_dirs + [ os.path.join(externallibs, 'soci') ] :
        if (os.path.isfile(i + "/lib/libsoci_core.so") or os.path.isfile(i + "/lib/libsoci_core.a") ) and os.path.isfile(i + "/include/soci/soci.h"):
           libs = "-lsoci_core"
           cflags = "-I${includedir} -I${includedir}/soci "
           if os.path.isfile(i + "/lib/libsoci_postgresql.so") or os.path.isfile(i + "/lib/libsoci_postgresql.a") :
               libs += " -lsoci_postgresql -lpq"
               cflags += " -DSOCI_HAVE_POSTGRESQL=1 "
           if os.path.isfile(i + "/lib/libsoci_oracle.so") or os.path.isfile(i + "/lib/libsoci_oracle.a") :
               libs += " -lsoci_oracle"
               if os.environ.get('SIRENA_WORKAROUND_ADD_LOCCI','0') == '1':
                   libs += " -locci"
               cflags += " -DSOCI_HAVE_ORACLE=1 "
           create_pc("soci", i, libs, Cflags=cflags)
           break

