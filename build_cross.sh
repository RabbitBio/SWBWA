#!/bin/bash

#set -x
segment1=123
segment1_len=123
segment2=123
segment2_len=123


awk -v segment1=${segment1} '{if (!replaced && /segment1/) {print "unsigned long segment1 = "segment1";";replaced=1} else print $0}' bwamem.c > tmp.c && rm -f bwamem.c && mv tmp.c bwamem.c
awk -v segment1_len=${segment1_len} '{if (!replaced && /segment1_len/) {print "unsigned long segment1_len = "segment1_len";";replaced=1} else print $0}' bwamem.c > tmp.c && rm -f bwamem.c && mv tmp.c bwamem.c

awk -v segment2=${segment2} '{if (!replaced && /segment2/) {print "unsigned long segment2 = "segment2";";replaced=1} else print $0}' bwamem.c > tmp.c && rm -f bwamem.c && mv tmp.c bwamem.c
awk -v segment2_len=${segment2_len} '{if (!replaced && /segment2_len/) {print "unsigned long segment2_len = "segment2_len";";replaced=1} else print $0}' bwamem.c > tmp.c && rm -f bwamem.c && mv tmp.c bwamem.c


#make clean
make -j


segment1=$(swreadelf -l ./bwa | sed -n '9p' | awk '{print $3}')
segment1_len=$(swreadelf -l ./bwa | sed -n '10p' | awk '{print $1}')
segment2=$(swreadelf -l ./bwa | sed -n '11p' | awk '{print $3}')
segment2_len=$(swreadelf -l ./bwa | sed -n '12p' | awk '{print $2}')


awk -v segment1=${segment1} '{if (!replaced && /segment1/) {print "unsigned long segment1 = "segment1";";replaced=1} else print $0}' bwamem.c > tmp.c && rm -f bwamem.c && mv tmp.c bwamem.c
awk -v segment1_len=${segment1_len} '{if (!replaced && /segment1_len/) {print "unsigned long segment1_len = "segment1_len";";replaced=1} else print $0}' bwamem.c > tmp.c && rm -f bwamem.c && mv tmp.c bwamem.c

awk -v segment2=${segment2} '{if (!replaced && /segment2/) {print "unsigned long segment2 = "segment2";";replaced=1} else print $0}' bwamem.c > tmp.c && rm -f bwamem.c && mv tmp.c bwamem.c
awk -v segment2_len=${segment2_len} '{if (!replaced && /segment2_len/) {print "unsigned long segment2_len = "segment2_len";";replaced=1} else print $0}' bwamem.c > tmp.c && rm -f bwamem.c && mv tmp.c bwamem.c


#make clean
make -j

python xlink.py bwa
swreadelf -r bwa > bwa.r 
python get_tls.py bwa.r 
