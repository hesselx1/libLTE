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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "liblte/phy/common/phy_common.h"
#include "liblte/phy/phch/regs.h"
#include "liblte/phy/utils/debug.h"

#define REG_IDX(r, i, n) r->k[i]+r->l*n*RE_X_RB


regs_reg_t *regs_find_reg(regs_t *h, uint32_t k, uint32_t l);
int regs_put_reg(regs_reg_t *reg, 
                            cf_t *reg_data, 
                            cf_t *slot_symbols, 
                            uint32_t nof_prb);

int regs_add_reg(regs_reg_t *reg, 
                            cf_t *reg_data, 
                            cf_t *slot_symbols, 
                            uint32_t nof_prb);

int regs_get_reg(regs_reg_t *reg, 
                            cf_t *slot_symbols, 
                            cf_t *reg_data, 
                            uint32_t nof_prb);

int regs_reset_reg(regs_reg_t *reg, cf_t *slot_symbols, uint32_t nof_prb);


/***************************************************************
 *
 * PDCCH REG ALLOCATION
 *
 ***************************************************************/

void regs_pdcch_free(regs_t *h) {
  int i;
  for (i=0;i<3;i++) {
    if (h->pdcch[i].regs) {
      free(h->pdcch[i].regs);
      h->pdcch[i].regs = NULL; 
    }
  }
}

#define PDCCH_NCOLS  32
const uint8_t PDCCH_PERM[PDCCH_NCOLS] =
    { 1, 17, 9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31, 0, 16, 8,
        24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30 };

/** Initialize REGs for PDCCH
 * 36.211 10.3 section 6.8.5
 */
int regs_pdcch_init(regs_t *h) {
  int i, m, cfi, nof_ctrl_symbols;
  int ret = LIBLTE_ERROR;
  int nrows, ndummy, j;
  uint32_t k, kp;
  regs_reg_t **tmp = NULL;

  bzero(&h->pdcch, sizeof(regs_ch_t));

  for (cfi=0;cfi<3;cfi++) {
    if (h->cell.nof_prb < 10) {
      nof_ctrl_symbols = cfi+2;
    } else {
      nof_ctrl_symbols = cfi+1;
    }

    tmp = malloc(sizeof(regs_reg_t*) * h->nof_regs);
    if (!tmp) {
      perror("malloc");
      goto clean_and_exit;
    }

    /* Number and count REGs for this CFI */
    m=0;
    for (i=0;i<h->nof_regs;i++) {
      if (h->regs[i].l < nof_ctrl_symbols && !h->regs[i].assigned) {
        tmp[m] = &h->regs[i];
        m++;
      }
    }

    h->pdcch[cfi].nof_regs = m;
    
    h->pdcch[cfi].regs = malloc(sizeof(regs_reg_t*) * h->pdcch[cfi].nof_regs);
    if (!h->pdcch[cfi].regs) {
      perror("malloc");
      goto clean_and_exit;
    }

    /* Interleave REGs */
    nrows = (h->pdcch[cfi].nof_regs-1)/PDCCH_NCOLS+1;
    ndummy = PDCCH_NCOLS*nrows - h->pdcch[cfi].nof_regs;
    if (ndummy < 0) {
      ndummy = 0;
    }

    k=0;
    for (j = 0; j < PDCCH_NCOLS; j++) {
      for (i = 0; i < nrows; i++) {
        if (i*PDCCH_NCOLS + PDCCH_PERM[j] >= ndummy) {
          m = i*PDCCH_NCOLS + PDCCH_PERM[j]-ndummy;
          if (k < h->cell.id) {
            kp = (h->pdcch[cfi].nof_regs + k-h->cell.id)%h->pdcch[cfi].nof_regs;
          } else {
            kp = (k-h->cell.id)%h->pdcch[cfi].nof_regs;            
          }
          h->pdcch[cfi].regs[m] = tmp[kp];
          k++;
        }
      }
    }
    h->pdcch[cfi].nof_regs = (h->pdcch[cfi].nof_regs/9)*9;
    INFO("Init PDCCH REG space CFI %d. %d useful REGs (%d CCEs)\n",cfi+1, 
         h->pdcch[cfi].nof_regs, h->pdcch[cfi].nof_regs/9);
    free(tmp);
    tmp = NULL;
  }

  ret = LIBLTE_SUCCESS;
clean_and_exit:
  if (tmp) {
    free(tmp);
  }
  if (ret == LIBLTE_ERROR) {
    regs_pdcch_free(h);
  }
  return ret;
}

int regs_pdcch_nregs(regs_t *h, uint32_t cfi) {
  if (cfi < 1 || cfi > 3) {
    fprintf(stderr, "Invalid CFI=%d\n", cfi);
    return LIBLTE_ERROR;
  } else {
    return (int) h->pdcch[cfi-1].nof_regs;
  }
}

/** Copy quadruplets to REGs and cyclic shift them, according to the
 * second part of 6.8.5 in 36.211
 */

int regs_pdcch_put_offset(regs_t *h, cf_t *pdcch_symbols, cf_t *slot_symbols, uint32_t start_reg, uint32_t nof_regs) {
  if (h->cfi_initiated) {
    if (start_reg + nof_regs <= h->pdcch[h->cfi].nof_regs) {
      uint32_t i, k;
      k = 0;
      for (i=start_reg;i<start_reg+nof_regs;i++) {
        regs_put_reg(h->pdcch[h->cfi].regs[i], &pdcch_symbols[k], slot_symbols, h->cell.nof_prb);
        k += 4;
      }
      return k;      
    } else {
      fprintf(stderr, "Out of range: start_reg + nof_reg must be lower than %d\n", h->pdcch[h->cfi].nof_regs);
      return LIBLTE_ERROR;      
    }       
  } else {
    fprintf(stderr, "Must call regs_set_cfi() first\n");
    return LIBLTE_ERROR;
  }
}

int regs_pdcch_put(regs_t *h, cf_t *pdcch_symbols, cf_t *slot_symbols) {
  return regs_pdcch_put_offset(h, pdcch_symbols, slot_symbols, 0, h->pdcch[h->cfi].nof_regs);
}

int regs_pdcch_get_offset(regs_t *h, cf_t *slot_symbols, cf_t *pdcch_symbols, uint32_t start_reg, uint32_t nof_regs) {
  if (h->cfi_initiated) {
    if (start_reg + nof_regs <= h->pdcch[h->cfi].nof_regs) {
      uint32_t i, k;  
      k = 0;
      for (i=start_reg;i<start_reg + nof_regs;i++) {
        regs_get_reg(h->pdcch[h->cfi].regs[i], slot_symbols, &pdcch_symbols[k], h->cell.nof_prb);
        k += 4;
      }
      return k;    
    } else {
      fprintf(stderr, "Out of range: start_reg + nof_reg must be lower than %d\n", h->pdcch[h->cfi].nof_regs);
      return LIBLTE_ERROR;
    }
  } else {
    fprintf(stderr, "Must call regs_set_cfi() first\n");
    return LIBLTE_ERROR;
  }
}


int regs_pdcch_get(regs_t *h, cf_t *slot_symbols, cf_t *pdcch_symbols) {
 return regs_pdcch_get_offset(h, slot_symbols, pdcch_symbols, 0, h->pdcch[h->cfi].nof_regs);
}




/***************************************************************
 *
 * PHICH REG ALLOCATION
 *
 ***************************************************************/

/** Initialize REGs for PHICH
 * 36.211 10.3 section 6.9.3
 */
int regs_phich_init(regs_t *h) {
  float ng;
  uint32_t i, ni, li, n[3], nreg, mi;
  regs_reg_t **regs_phich[3];
  int ret = LIBLTE_ERROR;

  switch(h->phich_res) {
  case R_1_6:
    ng = (float) 1/6;
    break;
  case R_1_2:
    ng = (float) 1/2;
    break;
  case R_1:
    ng = 1;
    break;
  case R_2:
    ng = 2;
    break;
  default:
    ng = 0;
    break;
  }
  h->ngroups_phich = (int) ceilf(ng * ((float) h->cell.nof_prb/8));
  h->phich = malloc(sizeof(regs_ch_t) * h->ngroups_phich);
  if (!h->phich) {
    perror("malloc");
    return -1;
  }
  INFO("Creating %d PHICH mapping units. %s length, Ng=%.2f\n",h->ngroups_phich,
      h->phich_len==PHICH_EXT?"Extended":"Normal",ng);
  for (i=0;i<h->ngroups_phich;i++) {
    h->phich[i].nof_regs = REGS_PHICH_REGS_X_GROUP;
    h->phich[i].regs = malloc(sizeof(regs_reg_t*) * REGS_PHICH_REGS_X_GROUP);
    if (!h->phich[i].regs) {
      perror("malloc");
      goto clean_and_exit;
    }
  }

  /** Here begins the mapping algorithm */

  /* Step 2. Count REGs not assigned to PCFICH */
  bzero(n, 3*sizeof(int));
  for (i=0;i<h->nof_regs;i++) {
    if (h->regs[i].l < 3 && !h->regs[i].assigned) {
      n[h->regs[i].l]++;
    }
  }

  bzero(regs_phich, sizeof(regs_reg_t*) * 3);
  for (i=0;i<3;i++) {
    regs_phich[i] = malloc(n[i] * sizeof(regs_reg_t*));
    if (!regs_phich[i]) {
      perror("malloc");
      goto clean_and_exit;
    }
  }

  bzero(n, 3 * sizeof(int));
  /* Step 3. Number REGs not assigned to PCFICH */
  for (i=0;i<h->nof_regs;i++) {
    // they are already sorted starting from the REG with the lowest frequency-domain index
    if (h->regs[i].l < 3 && !h->regs[i].assigned) {
      regs_phich[h->regs[i].l][n[h->regs[i].l]++] = &h->regs[i];
    }
  }

  nreg=0;
  for (mi=0;mi<h->ngroups_phich;mi++) { // here ngroups is the number of mapping units
    for (i=0;i<3;i++) {
      li=h->phich_len==PHICH_EXT?i:0; // Step 7
      ni=((h->cell.id*n[li]/n[0])+mi+i*n[li]/3) % n[li]; // Step 8
      h->phich[mi].regs[i] = regs_phich[li][ni];
      h->phich[mi].regs[i]->assigned = true;
      INFO("Assigned PHICH REG#%d (%d,%d)\n",nreg,h->phich[mi].regs[i]->k0,li);
      nreg++;
    }
  }

  // now the number of mapping units = number of groups for normal cp. For extended cp
  // ngroups = 2 * number mapping units
  if (CP_ISEXT(h->cell.cp)) {
    h->ngroups_phich *= 2;
  }
  ret = LIBLTE_SUCCESS;

clean_and_exit:
  if (ret == LIBLTE_ERROR) {
    if (h->phich) {
      for (i=0;i<h->ngroups_phich;i++) {
        if (h->phich[i].regs) {
          free(h->phich[i].regs);
        }
      }
      free(h->phich);
    }
  }
  for (i=0;i<3;i++) {
    if (regs_phich[i]) {
      free(regs_phich[i]);
    }
  }
  return ret;
}

void regs_phich_free(regs_t *h) {
  uint32_t i;
  if (h->phich) {
    if (CP_ISEXT(h->cell.cp)) {
      h->ngroups_phich /= 2;
    }
    for (i=0;i<h->ngroups_phich;i++) {
      if (h->phich[i].regs) {
        free(h->phich[i].regs);
        h->phich[i].regs = NULL;
      }
    }
    free(h->phich);
    h->phich = NULL; 
  }
}

uint32_t regs_phich_nregs(regs_t *h) {
  uint32_t i;
  uint32_t n;
  n=0;
  for (i=0;i<h->ngroups_phich;i++) {
    n += h->phich[i].nof_regs;
  }
  return n;
}


uint32_t regs_phich_ngroups(regs_t *h) {
  return h->ngroups_phich;
}

/**
 * Adds the PHICH symbols to the resource grid pointed by slot_symbols.
 *
 * Each subframe, the user shall call the regs_phich_reset function before adding PHICH symbols.
 *
 * Returns the number of written symbols, or -1 on error
 */
int regs_phich_add(regs_t *h, cf_t phich_symbols[REGS_PHICH_NSYM], uint32_t ngroup, cf_t *slot_symbols) {
  uint32_t i;
  if (ngroup >= h->ngroups_phich) {
    fprintf(stderr, "Error invalid ngroup %d\n", ngroup);
    return LIBLTE_ERROR_INVALID_INPUTS;
  }
  if (CP_ISEXT(h->cell.cp)) {
    ngroup /= 2;
  }
  regs_ch_t *rch = &h->phich[ngroup];
  for (i = 0; i < rch->nof_regs && i*REGS_RE_X_REG < REGS_PHICH_NSYM; i++) {
    regs_add_reg(rch->regs[i], &phich_symbols[i*REGS_RE_X_REG], slot_symbols, h->cell.nof_prb);
  }
  return i*REGS_RE_X_REG;
}

/**
 * Resets the PHICH symbols
 *
 * Returns the number of written symbols, or -1 on error
 */
int regs_phich_reset(regs_t *h, cf_t *slot_symbols) {
  uint32_t i;
  uint32_t ngroup, ng;
  for (ngroup = 0;ngroup < h->ngroups_phich;CP_ISEXT(h->cell.cp)?ngroup+=2:ngroup++) {
    if (CP_ISEXT(h->cell.cp)) {
      ng = ngroup/2;
    } else {
      ng = ngroup;
    }
    regs_ch_t *rch = &h->phich[ng];
    for (i = 0; i < rch->nof_regs && i*REGS_RE_X_REG < REGS_PHICH_NSYM; i++) {
      regs_reset_reg(rch->regs[i], slot_symbols, h->cell.nof_prb);
    }
  }
  return LIBLTE_SUCCESS;
}

/**
 * Gets the PHICH symbols from the resource grid pointed by slot_symbols
 *
 * Returns the number of written symbols, or -1 on error
 */
int regs_phich_get(regs_t *h, cf_t *slot_symbols, cf_t phich_symbols[REGS_PHICH_NSYM], uint32_t ngroup) {
  uint32_t i;
  if (ngroup >= h->ngroups_phich) {
    fprintf(stderr, "Error invalid ngroup %d\n", ngroup);
    return LIBLTE_ERROR_INVALID_INPUTS;
  }
  if (CP_ISEXT(h->cell.cp)) {
    ngroup /= 2;
  }
  regs_ch_t *rch = &h->phich[ngroup];
  for (i = 0; i < rch->nof_regs && i*REGS_RE_X_REG < REGS_PHICH_NSYM; i++) {
    regs_get_reg(rch->regs[i], slot_symbols, &phich_symbols[i*REGS_RE_X_REG], h->cell.nof_prb);
  }
  return i*REGS_RE_X_REG;
}










/***************************************************************
 *
 * PCFICH REG ALLOCATION
 *
 ***************************************************************/


/** Initialize REGs for PCFICH
 * 36.211 10.3 section 6.7.4
 */
int regs_pcfich_init(regs_t *h) {
  uint32_t i; 
  uint32_t k_hat, k;
  regs_ch_t *ch = &h->pcfich;

  ch->regs = malloc(sizeof(regs_reg_t*) * REGS_PCFICH_NREGS);
  if (!ch->regs) {
    perror("malloc");
    return LIBLTE_ERROR;
  }
  ch->nof_regs = REGS_PCFICH_NREGS;

  INFO("PCFICH allocating %d regs. CellID: %d, PRB: %d\n", ch->nof_regs, h->cell.id, h->cell.nof_prb);

  k_hat = (RE_X_RB / 2) * (h->cell.id % (2 * h->cell.nof_prb));
  for (i = 0; i < REGS_PCFICH_NREGS; i++) {

    k = (k_hat + (i * h->cell.nof_prb / 2) * (RE_X_RB / 2))
        % (h->cell.nof_prb * RE_X_RB);
    ch->regs[i] = regs_find_reg(h, k, 0);
    if (!ch->regs[i]) {
      fprintf(stderr, "Error allocating PCFICH: REG (%d,0) not found\n",
          k);
      return LIBLTE_ERROR;
    } else if (ch->regs[i]->assigned) {
      fprintf(stderr,
          "Error allocating PCFICH: REG (%d,0) already allocated\n",
          k);
      return LIBLTE_ERROR;
    } else {
      ch->regs[i]->assigned = true;     
      INFO("Assigned PCFICH REG#%d (%d,0)\n", i, k);
    }
  }
  return LIBLTE_SUCCESS;
}

void regs_pcfich_free(regs_t *h) {
  if (h->pcfich.regs) {
    free(h->pcfich.regs);
    h->pcfich.regs = NULL; 
  }
}

uint32_t regs_pcfich_nregs(regs_t *h) {
  return h->pcfich.nof_regs;
}

/**
 * Maps the PCFICH symbols to the resource grid pointed by slot_symbols
 *
 * Returns the number of written symbols, or -1 on error
 */
int regs_pcfich_put(regs_t *h, cf_t pcfich_symbols[REGS_PCFICH_NSYM], cf_t *slot_symbols) {
  regs_ch_t *rch = &h->pcfich;

  uint32_t i;
  for (i = 0; i < rch->nof_regs && i*REGS_RE_X_REG < REGS_PCFICH_NSYM; i++) {
    regs_put_reg(rch->regs[i], &pcfich_symbols[i*REGS_RE_X_REG], slot_symbols, h->cell.nof_prb);
  }
  return i*REGS_RE_X_REG;
}

/**
 * Gets the PCFICH symbols from the resource grid pointed by slot_symbols
 *
 * Returns the number of written symbols, or -1 on error
 */
int regs_pcfich_get(regs_t *h, cf_t *slot_symbols, cf_t ch_data[REGS_PCFICH_NSYM]) {
  regs_ch_t *rch = &h->pcfich;
  uint32_t i;
  for (i = 0; i < rch->nof_regs && i*REGS_RE_X_REG < REGS_PCFICH_NSYM; i++) {
    regs_get_reg(rch->regs[i], slot_symbols, &ch_data[i*REGS_RE_X_REG], h->cell.nof_prb);
  }
  return i*REGS_RE_X_REG;
}














/***************************************************************
 *
 * COMMON FUNCTIONS
 *
 ***************************************************************/

regs_reg_t *regs_find_reg(regs_t *h, uint32_t k, uint32_t l) {
  uint32_t i;
  for (i=0;i<h->nof_regs;i++) {
    if (h->regs[i].l == l && h->regs[i].k0 == k) {
      return &h->regs[i];
    }
  }
  return NULL;
}

/**
 * Returns the number of REGs in a PRB
 * 36.211 Section 6.2.4
 */
int regs_num_x_symbol(uint32_t symbol, uint32_t nof_port, lte_cp_t cp) {

  switch (symbol) {
  case 0:
    return 2;
  case 1:
    switch (nof_port) {
    case 1:
    case 2:
      return 3;
    case 4:
      return 2;
    default:
      fprintf(stderr, "Invalid number of ports %d\n", nof_port);
      return LIBLTE_ERROR;
    }
    break;
  case 2:
    return 3;
  case 3:
    if (CP_ISNORM(cp)) {
      return 3;
    } else {
      return 2;
    }
  default:
      fprintf(stderr, "Invalid symbol %d\n", symbol);
    return LIBLTE_ERROR;
  }
}

/**
 * Initializes the indices of a REG
 * 36.211 Section 6.2.4
 */
int regs_reg_init(regs_reg_t *reg, uint32_t symbol, uint32_t nreg, uint32_t k0, uint32_t maxreg, uint32_t vo) {
  uint32_t i, j, z;

  reg->l = symbol;
  reg->assigned = false;

  switch (maxreg) {
  case 2:
    reg->k0 = k0 + nreg * 6;
    /* there are two references in the middle */
    j = z = 0;
    for (i = 0; i < vo; i++) {
      reg->k[j] = k0 + nreg * 6 + i;
      j++;
    }
    for (i = 0; i < 2; i++) {
      reg->k[j] = k0 + nreg * 6 + i + vo + 1;
      j++;
    }
    z = j;
    for (i = 0; i < 4 - z; i++) {
      reg->k[j] = k0 + nreg * 6 + vo + 3 + i + 1;
      j++;
    }
    if (j != 4) {
      fprintf(stderr, "Something went wrong: expected 2 references\n");
      return LIBLTE_ERROR;
    }
    break;

  case 3:
    reg->k0 = k0 + nreg * 4;
    /* there is no reference */
    for (i = 0; i < 4; i++) {
      reg->k[i] = k0 + nreg * 4 + i;
    }
    break;
  default:
    fprintf(stderr, "Invalid number of REGs per PRB: %d\n", maxreg);
    return LIBLTE_ERROR;
  }
  return LIBLTE_SUCCESS;
}

void regs_free(regs_t *h) {
  if (h->regs) {
    free(h->regs);
  }
  regs_pcfich_free(h);
  regs_phich_free(h);
  regs_pdcch_free(h);

  bzero(h, sizeof(regs_t));
}

/** Sets the CFI value for this subframe (CFI must be in the range 1..3).
 */
int regs_set_cfi(regs_t *h, uint32_t cfi) {  
  if (cfi > 0 && cfi <= 3) {
    if (h->phich_len == PHICH_EXT &&
        ((h->cell.nof_prb < 10 && cfi < 2) || (h->cell.nof_prb >= 10 && cfi < 3))) {
      fprintf(stderr, "PHICH length is extended. The number of control symbols should be at least 3.\n");
      return LIBLTE_ERROR_INVALID_INPUTS;
    } else {
      h->cfi_initiated = true;
      h->cfi = cfi - 1;
      return LIBLTE_SUCCESS;
    }
  } else {
    fprintf(stderr, "Invalid CFI %d\n", cfi);
    return LIBLTE_ERROR_INVALID_INPUTS;
  }
}

/**
 * Initializes REGs structure.
 * Sets all REG indices and initializes PCFICH, PHICH and PDCCH REGs
 * Returns 0 if OK, -1 on error
 */
int regs_init(regs_t *h, lte_cell_t cell) {
  int ret = LIBLTE_ERROR_INVALID_INPUTS;
  uint32_t i, k;
  uint32_t j[4], jmax, prb;
  uint32_t n[4], vo;
  uint32_t max_ctrl_symbols;

  if (h != NULL &&
      lte_cell_isvalid(&cell))
  {
    bzero(h, sizeof(regs_t));
    ret = LIBLTE_ERROR;
    
    max_ctrl_symbols = cell.nof_prb<10?4:3;
    vo = cell.id % 3;
    h->cell = cell;
    h->max_ctrl_symbols = max_ctrl_symbols;
    h->cfi_initiated = false;
    h->phich_res = cell.phich_resources;
    h->phich_len = cell.phich_length;

    h->nof_regs = 0;
    for (i = 0; i < max_ctrl_symbols; i++) {
      n[i] = regs_num_x_symbol(i, h->cell.nof_ports, h->cell.cp);
      if (n[i] == -1) {
        goto clean_and_exit;
      }
      h->nof_regs += h->cell.nof_prb * n[i];
    }
    INFO("Indexing %d REGs. CellId: %d, %d PRB, CP: %s\n", h->nof_regs, h->cell.id, h->cell.nof_prb,
        CP_ISNORM(h->cell.cp)?"Normal":"Extended");
    h->regs = malloc(sizeof(regs_reg_t) * h->nof_regs);
    if (!h->regs) {
      perror("malloc");
      goto clean_and_exit;
    }

    /* Sort REGs according to PDCCH mapping, beggining from the lowest l index then k */
    bzero(j, sizeof(int) * 4);
    k = i = prb = jmax = 0;
    while (k < h->nof_regs) {
      if (n[i] == 3 || (n[i] == 2 && jmax != 1)) {
        if (regs_reg_init(&h->regs[k], i, j[i], prb * RE_X_RB, n[i], vo)) {
          fprintf(stderr, "Error initializing REGs\n");
          goto clean_and_exit;
        }
        /*DEBUG("Available REG #%3d: l=%d, prb=%d, nreg=%d (k0=%d)\n", k, i, prb, j[i],
            h->regs[k].k0);
        */
        j[i]++;
        k++;
      }
      i++;
      if (i == max_ctrl_symbols) {
        i = 0;
        jmax++;
      }
      if (jmax == 3) {
        prb++;
        bzero(j, sizeof(int) * 4);
        jmax = 0;
      }
    }
    if (regs_pcfich_init(h)) {
      fprintf(stderr, "Error initializing PCFICH REGs\n");
      goto clean_and_exit;
    }

    if (regs_phich_init(h)) {
      fprintf(stderr, "Error initializing PHICH REGs\n");
      goto clean_and_exit;
    }
    if (regs_pdcch_init(h)) {
      fprintf(stderr, "Error initializing PDCCH REGs\n");
      goto clean_and_exit;
    }

    ret = LIBLTE_SUCCESS;
  }
clean_and_exit:
  if (ret != LIBLTE_SUCCESS) {
    regs_free(h);
  }
  return ret;
}

/**
 * Puts one REG data (4 symbols) in the slot symbols array
 */
int regs_put_reg(regs_reg_t *reg, cf_t *reg_data, cf_t *slot_symbols, uint32_t nof_prb) {
  uint32_t i;
  for (i = 0; i < REGS_RE_X_REG; i++) {
    slot_symbols[REG_IDX(reg, i, nof_prb)] = reg_data[i];
  }
  return REGS_RE_X_REG;
}

/**
 * Adds one REG data (4 symbols) in the slot symbols array
 * Used by PHICH
 */
int regs_add_reg(regs_reg_t *reg, cf_t *reg_data, cf_t *slot_symbols, uint32_t nof_prb) {
  uint32_t i;
  for (i = 0; i < REGS_RE_X_REG; i++) {
    slot_symbols[REG_IDX(reg, i, nof_prb)] += reg_data[i];
  }
  return REGS_RE_X_REG;
}


/**
 * Reset REG data (4 symbols) in the slot symbols array
 */
int regs_reset_reg(regs_reg_t *reg, cf_t *slot_symbols, uint32_t nof_prb) {
  uint32_t i;
  for (i = 0; i < REGS_RE_X_REG; i++) {
    slot_symbols[REG_IDX(reg, i, nof_prb)] = 0;
  }
  return REGS_RE_X_REG;
}

/**
 * Gets one REG data (4 symbols) from the slot symbols array
 */
int regs_get_reg(regs_reg_t *reg, cf_t *slot_symbols, cf_t *reg_data, uint32_t nof_prb) {
  uint32_t i;
  for (i = 0; i < REGS_RE_X_REG; i++) {
    reg_data[i] = slot_symbols[REG_IDX(reg, i, nof_prb)];
  }
  return REGS_RE_X_REG;
}


