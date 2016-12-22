#!/usr/bin/env python3
# genconfig.py

import sys
import os
import argparse
import re

verbose = True

def log(*args):
  global verbose
  if verbose:
    print(*args)


def read_config(cfg_file):
  if not os.path.exists(cfg_file):
    log("config file does not exist: {}".format(cfg_file))
    return
  cfg = {}
  regex = re.compile(r'^CONFIG_[A-Z_]+$')
  with open(cfg_file, 'r') as fh:
    for line in fh:
      l = line.strip()
      if len(l) == 0:
        continue
      elif l[0] == '#':
        continue
      else:
        eq_pos = l.find('=')
        if eq_pos == -1:
          log("invalid config line: {}".format(l))
          return
        key, value = l.split('=')
        key, value = key.strip(), value.strip()
        if not regex.search(key):
          log("invalid key: {}".format(key))
          return
        key = key[len('CONFIG_'):]
        cfg[key] = value
  return cfg


def get_arch_dir(cfg):
  arch = cfg['ARCH']
  arch_dir = os.path.join('arch', arch)
  return arch_dir


def get_mach_dir(cfg):
  mach = cfg['MACH']
  arch_dir = get_arch_dir(cfg)
  mach_dir = os.path.join(arch_dir, 'mach-' + mach)
  return mach_dir


def check(cfg, src_dir):
  # check ARCH
  if 'ARCH' not in cfg:
    log("missing CONFIG_ARCH!")
    return False
  arch_dir = get_arch_dir(cfg)
  if not os.path.isdir(os.path.join(src_dir, arch_dir)):
    log("unsupported CONFIG_ARCH={}".format(arch))
    return False
  # check MCU
  if 'MCU' not in cfg:
    log("missing CONFIG_MCU!")
    return False
  # check MACH
  if 'MACH' not in cfg:
    log("missing CONFIG_MACH")
    return False
  mach_dir = get_mach_dir(cfg)
  if not os.path.isdir(os.path.join(src_dir, mach_dir)):
    log("unsupported CONFIG_MACH={}".format(mach))
  return True


def print_build_dir(cfg, build_dir):
  arch = cfg['ARCH']
  mcu = cfg['MCU']
  mach = cfg['MACH']
  name = '-'.join((arch, mcu, mach))
  path = os.path.join(build_dir, name)
  print(path)


def print_arch_dir(cfg, src_dir):
  print(get_arch_dir(cfg))


def print_mach_dir(cfg, src_dir):
  print(get_mach_dir(cfg))


def gen_c_header(cfg, output_file):
  with open(output_file, "w") as fh:
    print("/* AUTOGENERATED FILE. DO NOT MODIFY! */", file=fh)
    for k in sorted(cfg.keys()):
      v = cfg[k]
      k = "CONFIG_" + k
      if v in ('y', 'Y'):
        print("#define {}".format(k), file=fh)
      elif v in ('n', 'N'):
        print("/* #undef {} */".format(k), file=fh)
      else:
        print("#define {} {}".format(k, v), file=fh)

def gen_make(cfg, output_file):
  with open(output_file, "w") as fh:
    print("# AUTOGENERATED FILE. DO NOT MODIFY!", file=fh)
    for k in sorted(cfg.keys()):
      v = cfg[k]
      k = "CONFIG_" + k
      print("{}={}".format(k, v), file=fh)


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('config_file', help="input config file")
  parser.add_argument('-b', '--print-build-dir', action='store_true', default=False, help="print path of build dir")
  parser.add_argument('-a', '--print-arch-dir', action='store_true', default=False, help="print path of arch dir")
  parser.add_argument('-m', '--print-mach-dir', action='store_true', default=False, help="print path of mach dir")
  parser.add_argument('-c', '--gen-c-header', default=None, help="generate c header file")
  parser.add_argument('-k', '--gen-make', default=None, help="generate make file")
  parser.add_argument('-S', '--src_base-dir', default="src", help="base directory for source")
  parser.add_argument('-B', '--build-base-dir', default="BUILD", help="base directory for build")
  args = parser.parse_args()

  # print commands are silent
  global verbose
  if args.print_build_dir or args.print_arch_dir or args.print_mach_dir:
    verbose = False

  # read config file
  cfg = read_config(args.config_file)

  # some basic checks on config
  if cfg and not check(cfg, args.src_base_dir):
    cfg = None

  # output build dir for makefile
  if args.print_build_dir:
    if cfg:
      print_build_dir(cfg, args.build_base_dir)
    else:
      print("INVALID")
  if args.print_arch_dir:
    if cfg:
      print_arch_dir(cfg, args.src_base_dir)
    else:
      print("INVALID")
  if args.print_mach_dir:
    if cfg:
      print_mach_dir(cfg, args.src_base_dir)
    else:
      print("INVALID")

  # generate files
  if args.gen_c_header:
    gen_c_header(cfg, args.gen_c_header)
  if args.gen_make:
    gen_make(cfg, args.gen_make)

  return 0


if __name__ == '__main__':
  sys.exit(main())
