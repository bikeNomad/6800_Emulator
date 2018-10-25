/***********************************************************************************************************************
 * This file was generated by the MCUXpresso Config Tools. Any manual edits made to this file
 * will be overwritten if the respective MCUXpresso Config Tools is used to update this file.
 **********************************************************************************************************************/

#ifndef _CLOCK_CONFIG_H_
#define _CLOCK_CONFIG_H_

#include "fsl_common.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 ************************ BOARD_InitBootClocks function ************************
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*!
 * @brief This function executes default configuration of clocks.
 *
 */
void BOARD_InitBootClocks(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

/*******************************************************************************
 *********************** Configuration BOARD_FastClock *************************
 ******************************************************************************/
/*******************************************************************************
 * Definitions for BOARD_FastClock configuration
 ******************************************************************************/
#define BOARD_FASTCLOCK_CORE_CLOCK                168000000U  /*!< Core clock frequency: 168000000Hz */

/*! @brief SCG set for BOARD_FastClock configuration.
 */
extern const scg_sys_clk_config_t g_sysClkConfig_BOARD_FastClock;
/*! @brief System OSC set for BOARD_FastClock configuration.
 */
extern const scg_sosc_config_t g_scgSysOscConfig_BOARD_FastClock;
/*! @brief SIRC set for BOARD_FastClock configuration.
 */
extern const scg_sirc_config_t g_scgSircConfig_BOARD_FastClock;
/*! @brief FIRC set for BOARD_FastClock configuration.
 */
extern const scg_firc_config_t g_scgFircConfigBOARD_FastClock;
extern const scg_spll_config_t g_scgSysPllConfigBOARD_FastClock;
/*! @brief Low Power FLL set for BOARD_FastClock configuration.
 */

/*******************************************************************************
 * API for BOARD_FastClock configuration
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*!
 * @brief This function executes configuration of clocks.
 *
 */
void BOARD_FastClock(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

/*******************************************************************************
 *********************** Configuration BOARD_SlowClock *************************
 ******************************************************************************/
/*******************************************************************************
 * Definitions for BOARD_SlowClock configuration
 ******************************************************************************/
#define BOARD_SLOWCLOCK_CORE_CLOCK                  4000000U  /*!< Core clock frequency: 4000000Hz */

/*! @brief SCG set for BOARD_SlowClock configuration.
 */
extern const scg_sys_clk_config_t g_sysClkConfig_BOARD_SlowClock;
/*! @brief System OSC set for BOARD_SlowClock configuration.
 */
extern const scg_sosc_config_t g_scgSysOscConfig_BOARD_SlowClock;
/*! @brief SIRC set for BOARD_SlowClock configuration.
 */
extern const scg_sirc_config_t g_scgSircConfig_BOARD_SlowClock;
/*! @brief FIRC set for BOARD_SlowClock configuration.
 */
extern const scg_firc_config_t g_scgFircConfigBOARD_SlowClock;
extern const scg_spll_config_t g_scgSysPllConfigBOARD_SlowClock;
/*! @brief Low Power FLL set for BOARD_SlowClock configuration.
 */

/*******************************************************************************
 * API for BOARD_SlowClock configuration
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*!
 * @brief This function executes configuration of clocks.
 *
 */
void BOARD_SlowClock(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

#endif /* _CLOCK_CONFIG_H_ */
