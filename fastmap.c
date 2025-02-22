/* The MIT License

   Copyright (c) 2018-     Dana-Farber Cancer Institute
                 2009-2018 Broad Institute, Inc.
                 2008-2009 Genome Research Ltd. (GRL)

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/
#include <zlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <math.h>
#include "bwa.h"
#include "bwamem.h"
#include "kvec.h"
#include "utils.h"
#include "bntseq.h"
#include "kseq.h"

#include <athread.h>
#include <mpi.h>
//KSEQ_DECLARE(gzFile)
KSEQ_DECLARE(int)

extern unsigned char nst_nt4_table[256];



#define cpe_num 384
extern int cpe_is_over[cpe_num];


#define use_cgs_mode
#define use_cpe_step0
//#define use_my_mpi

//#define use_lwpf3

#ifdef use_lwpf3
#define LWPF_UNITS U(TEST)
#include "lwpf.h"
#endif

extern void SLAVE_FUN(state_init());
extern void SLAVE_FUN(state_print());

double t_malloc = 0;
double t_free = 0;

double t_tot = 0;
double t_step1 = 0;
double t_step1_1 = 0;
double t_step1_1_1 = 0;
double t_step1_read = 0;
double t_step2 = 0;
double t_step3 = 0;
double t_step3_1 = 0;

double t_work1 = 0;
double t_work2 = 0;

double t_work1_1 = 0;
double t_work1_2 = 0;
double t_work1_3 = 0;
double t_work1_4 = 0;
double t_work1_5 = 0;
double t_work1_6 = 0;


double t_work2_1 = 0;
double t_work2_2 = 0;
double t_work2_3 = 0;
double t_work2_4 = 0;
double t_work2_5 = 0;

double t_extend = 0;
double t_bwt_sa = 0;

long long s_reg_sum = 0;
long long c_px2 = 0;
long long s_px2 = 0;

void *kopen(const char *fn, int *_fd);
int kclose(void *a);
void kt_pipeline(int n_threads, void *(*func)(void*, int, void*), void *shared_data, int n_steps);
void kt_pipeline_single(int n_threads, void *(*func)(void*, int, void*), void *shared_data, int n_steps);
void kt_pipeline_thread(int n_threads, void *(*func)(void*, int, void*), void *shared_data, int n_steps);


FILE *file_out_sam = NULL;
int file_out_fd = -1;
char sam_out_file_name[256];

typedef struct {
	kseq_t *ks, *ks2;
	mem_opt_t *opt;
	mem_pestat_t *pes0;
	int64_t n_processed;
	int copy_comment, actual_chunk_size;
	bwaidx_t *idx;
} ktp_aux_t;

typedef struct {
	ktp_aux_t *aux;
	int n_seqs;
	bseq1_t *seqs;
    char* block_buffer;
    char* block_buffer2;
    long long block_size;
    long long block_size2;
} ktp_data_t;

void SkipToLineEnd(char *data_, long long *pos_, const long long size_) {
    while (*pos_ < size_ && data_[*pos_] != '\n') {
        ++(*pos_);
    }
}

int64_t GetNextFastq(char *data_, long long pos_, const long long size_) {
    if(pos_ < 0) pos_ = 0;
    SkipToLineEnd(data_, &pos_, size_);
    ++pos_;

    // find beginning of the next record
    while (data_[pos_] != '@') {
        SkipToLineEnd(data_, &pos_, size_);
        ++pos_;
    }
    int64_t pos0 = pos_;

    SkipToLineEnd(data_, &pos_, size_);
    ++pos_;

    if (data_[pos_] == '@')// previous one was a quality field
        return pos_;
    //-----[haoz:] is the following code necessary??-------------//
    SkipToLineEnd(data_, &pos_, size_);
    ++pos_;
    if (data_[pos_] != '+') printf("GetNextFastq core dump is pos: %d\n",  pos_);
    assert(data_[pos_] == '+');// pos0 was the start of tag
    return pos0;
}

#define pre_cpe_read_num 1024

#define eval_read_size 300


FILE *file1_ptr;
FILE *file2_ptr;

static void *process(void *shared, int step, void *_data)
{
	ktp_aux_t *aux = (ktp_aux_t*)shared;
	ktp_data_t *data = (ktp_data_t*)_data;
	int i;
	if (step == 0) {
        double t0 = GetTime();
#ifdef use_cpe_step0
        static long long now_file1_pos = 0;
        static long long now_file2_pos = 0;
        static long long tot_block_size = cpe_num * pre_cpe_read_num * eval_read_size;
        static long long block_size = pre_cpe_read_num * eval_read_size;
        char* block_buffer = (char*)malloc(tot_block_size + 1024);
        char* block_buffer2 = (char*)malloc(tot_block_size + 1024);
        ktp_data_t *ret;
        ret = calloc(1, sizeof(ktp_data_t));
        if (bwa_verbose >= 3)
            fprintf(stderr, "[M::%s] read block (%ld)...\n", __func__, tot_block_size);

//        printf("file ptr %p %p\n", file1_ptr, file2_ptr);
//        printf("seek pos %lld %lld\n", now_file1_pos, now_file2_pos);
        fseek(file1_ptr, now_file1_pos, SEEK_SET);
        long long read_size = fread(block_buffer, 1, tot_block_size, file1_ptr);
//        printf("read size1 : %lld\n", read_size);
        long long real_size;
        if(read_size != tot_block_size) {
            real_size = read_size;
        } else {
            real_size = GetNextFastq(block_buffer, read_size - (1 << 10), read_size);
        }
        now_file1_pos += real_size;
//        printf("file1 read size %lld, real size %lld\n", read_size, real_size);

        fseek(file2_ptr, now_file2_pos, SEEK_SET);
        long long read_size2 = fread(block_buffer2, 1, tot_block_size, file2_ptr);
//        printf("read size2 : %lld\n", read_size2);
        long long real_size2;
        if(read_size2 != tot_block_size) {
            real_size2 = read_size2;
        } else {
            real_size2 = GetNextFastq(block_buffer2, read_size2 - (1 << 10), read_size2);
        }
        now_file2_pos += real_size2;
//        printf("file2 read size %lld, real size %lld\n", read_size2, real_size2);

        assert(real_size == real_size2);
        if(real_size == 0) {
            free(block_buffer);
            free(block_buffer2);
            free(ret);
            return 0;
        }
        ret->block_buffer = block_buffer;
        ret->block_buffer2 = block_buffer2;
        ret->block_size = real_size;
        ret->block_size2 = real_size2;
        // print the first 10 char in buffer
        printf("buffer1 : ");
        for(int i = 0; i < 10; i++) {
            printf("%c", block_buffer[i]);
        }
        printf("\n");
        printf("buffer2 : ");
        for(int i = 0; i < 10; i++) {
            printf("%c", block_buffer2[i]);
        }
        printf("\n");

#else
		ktp_data_t *ret;
		int64_t size = 0;
		ret = calloc(1, sizeof(ktp_data_t));
        double t1 = GetTime();
		ret->seqs = bseq_read(aux->actual_chunk_size, &ret->n_seqs, aux->ks, aux->ks2);
        t_step1_1 += GetTime() - t1;
		if (ret->seqs == 0) {
			free(ret);
            t_step1 += GetTime() - t0;
			return 0;
		}
		if (!aux->copy_comment)
			for (i = 0; i < ret->n_seqs; ++i) {
				free(ret->seqs[i].comment);
				ret->seqs[i].comment = 0;
			}
		for (i = 0; i < ret->n_seqs; ++i) size += ret->seqs[i].l_seq;
		if (bwa_verbose >= 3)
			fprintf(stderr, "[M::%s] read %d sequences (%ld bp)...\n", __func__, ret->n_seqs, (long)size);
#endif
        t_step1 += GetTime() - t0;
		return ret;
	} else if (step == 1) {
        double t0 = GetTime();
		const mem_opt_t *opt = aux->opt;
		const bwaidx_t *idx = aux->idx;
		if (opt->flag & MEM_F_SMARTPE) {
			//TODO
            assert(0);
		} else {

#ifdef use_cpe_step0
            mem_process_seqs_merge2(opt, idx->bwt, idx->bns, idx->pac, aux->n_processed, &(data->n_seqs), &(data->seqs), data->block_buffer, data->block_buffer2, data->block_size, data->block_size2, aux->pes0);
#else
            mem_process_seqs_merge(opt, idx->bwt, idx->bns, idx->pac, aux->n_processed, data->n_seqs, data->seqs, aux->pes0);
//            mem_process_seqs(opt, idx->bwt, idx->bns, idx->pac, aux->n_processed, data->n_seqs, data->seqs, aux->pes0);
#endif
//            printf("now data->n_seqs %d\n", data->n_seqs);
        }
		aux->n_processed += data->n_seqs;
        t_step2 += GetTime() - t0;
		return data;
	} else if (step == 2) {
        double t0 = GetTime();
		for (i = 0; i < data->n_seqs; ++i) {
            //if(file_out_sam != NULL) {
            //if(file_out_fd != 0) {
            double t1 = GetTime();
            //if (data->seqs[i].sam) err_fputs(data->seqs[i].sam, file_out_sam);
//            printf("sam %p\n", data->seqs[i].sam);
            if (data->seqs[i].sam) my_align_write(data->seqs[i].sam, file_out_fd, 0);
            t_step3_1 += GetTime() - t1;
            //}
			//if (data->seqs[i].sam) err_fputs(data->seqs[i].sam, stdout);
#ifdef use_cpe_step0
#else
			free(data->seqs[i].name); free(data->seqs[i].comment);
			free(data->seqs[i].seq); free(data->seqs[i].qual);
            if (data->seqs[i].is_new_addr == 1) {
//				wrap_free(data->seqs[i].sam);
				free(data->seqs[i].sam);
			}
#endif
//            free(data->seqs[i].sam);
		}
//        printf("free seqs %p\n", data->seqs);
		free(data->seqs); free(data);
        t_step3 += GetTime() - t0;
		return 0;
	} else if(step == 3) {
        my_align_write(NULL, file_out_fd, 1, sam_out_file_name);
    }
	return 0;
}

static void update_a(mem_opt_t *opt, const mem_opt_t *opt0)
{
	if (opt0->a) { // matching score is changed
		if (!opt0->b) opt->b *= opt->a;
		if (!opt0->T) opt->T *= opt->a;
		if (!opt0->o_del) opt->o_del *= opt->a;
		if (!opt0->e_del) opt->e_del *= opt->a;
		if (!opt0->o_ins) opt->o_ins *= opt->a;
		if (!opt0->e_ins) opt->e_ins *= opt->a;
		if (!opt0->zdrop) opt->zdrop *= opt->a;
		if (!opt0->pen_clip5) opt->pen_clip5 *= opt->a;
		if (!opt0->pen_clip3) opt->pen_clip3 *= opt->a;
		if (!opt0->pen_unpaired) opt->pen_unpaired *= opt->a;
	}
}


typedef struct{
    char* big_buffer;
    long long cpe_buffer_size;
} Para_malloc;

int main_mem(int argc, char *argv[])
{

    int local_my_rank = 0;
#ifdef use_my_mpi
    MPI_Comm_rank(MPI_COMM_WORLD, &local_my_rank);
#endif
	mem_opt_t *opt, opt0;
	int fd, fd2;
    int i, c, ignore_alt = 0, no_mt_io = 0;
	int fixed_chunk_size = -1;
	//gzFile fp, fp2 = 0;
	char *p, *rg_line = 0, *hdr_line = 0;
	const char *mode = 0;
	void *ko = 0, *ko2 = 0;
	mem_pestat_t pes[4];
	ktp_aux_t aux;

	memset(&aux, 0, sizeof(ktp_aux_t));
	memset(pes, 0, 4 * sizeof(mem_pestat_t));
	for (i = 0; i < 4; ++i) pes[i].failed = 1;

	aux.opt = opt = mem_opt_init();
	memset(&opt0, 0, sizeof(mem_opt_t));
	while ((c = getopt(argc, argv, "51qpaMCSPVYjuk:c:v:s:r:t:R:A:B:O:E:U:w:L:d:T:Q:D:m:I:N:o:f:W:x:G:h:y:K:X:H:F:z:")) >= 0) {
		if (c == 'k') opt->min_seed_len = atoi(optarg), opt0.min_seed_len = 1;
		else if (c == '1') no_mt_io = 1;
		else if (c == 'x') mode = optarg;
		else if (c == 'w') opt->w = atoi(optarg), opt0.w = 1;
		else if (c == 'A') opt->a = atoi(optarg), opt0.a = 1;
		else if (c == 'B') opt->b = atoi(optarg), opt0.b = 1;
		else if (c == 'T') opt->T = atoi(optarg), opt0.T = 1;
		else if (c == 'U') opt->pen_unpaired = atoi(optarg), opt0.pen_unpaired = 1;
		else if (c == 't') opt->n_threads = atoi(optarg), opt->n_threads = opt->n_threads > 1? opt->n_threads : 1;
		else if (c == 'P') opt->flag |= MEM_F_NOPAIRING;
		else if (c == 'a') opt->flag |= MEM_F_ALL;
		else if (c == 'p') opt->flag |= MEM_F_PE | MEM_F_SMARTPE;
		else if (c == 'M') opt->flag |= MEM_F_NO_MULTI;
		else if (c == 'S') opt->flag |= MEM_F_NO_RESCUE;
		else if (c == 'Y') opt->flag |= MEM_F_SOFTCLIP;
		else if (c == 'V') opt->flag |= MEM_F_REF_HDR;
		else if (c == '5') opt->flag |= MEM_F_PRIMARY5 | MEM_F_KEEP_SUPP_MAPQ; // always apply MEM_F_KEEP_SUPP_MAPQ with -5
		else if (c == 'q') opt->flag |= MEM_F_KEEP_SUPP_MAPQ;
		else if (c == 'u') opt->flag |= MEM_F_XB;
		else if (c == 'c') opt->max_occ = atoi(optarg), opt0.max_occ = 1;
		else if (c == 'd') opt->zdrop = atoi(optarg), opt0.zdrop = 1;
		else if (c == 'v') bwa_verbose = atoi(optarg);
		else if (c == 'j') ignore_alt = 1;
		else if (c == 'r') opt->split_factor = atof(optarg), opt0.split_factor = 1.;
		else if (c == 'D') opt->drop_ratio = atof(optarg), opt0.drop_ratio = 1.;
		else if (c == 'm') opt->max_matesw = atoi(optarg), opt0.max_matesw = 1;
		else if (c == 's') opt->split_width = atoi(optarg), opt0.split_width = 1;
		else if (c == 'G') opt->max_chain_gap = atoi(optarg), opt0.max_chain_gap = 1;
		else if (c == 'N') opt->max_chain_extend = atoi(optarg), opt0.max_chain_extend = 1;
#ifdef use_my_mpi
        else if (c == 'o' || c == 'f') {
            sprintf(sam_out_file_name, "%s_%d", optarg, local_my_rank);
            //file_out_sam = fopen(sam_out_file_name, "wb");
            file_out_fd = open(sam_out_file_name, O_CREAT | O_RDWR | O_DIRECT, 0666);
            if (file_out_fd == -1) {
                perror("Failed to open file");
                fprintf(stderr, "Error code: %d\n", errno);
            }
            fprintf(stderr, "the output file is %s\n", sam_out_file_name);
        }
#else
        else if (c == 'o' || c == 'f') {
            //file_out_sam = fopen(optarg, "wb");
            sprintf(sam_out_file_name, "%s", optarg);
            file_out_fd = open(sam_out_file_name, O_CREAT | O_RDWR | O_DIRECT, 0666);
            if (file_out_fd == -1) {
                perror("Failed to open file");
                fprintf(stderr, "Error code: %d\n", errno);
            }
            fprintf(stderr, "the output file is %s\n", sam_out_file_name);
        }
#endif
        //xreopen(optarg, "wb", stdout);
        else if (c == 'W') opt->min_chain_weight = atoi(optarg), opt0.min_chain_weight = 1;
        else if (c == 'y') opt->max_mem_intv = atol(optarg), opt0.max_mem_intv = 1;
		else if (c == 'C') aux.copy_comment = 1;
		else if (c == 'K') fixed_chunk_size = atoi(optarg);
		else if (c == 'X') opt->mask_level = atof(optarg);
		else if (c == 'F') bwa_dbg = atoi(optarg);
		else if (c == 'h') {
			opt0.max_XA_hits = opt0.max_XA_hits_alt = 1;
			opt->max_XA_hits = opt->max_XA_hits_alt = strtol(optarg, &p, 10);
			if (*p != 0 && ispunct(*p) && isdigit(p[1]))
				opt->max_XA_hits_alt = strtol(p+1, &p, 10);
		}
		else if (c == 'z') opt->XA_drop_ratio = atof(optarg);
		else if (c == 'Q') {
			opt0.mapQ_coef_len = 1;
			opt->mapQ_coef_len = atoi(optarg);
			opt->mapQ_coef_fac = opt->mapQ_coef_len > 0? log(opt->mapQ_coef_len) : 0;
		} else if (c == 'O') {
			opt0.o_del = opt0.o_ins = 1;
			opt->o_del = opt->o_ins = strtol(optarg, &p, 10);
			if (*p != 0 && ispunct(*p) && isdigit(p[1]))
				opt->o_ins = strtol(p+1, &p, 10);
		} else if (c == 'E') {
			opt0.e_del = opt0.e_ins = 1;
			opt->e_del = opt->e_ins = strtol(optarg, &p, 10);
			if (*p != 0 && ispunct(*p) && isdigit(p[1]))
				opt->e_ins = strtol(p+1, &p, 10);
		} else if (c == 'L') {
			opt0.pen_clip5 = opt0.pen_clip3 = 1;
			opt->pen_clip5 = opt->pen_clip3 = strtol(optarg, &p, 10);
			if (*p != 0 && ispunct(*p) && isdigit(p[1]))
				opt->pen_clip3 = strtol(p+1, &p, 10);
		} else if (c == 'R') {
			if ((rg_line = bwa_set_rg(optarg)) == 0) return 1; // FIXME: memory leak
		} else if (c == 'H') {
			if (optarg[0] != '@') {
				FILE *fp;
				if ((fp = fopen(optarg, "r")) != 0) {
					char *buf;
					buf = calloc(1, 0x10000);
					while (fgets(buf, 0xffff, fp)) {
						i = strlen(buf);
						assert(buf[i-1] == '\n'); // a long line
						buf[i-1] = 0;
						hdr_line = bwa_insert_header(buf, hdr_line);
					}
					free(buf);
					fclose(fp);
				}
			} else hdr_line = bwa_insert_header(optarg, hdr_line);
		} else if (c == 'I') { // specify the insert size distribution
			aux.pes0 = pes;
			pes[1].failed = 0;
			pes[1].avg = strtod(optarg, &p);
			pes[1].std = pes[1].avg * .1;
			if (*p != 0 && ispunct(*p) && isdigit(p[1]))
				pes[1].std = strtod(p+1, &p);
			pes[1].high = (int)(pes[1].avg + 4. * pes[1].std + .499);
			pes[1].low  = (int)(pes[1].avg - 4. * pes[1].std + .499);
			if (pes[1].low < 1) pes[1].low = 1;
			if (*p != 0 && ispunct(*p) && isdigit(p[1]))
				pes[1].high = (int)(strtod(p+1, &p) + .499);
			if (*p != 0 && ispunct(*p) && isdigit(p[1]))
				pes[1].low  = (int)(strtod(p+1, &p) + .499);
			if (bwa_verbose >= 3)
				fprintf(stderr, "[M::%s] mean insert size: %.3f, stddev: %.3f, max: %d, min: %d\n",
						__func__, pes[1].avg, pes[1].std, pes[1].high, pes[1].low);
		}
		else return 1;
	}

	if (rg_line) {
		hdr_line = bwa_insert_header(rg_line, hdr_line);
		free(rg_line);
	}

	if (opt->n_threads < 1) opt->n_threads = 1;
	if (optind + 1 >= argc || optind + 3 < argc) {
		fprintf(stderr, "\n");
		fprintf(stderr, "Usage: bwa mem [options] <idxbase> <in1.fq> [in2.fq]\n\n");
		fprintf(stderr, "Algorithm options:\n\n");
		fprintf(stderr, "       -t INT        number of threads [%d]\n", opt->n_threads);
		fprintf(stderr, "       -k INT        minimum seed length [%d]\n", opt->min_seed_len);
		fprintf(stderr, "       -w INT        band width for banded alignment [%d]\n", opt->w);
		fprintf(stderr, "       -d INT        off-diagonal X-dropoff [%d]\n", opt->zdrop);
		fprintf(stderr, "       -r FLOAT      look for internal seeds inside a seed longer than {-k} * FLOAT [%g]\n", opt->split_factor);
		fprintf(stderr, "       -y INT        seed occurrence for the 3rd round seeding [%ld]\n", (long)opt->max_mem_intv);
//		fprintf(stderr, "       -s INT        look for internal seeds inside a seed with less than INT occ [%d]\n", opt->split_width);
		fprintf(stderr, "       -c INT        skip seeds with more than INT occurrences [%d]\n", opt->max_occ);
		fprintf(stderr, "       -D FLOAT      drop chains shorter than FLOAT fraction of the longest overlapping chain [%.2f]\n", opt->drop_ratio);
		fprintf(stderr, "       -W INT        discard a chain if seeded bases shorter than INT [0]\n");
		fprintf(stderr, "       -m INT        perform at most INT rounds of mate rescues for each read [%d]\n", opt->max_matesw);
		fprintf(stderr, "       -S            skip mate rescue\n");
		fprintf(stderr, "       -P            skip pairing; mate rescue performed unless -S also in use\n");
		fprintf(stderr, "\nScoring options:\n\n");
		fprintf(stderr, "       -A INT        score for a sequence match, which scales options -TdBOELU unless overridden [%d]\n", opt->a);
		fprintf(stderr, "       -B INT        penalty for a mismatch [%d]\n", opt->b);
		fprintf(stderr, "       -O INT[,INT]  gap open penalties for deletions and insertions [%d,%d]\n", opt->o_del, opt->o_ins);
		fprintf(stderr, "       -E INT[,INT]  gap extension penalty; a gap of size k cost '{-O} + {-E}*k' [%d,%d]\n", opt->e_del, opt->e_ins);
		fprintf(stderr, "       -L INT[,INT]  penalty for 5'- and 3'-end clipping [%d,%d]\n", opt->pen_clip5, opt->pen_clip3);
		fprintf(stderr, "       -U INT        penalty for an unpaired read pair [%d]\n\n", opt->pen_unpaired);
		fprintf(stderr, "       -x STR        read type. Setting -x changes multiple parameters unless overridden [null]\n");
		fprintf(stderr, "                     pacbio: -k17 -W40 -r10 -A1 -B1 -O1 -E1 -L0  (PacBio reads to ref)\n");
		fprintf(stderr, "                     ont2d: -k14 -W20 -r10 -A1 -B1 -O1 -E1 -L0  (Oxford Nanopore 2D-reads to ref)\n");
		fprintf(stderr, "                     intractg: -B9 -O16 -L5  (intra-species contigs to ref)\n");
		fprintf(stderr, "\nInput/output options:\n\n");
		fprintf(stderr, "       -p            smart pairing (ignoring in2.fq)\n");
		fprintf(stderr, "       -R STR        read group header line such as '@RG\\tID:foo\\tSM:bar' [null]\n");
		fprintf(stderr, "       -H STR/FILE   insert STR to header if it starts with @; or insert lines in FILE [null]\n");
		fprintf(stderr, "       -o FILE       sam file to output results to [stdout]\n");
		fprintf(stderr, "       -j            treat ALT contigs as part of the primary assembly (i.e. ignore <idxbase>.alt file)\n");
		fprintf(stderr, "       -5            for split alignment, take the alignment with the smallest query (not genomic) coordinate as primary\n");
		fprintf(stderr, "       -q            don't modify mapQ of supplementary alignments\n");
		fprintf(stderr, "       -K INT        process INT input bases in each batch regardless of nThreads (for reproducibility) []\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "       -v INT        verbosity level: 1=error, 2=warning, 3=message, 4+=debugging [%d]\n", bwa_verbose);
		fprintf(stderr, "       -T INT        minimum score to output [%d]\n", opt->T);
		fprintf(stderr, "       -h INT[,INT]  if there are <INT hits with score >%.2f%% of the max score, output all in XA [%d,%d]\n", 
				opt->XA_drop_ratio * 100.0,
				opt->max_XA_hits, opt->max_XA_hits_alt);
		fprintf(stderr, "                     A second value may be given for alternate sequences.\n");
		fprintf(stderr, "       -z FLOAT      The fraction of the max score to use with -h [%f].\n", opt->XA_drop_ratio);
		fprintf(stderr, "                     specify the mean, standard deviation (10%% of the mean if absent), max\n");
		fprintf(stderr, "       -a            output all alignments for SE or unpaired PE\n");
		fprintf(stderr, "       -C            append FASTA/FASTQ comment to SAM output\n");
		fprintf(stderr, "       -V            output the reference FASTA header in the XR tag\n");
		fprintf(stderr, "       -Y            use soft clipping for supplementary alignments\n");
		fprintf(stderr, "       -M            mark shorter split hits as secondary\n\n");
		fprintf(stderr, "       -I FLOAT[,FLOAT[,INT[,INT]]]\n");
		fprintf(stderr, "                     specify the mean, standard deviation (10%% of the mean if absent), max\n");
		fprintf(stderr, "                     (4 sigma from the mean if absent) and min of the insert size distribution.\n");
		fprintf(stderr, "                     FR orientation only. [inferred]\n");
		fprintf(stderr, "       -u            output XB instead of XA; XB is XA with the alignment score and mapping quality added.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Note: Please read the man page for detailed description of the command line and options.\n");
		fprintf(stderr, "\n");
		free(opt);
		return 1;
	}

	if (mode) {
		if (strcmp(mode, "intractg") == 0) {
			if (!opt0.o_del) opt->o_del = 16;
			if (!opt0.o_ins) opt->o_ins = 16;
			if (!opt0.b) opt->b = 9;
			if (!opt0.pen_clip5) opt->pen_clip5 = 5;
			if (!opt0.pen_clip3) opt->pen_clip3 = 5;
		} else if (strcmp(mode, "pacbio") == 0 || strcmp(mode, "pbref") == 0 || strcmp(mode, "ont2d") == 0) {
			if (!opt0.o_del) opt->o_del = 1;
			if (!opt0.e_del) opt->e_del = 1;
			if (!opt0.o_ins) opt->o_ins = 1;
			if (!opt0.e_ins) opt->e_ins = 1;
			if (!opt0.b) opt->b = 1;
			if (opt0.split_factor == 0.) opt->split_factor = 10.;
			if (strcmp(mode, "ont2d") == 0) {
				if (!opt0.min_chain_weight) opt->min_chain_weight = 20;
				if (!opt0.min_seed_len) opt->min_seed_len = 14;
				if (!opt0.pen_clip5) opt->pen_clip5 = 0;
				if (!opt0.pen_clip3) opt->pen_clip3 = 0;
			} else {
				if (!opt0.min_chain_weight) opt->min_chain_weight = 40;
				if (!opt0.min_seed_len) opt->min_seed_len = 17;
				if (!opt0.pen_clip5) opt->pen_clip5 = 0;
				if (!opt0.pen_clip3) opt->pen_clip3 = 0;
			}
		} else {
			fprintf(stderr, "[E::%s] unknown read type '%s'\n", __func__, mode);
			return 1; // FIXME memory leak
		}
	} else update_a(opt, &opt0);
	bwa_fill_scmat(opt->a, opt->b, opt->mat);

	aux.idx = bwa_idx_load_from_shm(argv[optind]);
	if (aux.idx == 0) {
		if ((aux.idx = bwa_idx_load(argv[optind], BWA_IDX_ALL)) == 0) return 1; // FIXME: memory leak
	} else if (bwa_verbose >= 3)
		fprintf(stderr, "[M::%s] load the bwa index from shared memory\n", __func__);
	if (ignore_alt)
		for (i = 0; i < aux.idx->bns->n_seqs; ++i)
			aux.idx->bns->anns[i].is_alt = 0;

    int in_rank = local_my_rank + 1;
    //int in_rank = (local_my_rank + 1 + 1) % 6 + 1;
    //int in_rank = 2;
#ifdef use_my_mpi
    char in_filename1[256];
    sprintf(in_filename1, "%s_%d", argv[optind + 1], in_rank);
    fprintf(stderr, "the input file1 is %s\n", in_filename1);
    file1_ptr = fopen(in_filename1, "r");
	ko = kopen(in_filename1, &fd);
#else
    file1_ptr = fopen(argv[optind + 1], "r");
	ko = kopen(argv[optind + 1], &fd);
#endif
    
	if (ko == 0) {
		if (bwa_verbose >= 1) fprintf(stderr, "[E::%s] fail to open file `%s'.\n", __func__, argv[optind + 1]);
		return 1;
	}
	//fp = gzdopen(fd, "r");
	//aux.ks = kseq_init(fp);
	aux.ks = kseq_init(fd);
	if (optind + 2 < argc) {
		if (opt->flag&MEM_F_PE) {
			if (bwa_verbose >= 2)
				fprintf(stderr, "[W::%s] when '-p' is in use, the second query file is ignored.\n", __func__);
		} else {
#ifdef use_my_mpi
            char in_filename2[256];
            sprintf(in_filename2, "%s_%d", argv[optind + 2], in_rank);
            fprintf(stderr, "the input file2 is %s\n", in_filename2);
            file2_ptr = fopen(in_filename2, "r");
            ko2 = kopen(in_filename2, &fd2);
#else
            file2_ptr = fopen(argv[optind + 2], "r");
            ko2 = kopen(argv[optind + 2], &fd2);
#endif
			if (ko2 == 0) {
				if (bwa_verbose >= 1) fprintf(stderr, "[E::%s] fail to open file `%s'.\n", __func__, argv[optind + 2]);
				return 1;
			}
			//fp2 = gzdopen(fd2, "r");
			//aux.ks2 = kseq_init(fp2);
			aux.ks2 = kseq_init(fd2);
			opt->flag |= MEM_F_PE;
		}
	}
	//bwa_print_sam_hdr(aux.idx->bns, hdr_line);
	aux.actual_chunk_size = fixed_chunk_size > 0? fixed_chunk_size : opt->chunk_size * opt->n_threads;

#ifdef use_cgs_mode
    athread_init_cgs();
#else
    athread_init();
#endif


#define cpe_malloc_tot_size (1ll * 384 * (24ll << 20))

#ifdef use_cgs_mode
    char* big_buffer = (char*)_sw_xmalloc(cpe_malloc_tot_size);
    memset(big_buffer, 0, cpe_malloc_tot_size);
    Para_malloc p_malloc;
    p_malloc.big_buffer = big_buffer;
    p_malloc.cpe_buffer_size = cpe_malloc_tot_size / cpe_num;
    __real_athread_spawn_cgs((void*)slave_state_init, &p_malloc, 1);
    athread_join_cgs();
#endif


#ifdef use_lwpf3
    lwpf_init(NULL);
#endif

//    htmalloccount_init();

    double t0 = GetTime();
    if(no_mt_io) kt_pipeline_single(1, process, &aux, 3);
    else kt_pipeline_queue(3, process, &aux, 3);

//    htmalloccount_print();

#ifdef use_lwpf3
    char filename[50];
#ifdef use_cgs_mode
    sprintf(filename, "lwpf_%d_6CG.log", local_my_rank);
#else
    sprintf(filename, "lwpf_%d_1CG.log", local_my_rank);
#endif
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Failed to open lwpf.log");
        return 1;
    }
    lwpf_report_summary(file);
    lwpf_report_detail(file);
    fclose(file);
#endif

    //__real_athread_spawn_cgs((void*)slave_state_print, 0, 1);
    //athread_join_cgs();



	//printf("print spwan\n");
    //for(int i = 0; i < cpe_num; i++) cpe_is_over[i] = 0;

    //for(long cg_id = 0; cg_id < 6; cg_id++) {
    //    for(long cpe_id = 0; cpe_id < 64; cpe_id++) {
    //        *((long*)(0x800000008100 + (cg_id << 40ll) + (cpe_id << 24ll))) = 3;
    //    }
    //}
    //for(long cg_id = 0; cg_id < 6; cg_id++) {
    //    for(long cpe_id = 0; cpe_id < 64; cpe_id++) {
    //        *((long*)(0x800000008000 + (cg_id << 40ll) + (cpe_id << 24ll))) = slave_state_print;
    //    }
    //}
    //for(long cg_id = 0; cg_id < 6; cg_id++) {
    //    for(long cpe_id = 0; cpe_id < 64; cpe_id++) {
    //        *((long*)(0x800000008100 + (cg_id << 40ll) + (cpe_id << 24ll))) = 0;
    //    }
    //}
    //asm volatile("memb\n\t":::);

    //while(1) {
    //    int sum = 0;
    //    for(int i = 0; i < cpe_num; i++)
    //        sum += cpe_is_over[i];
    //    //printf("mpe sum %d\n", sum);
    //    if(sum == cpe_num) break;
    //    sleep(1);
    //}

    

    //athread_halt();

    t_tot += GetTime() - t0;


    fprintf(stderr, "rank %d [timer] tot : %.2f, step1 : %.2f (%.2f [%.2f]), step2 : %.2f, step3 : %.2f (%.2f), malloc : %.2f, free : %.2f\n", local_my_rank, t_tot, t_step1, t_step1_1, t_step1_read, t_step2, t_step3, t_step3_1, t_malloc, t_free);
    fprintf(stderr, "rank %d [timer] step2 --- work1 : %.2f (1 : %.2f, 2 : %.2f, 3 : %.2f, 4 : %.2f, 5 : %.2f, 6 : %.2f), work2 : %.2f (1 : %.2f, 2 : %.2f, 3 : %.2f, 4 : %.2f, 5 : %.2f)\n", 
            local_my_rank, t_work1, t_work1_1, t_work1_2, t_work1_3, t_work1_4, t_work1_5, t_work1_6, t_work2, t_work2_1, t_work2_2, t_work2_3, t_work2_4, t_work2_5);


	free(hdr_line);
	free(opt);
	bwa_idx_destroy(aux.idx);
	kseq_destroy(aux.ks);
	//err_gzclose(fp);
    kclose(ko);
	if (aux.ks2) {
		kseq_destroy(aux.ks2);
		//err_gzclose(fp2);
        kclose(ko2);
	}
    if(file_out_sam != NULL) fclose(file_out_sam);
    //if(file_out_fd != -1) close(file_out_fd);
	return 0;
}

int main_fastmap(int argc, char *argv[])
{
	int c, i, min_iwidth = 20, min_len = 17, print_seq = 0, min_intv = 1, max_len = INT_MAX;
	uint64_t max_intv = 0;
	kseq_t *seq;
	bwtint_t k;
	gzFile fp;
	smem_i *itr;
	const bwtintv_v *a;
	bwaidx_t *idx;

	while ((c = getopt(argc, argv, "w:l:pi:I:L:")) >= 0) {
		switch (c) {
			case 'p': print_seq = 1; break;
			case 'w': min_iwidth = atoi(optarg); break;
			case 'l': min_len = atoi(optarg); break;
			case 'i': min_intv = atoi(optarg); break;
			case 'I': max_intv = atol(optarg); break;
			case 'L': max_len  = atoi(optarg); break;
		    default: return 1;
		}
	}
	if (optind + 1 >= argc) {
		fprintf(stderr, "\n");
		fprintf(stderr, "Usage:   bwa fastmap [options] <idxbase> <in.fq>\n\n");
		fprintf(stderr, "Options: -l INT    min SMEM length to output [%d]\n", min_len);
		fprintf(stderr, "         -w INT    max interval size to find coordiantes [%d]\n", min_iwidth);
		fprintf(stderr, "         -i INT    min SMEM interval size [%d]\n", min_intv);
		fprintf(stderr, "         -L INT    max MEM length [%d]\n", max_len);
		fprintf(stderr, "         -I INT    stop if MEM is longer than -l with a size less than INT [%ld]\n", (long)max_intv);
		fprintf(stderr, "\n");
		return 1;
	}

	fp = xzopen(argv[optind + 1], "r");
	seq = kseq_init(fp);
	if ((idx = bwa_idx_load(argv[optind], BWA_IDX_BWT|BWA_IDX_BNS)) == 0) return 1;
	itr = smem_itr_init(idx->bwt);
	smem_config(itr, min_intv, max_len, max_intv);
	while (kseq_read(seq) >= 0) {
		err_printf("SQ\t%s\t%ld", seq->name.s, seq->seq.l);
		if (print_seq) {
			err_putchar('\t');
			err_puts(seq->seq.s);
		} else err_putchar('\n');
		for (i = 0; i < seq->seq.l; ++i)
			seq->seq.s[i] = nst_nt4_table[(int)seq->seq.s[i]];
		smem_set_query(itr, seq->seq.l, (uint8_t*)seq->seq.s);
		while ((a = smem_next(itr)) != 0) {
			for (i = 0; i < a->n; ++i) {
				bwtintv_t *p = &a->a[i];
				if ((uint32_t)p->info - (p->info>>32) < min_len) continue;
				err_printf("EM\t%d\t%d\t%ld", (uint32_t)(p->info>>32), (uint32_t)p->info, (long)p->x[2]);
				if (p->x[2] <= min_iwidth) {
					for (k = 0; k < p->x[2]; ++k) {
						bwtint_t pos;
						int len, is_rev, ref_id;
						len  = (uint32_t)p->info - (p->info>>32);
						pos = bns_depos(idx->bns, bwt_sa(idx->bwt, p->x[0] + k), &is_rev);
						if (is_rev) pos -= len - 1;
						bns_cnt_ambi(idx->bns, pos, len, &ref_id);
						err_printf("\t%s:%c%ld", idx->bns->anns[ref_id].name, "+-"[is_rev], (long)(pos - idx->bns->anns[ref_id].offset) + 1);
					}
				} else err_puts("\t*");
				err_putchar('\n');
			}
		}
		err_puts("//");
	}

	smem_itr_destroy(itr);
	bwa_idx_destroy(idx);
	kseq_destroy(seq);
	err_gzclose(fp);
	return 0;
}
