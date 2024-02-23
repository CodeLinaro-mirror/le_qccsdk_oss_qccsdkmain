 /*
 */
/************************************************************************/
/* Header file for BDF module                                           */
/************************************************************************/
// 0.0.8
#ifndef  _BDF_TEMPLATE_H_
#define _BDF_TEMPLATE_H_
#include <stdint.h>

// Custom Macros
#define HALPHY_NUM_BAND_EDGES_2G_11B	6
#define HALPHY_NUM_BAND_EDGES_2G_11G	6
#define HALPHY_NUM_BAND_EDGES_2G_HT20	6
#define HALPHY_NUM_BAND_EDGES_5G_11A	12
#define HALPHY_NUM_BAND_EDGES_5G_HT20	12
#define HALPHY_NUM_BAND_EDGES_6G_EXT_11A	4
#define HALPHY_NUM_BAND_EDGES_6G_EXT_HT20	4

// Macros Dim1

// Macros Dim2

// Macros Dim3

// Typedef enums
typedef enum {
	HALPHY_RF_SUBBAND_2G_1_14	= 0,
	HALPHY_RF_SUBBAND_2G_MAX,
} HALPHY_RF_SUBBAND_2G;

typedef enum {
	HALPHY_RF_SUBBAND_5G_36_64	= 0,
	HALPHY_RF_SUBBAND_5G_100_144,
	HALPHY_RF_SUBBAND_5G_149_181,
	HALPHY_RF_SUBBAND_5G_MAX,
} HALPHY_RF_SUBBAND_5G;

typedef enum {
	HALPHY_RF_SUBBAND_6G_1_93	= 0,
	HALPHY_RF_SUBBAND_6G_97_113,
	HALPHY_RF_SUBBAND_6G_117_141,
	HALPHY_RF_SUBBAND_6G_MAX,
} HALPHY_RF_SUBBAND_6G;

typedef enum {
	HALPHY_TPC_TEMP_ADJ_RATE_1M_2M_CCK 	= 0,
	HALPHY_TPC_TEMP_ADJ_RATE_6M_LEGACY,
	HALPHY_TPC_TEMP_ADJ_RATE_MCS0,
} HALPHY_RATE_IDX_FOR_TEMP_BASED_TPC_ADJ;

#endif //_BDF_TEMPLATE_H_
