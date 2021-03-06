/*
 * Generated by asn1c-0.9.22 (http://lionet.info/asn1c)
 * From ASN.1 module "EUTRA-RRC-Definitions"
 * 	found in "./asn1c/ASN1_files/EUTRA-RRC-Definitions.asn"
 * 	`asn1c -gen-PER -fcompound-names -fnative-types`
 */

#ifndef	_QuantityConfig_H_
#define	_QuantityConfig_H_


#include <liblte/rrc/asn/asn_application.h>

/* Including external dependencies */
#include <liblte/rrc/asn/constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct QuantityConfigEUTRA;
struct QuantityConfigUTRA;
struct QuantityConfigGERAN;
struct QuantityConfigCDMA2000;
struct QuantityConfigUTRA_v1020;

/* QuantityConfig */
typedef struct QuantityConfig {
	struct QuantityConfigEUTRA	*quantityConfigEUTRA	/* OPTIONAL */;
	struct QuantityConfigUTRA	*quantityConfigUTRA	/* OPTIONAL */;
	struct QuantityConfigGERAN	*quantityConfigGERAN	/* OPTIONAL */;
	struct QuantityConfigCDMA2000	*quantityConfigCDMA2000	/* OPTIONAL */;
	struct QuantityConfigUTRA_v1020	*quantityConfigUTRA_v1020	/* OPTIONAL */;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} QuantityConfig_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_QuantityConfig;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "liblte/rrc/asn/QuantityConfigEUTRA.h"
#include "liblte/rrc/asn/QuantityConfigUTRA.h"
#include "liblte/rrc/asn/QuantityConfigGERAN.h"
#include "liblte/rrc/asn/QuantityConfigCDMA2000.h"
#include "liblte/rrc/asn/QuantityConfigUTRA-v1020.h"

#endif	/* _QuantityConfig_H_ */
#include <liblte/rrc/asn/asn_internal.h>
