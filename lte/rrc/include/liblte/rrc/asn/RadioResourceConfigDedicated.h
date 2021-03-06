/*
 * Generated by asn1c-0.9.22 (http://lionet.info/asn1c)
 * From ASN.1 module "EUTRA-RRC-Definitions"
 * 	found in "./asn1c/ASN1_files/EUTRA-RRC-Definitions.asn"
 * 	`asn1c -gen-PER -fcompound-names -fnative-types`
 */

#ifndef	_RadioResourceConfigDedicated_H_
#define	_RadioResourceConfigDedicated_H_


#include <liblte/rrc/asn/asn_application.h>

/* Including external dependencies */
#include "liblte/rrc/asn/MAC-MainConfig.h"
#include <liblte/rrc/asn/NULL.h>
#include <liblte/rrc/asn/constr_CHOICE.h>
#include <liblte/rrc/asn/constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum RadioResourceConfigDedicated__mac_MainConfig_PR {
	RadioResourceConfigDedicated__mac_MainConfig_PR_NOTHING,	/* No components present */
	RadioResourceConfigDedicated__mac_MainConfig_PR_explicitValue,
	RadioResourceConfigDedicated__mac_MainConfig_PR_defaultValue
} RadioResourceConfigDedicated__mac_MainConfig_PR;

/* Forward declarations */
struct SRB_ToAddModList;
struct DRB_ToAddModList;
struct DRB_ToReleaseList;
struct SPS_Config;
struct PhysicalConfigDedicated;
struct RLF_TimersAndConstants_r9;
struct MeasSubframePatternPCell_r10;
struct SPS_RA_ConfigList_rlola;

/* RadioResourceConfigDedicated */
typedef struct RadioResourceConfigDedicated {
	struct SRB_ToAddModList	*srb_ToAddModList	/* OPTIONAL */;
	struct DRB_ToAddModList	*drb_ToAddModList	/* OPTIONAL */;
	struct DRB_ToReleaseList	*drb_ToReleaseList	/* OPTIONAL */;
	struct RadioResourceConfigDedicated__mac_MainConfig {
		RadioResourceConfigDedicated__mac_MainConfig_PR present;
		union RadioResourceConfigDedicated__mac_MainConfig_u {
			MAC_MainConfig_t	 explicitValue;
			NULL_t	 defaultValue;
		} choice;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} *mac_MainConfig;
	struct SPS_Config	*sps_Config	/* OPTIONAL */;
	struct PhysicalConfigDedicated	*physicalConfigDedicated	/* OPTIONAL */;
	struct RLF_TimersAndConstants_r9	*rlf_TimersAndConstants_r9	/* OPTIONAL */;
	struct MeasSubframePatternPCell_r10	*measSubframePatternPCell_r10	/* OPTIONAL */;
	struct SPS_RA_ConfigList_rlola	*sps_RA_ConfigList_rlola	/* OPTIONAL */;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} RadioResourceConfigDedicated_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_RadioResourceConfigDedicated;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "liblte/rrc/asn/SRB-ToAddModList.h"
#include "liblte/rrc/asn/DRB-ToAddModList.h"
#include "liblte/rrc/asn/DRB-ToReleaseList.h"
#include "liblte/rrc/asn/SPS-Config.h"
#include "liblte/rrc/asn/PhysicalConfigDedicated.h"
#include "liblte/rrc/asn/RLF-TimersAndConstants-r9.h"
#include "liblte/rrc/asn/MeasSubframePatternPCell-r10.h"
#include "liblte/rrc/asn/SPS-RA-ConfigList-rlola.h"

#endif	/* _RadioResourceConfigDedicated_H_ */
#include <liblte/rrc/asn/asn_internal.h>
