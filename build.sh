#!/bin/sh

# Prep for Phase 0
cp vm/kitlibs.hseed vm/kitlibs.h
cp vm/kitlibs.makeseed vm/kitlibs.make
rm -f kits/kitmake.make


rm -f .phase2
unset PHASE2
make

if [ ! -e vm/cel ]; then
  echo "PHASE 1 BUILD FAILED"
  exit 1
fi



# if the make is good a cel will exist
# let it take over and give it arguments
#####

export PHASE2=1
touch .phase2
cd kits
../vm/cel kits/Array.cel
../vm/cel kits/Setup.cel
cd ..

#   have it build the proper Makefile based on the Kits/Setup (kits/kitmake.make)
#
#   Have it include additional libs for vm/Makefile (kitlibs.make)
#   have it build a kitlibs.h - (kitlibs.h)
#####

#   then run make in the kits dir

make allkits

#   then link it all together into one .a lib
#   then have the Main Cel relink to this 

make

#   When Cel starts, and it gets an 'import', it will
#   check to see if it is around. If it is, it will just 
#   link to it somehow?

if [ ! -e .phase2 ]
  then
    echo "************************"
    echo "PHASE 2 BUILD --FAILED--"
    echo "************************"
    exit 1
  else
    echo "************************"
    echo "CEL BUILD IS COMPLETE!!"
    echo "************************"
    exit 0
fi

#
#   have a test suite  'test.sh'
#
