/**
 * @file  bdb_options.h
 * @brief Configuration options
 */

#ifndef BDB_OPTIONS_H
#define BDB_OPTIONS_H

#include <jendefs.h>

#if (SINGLE_CHANNEL < 11 || SINGLE_CHANNEL > 26)
#define BDB_PRIMARY_CHANNEL_SET       (0x02108800)
#define BDB_SECONDARY_CHANNEL_SET     (0x07FFF800 ^ BDB_PRIMARY_CHANNEL_SET)
#define BDBC_TL_PRIMARY_CHANNEL_SET   (0x02108800)
#define BDBC_TL_SECONDARY_CHANNEL_SET (0x07fff800 ^ BDBC_TL_PRIMARY_CHANNEL_SET)
#else
#warning Single channel only!
#define BDB_PRIMARY_CHANNEL_SET       (1 << SINGLE_CHANNEL)
#define BDB_SECONDARY_CHANNEL_SET     (0)
#define BDBC_TL_PRIMARY_CHANNEL_SET   (0x02108800)
#define BDBC_TL_SECONDARY_CHANNEL_SET (0x07fff800 ^ BDBC_TL_PRIMARY_CHANNEL_SET)
#endif

/* BDB Constants used by all nodes
   Note - Must not change for final app */
#define BDBC_MAX_SAME_NETWORK_RETRY_ATTEMPTS (10)
#define BDBC_MIN_COMMISSIONING_TIME          (180)
#define BDBC_REC_SAME_NETWORK_RETRY_ATTEMPTS (3)
#define BDBC_TC_LINK_KEY_EXCHANGE_TIMEOUT    (5)

/* BDB Constants used by nodes supporting touchlink
   Note - Must not change for final app */
#define BDBC_TL_INTERPAN_TRANS_ID_LIFETIME (8)
#define BDBC_TL_MIN_STARTUP_DELAY_TIME     (2)
#define BDBC_TL_RX_WINDOW_DURATION         (5)
#define BDBC_TL_SCAN_TIME_BASE_DURATION_MS (250)

/* BDB Attribute values */
#define BDB_COMMISSIONING_GROUP_ID     (0xFFFF)
#define BDB_COMMISSIONING_MODE         (0x0F)
#define BDB_COMMISSIONING_STATUS       (0x00)
#define BDB_JOINING_NODE_EUI64         (0x0000000000000000)
#define BDB_JOIN_USES_INSTALL_CODE_KEY (FALSE)

#define BDB_NODE_JOIN_LINK_KEY_TYPE (0x00)

#define BDB_SCAN_DURATION                     (0x04)
#define BDB_TC_LINK_KEY_EXCHANGE_ATTEMPTS     (0x00)
#define BDB_TC_LINK_KEY_EXCHANGE_ATTEMPTS_MAX (0x03)
#define BDB_TC_LINK_KEY_EXCHANGE_METHOD       (0x00)
#define BDB_TRUST_CENTER_NODE_JOIN_TIMEOUT    (0x0F)
#define BDB_TRUST_CENTER_REQUIRE_KEYEXCHANGE  (TRUE)
#define BDB_SET_DEFAULT_TC_POLICY             (FALSE)

#endif /* BDB_OPTIONS_H */
