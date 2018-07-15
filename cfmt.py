#!/usr/bin/env python3
# Format the project source code per the standard (linux style)

import glob
import subprocess
import os

src_dirs = (
  'work/objects/noise',
  'work/objects/osc',
  'work/objects/sx1509',
  'work/objects/rei2c',
  'work/objects/adxl345',
  'work/objects/hmc5883l',
  'work/objects/input',
  'work/objects/itg3200',
  'repos/axoloti-contrib/objects/deadsy/mpr121',
  'repos/axoloti-contrib/objects/deadsy/ttp229',
  'repos/axoloti-contrib/objects/deadsy/input',
)

src_filter_out = (
)

indent_exec = '/usr/bin/indent'

def exec_cmd(cmd):
  """execute a command, return the output and return code"""
  output = ''
  rc = 0
  try:
    output = subprocess.check_output(cmd, shell=True)
  except subprocess.CalledProcessError as x:
    rc = x.returncode
  return output, rc

def get_files(dirs, filter_out):
  files = []
  for d in dirs:
    files.extend(glob.glob('%s/*.h' % d))
    files.extend(glob.glob('%s/*.c' % d))
  return [f for f in files if f not in filter_out]

def format(f):
  exec_cmd('%s -brf -linux -l10000 %s' % (indent_exec, f))
  os.unlink('%s~' % f)

def main():
  files = get_files(src_dirs, src_filter_out)
  for f in files:
    format(f)

main()
