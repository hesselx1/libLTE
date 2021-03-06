/*
 * Generated by asn1c-0.9.22 (http://lionet.info/asn1c)
 * From ASN.1 module "EUTRA-RRC-Definitions"
 * 	found in "./asn1c/ASN1_files/EUTRA-RRC-Definitions.asn"
 * 	`asn1c -gen-PER -fcompound-names -fnative-types`
 */

#ifndef	_MeasObjectCDMA2000_H_
#define	_MeasObjectCDMA2000_H_


#include <liblte/rrc/asn/asn_application.h>

/* Including external dependencies */
#include "liblte/rrc/asn/CDMA2000-Type.h"
#include "liblte/rrc/asn/CarrierFreqCDMA2000.h"
#include <liblte/rrc/asn/NativeInteger.h>
#include "liblte/rrc/asn/Q-OffsetRangeInterRAT.h"
#include "liblte/rrc/asn/PhysCellIdCDMA2000.h"
#include <liblte/rrc/asn/constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct CellIndexList;
struct CellsToAddModListCDMA2000;

/* MeasObjectCDMA2000 */
typedef struct MeasObjectCDMA2000 {
	CDMA2000_Type_t	 cdma2000_Type;
	CarrierFreqCDMA2000_t	 carrierFreq;
	long	*searchWindowSize	/* OPTIONAL */;
	Q_OffsetRangeInterRAT_t	 offsetFreq	/* DEFAULT 0 */;
	struct CellIndexList	*cellsToRemoveList	/* OPTIONAL */;
	struct CellsToAddModListCDMA2000	*cellsToAddModList	/* OPTIONAL */;
	PhysCellIdCDMA2000_t	*cellForWhichToReportCGI	/* OPTIONAL */;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} MeasObjectCDMA2000_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_MeasObjectCDMA2000;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "liblte/rrc/asn/CellIndexList.h"
#include "liblte/rrc/asn/CellsToAddModListCDMA2000.h"

#endif	/* _MeasObjectCDMA2000_H_ */
#include <liblte/rrc/asn/asn_internal.h>
