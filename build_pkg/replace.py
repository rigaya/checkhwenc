#!/usr/bin/env python3
import sys
import os

def replace_pkg_template(input_file, output_file, pkg_name, pkg_bin, pkg_version, pkg_arch, pkg_maintainer, pkg_depends, pkg_desc, pkg_license, for_rpm):
    if not for_rpm:
        pkg_desc = pkg_desc.replace("\n", "\n ")
        pkg_desc = pkg_desc.replace("\n \n", "\n .\n")
    if (len(pkg_desc) == 0):
        pkg_desc = pkg_name

    with open(input_file, "r") as filein:
        with open(output_file, "w") as fileout:
            for line in filein.readlines():
                if "$PACKAGE_DEPENDS$" in line and len(pkg_depends) == 0 and for_rpm:
                    continue
                line = line.replace("$PACKAGE_NAME$",        pkg_name)
                line = line.replace("$PACKAGE_BIN$",         pkg_bin)
                line = line.replace("$PACKAGE_VERSION$",     pkg_version)
                line = line.replace("$PACKAGE_ARCH$",        pkg_arch)
                line = line.replace("$PACKAGE_MAINTAINER$",  pkg_maintainer)
                line = line.replace("$PACKAGE_DESCRIPTION$", pkg_desc)
                line = line.replace("$PACKAGE_LICENSE$",     pkg_license)
                line = line.replace("$PACKAGE_DEPENDS$",     pkg_depends)
                fileout.write(line)

if __name__ == '__main__':
    input_file = None
    output_file = None
    pkg_name = None
    pkg_bin = None
    pkg_version = None
    pkg_arch = "amd64"
    pkg_maintainer = ""
    pkg_depends = ""
    pkg_desc = ""
    pkg_license = ""
    for_rpm = False

    iarg = 0
    while iarg+1 < len(sys.argv):
        if sys.argv[iarg+1][0:6] != "--pkg-":
            if sys.argv[iarg] == "-i":
                iarg=iarg+1
                input_file = sys.argv[iarg]
            elif sys.argv[iarg] == "-o":
                iarg=iarg+1
                output_file = sys.argv[iarg]
            elif sys.argv[iarg] == "--pkg-name":
                iarg=iarg+1
                pkg_name = sys.argv[iarg]
            elif sys.argv[iarg] == "--pkg-bin":
                iarg=iarg+1
                pkg_bin = sys.argv[iarg]
            elif sys.argv[iarg] == "--pkg-version":
                iarg=iarg+1
                pkg_version = sys.argv[iarg]
            elif sys.argv[iarg] == "--pkg-arch":
                iarg=iarg+1
                pkg_arch = sys.argv[iarg]
            elif sys.argv[iarg] == "--pkg-maintainer":
                iarg=iarg+1
                pkg_maintainer = sys.argv[iarg]
            elif sys.argv[iarg] == "--pkg-depends":
                iarg=iarg+1
                pkg_depends = sys.argv[iarg]
            elif sys.argv[iarg] == "--pkg-desc":
                iarg=iarg+1
                pkg_desc = sys.argv[iarg]
            elif sys.argv[iarg] == "--pkg-license":
                iarg=iarg+1
                pkg_license = sys.argv[iarg]
            elif sys.argv[iarg] == "--rpm":
                for_rpm = True
        iarg=iarg+1

    if (input_file == None):
        print("input file not specified!")
        exit(1)
    if (not os.path.isfile(input_file)):
        print("input file does not exist!")
        exit(1)
    if (output_file == None):
        print("output file not specified!")
        exit(1)
    if (pkg_name == None):
        print("package name not specified!")
        exit(1)
    if (pkg_bin == None):
        print("package binary not specified!")
        exit(1)

    try:
        replace_pkg_template(input_file, output_file, pkg_name, pkg_bin, pkg_version, pkg_arch, pkg_maintainer, pkg_depends, pkg_desc, pkg_license, for_rpm)
    except:
        exit(1)

    exit(0)