/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2014 The libLTE Developers. See the
 * COPYRIGHT file at the top-level directory of this distribution.
 *
 * \section LICENSE
 *
 * This file is part of the libLTE library.
 *
 * libLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */


#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>

#include "liblte/phy/sync/pss.h"
#include "liblte/phy/utils/dft.h"
#include "liblte/phy/utils/vector.h"
#include "liblte/phy/utils/convolution.h"
#include "liblte/phy/utils/debug.h"


int pss_synch_init_N_id_2(cf_t *pss_signal_time, cf_t *pss_signal_freq, uint32_t N_id_2, uint32_t fft_size) {
  dft_plan_t plan;
  cf_t pss_signal_pad[2048];
  int ret = LIBLTE_ERROR_INVALID_INPUTS;
  
  if (lte_N_id_2_isvalid(N_id_2)    && 
      fft_size                  <= 2048) 
  {
    
    pss_generate(pss_signal_time, N_id_2);

    bzero(pss_signal_pad, fft_size * sizeof(cf_t));
    bzero(pss_signal_freq, fft_size * sizeof(cf_t));
    memcpy(&pss_signal_pad[(fft_size-PSS_LEN)/2], pss_signal_time, PSS_LEN * sizeof(cf_t));

    if (dft_plan(&plan, fft_size, BACKWARD, COMPLEX)) {
      return LIBLTE_ERROR;
    }
    
    dft_plan_set_mirror(&plan, true);
    dft_plan_set_dc(&plan, true);
    dft_plan_set_norm(&plan, true);
    dft_run_c(&plan, pss_signal_pad, pss_signal_freq);

    vec_conj_cc(pss_signal_freq, pss_signal_freq, fft_size);
    vec_sc_prod_cfc(pss_signal_freq, 1.0/PSS_LEN, pss_signal_freq, fft_size);

    dft_plan_free(&plan);
        
    ret = LIBLTE_SUCCESS;
  }
  return ret;
}

/* Initializes the PSS synchronization object with fft_size=128
 */
int pss_synch_init(pss_synch_t *q, uint32_t frame_size) {
  return pss_synch_init_fft(q, frame_size, 128);
}
/* Initializes the PSS synchronization object. 
 * 
 * It correlates a signal of frame_size samples with the PSS sequence in the frequency 
 * domain. The PSS sequence is transformed using fft_size samples. 
 */
int pss_synch_init_fft(pss_synch_t *q, uint32_t frame_size, uint32_t fft_size) {
  int ret = LIBLTE_ERROR_INVALID_INPUTS;
    
  if (q != NULL) {
  
    uint32_t N_id_2; 
    uint32_t buffer_size; 
    bzero(q, sizeof(pss_synch_t));
    
    q->N_id_2 = 10;
    q->fft_size = fft_size;
    q->frame_size = frame_size;
    q->ema_alpha = 0.1; 

    buffer_size = fft_size + frame_size + 1;
    
    
    if (dft_plan(&q->dftp_input, fft_size, FORWARD, COMPLEX)) {
      fprintf(stderr, "Error creating DFT plan \n");
      goto clean_and_exit;
    }
    dft_plan_set_mirror(&q->dftp_input, true);
    dft_plan_set_dc(&q->dftp_input, true);
    dft_plan_set_norm(&q->dftp_input, true);

    q->tmp_input = vec_malloc(buffer_size * sizeof(cf_t));
    if (!q->tmp_input) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }

    bzero(&q->tmp_input[q->frame_size], q->fft_size * sizeof(cf_t));

    q->conv_output = vec_malloc(buffer_size * sizeof(cf_t));
    if (!q->conv_output) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }
    bzero(q->conv_output, sizeof(cf_t) * buffer_size);
    q->conv_output_avg = vec_malloc(buffer_size * sizeof(float));
    if (!q->conv_output_avg) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }
    bzero(q->conv_output_avg, sizeof(float) * buffer_size);
#ifdef PSS_ACCUMULATE_ABS
    q->conv_output_abs = vec_malloc(buffer_size * sizeof(float));
    if (!q->conv_output_abs) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }
    bzero(q->conv_output_abs, sizeof(float) * buffer_size);
#endif
    
    for (N_id_2=0;N_id_2<3;N_id_2++) {
      q->pss_signal_freq[N_id_2] = vec_malloc(buffer_size * sizeof(cf_t));
      if (!q->pss_signal_freq[N_id_2]) {
        fprintf(stderr, "Error allocating memory\n");
        goto clean_and_exit;
      }
      /* The PSS is translated into the frequency domain for each N_id_2  */
      if (pss_synch_init_N_id_2(q->pss_signal_time[N_id_2], q->pss_signal_freq[N_id_2], N_id_2, fft_size)) {
        fprintf(stderr, "Error initiating PSS detector for N_id_2=%d fft_size=%d\n", N_id_2, fft_size);
        goto clean_and_exit;
      }      
      bzero(&q->pss_signal_freq[N_id_2][q->fft_size], q->frame_size * sizeof(cf_t));

    }    
    #ifdef CONVOLUTION_FFT
    if (conv_fft_cc_init(&q->conv_fft, frame_size, fft_size)) {
      fprintf(stderr, "Error initiating convolution FFT\n");
      goto clean_and_exit;
    }
    #endif
    
    pss_synch_reset(q);
    
    ret = LIBLTE_SUCCESS;
  }

clean_and_exit: 
  if (ret == LIBLTE_ERROR) {
    pss_synch_free(q);
  }
  return ret;
}

void pss_synch_free(pss_synch_t *q) {
  uint32_t i;

  if (q) {
    for (i=0;i<3;i++) {
      if (q->pss_signal_freq[i]) {
        free(q->pss_signal_freq[i]);
      }
    }
  #ifdef CONVOLUTION_FFT
    conv_fft_cc_free(&q->conv_fft);
    
  #endif
    if (q->tmp_input) {
      free(q->tmp_input);
    }
    if (q->conv_output) {
      free(q->conv_output);
    }
    if (q->conv_output_abs) {
      free(q->conv_output_abs);
    }
    if (q->conv_output_avg) {
      free(q->conv_output_avg);
    }

    bzero(q, sizeof(pss_synch_t));    
  }
}

void pss_synch_reset(pss_synch_t *q) {
  uint32_t buffer_size = q->fft_size + q->frame_size + 1;
  bzero(q->conv_output_avg, sizeof(float) * buffer_size);
}

/**
 * This function calculates the Zadoff-Chu sequence.
 * @param signal Output array.
 */
int pss_generate(cf_t *signal, uint32_t N_id_2) {
  int i;
  float arg;
  const float root_value[] = { 25.0, 29.0, 34.0 };
  int root_idx;

  int sign = -1;

  if (N_id_2 > 2) {
    fprintf(stderr, "Invalid N_id_2 %d\n", N_id_2);
    return -1;
  }

  root_idx = N_id_2;

  for (i = 0; i < PSS_LEN / 2; i++) {
    arg = (float) sign * M_PI * root_value[root_idx]
        * ((float) i * ((float) i + 1.0)) / 63.0;
    __real__ signal[i] = cosf(arg);
    __imag__ signal[i] = sinf(arg);
  }
  for (i = PSS_LEN / 2; i < PSS_LEN; i++) {
    arg = (float) sign * M_PI * root_value[root_idx]
        * (((float) i + 2.0) * ((float) i + 1.0)) / 63.0;
    __real__ signal[i] = cosf(arg);
    __imag__ signal[i] = sinf(arg);
  }
  return 0;
}

/** 36.211 10.3 section 6.11.1.2
 */
void pss_put_slot(cf_t *pss_signal, cf_t *slot, uint32_t nof_prb, lte_cp_t cp) {
  int k;
  k = (CP_NSYMB(cp) - 1) * nof_prb * RE_X_RB + nof_prb * RE_X_RB / 2 - 31;
  memset(&slot[k - 5], 0, 5 * sizeof(cf_t));
  memcpy(&slot[k], pss_signal, PSS_LEN * sizeof(cf_t));
  memset(&slot[k + PSS_LEN], 0, 5 * sizeof(cf_t));
}


/** Sets the current N_id_2 value. Returns -1 on error, 0 otherwise
 */
int pss_synch_set_N_id_2(pss_synch_t *q, uint32_t N_id_2) {
  if (!lte_N_id_2_isvalid((N_id_2))) {
    fprintf(stderr, "Invalid N_id_2 %d\n", N_id_2);
    return -1;
  } else {
    q->N_id_2 = N_id_2;
    return 0;
  }
}

/* Sets the weight factor alpha for the exponential moving average of the PSS correlation output
 */
void pss_synch_set_ema_alpha(pss_synch_t *q, float alpha) {
  q->ema_alpha = alpha; 
}

/** Performs time-domain PSS correlation. 
 * Returns the index of the PSS correlation peak in a subframe.
 * The frame starts at corr_peak_pos-subframe_size/2.
 * The value of the correlation is stored in corr_peak_value.
 *
 * Input buffer must be subframe_size long.
 */
int pss_synch_find_pss(pss_synch_t *q, cf_t *input, float *corr_peak_value) 
{
  int ret = LIBLTE_ERROR_INVALID_INPUTS;
  
  if (q                 != NULL  && 
      input             != NULL)
  {

    uint32_t corr_peak_pos;
    uint32_t conv_output_len;
    
    if (!lte_N_id_2_isvalid(q->N_id_2)) {
      fprintf(stderr, "Error finding PSS peak, Must set N_id_2 first\n");
      return LIBLTE_ERROR;
    }

    /* Correlate input with PSS sequence */
    if (q->frame_size >= q->fft_size) {
    #ifdef CONVOLUTION_FFT
      memcpy(q->tmp_input, input, q->frame_size * sizeof(cf_t));
            
      conv_output_len = conv_fft_cc_run(&q->conv_fft, q->tmp_input,
          q->pss_signal_freq[q->N_id_2], q->conv_output);
    #else
      conv_output_len = conv_cc(input, q->pss_signal_freq[q->N_id_2], q->conv_output, q->frame_size, q->fft_size);
    #endif
    } else {
      for (int i=0;i<q->frame_size;i++) {
        q->conv_output[i] = vec_dot_prod_ccc(q->pss_signal_freq[q->N_id_2], &input[i], q->fft_size);
      }
      conv_output_len = q->frame_size; 
    }

   
  #ifdef PSS_ABS_SQUARE
      vec_abs_square_cf(q->conv_output, q->conv_output_abs, conv_output_len-1);
  #else
      vec_abs_cf(q->conv_output, q->conv_output_abs, conv_output_len-1);
  #endif
    
    vec_sc_prod_fff(q->conv_output_abs, q->ema_alpha, q->conv_output_abs, conv_output_len-1);    
    vec_sc_prod_fff(q->conv_output_avg, 1-q->ema_alpha, q->conv_output_avg, conv_output_len-1);    

    vec_sum_fff(q->conv_output_abs, q->conv_output_avg, q->conv_output_avg, conv_output_len-1);
    
    /* Find maximum of the absolute value of the correlation */
    corr_peak_pos = vec_max_fi(q->conv_output_avg, conv_output_len-1);
    
    // save absolute value 
    q->peak_value = q->conv_output_avg[corr_peak_pos];
    
#ifdef PSS_RETURN_PSR    
    // Find second side lobe
    
    // Find end of peak lobe to the right
    int pl_ub = corr_peak_pos+1;
    while(q->conv_output_avg[pl_ub+1] <= q->conv_output_avg[pl_ub] && pl_ub < conv_output_len) {
      pl_ub ++; 
    }
    // Find end of peak lobe to the left
    int pl_lb; 
    if (corr_peak_pos > 0) {
      pl_lb = corr_peak_pos-1;
        while(q->conv_output_avg[pl_lb-1] <= q->conv_output_avg[pl_lb] && pl_lb > 1) {
        pl_lb --; 
      }      
    } else {
      pl_lb = 0; 
    }

    int sl_distance_right = conv_output_len-1-pl_ub; 
    if (sl_distance_right < 0) {
      sl_distance_right = 0; 
    }
    int sl_distance_left = pl_lb; 
    
    int sl_right = pl_ub+vec_max_fi(&q->conv_output_avg[pl_ub], sl_distance_right);
    int sl_left = vec_max_fi(q->conv_output_avg, sl_distance_left);    
    float side_lobe_value = MAX(q->conv_output_avg[sl_right], q->conv_output_avg[sl_left]);    
    if (corr_peak_value) {
      *corr_peak_value = q->conv_output_avg[corr_peak_pos]/side_lobe_value;
      
      if (*corr_peak_value < 2.0) {
        DEBUG("pl_ub=%d, pl_lb=%d, sl_right: %d (%.2f), sl_left: %d (%.2f), PSR: %.2f/%.2f=%.2f\n", pl_ub, pl_lb, 
             sl_right, 1000000*q->conv_output_avg[sl_right], 
             sl_left, 1000000*q->conv_output_avg[sl_left], 
          1000000*q->conv_output_avg[corr_peak_pos], 1000000*side_lobe_value,*corr_peak_value
        );
      }      
    }
#else
    if (corr_peak_value) {
      *corr_peak_value = q->conv_output_avg[corr_peak_pos];
    }
#endif

    if (q->frame_size >= q->fft_size) {
      ret = (int) corr_peak_pos;                
    } else {
      ret = (int) corr_peak_pos + q->fft_size;
    }
  } 
  return ret;
}

LIBLTE_API cf_t *tmp2; 

/* Computes frequency-domain channel estimation of the PSS symbol 
 * input signal is in the time-domain. 
 * ce is the returned frequency-domain channel estimates. 
 */
int pss_synch_chest(pss_synch_t *q, cf_t *input, cf_t ce[PSS_LEN]) {
  int ret = LIBLTE_ERROR_INVALID_INPUTS;
  cf_t input_fft[SYMBOL_SZ_MAX];

  if (q                 != NULL  && 
      input             != NULL)
  {

    if (!lte_N_id_2_isvalid(q->N_id_2)) {
      fprintf(stderr, "Error finding PSS peak, Must set N_id_2 first\n");
      return LIBLTE_ERROR;
    }
    
    tmp2 = input_fft; 
    
    /* Transform to frequency-domain */
    dft_run_c(&q->dftp_input, input, input_fft);
    
    /* Compute channel estimate taking the PSS sequence as reference */
    vec_prod_conj_ccc(&input_fft[(q->fft_size-PSS_LEN)/2], q->pss_signal_time[q->N_id_2], ce, PSS_LEN);
      
    ret = LIBLTE_SUCCESS;
  }
  return ret; 
}

/* Returns the CFO estimation given a PSS received sequence
 *
 * Source: An Efﬁcient CFO Estimation Algorithm for the Downlink of 3GPP-LTE
 *       Feng Wang and Yu Zhu
 */
float pss_synch_cfo_compute(pss_synch_t* q, cf_t *pss_recv) {
  cf_t y0, y1, yr;

  y0 = vec_dot_prod_ccc(q->pss_signal_freq[q->N_id_2], pss_recv, q->fft_size/2);
  y1 = vec_dot_prod_ccc(&q->pss_signal_freq[q->N_id_2][q->fft_size/2], &pss_recv[q->fft_size/2], q->fft_size/2);
  
  yr = conjf(y0) * y1;

  return atan2f(__imag__ yr, __real__ yr) / M_PI;
}

