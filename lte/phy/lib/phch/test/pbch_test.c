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
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>

#include "liblte/phy/phy.h"

lte_cell_t cell = {
  6,            // nof_prb
  1,            // nof_ports
  1,            // cell_id
  CPNORM,       // cyclic prefix
  R_1,          // PHICH resources      
  PHICH_NORM    // PHICH length
};

void usage(char *prog) {
  printf("Usage: %s [cpv]\n", prog);
  printf("\t-c cell id [Default %d]\n", cell.id);
  printf("\t-p cell.nof_ports [Default %d]\n", cell.nof_ports);
  printf("\t-n cell.nof_prb [Default %d]\n", cell.nof_prb);
  printf("\t-v [set verbose to debug, default none]\n");
}

void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "cpnv")) != -1) {
    switch(opt) {
    case 'p':
      cell.nof_ports = atoi(argv[optind]);
      break;
    case 'n':
      cell.nof_prb = atoi(argv[optind]);
      break;
    case 'c':
      cell.id = atoi(argv[optind]);
      break;
    case 'v':
      verbose++;
      break;
    default:
      usage(argv[0]);
      exit(-1);
    }
  }
}


int main(int argc, char **argv) {
  pbch_t pbch;
  uint8_t bch_payload_tx[BCH_PAYLOAD_LEN], bch_payload_rx[BCH_PAYLOAD_LEN];
  int i, j;
  cf_t *ce[MAX_PORTS];
  int nof_re;
  cf_t *slot1_symbols[MAX_PORTS];
  uint32_t nof_rx_ports; 

  parse_args(argc,argv);

  nof_re = SLOT_LEN_RE(cell.nof_prb, CPNORM); 

  /* init memory */
  for (i=0;i<cell.nof_ports;i++) {
    ce[i] = malloc(sizeof(cf_t) * nof_re);
    if (!ce[i]) {
      perror("malloc");
      exit(-1);
    }
    for (j=0;j<nof_re;j++) {
      ce[i][j] = 1;
    }
    slot1_symbols[i] = malloc(sizeof(cf_t) * nof_re);
    if (!slot1_symbols[i]) {
      perror("malloc");
      exit(-1);
    }

  }
  if (pbch_init(&pbch, cell)) {
    fprintf(stderr, "Error creating PBCH object\n");
    exit(-1);
  }

  srand(time(NULL));
  for (i=0;i<BCH_PAYLOAD_LEN;i++) {
    bch_payload_tx[i] = rand()%2;
  }

  pbch_encode(&pbch, bch_payload_tx, slot1_symbols);

  /* combine outputs */
  for (i=1;i<cell.nof_ports;i++) {
    for (j=0;j<nof_re;j++) {
      slot1_symbols[0][j] += slot1_symbols[i][j];
    }
  }
  
  pbch_decode_reset(&pbch);
  if (1 != pbch_decode(&pbch, slot1_symbols[0], ce, 0, bch_payload_rx, &nof_rx_ports, NULL)) {
    printf("Error decoding\n");
    exit(-1);
  }

  pbch_free(&pbch);

  for (i=0;i<cell.nof_ports;i++) {
    free(ce[i]);
    free(slot1_symbols[i]);
  }
  printf("Tx ports: %d - Rx ports: %d\n", cell.nof_ports, nof_rx_ports);
  printf("Tx payload: ");
  vec_fprint_hex(stdout, bch_payload_tx, BCH_PAYLOAD_LEN);
  printf("Rx payload: ");
  vec_fprint_hex(stdout, bch_payload_rx, BCH_PAYLOAD_LEN);

  if (nof_rx_ports == cell.nof_ports && !memcmp(bch_payload_rx, bch_payload_tx, sizeof(uint8_t) * BCH_PAYLOAD_LEN)) {
    printf("OK\n");
    exit(0);
  } else {
    printf("Error\n");
    exit(-1);
  }
}
