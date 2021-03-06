/*
 * Generated by asn1c-0.9.22 (http://lionet.info/asn1c)
 * From ASN.1 module "EUTRA-RRC-Definitions"
 * 	found in "./asn1c/ASN1_files/EUTRA-RRC-Definitions.asn"
 * 	`asn1c -gen-PER -fcompound-names -fnative-types`
 */

#ifndef	_SchedulingRequestConfig_v1020_H_
#define	_SchedulingRequestConfig_v1020_H_


#include <liblte/rrc/asn/asn_application.h>

/* Including external dependencies */
#include <liblte/rrc/asn/NativeInteger.h>
#include <liblte/rrc/asn/constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SchedulingRequestConfig-v1020 */
typedef struct SchedulingRequestConfig_v1020 {
	long	*sr_PUCCH_ResourceIndexP1_r10	/* OPTIONAL */;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} SchedulingRequestConfig_v1020_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_SchedulingRequestConfig_v1020;

#ifdef __cplusplus
}
#endif

#endif	/* _SchedulingRequestConfig_v1020_H_ */
#include <liblte/rrc/asn/asn_internal.h>
