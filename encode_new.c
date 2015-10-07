#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "common.h"
#include "options.h"
#include "bitstream.h"
#include "availbits.h"
#include "encode_new.h"

#define NUMTABLES 5
int vbrstats_new[15] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* There are really only 9 distinct lines in the allocation tables 
   each member of this table is an index into */
/* step_index[linenumber][index] */
static int step_index[9][16] = { 
  /*0*/ {0, 1, 3, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17},
  /*1*/ {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,17},
  /*2*/ {0, 1, 2, 3, 4, 5, 6,17, 0, 0, 0, 0, 0, 0, 0, 0},
  /*3*/ {0, 1, 2, 17,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  /*4*/ {0, 1, 2, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16},
  /*5*/ {0, 1, 2, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0},
  /* From ISO13818 Table B.1 */
  /*6*/ {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15},
  /*7*/ {0, 1, 2, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0},
  /*8*/ {0, 1, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static int nbal[9] = {4, 4, 3, 2, 4, 3, 4, 3, 2};


/*                      0, 1, 2, 3, 4,  5,  6,  7,  8,   9,   10,   11,   12,   13,   14,   15,    16,    17 */
/* The number of steps allowed */
static int steps[18] = {0, 3, 5, 7, 9,  15, 31, 63, 127, 255, 511,  1023, 2047, 4095, 8191, 16383, 32767, 65535};
/* The power of 2 just under the steps value */
static int steps2n[18]={0, 2, 4, 4, 8,  8,  16, 32, 64,  128, 256,  512,  1024, 2048, 4096, 8192,  16384, 32768};
/* The bits per codeword from TableB.4 */
static int bits[18] =  {0, 5, 7, 3, 10, 4,  5,  6,  7,   8,   9,    10,   11,   12,   13,   14,    15,    16};
/* Samples per codeword Table B.4 Page 53 */
//static int group[18] = {0, 3, 3, 1, 3,  1,  1,  1,  1,   1,   1,    1,    1,    1,    1,    1,     1,     1};
static int group[18] = {0, 1, 1, 3, 1,  3,  3,  3,  3,   3,   3,    3,    3,    3,    3,    3,     3,     3};

/* nbal */

/* The sblimits of the 5 allocation tables
   4 tables for MPEG-1
   1 table for MPEG-2 LSF */
static int table_sblimit[5] = {27, 30, 8, 12, 30};

/* Each table contains a list of allowable quantization steps.
   There are only 9 distinct lists of steps.
   This table gives the index of which of the 9 lists is being used 
   A "-1" entry means that it is above the sblimit for this table */
static int line[5][SBLIMIT] = {
 /*00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 */
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3,-1,-1,-1,-1,-1},
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3,-1,-1},
  {4, 4, 5, 5, 5, 5, 5, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, 
  {4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  /* LSF Table */
  {6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8}
};

/* This is ISO11172 Table B.1 */
double scalefactor[64] = { /* Equation for nth element = 2 / (cuberoot(2) ^ n) */
  2.00000000000000, 1.58740105196820, 1.25992104989487,
  1.00000000000000, 0.79370052598410, 0.62996052494744, 0.50000000000000,
  0.39685026299205, 0.31498026247372, 0.25000000000000, 0.19842513149602,
  0.15749013123686, 0.12500000000000, 0.09921256574801, 0.07874506561843,
  0.06250000000000, 0.04960628287401, 0.03937253280921, 0.03125000000000,
  0.02480314143700, 0.01968626640461, 0.01562500000000, 0.01240157071850,
  0.00984313320230, 0.00781250000000, 0.00620078535925, 0.00492156660115,
  0.00390625000000, 0.00310039267963, 0.00246078330058, 0.00195312500000,
  0.00155019633981, 0.00123039165029, 0.00097656250000, 0.00077509816991,
  0.00061519582514, 0.00048828125000, 0.00038754908495, 0.00030759791257,
  0.00024414062500, 0.00019377454248, 0.00015379895629, 0.00012207031250,
  0.00009688727124, 0.00007689947814, 0.00006103515625, 0.00004844363562,
  0.00003844973907, 0.00003051757813, 0.00002422181781, 0.00001922486954,
  0.00001525878906, 0.00001211090890, 0.00000961243477, 0.00000762939453,
  0.00000605545445, 0.00000480621738, 0.00000381469727, 0.00000302772723,
  0.00000240310869, 0.00000190734863, 0.00000151386361, 0.00000120155435,
  1E-20
};

/* ISO11172 Table C.5 Layer II Signal to Noise Raios 
   MFC FIX find a reference for these in terms of bits->SNR value
   Index into table is the steps index 
   index   steps    SNR
       0       0    0.00
       1       3    7.00
       2       5   11.00
       3       7   16.00
       4       9   20.84
  etc
*/
static double SNR[18] = { 0.00, 7.00, 11.00, 16.00, 20.84,
  25.28, 31.59, 37.75, 43.84,
  49.89, 55.93, 61.96, 67.98, 74.01,
  80.03, 86.05, 92.01, 98.01
};

int tablenum=0;

int encode_init(frame_info *frame) {
  int ws, bsp, br_per_ch, sfrq;

  bsp = frame->header->bitrate_index;
  br_per_ch = bitrate[frame->header->version][bsp] / frame->nch;
  ws = frame->header->sampling_frequency;
  sfrq = s_freq[frame->header->version][ws];
  /* decision rules refer to per-channel bitrates (kbits/sec/chan) */
  if (frame->header->version == MPEG_AUDIO_ID) {	/* MPEG-1 */
    if ((sfrq == 48 && br_per_ch >= 56)
	|| (br_per_ch >= 56 && br_per_ch <= 80))
      tablenum = 0;
    else if (sfrq != 48 && br_per_ch >= 96)
      tablenum = 1;
    else if (sfrq != 32 && br_per_ch <= 48)
      tablenum = 2;
    else
      tablenum = 3;
  } else {			/* MPEG-2 LSF */
    tablenum = 4;
  }
  fprintf(stdout,"encode_init: using tablenum %i with sblimit %i\n",tablenum, table_sblimit[tablenum]);

#define DUMPTABLESx
#ifdef DUMPTABLES 
  {
    int tablenumber,j,sblimit, sb;
    fprintf(stdout,"Tables B.21,b,c,d from ISO11172 and the LSF table from ISO13818\n");
    for (tablenumber=0;tablenumber<NUMTABLES;tablenumber++) {
      /* Print Table Header */
      fprintf(stdout,"Tablenum %i\n",tablenumber);
      fprintf(stdout,"sb nbal ");
      for (j=0;j<16;j++)
	fprintf(stdout,"%6i ",j);
      fprintf(stdout,"\n");
      fprintf(stdout,"-----------------------------------------------------------------------------------------------------------------------\n");
      
      sblimit = table_sblimit[tablenumber];
      for (sb=0;sb<SBLIMIT;sb++) {
	int thisline = line[tablenumber][sb];
	fprintf(stdout,"%2i %4i ",sb,nbal[thisline]);
	if (nbal[thisline] != 0)
	  for (j=0; j<(1<<nbal[thisline]); j++)
	    fprintf(stdout,"%6i ", steps[ step_index[thisline][j] ]);
	fprintf(stdout,"\n");
      }
      fprintf(stdout,"\n");
    }
    exit(0);
  }
#endif
  return (table_sblimit[tablenum]);
}

/* 
   scale_factor_calc
   pick_scale
   if JOINTSTEREO 
         combine_LR
         scale_factor_calc
   use psy model to determine SMR
   transmission pattern
   main_bit_allocation
   if (error protection)
         calc CRC
   encode_info
   if (error_protection)
         encode_CRC
   encode_bit_alloc
   encode_scale
   subband_quantization
   sample_encoding
*/


void scalefactor_calc_new (double sb_sample[][3][SCALE_BLOCK][SBLIMIT],
			unsigned int sf_index[][3][SBLIMIT], int nch,
			int sblimit)
{
  /* Optimized to use binary search instead of linear scan through the
     scalefactor table; guarantees to find scalefactor in only 5
     jumps/comparisons and not in {0 (lin. best) to 63 (lin. worst)}.
     Scalefactors for subbands > sblimit are no longer computed.
     Uses a single sblimit-loop.
     Patrick De Smet Oct 1999.   */
  int ch, gr;
  /* Using '--' loops to avoid possible "cmp value + bne/beq" compiler  */
  /* inefficiencies. Below loops should compile to "bne/beq" only code  */
  for (ch = nch; ch--;)
    for (gr = 3; gr--;) {
      int sb;
      for (sb = sblimit; sb--;) {
	int j;
	unsigned int l;
	register double temp;
	unsigned int scale_fac;
	/* Determination of max. over each set of 12 subband samples:  */
	/* PDS TODO: maybe this could/should ??!! be integrated into   */
	/* the subband filtering routines?                             */
	register double cur_max = fabs (sb_sample[ch][gr][SCALE_BLOCK - 1][sb]);
	for (j = SCALE_BLOCK - 1; j--;) {
	  if ((temp = fabs (sb_sample[ch][gr][j][sb])) > cur_max)
	    cur_max = temp;
	}
	/* PDS: binary search in the scalefactor table: */
	/* This is the real speed up: */
	for (l = 16, scale_fac = 32; l; l >>= 1) {
	  if (cur_max <= scalefactor[scale_fac])
	    scale_fac += l;
	  else
	    scale_fac -= l;
	}
	if (cur_max > scalefactor[scale_fac])
	  scale_fac--;
	sf_index[ch][gr][sb] = scale_fac;
	/* There is a direct way of working out the index, if the 
	   maximum value is known but since
	   it involves a log it isn't really speedy.
	   Items in the scalefactor[] table are calculated by:
	     the n'th entry = 2 / (cuberoot(2) ^ n)
	   And so using a bit of maths you get:
	  index = (int)(log(2.0/cur_max) / LNCUBEROOTTWO);
	  fprintf(stdout,"cur_max %.14lf scalefactorindex %i multiple %.14lf\n",cur_max, scale_fac, scalefactor[scale_fac]);
	*/
      }
    }
}
INLINE double mod (double a)
{
  return (a > 0) ? a : -a;
}

/* Combine L&R channels into a mono joint stereo channel */
void combine_LR_new (double sb_sample[2][3][SCALE_BLOCK][SBLIMIT],
		     double joint_sample[3][SCALE_BLOCK][SBLIMIT], int sblimit) {
  int sb, sample, gr;

  for (sb = 0; sb < sblimit; ++sb)
    for (sample = 0; sample < SCALE_BLOCK; ++sample)
      for (gr = 0; gr < 3; ++gr)
	joint_sample[gr][sample][sb] =
	  .5 * (sb_sample[0][gr][sample][sb] + sb_sample[1][gr][sample][sb]);
}

/* PURPOSE:For each subband, puts the smallest scalefactor of the 3
   associated with a frame into #max_sc#.  This is used
   used by Psychoacoustic Model I.
   Someone in dist10 source code's history, somebody wrote the following:
   "(I would recommend changin max_sc to min_sc)"
   
   In psy model 1, the *maximum* out of the scale picked here and
   the maximum SPL within each subband is selected. So I'd think that 
   a maximum here makes heaps of sense.

   MFC FIX: Feb 2003 - is this only needed for psy model 1?
*/
void find_sf_max (unsigned int sf_index[2][3][SBLIMIT], frame_info * frame,
		 double sf_max[2][SBLIMIT])
{
  int sb, gr, ch;
  int lowest_sf_index;
  int nch = frame->nch;
  int sblimit = frame->sblimit;

  for (ch = 0; ch < nch; ch++)
    for (sb = 0; sb < sblimit; sb++) {
      for (gr = 1, lowest_sf_index = sf_index[ch][0][sb]; gr < 3; gr++)
	if (lowest_sf_index > sf_index[ch][gr][sb])
	  lowest_sf_index = sf_index[ch][gr][sb];
      sf_max[ch][sb] = multiple[lowest_sf_index];
    }
  for (sb = sblimit; sb < SBLIMIT; sb++)
    sf_max[0][sb] = sf_max[1][sb] = 1E-20;
}

/*  sf_transmission_pattern  
    PURPOSE:For a given subband, determines whether to send 1, 2, or
    all 3 of the scalefactors, and fills in the scalefactor
    select information accordingly

    This is From ISO11172 Sect C.1.5.2.5 "coding of scalefactors"
    and
    Table C.4 "LayerII Scalefactors Transmission Pattern"
*/
void sf_transmission_pattern (unsigned int sf_index[2][3][SBLIMIT],
			   unsigned int sf_selectinfo[2][SBLIMIT],
			   frame_info * frame)
{
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int dscf[2];
  int class[2], i, j, k;
  static int pattern[5][5] = { {0x123, 0x122, 0x122, 0x133, 0x123},
  {0x113, 0x111, 0x111, 0x444, 0x113},
  {0x111, 0x111, 0x111, 0x333, 0x113},
  {0x222, 0x222, 0x222, 0x333, 0x123},
  {0x123, 0x122, 0x122, 0x133, 0x123}
  };

  for (k = 0; k < nch; k++)
    for (i = 0; i < sblimit; i++) {
      dscf[0] = (sf_index[k][0][i] - sf_index[k][1][i]);
      dscf[1] = (sf_index[k][1][i] - sf_index[k][2][i]);
      for (j = 0; j < 2; j++) {
	if (dscf[j] <= -3)
	  class[j] = 0;
	else if (dscf[j] > -3 && dscf[j] < 0)
	  class[j] = 1;
	else if (dscf[j] == 0)
	  class[j] = 2;
	else if (dscf[j] > 0 && dscf[j] < 3)
	  class[j] = 3;
	else
	  class[j] = 4;
      }
      switch (pattern[class[0]][class[1]]) {
      case 0x123:
	sf_selectinfo[k][i] = 0;
	break;
      case 0x122:
	sf_selectinfo[k][i] = 3;
	sf_index[k][2][i] = sf_index[k][1][i];
	break;
      case 0x133:
	sf_selectinfo[k][i] = 3;
	sf_index[k][1][i] = sf_index[k][2][i];
	break;
      case 0x113:
	sf_selectinfo[k][i] = 1;
	sf_index[k][1][i] = sf_index[k][0][i];
	break;
      case 0x111:
	sf_selectinfo[k][i] = 2;
	sf_index[k][1][i] = sf_index[k][2][i] = sf_index[k][0][i];
	break;
      case 0x222:
	sf_selectinfo[k][i] = 2;
	sf_index[k][0][i] = sf_index[k][2][i] = sf_index[k][1][i];
	break;
      case 0x333:
	sf_selectinfo[k][i] = 2;
	sf_index[k][0][i] = sf_index[k][1][i] = sf_index[k][2][i];
	break;
      case 0x444:
	sf_selectinfo[k][i] = 2;
	if (sf_index[k][0][i] > sf_index[k][2][i])
	  sf_index[k][0][i] = sf_index[k][2][i];
	sf_index[k][1][i] = sf_index[k][2][i] = sf_index[k][0][i];
      }
    }
}

void write_header (frame_info * frame, Bit_stream_struc * bs)
{
  frame_header *header = frame->header;

  putbits (bs, 0xfff, 12);	/* syncword 12 bits */
  put1bit (bs, header->version);	/* ID        1 bit  */
  putbits (bs, 4 - header->lay, 2);	/* layer     2 bits */
  put1bit (bs, !header->error_protection);	/* bit set => no err prot */
  putbits (bs, header->bitrate_index, 4);
  putbits (bs, header->sampling_frequency, 2);
  put1bit (bs, header->padding);
  put1bit (bs, header->extension);	/* private_bit */
  putbits (bs, header->mode, 2);
  putbits (bs, header->mode_ext, 2);
  put1bit (bs, header->copyright);
  put1bit (bs, header->original);
  putbits (bs, header->emphasis, 2);
}

/*************************************************************************
 encode_bit_alloc (Layer II)

 PURPOSE:Writes bit allocation information onto bitstream

 4,3,2, or 0 bits depending on the quantization table used.

************************************************************************/
void write_bit_alloc (unsigned int bit_alloc[2][SBLIMIT],
		       frame_info * frame, Bit_stream_struc * bs)
{
  int sb, ch;
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;

  for (sb = 0; sb < sblimit; sb++) {
    if (sb < jsbound) {
      for (ch = 0; ch < ((sb < jsbound) ? nch : 1); ch++)
	putbits (bs, bit_alloc[ch][sb], nbal[ line[tablenum][sb] ]); // (*alloc)[sb][0].bits);
    }
    else
      putbits (bs, bit_alloc[0][sb], nbal[ line[tablenum][sb] ]); //(*alloc)[sb][0].bits);
  }
}

/************************************************************************
 write_scalefactors

 PURPOSE:The encoded scalar factor information is arranged and
 queued into the output fifo to be transmitted.

 The three scale factors associated with
 a given subband and channel are transmitted in accordance
 with the scfsi, which is transmitted first.

************************************************************************/

void write_scalefactors (unsigned int bit_alloc[2][SBLIMIT],
	      unsigned int sf_selectinfo[2][SBLIMIT],
	      unsigned int sf_index[2][3][SBLIMIT], frame_info * frame,
	      Bit_stream_struc * bs)
{
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int sb, gr, ch;

  /* Write out the scalefactor selection information */
  for (sb = 0; sb < sblimit; sb++)
    for (ch = 0; ch < nch; ch++)
      if (bit_alloc[ch][sb])
	putbits (bs, sf_selectinfo[ch][sb], 2);

  for (sb = 0; sb < sblimit; sb++)
    for (ch = 0; ch < nch; ch++)
      if (bit_alloc[ch][sb])	/* above jsbound, bit_alloc[0][i] == ba[1][i] */
	switch (sf_selectinfo[ch][sb]) {
	case 0:
	  for (gr = 0; gr < 3; gr++)
	    putbits (bs, sf_index[ch][gr][sb], 6);
	  break;
	case 1:
	case 3:
	  putbits (bs, sf_index[ch][0][sb], 6);
	  putbits (bs, sf_index[ch][2][sb], 6);
	  break;
	case 2:
	  putbits (bs, sf_index[ch][0][sb], 6);
	}
}


/* ISO11172 Table C.6 Layer II quantization co-efficients */
static double a[18] = {
  0, 
  0.750000000, 0.625000000, 0.875000000, 0.562500000, 0.937500000,
  0.968750000, 0.984375000, 0.992187500, 0.996093750, 0.998046875,
  0.999023438, 0.999511719, 0.999755859, 0.999877930, 0.999938965,
  0.999969482, 0.999984741
};

static double b[18] = {
  0,
  -0.250000000, -0.375000000, -0.125000000, -0.437500000, -0.062500000,
  -0.031250000, -0.015625000, -0.007812500, -0.003906250, -0.001953125,
  -0.000976563, -0.000488281, -0.000244141, -0.000122070, -0.000061035,
  -0.000030518, -0.000015259
};

/************************************************************************
   subband_quantization (Layer II)

 PURPOSE:Quantizes subband samples to appropriate number of bits

 SEMANTICS:  Subband samples are divided by their scalefactors, which
 makes the quantization more efficient. The scaled samples are
 quantized by the function a*x+b, where a and b are functions of
 the number of quantization levels. The result is then truncated
 to the appropriate number of bits and the MSB is inverted.

 Note that for fractional 2's complement, inverting the MSB for a
 negative number x is equivalent to adding 1 to it.

************************************************************************/
void
subband_quantization_new (unsigned int sf_index[2][3][SBLIMIT],
		      double sb_samples[2][3][SCALE_BLOCK][SBLIMIT],
		      unsigned int j_scale[3][SBLIMIT],
		      double j_samps[3][SCALE_BLOCK][SBLIMIT],
		      unsigned int bit_alloc[2][SBLIMIT],
		      unsigned int sbband[2][3][SCALE_BLOCK][SBLIMIT],
		      frame_info * frame)
{
  int sb, j, ch, gr, qnt_coeff_index, sig;
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;
  double d;

    for (gr = 0; gr < 3; gr++)
      for (j = 0; j < SCALE_BLOCK; j++)
	for (sb = 0; sb < sblimit; sb++)
	  for (ch = 0; ch < ((sb < jsbound) ? nch : 1); ch++)

	  if (bit_alloc[ch][sb]) {
	    /* scale and quantize FLOATing point sample */
	    if (nch == 2 && sb >= jsbound)	/* use j-stereo samples */
	      d = j_samps[gr][j][sb] / scalefactor[j_scale[gr][sb]];
	    else
	      d = sb_samples[ch][gr][j][sb] / scalefactor[sf_index[ch][gr][sb]];

	    /* Check that the wrong scale factor hasn't been chosen -
	       which would result in a scaled sample being > 1.0 
	       This error shouldn't ever happen *unless* something went wrong in 
	       scalefactor calc

	     if (mod (d) > 1.0)
	       fprintf (stderr, "Not scaled properly %d %d %d %d\n", ch, gr, j,
	      sb); 
	    */
	    
	    {
	      /* 'index' indicates which "step line" we are using */
	      int index = line[tablenum][sb];
	      
	      /* Find the "step index" within that line */
	      qnt_coeff_index = step_index[index][bit_alloc[ch][sb]];
	    }
	    d = d * a[qnt_coeff_index] + b[qnt_coeff_index];

	    /* extract MSB N-1 bits from the FLOATing point sample */
	    if (d >= 0)
	      sig = 1;
	    else {
	      sig = 0;
	      d += 1.0;
	    }

	    sbband[ch][gr][j][sb] = (unsigned int) (d * (double)steps2n[qnt_coeff_index]);
	    /* tag the inverted sign bit to sbband at position N */
	    /* The bit inversion is a must for grouping with 3,5,9 steps
	       so it is done for all subbands */
	    if (sig)
	      sbband[ch][gr][j][sb] |= steps2n[qnt_coeff_index];
	  }

  /* Set everything above the sblimit to 0 */
  for (ch = 0; ch < nch; ch++)
    for (gr = 0; gr < 3; gr++)
      for (sb = 0; sb < SCALE_BLOCK; sb++)
	for (j = sblimit; j < SBLIMIT; j++)
	  sbband[ch][gr][sb][j] = 0;
}

/************************************************************************
    sample_encoding  

 PURPOSE:Put one frame of subband samples on to the bitstream

 SEMANTICS:  The number of bits allocated per sample is read from
 the bit allocation information #bit_alloc#.  Layer 2
 supports writing grouped samples for quantization steps
 that are not a power of 2.

***********************************************************************/
void write_samples_new (unsigned int sbband[2][3][SCALE_BLOCK][SBLIMIT],
		      unsigned int bit_alloc[2][SBLIMIT],
		      frame_info * frame, Bit_stream_struc * bs)
{
  unsigned int temp;
  unsigned int sb, j, ch, gr, x, y;
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;

  for (gr = 0; gr < 3; gr++)
    for (j = 0; j < SCALE_BLOCK; j += 3)
      for (sb = 0; sb < sblimit; sb++)
	for (ch = 0; ch < ((sb < jsbound) ? nch : 1); ch++)

	  if (bit_alloc[ch][sb]) {
	    int thisline = line[tablenum][sb];
	    int thisstep_index = step_index[thisline][bit_alloc[ch][sb]];
	    /* Check how many samples per codeword */
	    if (group[thisstep_index] == 3) {
	      /* Going to send 1 sample per codeword -> 3 samples */
	      for (x = 0; x < 3; x++) {
		putbits (bs, sbband[ch][gr][j + x][sb], bits[thisstep_index]); 
	      }
	    } else {
	      /* ISO11172 Sec C.1.5.2.8 
		 If steps=3, 5 or 9, then three consecutive samples are coded
		 as one codeword i.e. only one value (V) is transmitted for this 
		 triplet. If the 3 subband samples are x,y,z then
		 V = (steps*steps)*z + steps*y +x
	      */
	      y = steps[thisstep_index];
	      temp =
		sbband[ch][gr][j][sb] + sbband[ch][gr][j + 1][sb] * y +
		sbband[ch][gr][j + 2][sb] * y * y;
	      putbits (bs, temp, bits[thisstep_index]); 
	    }
	  }
}


//#include "bit_alloc_new.c"
/***************************************************************************************/
/* Bit Allocation Routines */


/************************************************************************
*
* bits_for_nonoise (Layer II)
*
* PURPOSE:Returns the number of bits required to produce a
* mask-to-noise ratio better or equal to the noise/no_noise threshold.
*
* SEMANTICS:
* bbal = # bits needed for encoding bit allocation
* bsel = # bits needed for encoding scalefactor select information
* banc = # bits needed for ancillary data (header info included)
*
* For each subband and channel, will add bits until one of the
* following occurs:
* - Hit maximum number of bits we can allocate for that subband
* - MNR is better than or equal to the minimum masking level
*   (NOISY_MIN_MNR)
* Then the bits required for scalefactors, scfsi, bit allocation,
* and the subband samples are tallied (#req_bits#) and returned.
*
* (NOISY_MIN_MNR) is the smallest MNR a subband can have before it is
* counted as 'noisy' by the logic which chooses the number of JS
* subbands.
*
* Joint stereo is supported.
*
************************************************************************/

int bits_for_nonoise_new (double SMR[2][SBLIMIT],
			  unsigned int scfsi[2][SBLIMIT], frame_info * frame, float min_mnr,
			  unsigned int bit_alloc[2][SBLIMIT])
{
  int sb, ch, ba;
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;
  int req_bits = 0, bbal = 0, berr = 0, banc = 32;
  int maxAlloc, sel_bits, sc_bits, smp_bits;
  static int sfsPerScfsi[] = { 3, 2, 1, 2 };	/* lookup # sfs per scfsi */

  /* MFC Feb 2003
     This works out the basic number of bits just to get a valid (but empty)
     frame. 
     This needs to be done for every frame, since a joint_stereo frame
     will change the number of basic bits (depending on the sblimit in 
     the particular js mode that's been selected */

  /* Make sure there's room for the error protection bits */
  if (frame->header->error_protection)
    berr = 16;
  else
    berr = 0;

  /* Count the number of bits required to encode the quantization index for both 
     channels in each subband. If we're above the jsbound, then pretend we only
     have one channel */
  for (sb = 0; sb < jsbound; ++sb)
    bbal += nch * nbal[ line[tablenum][sb] ]; //(*alloc)[sb][0].bits;
  for (sb = jsbound; sb < sblimit; ++sb)
    bbal += nbal[ line[tablenum][sb] ]; //(*alloc)[sb][0].bits;
  req_bits = banc + bbal + berr;

  for (sb = 0; sb < sblimit; ++sb)
    for (ch = 0; ch < ((sb < jsbound) ? nch : 1); ++ch) {
      int thisline = line[tablenum][sb];
      
      /* How many possible steps are there to choose from ? */
      maxAlloc = (1 << nbal[ line[tablenum][sb] ]) -1; //(*alloc)[sb][0].bits) - 1;
      sel_bits = sc_bits = smp_bits = 0;
      /* Keep choosing the next number of steps (and hence our SNR value)
	 until we have the required MNR value */
      for (ba = 0; ba < maxAlloc - 1; ++ba) {
        int thisstep_index = step_index[thisline][ba];
	if ((SNR[thisstep_index] - SMR[ch][sb]) >= min_mnr)
	  break;		/* we found enough bits */
      }
      if (nch == 2 && sb >= jsbound)	/* check other JS channel */
	for (; ba < maxAlloc - 1; ++ba) {
	  int thisstep_index = step_index[thisline][ba];
	  if ((SNR[thisstep_index] - SMR[1-ch][sb]) >= min_mnr)
	    break;
	}
      if (ba > 0) {
	//smp_bits = SCALE_BLOCK * ((*alloc)[sb][ba].group * (*alloc)[sb][ba].bits);
	int thisstep_index = step_index[thisline][ba];
	smp_bits = SCALE_BLOCK * group[thisstep_index] * bits[thisstep_index];
	/* scale factor bits required for subband */
	sel_bits = 2;
	sc_bits = 6 * sfsPerScfsi[scfsi[ch][sb]];
	if (nch == 2 && sb >= jsbound) {
	  /* each new js sb has L+R scfsis */
	  sel_bits += 2;
	  sc_bits += 6 * sfsPerScfsi[scfsi[1 - ch][sb]];
	}
	req_bits += smp_bits + sel_bits + sc_bits;
      }
      bit_alloc[ch][sb] = ba;
    }
  return req_bits;
}



/************************************************************************
*
* main_bit_allocation  (Layer II)
*
* PURPOSE:For joint stereo mode, determines which of the 4 joint
* stereo modes is needed.  Then calls *_a_bit_allocation(), which
* allocates bits for each of the subbands until there are no more bits
* left, or the MNR is at the noise/no_noise threshold.
*
* SEMANTICS:
*
* For joint stereo mode, joint stereo is changed to stereo if
* there are enough bits to encode stereo at or better than the
* no-noise threshold (NOISY_MIN_MNR).  Otherwise, the system
* iteratively allocates less bits by using joint stereo until one
* of the following occurs:
* - there are no more noisy subbands (MNR >= NOISY_MIN_MNR)
* - mode_ext has been reduced to 0, which means that all but the
*   lowest 4 subbands have been converted from stereo to joint
*   stereo, and no more subbands may be converted
*
*     This function calls *_bits_for_nonoise() and *_a_bit_allocation().
*
************************************************************************/
void main_bit_allocation_new (double SMR[2][SBLIMIT],
			      unsigned int scfsi[2][SBLIMIT],
			      unsigned int bit_alloc[2][SBLIMIT], int *adb,
			      frame_info * frame, options * glopts)
{
  int noisy_sbs;
  int mode, mode_ext, lay;
  int rq_db;			/* av_db = *adb; Not Used MFC Nov 99 */

  /* these are the tables which specify the limits within which the VBR can vary 
     You can't vary outside these ranges, otherwise a new alloc table would have to 
     be loaded in the middle of encoding. This VBR hack is dodgy - the standard
     says that LayerII decoders don't have to support a variable bitrate, but Layer3
     decoders must do so. Hence, it is unlikely that a compliant layer2 decoder would be 
     written to dynmically change allocation tables. *BUT* a layer3 encoder might handle it
     by default, meaning we could switch tables mid-encode and enjoy a wider range of bitrates
     for the VBR encoding. 
     None of this needs to be done for LSF, since there is only *one* possible alloc table in LSF 
     MFC Feb 2003 */
  int vbrlimits[2][3][2] = {
    /* MONO */
    { /* 44 */ {6, 10},
     /* 48 */ {3, 10},
     /* 32 */ {6, 10}},
    /* STEREO */
    { /* 44 */ {10, 14},
     /* 48 */ {7, 14},
     /* 32 */ {10, 14}}
  };

  static int init = 0;
  static int lower = 10, upper = 10;
  static int bitrateindextobits[15] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  int guessindex = 0;

  if (init == 0) {
    int nch = 2;
    int sfreq;
    frame_header *header = frame->header;
    init++;
    if (header->version == 0) {
      /* LSF: so can use any bitrate index from 1->15 */
      lower = 1;
      upper = 14;
    } else {
      if (frame->actual_mode == MPG_MD_MONO)
	nch = 1;
      sfreq = header->sampling_frequency;
      lower = vbrlimits[nch-1][sfreq][0];
      upper = vbrlimits[nch-1][sfreq][1];
    }
    if (glopts->verbosity > 2)
      fprintf (stdout, "VBR bitrate index limits [%i -> %i]\n", lower, upper);

    {				
      /* set up a conversion table for bitrateindex->bits for this version/sampl freq 
	 This will be used to find the best bitrate to cope with the number of bits that
	 are needed (as determined by VBR_bits_for_nonoise) */
      int brindex;
      frame_header *header = frame->header;
      for (brindex = lower; brindex <= upper; brindex++) {
	bitrateindextobits[brindex] =
	  (int) (1152.0 / s_freq[header->version][header->sampling_frequency]) *
	  ((double) bitrate[header->version][brindex]);
      }
    }

  }

  if ((mode = frame->actual_mode) == MPG_MD_JOINT_STEREO) {
    frame->header->mode = MPG_MD_STEREO;
    frame->header->mode_ext = 0;
    frame->jsbound = frame->sblimit;
    if ((rq_db = bits_for_nonoise_new (SMR, scfsi, frame, 0, bit_alloc)) > *adb) {
      frame->header->mode = MPG_MD_JOINT_STEREO;
      mode_ext = 4;		/* 3 is least severe reduction */
      lay = frame->header->lay;
      do {
	--mode_ext;
	frame->jsbound = js_bound (mode_ext);
	rq_db = bits_for_nonoise_new (SMR, scfsi, frame, 0, bit_alloc);
      }
      while ((rq_db > *adb) && (mode_ext > 0));
      frame->header->mode_ext = mode_ext;
    }				/* well we either eliminated noisy sbs or mode_ext == 0 */
  }

  /* decide on which bit allocation method to use */
  if (glopts->vbr == FALSE) {
    /* Just do the old bit allocation method */
    noisy_sbs = a_bit_allocation_new (SMR, scfsi, bit_alloc, adb, frame);
  } else {			
    /* do the VBR bit allocation method */
    frame->header->bitrate_index = lower;
    *adb = available_bits (frame->header, glopts);
    {
      int brindex;
      int found = FALSE;

      /* Work out how many bits are needed for there to be no noise (ie all MNR > VBRLEVEL) */
      int req =
	bits_for_nonoise_new (SMR, scfsi, frame, glopts->vbrlevel, bit_alloc);

      /* Look up this value in the bitrateindextobits table to find what bitrate we should use for 
         this frame */
      for (brindex = lower; brindex <= upper; brindex++) {
	if (bitrateindextobits[brindex] > req) {
	  /* this method always *overestimates* the bits that are needed
	     i.e. it will usually  guess right but
	     when it's wrong it'll guess a higher bitrate than actually required.
	     e.g. on "messages from earth" track 6, the guess was 
	     wrong on 75/36341 frames. each time it guessed higher. 
	     MFC Feb 2003 */
	  guessindex = brindex;
	  found = TRUE;
	  break;
	}
      }
      /* Just for sanity */
      if (found == FALSE)
	guessindex = upper;
    }

    frame->header->bitrate_index = guessindex;
    *adb = available_bits (frame->header, glopts);

    /* update the statistics */
    vbrstats_new[frame->header->bitrate_index]++;

    if (glopts->verbosity > 2) {
      /* print out the VBR stats every 1000th frame */
      static int count = 0;
      int i;
      if ((count++ % 1000) == 0) {
	for (i = 1; i < 15; i++)
	  fprintf (stdout, "%4i ", vbrstats_new[i]);
	fprintf (stdout, "\n");
      }

      /* Print out *every* frames bitrateindex, bits required, and bits available at this bitrate */
      if (glopts->verbosity > 5)
	fprintf (stdout,
		 "> bitrate index %2i has %i bits available to encode the %i bits\n",
		 frame->header->bitrate_index, *adb,
		 bits_for_nonoise_new (SMR, scfsi, frame,
				       glopts->vbrlevel, bit_alloc));

    }

    noisy_sbs =
      VBR_bit_allocation_new (SMR, scfsi, bit_alloc, adb, frame, glopts);
  }
}

void VBR_maxmnr_new (double mnr[2][SBLIMIT], char used[2][SBLIMIT], int sblimit,
		 int nch, int *min_sb, int *min_ch, options * glopts)
{
  int sb, ch;
  double small;

  small = 999999.0;
  *min_sb = -1;
  *min_ch = -1;

#define NEWBITx
#ifdef NEWBIT
  /* Keep going until all subbands have reached the MNR level that we specified */
  for (ch=0;ch<nch;ch++)
    for (sb=0;sb<sblimit;sb++)
      if (mnr[ch][sb] < glopts->vbrlevel) {
	*min_sb = sb;
	*min_ch = ch;
	//fprintf(stdout,".");
	//fflush(stdout);
	return;
      }
#endif

  /* Then start adding bits to whichever is the min MNR */
  for (ch = 0; ch < nch; ++ch)
    for (sb = 0; sb < sblimit; sb++)
      if (used[ch][sb] != 2 && small > mnr[ch][sb]) {
	small = mnr[ch][sb];
	*min_sb = sb;
	*min_ch = ch;
      }
  //fprintf(stdout,"Min sb: %i\n",*min_sb);
}
/********************
MFC Feb 2003
VBR_bit_allocation is different to the normal a_bit_allocation in that
it is known beforehand that there are definitely enough bits to do what we 
have to - i.e. a bitrate was specificially chosen in main_bit_allocation so
that we have enough bits to encode what we have to.
This function should take that into account and just greedily assign
the bits, rather than fussing over the minimum MNR subband - we know
each subband gets its required bits, why quibble?
This function doesn't chew much CPU, so I haven't made any attempt
to do this yet.
*********************/
int VBR_bit_allocation_new (double SMR[2][SBLIMIT],
			    unsigned int scfsi[2][SBLIMIT],
			    unsigned int bit_alloc[2][SBLIMIT], int *adb,
			    frame_info * frame, options *glopts)
{
  int sb, min_ch, min_sb, oth_ch, ch, increment, scale, seli, ba;
  int bspl, bscf, bsel, ad, bbal = 0;
  double mnr[2][SBLIMIT];
  char used[2][SBLIMIT];
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;
  //al_table *alloc = frame->alloc;
  static char init = 0;
  static int banc = 32, berr = 0;
  static int sfsPerScfsi[] = { 3, 2, 1, 2 };	/* lookup # sfs per scfsi */

  int thisstep_index;

  if (!init) {
    init = 1;
    if (frame->header->error_protection)
      berr = 16;		/* added 92-08-11 shn */
  }

  /* No need to worry about jsbound here as JS is disabled for VBR mode */
  for (sb = 0; sb < sblimit; sb++)
    bbal += nch * nbal[ line[tablenum][sb] ]; 
  *adb -= bbal + berr + banc;
  ad = *adb;

  for (sb = 0; sb < sblimit; sb++)
    for (ch = 0; ch < nch; ch++) {
      mnr[ch][sb] = SNR[0] - SMR[ch][sb];
      bit_alloc[ch][sb] = 0;
      used[ch][sb] = 0;
    }
  bspl = bscf = bsel = 0;

  do {
    /* locate the subband with minimum SMR */
    VBR_maxmnr_new (mnr, used, sblimit, nch, &min_sb, &min_ch, glopts);

    if (min_sb > -1) {		/* there was something to find */
      int thisline = line[tablenum][min_sb]; {
	/* find increase in bit allocation in subband [min] */
	int nextstep_index = step_index[thisline][bit_alloc[min_ch][min_sb]+1];					  
	increment = SCALE_BLOCK * group[nextstep_index] * bits[nextstep_index];
      }
      if (used[min_ch][min_sb]) {
	/* If we've already increased the limit on this ch/sb, then
	   subtract the last thing that we added */
	thisstep_index = step_index[thisline][bit_alloc[min_ch][min_sb]];
	increment -= SCALE_BLOCK * group[thisstep_index] * bits[thisstep_index];
      }

      /* scale factor bits required for subband [min] */
      oth_ch = 1 - min_ch;	/* above js bound, need both chans */
      if (used[min_ch][min_sb])
	scale = seli = 0;
      else {			/* this channel had no bits or scfs before */
	seli = 2;
	scale = 6 * sfsPerScfsi[scfsi[min_ch][min_sb]];
	if (nch == 2 && min_sb >= jsbound) {
	  /* each new js sb has L+R scfsis */
	  seli += 2;
	  scale += 6 * sfsPerScfsi[scfsi[oth_ch][min_sb]];
	}
      }

      /* check to see enough bits were available for */
      /* increasing resolution in the minimum band */
      if (ad >= bspl + bscf + bsel + seli + scale + increment) {
	/* Then there are enough bits to have another go at allocating */
	ba = ++bit_alloc[min_ch][min_sb];	/* next up alloc */
	bspl += increment;	/* bits for subband sample */
	bscf += scale;		/* bits for scale factor */
	bsel += seli;		/* bits for scfsi code */
	used[min_ch][min_sb] = 1;	/* subband has bits */
	thisstep_index = step_index[thisline][ba];
	mnr[min_ch][min_sb] = SNR[thisstep_index] - SMR[min_ch][min_sb];
	/* Check if this min_sb subband has been fully allocated max bits */
	if (ba >= (1 << nbal[ line[tablenum][min_sb] ]) -1 ) //(*alloc)[min_sb][0].bits) - 1)
	  used[min_ch][min_sb] = 2;	/* don't let this sb get any more bits */
      } else
	used[min_ch][min_sb] = 2;	/* can't increase this alloc */
    }
  }
  while (min_sb > -1);		/* until could find no channel */

  /* Calculate the number of bits left */
  ad -= bspl + bscf + bsel;
  *adb = ad;
  for (ch = 0; ch < nch; ch++)
    for (sb = sblimit; sb < SBLIMIT; sb++)
      bit_alloc[ch][sb] = 0;

  return 0;
}



/************************************************************************
*
* a_bit_allocation (Layer II)
*
* PURPOSE:Adds bits to the subbands with the lowest mask-to-noise
* ratios, until the maximum number of bits for the subband has
* been allocated.
*
* SEMANTICS:
* 1. Find the subband and channel with the smallest MNR (#min_sb#,
*    and #min_ch#)
* 2. Calculate the increase in bits needed if we increase the bit
*    allocation to the next higher level
* 3. If there are enough bits available for increasing the resolution
*    in #min_sb#, #min_ch#, and the subband has not yet reached its
*    maximum allocation, update the bit allocation, MNR, and bits
    available accordingly
* 4. Repeat until there are no more bits left, or no more available
*    subbands. (A subband is still available until the maximum
*    number of bits for the subband has been allocated, or there
*    aren't enough bits to go to the next higher resolution in the
    subband.)
*
************************************************************************/

void maxmnr_new (double mnr[2][SBLIMIT], char used[2][SBLIMIT], int sblimit,
	     int nch, int *min_sb, int *min_ch)
{
  int sb, ch;
  double small;

  small = 999999.0;
  *min_sb = -1;
  *min_ch = -1;
  for (ch = 0; ch < nch; ++ch)
    for (sb = 0; sb < sblimit; sb++)
      if (used[ch][sb] != 2 && small > mnr[ch][sb]) {
	small = mnr[ch][sb];
	*min_sb = sb;
	*min_ch = ch;
      }
}
int a_bit_allocation_new (double SMR[2][SBLIMIT],
			    unsigned int scfsi[2][SBLIMIT],
			    unsigned int bit_alloc[2][SBLIMIT], int *adb,
			    frame_info * frame)
{
  int sb, min_ch, min_sb, oth_ch, ch, increment, scale, seli, ba;
  int bspl, bscf, bsel, ad, bbal = 0;
  double mnr[2][SBLIMIT];
  char used[2][SBLIMIT];
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;
  //al_table *alloc = frame->alloc;
  static char init = 0;
  static int banc = 32, berr = 0;
  static int sfsPerScfsi[] = { 3, 2, 1, 2 };	/* lookup # sfs per scfsi */

  int thisstep_index;

  if (!init) {
    init = 1;
    if (frame->header->error_protection)
      berr = 16;		/* added 92-08-11 shn */
  }

  for (sb = 0; sb < jsbound; sb++)
    bbal += nch * nbal[ line[tablenum][sb] ]; //(*alloc)[sb][0].bits;
  for (sb = jsbound; sb < sblimit; sb++)
    bbal += nbal[ line[tablenum][sb] ]; //(*alloc)[sb][0].bits;
  *adb -= bbal + berr + banc;
  ad = *adb;

  for (sb = 0; sb < sblimit; sb++)
    for (ch = 0; ch < nch; ch++) {
      mnr[ch][sb] = SNR[0] - SMR[ch][sb];
      bit_alloc[ch][sb] = 0;
      used[ch][sb] = 0;
    }
  bspl = bscf = bsel = 0;

  do {
    /* locate the subband with minimum SMR */
    maxmnr_new (mnr, used, sblimit, nch, &min_sb, &min_ch);

    if (min_sb > -1) {		/* there was something to find */
      int thisline = line[tablenum][min_sb]; {
	/* find increase in bit allocation in subband [min] */
	int nextstep_index = step_index[thisline][bit_alloc[min_ch][min_sb]+1];					  
	increment = SCALE_BLOCK * group[nextstep_index] * bits[nextstep_index];
      }
      if (used[min_ch][min_sb]) {
	/* If we've already increased the limit on this ch/sb, then
	   subtract the last thing that we added */
	thisstep_index = step_index[thisline][bit_alloc[min_ch][min_sb]];
	increment -= SCALE_BLOCK * group[thisstep_index] * bits[thisstep_index];
      }

      /* scale factor bits required for subband [min] */
      oth_ch = 1 - min_ch;	/* above js bound, need both chans */
      if (used[min_ch][min_sb])
	scale = seli = 0;
      else {			/* this channel had no bits or scfs before */
	seli = 2;
	scale = 6 * sfsPerScfsi[scfsi[min_ch][min_sb]];
	if (nch == 2 && min_sb >= jsbound) {
	  /* each new js sb has L+R scfsis */
	  seli += 2;
	  scale += 6 * sfsPerScfsi[scfsi[oth_ch][min_sb]];
	}
      }

      /* check to see enough bits were available for */
      /* increasing resolution in the minimum band */
      if (ad >= bspl + bscf + bsel + seli + scale + increment) {
	/* Then there are enough bits to have another go at allocating */
	ba = ++bit_alloc[min_ch][min_sb];	/* next up alloc */
	bspl += increment;	/* bits for subband sample */
	bscf += scale;		/* bits for scale factor */
	bsel += seli;		/* bits for scfsi code */
	used[min_ch][min_sb] = 1;	/* subband has bits */
	thisstep_index = step_index[thisline][ba];
	mnr[min_ch][min_sb] = SNR[thisstep_index] - SMR[min_ch][min_sb];
	/* Check if this min_sb subband has been fully allocated max bits */
	if (ba >= (1 << nbal[ line[tablenum][min_sb] ]) -1 ) //(*alloc)[min_sb][0].bits) - 1)
	  used[min_ch][min_sb] = 2;	/* don't let this sb get any more bits */
      } else
	used[min_ch][min_sb] = 2;	/* can't increase this alloc */

      if (min_sb >= jsbound && nch == 2) {
	/* above jsbound, alloc applies L+R */
	ba = bit_alloc[oth_ch][min_sb] = bit_alloc[min_ch][min_sb];
	used[oth_ch][min_sb] = used[min_ch][min_sb];
	thisstep_index = step_index[thisline][ba];
	mnr[oth_ch][min_sb] = SNR[thisstep_index] - SMR[oth_ch][min_sb];
	//mnr[oth_ch][min_sb] = SNR[(*alloc)[min_sb][ba].quant + 1] - SMR[oth_ch][min_sb];
      }

    }
  }
  while (min_sb > -1);		/* until could find no channel */

  /* Calculate the number of bits left */
  ad -= bspl + bscf + bsel;
  *adb = ad;
  for (ch = 0; ch < nch; ch++)
    for (sb = sblimit; sb < SBLIMIT; sb++)
      bit_alloc[ch][sb] = 0;

  return 0;
}

