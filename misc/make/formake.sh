#!/bin/bash

adir=../..

# New build directory
if ((0)); then
#prog_="bfs dgemm image pager randa rtb sort spmv strm"
prog_="bfs dgemm image pager sort spmv strm"
build_="zup"
for prog in $prog_ ; do
  for build in $build_ ; do
    dst=$adir/$prog/$build
    echo "mkdir -p $dst; cp -p $adir/randa/$build/cpu_lscript.ld $dst; cp -p Makefile $dst"
    mkdir -p $dst; cp -p $adir/randa/$build/cpu_lscript.ld $dst; cp -p Makefile $dst
  done
done
fi

# Distribute file(s) to each build directory
if ((0)); then
#prog_="bfs dgemm image pager randa rtb sort spmv strm"
prog_="bfs dgemm image pager randa rtb sort spmv strm"
#build_="arm_64 x86_64 mcu zup zynq"
build_="arm_64 x86_64 mcu zup zynq"
src=Makefile
for prog in $prog_ ; do
  for build in $build_ ; do
    dst=$adir/$prog/$build
    if [ -d "$dst" ]; then
      echo "cp -p $src $dst"
      cp -p $src $dst
    fi
  done
done
fi

# Process file(s) in each build directory
if ((1)); then
#prog_="bfs dgemm image pager randa rtb sort spmv strm"
prog_="bfs dgemm image pager randa rtb sort spmv strm"
#build_="arm_64 x86_64 mcu zup zynq"
build_="arm_64 x86_64 mcu zup zynq"
for prog in $prog_ ; do
  for build in $build_ ; do
    dst=$adir/$prog/$build
    if [ -d "$dst" ]; then
      echo "rm -f $dst/Makefile.old"
      rm -f $dst/Makefile.old
    fi
  done
done
fi
