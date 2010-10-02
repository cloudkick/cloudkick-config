#
# Copyright 2010 Cloudkick Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#

EnsureSConsVersion(1, 1, 0)

import os
import re
from os.path import join as pjoin
from site_scons import ac

opts = Variables('build.py')

opts.Add(PathVariable('CURL', 'Path to curl-config', WhereIs('curl-config')))

env = Environment(options=opts,
                  ENV = os.environ.copy(),
                  TARFLAGS = '-c -z',
                  tools=['default', 'subst', 'packaging', 'install'])

#TODO: convert this to a configure builder, so it gets cached
def read_version(prefix, path):
  version_re = re.compile("(.*)%s_VERSION_(?P<id>MAJOR|MINOR|PATCH)(\s+)(?P<num>\d)(.*)" % prefix)
  versions = {}
  fp = open(path, 'rb')
  for line in fp.readlines():
    m = version_re.match(line)
    if m:
      versions[m.group('id')] = int(m.group('num'))
  fp.close()
  return (versions['MAJOR'], versions['MINOR'], versions['PATCH'])

env['version_major'], env['version_minor'], env['version_patch'] = read_version('CKC', 'src/ckc_version.h')
env['version_string'] = "%d.%d.%d"  % (env['version_major'], env['version_minor'], env['version_patch'])

dvpath = "/etc/debian_version"
if os.path.exists(dvpath):
  contents = open(dvpath).read().strip()
  # TODO: don't hard code this
  if contents == "4.0":
    env['version_string'] += "~bpo40"

conf = Configure(env, custom_tests = {'CheckCurlPrefix': ac.CheckCurlPrefix,
                                      'CheckCurlLibs': ac.CheckCurlLibs,
                                      'CheckDpkgArch': ac.CheckDpkgArch})

cc = conf.env.WhereIs('/Developer/usr/bin/clang')
if os.environ.has_key('CC'):
  cc = os.environ['CC']

if cc:
  conf.env['CC'] = cc


if not conf.CheckFunc('floor'):
  conf.env.AppendUnique(LIBS=['m'])

cprefix = conf.CheckCurlPrefix()
if not cprefix[0]:
  Exit("Error: Unable to detect curl prefix")

clibs = conf.CheckCurlLibs()
if not clibs[0]:
  Exit("Error: Unable to detect curl libs")

if conf.env.WhereIs('dpkg'):
  conf.env['HAVE_DPKG'] = True
  (st, env['debian_arch']) = conf.CheckDpkgArch()
  if not st:
    Exit()

if conf.env.WhereIs('emerge'):
  conf.env['HAVE_EMERGE'] = True
  if conf.CheckDeclaration("__i386__"):
    platform = 'i386'
  elif conf.CheckDeclaration("__amd64__"):
    platform = 'amd64'
  else:
    Exit()
  conf.env['gentoo_arch'] = platform

if conf.env.WhereIs('rpmbuild'):
    conf.env['HAVE_RPMBUILD'] = True

conf.env.AppendUnique(CPPPATH = [pjoin(cprefix[1], "include")])

# TOOD: this is less than optimal, since curl-config polutes this quite badly :(
d = conf.env.ParseFlags(clibs[1])
conf.env.MergeFlags(d)
conf.env.AppendUnique(CPPFLAGS = ["-Wall", "-g", "-O0"])
# this is needed on solaris because of its dumb library path issues
conf.env.AppendUnique(RPATH = conf.env.get('LIBPATH'))
env = conf.Finish()

Export("env")

ckc = SConscript("src/SConscript")

targets = [ckc]
target_packages = []

def locate(pattern, root=os.curdir):
    '''Locate all files matching supplied filename pattern in and below
    supplied root directory.'''
    for path, dirs, files in os.walk(os.path.abspath(root)):
        for filename in fnmatch.filter(files, pattern):
            yield os.path.join(path, filename)

site_files = []
site_files.extend(env.Glob("site_scons/*/*.py"))
site_files.extend(env.Glob("site_scons/*.py"))
site_files.extend(env.Glob("src/*.h"))
site_files.extend(env.Glob("build.py"))
site_files.extend(locate('*', 'extern'))
env.Depends('.', site_files)

if env.get('HAVE_RPMBUILD') or env.get('HAVE_DPKG') or env.get('HAVE_EMERGE'):
  pkgbase = "%s-%s" % ("cloudkick-config", env['version_string'])
  subst = {}

  substkeys = Split("""
  version_string
  debian_arch""")

  for x in substkeys:
      subst['%' + str(x) + '%'] = str(env.get(x))

if env.get('HAVE_RPMBUILD'):
  env.Install('/usr/bin/', ckc[0])
  packaging = {'NAME': 'cloudkick-config',
                'VERSION': env['version_string'],
                'PACKAGEVERSION':  0,
                'LICENSE': 'Proprietary',
                'SUMMARY': 'Cloudkick Configuration Tool',
                'DESCRIPTION': 'Cloudkick Configuration Tool',
                'X_RPM_GROUP': 'System/Monitoring',
                'source': [],
                'PACKAGETYPE': 'rpm'}
  target_packages.append(env.Package(**packaging))

  
if env.get('HAVE_DPKG'):
  debname = pkgbase + "_" + env['debian_arch'] +".deb"

  deb_control = env.SubstFile('packaging/debian.control.in', SUBST_DICT = subst)
  deb_conffiles = env.SubstFile('packaging/debian.conffiles.in', SUBST_DICT = subst)
  fr = ""
  if env.WhereIs('fakeroot'):
    fr = env.WhereIs('fakeroot')
  debroot = "debian_temproot"
  deb = env.Command(debname, [ckc[0], deb_control, deb_conffiles],
                [
                  Delete(debroot),
                  Mkdir(debroot),
                  Mkdir(pjoin(debroot, "DEBIAN")),
                  Copy(pjoin(debroot, 'DEBIAN', 'control'), deb_control[0]),
                  Copy(pjoin(debroot, 'DEBIAN', 'conffiles'), deb_conffiles[0]),
                  Mkdir(pjoin(debroot, "usr", "bin")),
                  Copy(pjoin(debroot, "usr", "bin", 'cloudkick-config'), ckc[0][0]),
                  fr +" dpkg-deb -b "+debroot+" $TARGET",
                  Delete(debroot),
                ])

  target_packages.append(deb)

if env.get('HAVE_EMERGE'):
  tgzroot = 'tgz_temproot'
  tgzname = pkgbase +"_"+ env['gentoo_arch'] +".tar.gz"
  tgz = env.Tar(tgzname, [ckc,])
  target_packages.append(tgz)

targets.extend(target_packages)

env.Default(targets)
