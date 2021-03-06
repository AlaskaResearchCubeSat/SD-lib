#!/usr/bin/env python

import shutil
import os
import subprocess
import sys
import re

inputDir="./"
prefix="Z:\Software"
lib=os.path.join(prefix,"lib")
include=os.path.join(prefix,"include")
basename="SD-lib"

#find which crossbuild to use
rowleyPath="C:\\Program Files (x86)\\Rowley Associates Limited\\"
#list rowley folder in program files
dirs=os.listdir(rowleyPath);
#initialize variables
path=None
version=None
#search for MSP430 crossworks
for folder in dirs:
	m=re.search("CrossWorks for MSP430 ([0-9\\.])",folder)
	if m is not None:
		#get version tuple
		ver=tuple(map(int,m.group(1)[0].split('.')))
		#check if a version was found
		if version is None or ver>version:
			version=ver
			path=folder

#get bath to crossbuild
crossbuild=os.path.join(rowleyPath,path,'bin','crossbuild.exe')

for config in ("UCA1 Release","UCA1 Debug",
			   "UCA2 Release","UCA2 Debug",
			   "UCA3 Release","UCA3 Debug",
			   "UCB1 Release","UCB1 Debug"):
    
	#build using crossbuild
	print("Building "+config);
	rc=subprocess.call([crossbuild,'-config',config,'-project','SD-lib',basename+'.hzp'])
	#check return code
	if rc!=0:
		print("Error : project did not build exiting")
		exit(rc)

	outname=basename+"_"+"_".join(config.split())+".hza"
	outpath=os.path.join(lib,outname)
	inpath=os.path.join(inputDir,os.path.join(basename+" "+config,basename+".hza"))
	print("Copying "+inpath+" to "+outpath)
	shutil.copyfile(inpath,outpath)

for config in ("UCA1 Release","UCA1 Debug"):
	#build using crossbuild
	print("Building error-only "+config)
	rc=subprocess.call([crossbuild,'-config',config,'-project','error-only',basename+'.hzp'])
	#check return code
	if rc!=0:
		print("Error : project did not build exiting")
		exit(rc)

	outname=basename+"_err_"+config.split()[1]+".hza"
	outpath=os.path.join(lib,outname)
	inpath=os.path.join(inputDir,os.path.join("error-only "+config,"error-only.hza"))
	print("Copying "+inpath+" to "+outpath)
	shutil.copyfile(inpath,outpath)

for file in ("SDlib.h",):
    outpath=os.path.join(include,file)
    inpath=os.path.join(inputDir,file)
    print("Copying "+inpath+" to "+outpath)
    shutil.copyfile(inpath,outpath)

