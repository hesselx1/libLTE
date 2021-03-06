/*
 * Generated by asn1c-0.9.22 (http://lionet.info/asn1c)
 * From ASN.1 module "EUTRA-RRC-Definitions"
 * 	found in "./asn1c/ASN1_files/EUTRA-RRC-Definitions.asn"
 * 	`asn1c -gen-PER -fcompound-names -fnative-types`
 */

#ifndef	_MeasResult2UTRA_r9_H_
#define	_MeasResult2UTRA_r9_H_


#include <liblte/rrc/asn/asn_application.h>

/* Including external dependencies */
#include "liblte/rrc/asn/ARFCN-ValueUTRA.h"
#include "liblte/rrc/asn/MeasResultListUTRA.h"
#include <liblte/rrc/asn/constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MeasResult2UTRA-r9 */
typedef struct MeasResult2UTRA_r9 {
	ARFCN_ValueUTRA_t	 carrierFreq_r9;
	MeasResultListUTRA_t	 measResultList_r9;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} MeasResult2UTRA_r9_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_MeasResult2UTRA_r9;

#ifdef __cplusplus
}
#endif

#endif	/* _MeasResult2UTRA_r9_H_ */
#include <liblte/rrc/asn/asn_internal.h>
