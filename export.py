#!/usr/bin/python

import shutil
import os

inputDir="./"
prefix="Z:\Software"
lib=os.path.join(prefix,"lib")
include=os.path.join(prefix,"include")
basename="SD-lib"

for folder in ("UCA1 Release","UCA1 Debug","UCB1 Release","UCB1 Debug"):
    outname=basename+"_"+"_".join(folder.split())+".hza"
    outpath=os.path.join(lib,outname)
    inpath=os.path.join(inputDir,os.path.join(folder,basename+".hza"))
    print("Copying "+inpath+" to "+outpath)
    shutil.copyfile(inpath,outpath)

for file in ("SDlib.h",):
    outpath=os.path.join(include,file)
    inpath=os.path.join(inputDir,file)
    print("Copying "+inpath+" to "+outpath)
    shutil.copyfile(inpath,outpath)

