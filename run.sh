rm -rf test.sam && bsub -b -I -q q_sw_expr -n 1 -cgsp 64 -J test -priv_size 16 -share_size 14000 ./bwa mem -t 1 -1 -o test.sam ../bwa_index/GRCh38.d1.vd1.fa 1.fq 2.fq
