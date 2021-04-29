import sys
import os
import subprocess

manual = [
    "PACKAGE INSTALLER FOR COMMANDLINE INTERFACE OF IR MODULE",
    "Make sure the following dependencies are met:",
    "Homebrew permission granted",
    "Python 3.9 (at least)",
    "pip 20.0.1.(at least)",
    "MacOS Big Sur (11.2.3)",
    "If these dependencies are not met, some things might not work as expected.",
]

for line in manual:
    print("<< "+line)

s = input("<< Do you want to continue? (y/n): ")
if s.lower() == "n":
    sys.exit()

s = input("<< 1. Packages 'cairo' and 'gobject-introspection' need to be installed using Homebrew.\n<< Ok? (y/n): ")
if s.lower() == "y":
    os.system("brew list 'cairo' || brew install 'cairo'")
    os.system("brew list 'gobject-introspection' || brew install 'gobject-introspection'")
elif s.lower() == "n":
    print("<< Skipped installation. This might raise some exceptions later on.\n<< When that's the case, rerun setup.py.")

print("<< Installing needed python packages.")

packageList = [
    'wifiPassword',
    'Foundation',
    'Pillow',
    'pyobjc-framework-Quartz',
    'appscript',
    'PyAutoGUI',
    'pyrebase',
    'pyobjc',
    'pyserial',
    'random2',
    'datetime',
    'wheel',
    'pycairo',
    'AppKit',
]

for i,package in enumerate(packageList):
    print("<< ===============================")
    print("<< Installing:",package,"("+str(i+1)+"/"+str(len(packageList))+")")
    print("<< ===============================")
    subprocess.check_call([sys.executable, '-m', 'pip', 'install', package])
    print()

# Source: https://stackoverflow.com/questions/53808723/importerror-no-module-named-appkit-after-installing-appkit-and-pyobjc
# This Fixes: NSWorkspace not found. 
print("<< Almost done...")
subprocess.check_call([sys.executable, '-m', 'pip', 'install', '--upgrade','--force-reinstall', 'PyObjC', 'PyObjC-core'])

# This Fixes: No module named Foundation (Raised by AppKit).
# This error occurs, because installing the Foundation package, actually installs the package
# in a folder named 'foundation'. AppKit imports 'Foundation' and thus can not find 'foundation'...
# 1. Find site-packages path for used python version.
# List all paths the current python version is using.
paths = sys.path
def findPackagesPath():
    for path in paths:
        if path.endswith('site-packages') or path.endswith('site-packages/'):
            return path
packagesPath = findPackagesPath()
print("<< Final step: Rename 'foundation' package.")
# 2. Change working directory to the found '/site-packages' directory.
absoluteP = os.path.abspath(packagesPath)
os.chdir(absoluteP)
# 3. Finally cange directory name.
installedPackagesList = os.listdir(absoluteP)
if 'foundation' in installedPackagesList:
    os.rename('foundation','Foundation')
elif 'Foundation' in installedPackagesList:
    print('<< Package is already named correctly.')
else:
    raise Exception("<< Error in final step.\n<< Go to "+packagesPath+" and rename 'foundation' folder to 'Foundation' manually!")

print("<< Setup success")