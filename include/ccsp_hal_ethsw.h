/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file ccsp_hal_ethsw.h
 * @brief Declares the Ethernet Switch (EthSW) HAL: port status and
 * configuration, administrative state, MAC address-table aging, port lookup by
 * MAC address, associated-device enumeration and notification, Ethernet WAN
 * enable and port selection with provisioning callbacks, and per-port
 * statistics.
 *
 * This header is the whole contract between RDK-B middleware, which calls the
 * interface, and a vendor implementation, which provides it. The declared
 * surface is exactly the following twenty functions:
 *
 * - Lifecycle: `CcspHalEthSwInit()`.
 * - Port status and configuration: `CcspHalEthSwGetPortStatus()`,
 *   `CcspHalEthSwGetPortCfg()`, `CcspHalEthSwSetPortCfg()`,
 *   `CcspHalEthSwGetPortAdminStatus()`, `CcspHalEthSwSetPortAdminStatus()`.
 * - Forwarding-table maintenance: `CcspHalEthSwSetAgingSpeed()`,
 *   `CcspHalEthSwLocatePortByMacAddress()`.
 * - Connected devices: `CcspHalExtSw_getAssociatedDevice()`,
 *   `CcspHalExtSw_ethAssociatedDevice_callback_register()`.
 * - Ethernet WAN: `CcspHalExtSw_ethPortConfigure()`,
 *   `CcspHalExtSw_getEthWanEnable()`, `CcspHalExtSw_setEthWanEnable()`,
 *   `CcspHalExtSw_getCurrentWanHWConf()`, `CcspHalExtSw_getEthWanPort()`,
 *   `CcspHalExtSw_setEthWanPort()`, `GWP_RegisterEthWan_Callback()`,
 *   `GWP_GetEthWanLinkStatus()`, `GWP_GetEthWanInterfaceName()`.
 * - Statistics: `CcspHalEthSwGetEthPortStats()`.
 *
 * @note Interface scope. No VLAN creation or modification, bridging,
 * link-aggregation, QoS or DSCP priority, ACL, or IGMP/MLD multicast control
 * function is declared here, so no such capability is part of this contract.
 * `eth_vlanid` in ::eth_device_t is a reported attribute of an observed device,
 * not a VLAN-management entry point. [derived from the declarations in this
 * file]
 *
 * @note Error reporting is a single channel. Every status-returning function in
 * this interface returns only `RETURN_OK` or `RETURN_ERR`; the interface defines
 * no error-code enumeration, so a caller cannot distinguish an invalid argument
 * from a hardware or communication failure by the return value alone. A caller
 * that needs to tell those cases apart must validate its own arguments before
 * the call and consult the vendor log. Errors are reported synchronously as the
 * return value. [the HAL specification, "Internal Error Handling";
 * `RETURN_OK` and `RETURN_ERR` in this file]
 *
 * @note Threading. This interface is not thread safe: a module invoking it must
 * serialise its own calls. [the HAL specification, "Threading Model"]
 *
 * @note Blocking. Implementations are required to complete synchronously and
 * must not block or suspend the calling thread, because the interface is called
 * from a single-threaded context. `CcspHalEthSwInit()` is the one documented
 * exception. [the HAL specification, "Blocking calls" and
 * "Initialization and Startup"]
 *
 * @note Persistence. This interface places no requirement on the vendor
 * implementation to persist any setting across a restart, so a caller must
 * re-apply configuration after re-initialisation rather than assume it
 * survived. [the HAL specification, "Persistence Model"]
 *
 * @note Diagnostics. Vendor logging is written to `ethsw_vendor_hal.log` in
 * either `/var/tmp/` or `/rdklogs/logs/`, and the implementation is delivered as
 * the shared library `libhal_ethsw.so`. [the HAL specification,
 * "Logging and debugging requirements" and "Build Requirements"]
 */

#ifndef __CCSP_HAL_ETHSW_H__
#define __CCSP_HAL_ETHSW_H__

/**
 * @defgroup ETHSW_HAL Ethernet Switch HAL Interface
 * @brief Provides an interface for interacting with Ethernet switch hardware.
 *
 * This component enables control and monitoring of Ethernet switch functionality, including port configuration, link status, and statistics.
 *
 * @{
 * @defgroup ETHSW_HAL_TYPES Data Types
 * @defgroup ETHSW_HAL_APIS APIs
 * @}
 */

/**
 * @addtogroup ETHSW_HAL_TYPES
 * @{
 */

/**********************************************************************
               CONSTANT DEFINITIONS
**********************************************************************/

#define UP "up" /*!< String an implementation reports for an interface that is up; compare against it rather than against a locally spelled literal. */
#define DOWN "down" /*!< String an implementation reports for an interface that is down; compare against it rather than against a locally spelled literal. */

/**
 * The seven type aliases below exist so that this header can be included by a
 * translation unit that has no RDK-B platform type header available. Each is
 * guarded, so the definition here is used only when the including unit has not
 * already provided that name; where the platform does define it, the platform
 * definition stands and this header adds nothing. A caller should therefore
 * treat these as spellings of the underlying C types, not as distinct types.
 */

#ifndef ULONG
#define ULONG unsigned long /*!< Unsigned long used for array sizes and buffer lengths in this interface. */
#endif

#ifndef ULLONG
#define ULLONG unsigned long long /*!< Unsigned long long used for the 64-bit byte counters in ::CCSP_HAL_ETH_STATS. */
#endif

#ifndef CHAR
#define CHAR  char /*!< Signed character type; declared for platform compatibility and not used by any declaration in this header. */
#endif

#ifndef UCHAR
#define UCHAR unsigned char /*!< Unsigned character type, used for the MAC address octets in ::eth_device_t. */
#endif

#ifndef BOOLEAN
#define BOOLEAN  UCHAR /*!< Truth value carried as an unsigned char; set it from `TRUE` or `FALSE` and never assume any other value is accepted. */
#endif

#ifndef INT
#define INT   int /*!< Signed integer type; the return type of every status-returning function declared here. */
#endif

#ifndef UINT
#define UINT unsigned int /*!< Unsigned integer type, used for the Ethernet WAN port index. */
#endif

#ifndef TRUE
#define TRUE     1 /*!< The true value for a `BOOLEAN` argument or output of this interface. */
#endif

#ifndef FALSE
#define FALSE    0 /*!< The false value for a `BOOLEAN` argument or output of this interface. */
#endif

#ifndef ENABLE
#define ENABLE   1 /*!< Legacy enable constant. No declaration in this header takes or returns it, so a caller has no use for it; it is retained only so that including this header does not change the meaning of the name in a translation unit that expects it. */
#endif

#ifndef RETURN_OK
#define RETURN_OK   0 /*!< Success. The only value that indicates a status-returning function of this interface completed and honoured its post-conditions. */
#endif

#ifndef RETURN_ERR
#define RETURN_ERR   -1 /*!< Failure. The interface's only error value: it identifies that the operation failed but not why, so a caller cannot discriminate the cause from the return value. */
#endif

#ifndef ETHWAN_DEF_INTF_NUM

/**
 *  @brief Default Ethernet WAN interface index.
 *
 *  Identifies the physical interface that the Auto WAN feature in provisioning
 *  apps, the CCSP Ethernet Agent with the Ethernet WAN feature (see
 *  `CcspHalExtSw_setEthWanPort()`) and the Ethernet WAN HAL use when no port has
 *  been selected explicitly.
 *
 *  The index is 0-based, so the first physical port is 0. This differs from
 *  ::CCSP_HAL_ETHSW_PORT, whose enumerators start at 1; the two numbering
 *  schemes are not interchangeable.
 *
 *  The value is fixed when this header is compiled and is selected by the
 *  first matching hardware-configuration macro:
 *  * `ETH_6_PORTS` defined: 5
 *  * `ETH_5_PORTS` defined: 4
 *  * `ETH_4_PORTS` defined: 3
 *  * `ETH_2_PORTS` and `MODEM_ONLY_SUPPORT` both defined: 0
 *  * none of the above: 0
 *
 *  This interface provides no runtime override of the default: a caller that
 *  needs a different Ethernet WAN port calls `CcspHalExtSw_setEthWanPort()`,
 *  which changes the selected port and leaves this compile-time default
 *  unchanged.
 */

#if defined (ETH_6_PORTS)
#define ETHWAN_DEF_INTF_NUM 5      /*!< Six-port build: the Ethernet WAN default is port index 5, the last physical port. */
#elif defined (ETH_5_PORTS)
#define ETHWAN_DEF_INTF_NUM 4      /*!< Five-port build: the Ethernet WAN default is port index 4, the last physical port. */
#elif defined (ETH_4_PORTS)
#define ETHWAN_DEF_INTF_NUM 3      /*!< Four-port build: the Ethernet WAN default is port index 3, the last physical port. */
#elif defined (ETH_2_PORTS) && defined (MODEM_ONLY_SUPPORT)
#define ETHWAN_DEF_INTF_NUM 0      /*!< Two-port modem-only build: the Ethernet WAN default is port index 0, because no dedicated WAN port exists on this variant. */
#else
/* Default to the first physical port for Ethernet WAN. */
#define ETHWAN_DEF_INTF_NUM 0
#endif
#endif

#ifndef ETHWAN_INTERFACE_NAME_MAX_LENGTH 

/**
 *  @brief Maximum Ethernet WAN interface name length declared by this
 *  interface.
 *
 * This is the only interface-name length bound this header declares, and it is
 * the value a caller should size a `GWP_GetEthWanInterfaceName()` buffer
 * against, passing the same size as that function's `maxSize` argument.
 *
 * @warning The interface is not self-consistent on this bound, and a caller
 * must not assume the three statements agree. This macro declares 32; the
 * `GWP_GetEthWanInterfaceName()` documentation describes `Interface` as a buffer
 * of at least 64 bytes and constrains `maxSize` to an 11-to-262-byte range.
 * Nothing in this interface reconciles the three, and no vendor-independent
 * minimum or maximum can be derived from them. Sizing the buffer to this macro
 * and passing that size as `maxSize` is the only choice grounded in a
 * declaration; a caller needing a larger name must confirm the bound with the
 * vendor implementation.
 */
#define ETHWAN_INTERFACE_NAME_MAX_LENGTH 32
#endif

/**********************************************************************
                ENUMERATION DEFINITIONS
**********************************************************************/

/**
 * @brief Identifies a port of the Ethernet switch.
 *
 * Every port-scoped function in this interface takes one of these enumerators
 * as its `PortId` argument. The enumeration covers the switch's external
 * Ethernet ports, its MoCA and WLAN ports, its processor-facing ports, its
 * interconnect ports and its management port; which of them a given product
 * actually implements is a property of that product, not of this interface, so a
 * caller must be prepared for a valid enumerator to be rejected on hardware
 * that has no such port.
 *
 * @note This enumeration is 1-based: `CCSP_HAL_ETHSW_EthPort1` is 1, so the
 * external Ethernet ports occupy 1 through 8 and the remaining enumerators
 * follow in declaration order. It is therefore not interchangeable with the
 * 0-based Ethernet WAN port index used by `ETHWAN_DEF_INTF_NUM`,
 * `CcspHalExtSw_getEthWanPort()`, `CcspHalExtSw_setEthWanPort()` and
 * `eth_device_t::eth_port`, nor with the port number returned by
 * `CcspHalEthSwLocatePortByMacAddress()`. Converting between the schemes is the
 * caller's responsibility and this interface defines no mapping for it.
 *
 * @warning `CCSP_HAL_ETHSW_PortMax` is a count and sentinel, not a port: passing
 * it to a port-scoped function is an invalid argument.
 */
typedef enum
_CCSP_HAL_ETHSW_PORT
{
    CCSP_HAL_ETHSW_EthPort1  = 1,           /*!< External Ethernet port 1; the first external port, and the lowest valid value of this enumeration. */
    CCSP_HAL_ETHSW_EthPort2,                /*!< External Ethernet port 2. */
    CCSP_HAL_ETHSW_EthPort3,                /*!< External Ethernet port 3. */
    CCSP_HAL_ETHSW_EthPort4,                /*!< External Ethernet port 4. */
    CCSP_HAL_ETHSW_EthPort5,                /*!< External Ethernet port 5. */
    CCSP_HAL_ETHSW_EthPort6,                /*!< External Ethernet port 6. */
    CCSP_HAL_ETHSW_EthPort7,                /*!< External Ethernet port 7. */
    CCSP_HAL_ETHSW_EthPort8,                /*!< External Ethernet port 8; the last external Ethernet port this enumeration names. */

    CCSP_HAL_ETHSW_Moca1,                   /*!< Switch port facing the first MoCA interface. */
    CCSP_HAL_ETHSW_Moca2,                   /*!< Switch port facing the second MoCA interface. */

    CCSP_HAL_ETHSW_Wlan1,                   /*!< Switch port facing the first WLAN interface. */
    CCSP_HAL_ETHSW_Wlan2,                   /*!< Switch port facing the second WLAN interface. */
    CCSP_HAL_ETHSW_Wlan3,                   /*!< Switch port facing the third WLAN interface. */
    CCSP_HAL_ETHSW_Wlan4,                   /*!< Switch port facing the fourth WLAN interface. */

    CCSP_HAL_ETHSW_Processor1,              /*!< Internal switch port facing the first host processor. */
    CCSP_HAL_ETHSW_Processor2,              /*!< Internal switch port facing the second host processor. */

    CCSP_HAL_ETHSW_InterconnectPort1,       /*!< First port interconnecting this switch with another switch or SoC block. */
    CCSP_HAL_ETHSW_InterconnectPort2,       /*!< Second port interconnecting this switch with another switch or SoC block. */

    CCSP_HAL_ETHSW_MgmtPort,                /*!< The switch management port, which carries switch management traffic rather than subscriber traffic. */
    CCSP_HAL_ETHSW_PortMax                  /*!< Count of the enumerators above and the exclusive upper bound of this enumeration; not itself a port and not a valid `PortId`. */
}
CCSP_HAL_ETHSW_PORT, *PCCSP_HAL_ETHSW_PORT; /*!< Pointer form of ::CCSP_HAL_ETHSW_PORT, provided for `[out]` parameters that return a port identifier. No function declared in this header currently takes it. */

/**
 * @brief Speed of an Ethernet switch port link.
 *
 * Returned by `CcspHalEthSwGetPortStatus()` as the rate the link is running at
 * and by `CcspHalEthSwGetPortCfg()` as the rate the port is configured for, and
 * accepted by `CcspHalEthSwSetPortCfg()` as the rate to configure. Which rates a
 * port supports depends on the hardware, so a caller must handle
 * `CcspHalEthSwSetPortCfg()` rejecting a rate this enumeration can express.
 */
typedef enum _CCSP_HAL_ETHSW_LINK_RATE {
    CCSP_HAL_ETHSW_LINK_NULL = 0,  /*!< No rate: reported when the port has no link established, and not a rate to configure. */
    CCSP_HAL_ETHSW_LINK_10Mbps,   /*!< 10 Mbit/s. */
    CCSP_HAL_ETHSW_LINK_100Mbps,  /*!< 100 Mbit/s. */
    CCSP_HAL_ETHSW_LINK_1Gbps,    /*!< 1 Gbit/s. */
    CCSP_HAL_ETHSW_LINK_2_5Gbps,  /*!< 2.5 Gbit/s. */
    CCSP_HAL_ETHSW_LINK_5Gbps,    /*!< 5 Gbit/s. */
    CCSP_HAL_ETHSW_LINK_10Gbps,   /*!< 10 Gbit/s. */
    CCSP_HAL_ETHSW_LINK_Auto     /*!< Auto-negotiate the rate. Set this to let the port negotiate; a get returns the negotiated rate rather than this value once a link is up. */
} CCSP_HAL_ETHSW_LINK_RATE, *PCCSP_HAL_ETHSW_LINK_RATE; /*!< Pointer form of ::CCSP_HAL_ETHSW_LINK_RATE; this is the type of the `pLinkRate` `[out]` parameter of `CcspHalEthSwGetPortStatus()` and `CcspHalEthSwGetPortCfg()`. */

/**
 * @brief Duplex mode of an Ethernet switch port.
 *
 * Returned by `CcspHalEthSwGetPortStatus()` and `CcspHalEthSwGetPortCfg()`, and
 * accepted by `CcspHalEthSwSetPortCfg()`.
 */
typedef enum _CCSP_HAL_ETHSW_DUPLEX_MODE
{
    CCSP_HAL_ETHSW_DUPLEX_Auto = 0, /*!< Auto-negotiate the duplex mode; a get returns the negotiated mode rather than this value once a link is up. */
    CCSP_HAL_ETHSW_DUPLEX_Half,     /*!< Half duplex: the port transmits or receives, but not both at once. */
    CCSP_HAL_ETHSW_DUPLEX_Full      /*!< Full duplex: the port transmits and receives simultaneously. */
} CCSP_HAL_ETHSW_DUPLEX_MODE, *PCCSP_HAL_ETHSW_DUPLEX_MODE; /*!< Pointer form of ::CCSP_HAL_ETHSW_DUPLEX_MODE; this is the type of the `pDuplexMode` `[out]` parameter of `CcspHalEthSwGetPortStatus()` and `CcspHalEthSwGetPortCfg()`. */

/**
 * @brief Operational link state of an Ethernet switch port.
 *
 * Returned by `CcspHalEthSwGetPortStatus()`. This interface reports the state as
 * a value and does not define which transitions between these values are legal
 * or in what order they occur, so a caller must poll for the state it needs
 * rather than infer a sequence.
 */
typedef enum _CCSP_HAL_ETHSW_LINK_STATUS
{
    CCSP_HAL_ETHSW_LINK_Up = 0,         /*!< A link is established and the port can carry traffic. */
    CCSP_HAL_ETHSW_LINK_Down,           /*!< No link is established although a peer may be attached. */
    CCSP_HAL_ETHSW_LINK_Disconnected   /*!< Nothing is attached to the port. */
} CCSP_HAL_ETHSW_LINK_STATUS, *PCCSP_HAL_ETHSW_LINK_STATUS; /*!< Pointer form of ::CCSP_HAL_ETHSW_LINK_STATUS; this is the type of the `pStatus` `[out]` parameter of `CcspHalEthSwGetPortStatus()`. */

/**
 * @brief Administrative state of an Ethernet switch port.
 *
 * The administrative state is the state an operator has requested, which is
 * independent of whether a link is actually up: a port may be administratively
 * up with ::CCSP_HAL_ETHSW_LINK_Down reported by `CcspHalEthSwGetPortStatus()`.
 * Read with `CcspHalEthSwGetPortAdminStatus()` and set with
 * `CcspHalEthSwSetPortAdminStatus()`.
 */
typedef enum _CCSP_HAL_ETHSW_ADMIN_STATUS {
    CCSP_HAL_ETHSW_AdminUp = 0,   /*!< The port is administratively enabled and permitted to establish a link. */
    CCSP_HAL_ETHSW_AdminDown,     /*!< The port is administratively disabled and will not establish a link. */
    CCSP_HAL_ETHSW_AdminTest      /*!< The port is in a vendor-defined test mode. This interface does not specify the behaviour of a port in this state, so a caller must not assume it forwards traffic. */
} CCSP_HAL_ETHSW_ADMIN_STATUS, *PCCSP_HAL_ETHSW_ADMIN_STATUS; /*!< Pointer form of ::CCSP_HAL_ETHSW_ADMIN_STATUS; this is the type of the `pAdminStatus` `[out]` parameter of `CcspHalEthSwGetPortAdminStatus()`. */

/**********************************************************************
                STRUCTURE DEFINITIONS
**********************************************************************/

/**
 * @brief Traffic and error counters for one Ethernet switch port.
 *
 * Filled in by `CcspHalEthSwGetEthPortStats()` into storage the caller owns.
 *
 * @note This interface does not specify the unit of any counter beyond the
 * byte-or-packet distinction the member names carry, nor whether a counter is a
 * total since the port last started or a value over some shorter window, nor how
 * it wraps at the width of its type. It defines no way to reset a counter. A
 * caller that needs a rate must therefore take two samples and divide by its own
 * elapsed time, and must tolerate a counter that wraps or restarts.
 */
typedef struct _CCSP_HAL_ETH_STATS {
    ULLONG BytesSent;           /*!< Bytes transmitted on the port. */
    ULLONG BytesReceived;        /*!< Bytes received on the port. */
    ULONG PacketsSent;          /*!< Packets transmitted on the port. */
    ULONG PacketsReceived;       /*!< Packets received on the port. */
    ULONG ErrorsSent;           /*!< Outbound packets not transmitted because of an error. */
    ULONG ErrorsReceived;        /*!< Inbound packets that contained an error. */
    ULONG UnicastPacketsSent;   /*!< Packets transmitted to a single destination address. */
    ULONG UnicastPacketsReceived; /*!< Packets received that were addressed to a single destination. */
    ULONG DiscardPacketsSent;    /*!< Outbound packets discarded although no error was detected, for example because a queue was full. */
    ULONG DiscardPacketsReceived; /*!< Inbound packets discarded although no error was detected. */
    ULONG MulticastPacketsSent;  /*!< Packets transmitted to a multicast address. */
    ULONG MulticastPacketsReceived; /*!< Packets received that were addressed to a multicast address. */
    ULONG BroadcastPacketsSent;   /*!< Packets transmitted to the broadcast address. */
    ULONG BroadcastPacketsReceived; /*!< Packets received that were addressed to the broadcast address. */
    ULONG UnknownProtoPacketsReceived; /*!< Packets received carrying a protocol the port does not recognise. */
} CCSP_HAL_ETH_STATS,*PCCSP_HAL_ETH_STATS; /*!< Pointer form of ::CCSP_HAL_ETH_STATS; this is the type of the `pStats` `[out]` parameter of `CcspHalEthSwGetEthPortStats()`. */

/**
 * @}
 */



/**
 * @addtogroup ETHSW_HAL_APIS
 * @brief Functions a caller invokes to query and control the Ethernet switch.
 *
 * Four conventions hold for every function in this group, and are stated here
 * once rather than repeated in full on each declaration.
 *
 * - Ordering. `CcspHalEthSwInit()` is called once during bootup and must
 *   precede every other function of this interface.
 *   [the HAL specification, "Initialization and Startup"]
 * - Result. A status-returning function reports success as `RETURN_OK` and any
 *   failure as `RETURN_ERR`, synchronously, as its return value. Because that is
 *   the interface's only error value, the cause of a failure cannot be recovered
 *   from the return value; a caller distinguishes an argument mistake from a
 *   hardware fault by validating its own arguments first and otherwise consults
 *   `ethsw_vendor_hal.log`.
 *   [the HAL specification, "Internal Error Handling" and
 *   "Logging and debugging requirements"]
 * - Concurrency. This interface is not thread safe, so a caller serialises its
 *   own calls; separate processes may call it concurrently, and the vendor
 *   implementation is required to protect itself against that.
 *   [the HAL specification, "Threading Model" and "Process Model"]
 * - Memory. Storage exchanged across this interface is allocated and released by
 *   the caller, except where a declaration states otherwise;
 *   `CcspHalExtSw_getAssociatedDevice()` is the one such exception.
 *   [the HAL specification, "Memory Model"]
 *
 * @{
 */

/**********************************************************************************
 *
 *  Subsystem level function prototypes
 *
**********************************************************************************/


/**
 * @brief Prepares the Ethernet switch and the HAL for use.
 *
 * Sets up the data structures, threads and hardware access that every other
 * function of this interface depends on.
 *
 * @pre A caller invokes this once during bootup and before any other function of
 * this interface, so there is nothing of this interface to call first and no
 * argument to prepare. What the interface does not define bounds that ordering:
 * there is no teardown counterpart, and the effect of a second call is not
 * defined, so a caller neither releases what this call set up nor repeats the
 * call to recover from anything.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` the data structures, threads and hardware access named
 * above are in place, and every other function of this interface may be called.
 * On `RETURN_ERR` this interface does not define how much of that setup was
 * done, so a caller treats none of it as available and calls no other function
 * of this interface.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The HAL is initialised and the rest of this interface may
 * now be called.
 * @retval RETURN_ERR - Initialization did not complete. This interface defines
 * one failure value and does not enumerate the conditions that produce it, so
 * the value reports that the call did not succeed and never why; this function
 * takes no argument, so there is nothing in the declaration a caller can
 * re-check on the strength of the value.
 *
 * @note Error handling: the value does not say what failed, so recovery is
 * limited to retrying the call and reporting the failure; until one succeeds,
 * the post-condition above governs what a caller may call.
 * @note Blocking: this is the one function of this interface that is documented
 * as able to block, and it may do so while the switch hardware is not yet ready.
 * A caller must not invoke it from a context that cannot tolerate a delay.
 * [the HAL specification, "Initialization and Startup"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT CcspHalEthSwInit(void); 

/**
 * @brief Reads the live link rate, duplex mode and link state of one switch
 * port.
 *
 * Reports what the port has actually negotiated, which is not necessarily what
 * it was configured for: use `CcspHalEthSwGetPortCfg()` for the configured
 * values.
 *
 * @param[in] PortId - Port to query. Valid values are the enumerators of
 * ::CCSP_HAL_ETHSW_PORT except ::CCSP_HAL_ETHSW_PortMax, which is a sentinel.
 * Whether a given enumerator is implemented depends on the product.
 * @param[out] pLinkRate - Caller-allocated ::CCSP_HAL_ETHSW_LINK_RATE the
 * negotiated rate is written to; ::CCSP_HAL_ETHSW_LINK_NULL when the port has no
 * link. Must not be NULL.
 * @param[out] pDuplexMode - Caller-allocated ::CCSP_HAL_ETHSW_DUPLEX_MODE the
 * negotiated duplex mode is written to. Must not be NULL.
 * @param[out] pStatus - Caller-allocated ::CCSP_HAL_ETHSW_LINK_STATUS the link
 * state is written to. Must not be NULL.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` all three outputs have been written. On `RETURN_ERR` this
 * interface does not define whether any of them was written, so a caller treats
 * all three as unset.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The three values were read and stored.
 * @retval RETURN_ERR - The port status was not reported. This interface
 * defines one failure value and does not enumerate the conditions that produce
 * it, so the value reports that the call did not succeed and never why; the
 * conditions this call requires are stated on its parameters and preconditions
 * above, and a caller cannot tell from the value which of them was not met.
 *
 * @note Error handling: a caller re-checks that `PortId` is an implemented port
 * and that no pointer is NULL, then retries or reports; the return value does not
 * identify the cause.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT CcspHalEthSwGetPortStatus (
    CCSP_HAL_ETHSW_PORT PortId, 
    PCCSP_HAL_ETHSW_LINK_RATE pLinkRate, 
    PCCSP_HAL_ETHSW_DUPLEX_MODE pDuplexMode, 
    PCCSP_HAL_ETHSW_LINK_STATUS pStatus
);

/**
 * @brief Reads the configured link rate and duplex mode of one switch port.
 *
 * Reports the values the port is set to, which may be the auto-negotiation
 * enumerators ::CCSP_HAL_ETHSW_LINK_Auto and ::CCSP_HAL_ETHSW_DUPLEX_Auto even
 * where `CcspHalEthSwGetPortStatus()` reports a concrete negotiated result.
 *
 * @param[in]  PortId - Port to query. Valid values are the enumerators of
 * ::CCSP_HAL_ETHSW_PORT except ::CCSP_HAL_ETHSW_PortMax.
 * @param[out] pLinkRate - Caller-allocated ::CCSP_HAL_ETHSW_LINK_RATE the
 * configured rate is written to. Must not be NULL.
 * @param[out] pDuplexMode - Caller-allocated ::CCSP_HAL_ETHSW_DUPLEX_MODE the
 * configured duplex mode is written to. Must not be NULL.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` both outputs have been written. On `RETURN_ERR` neither is
 * defined and a caller treats both as unset.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - Both values were read and stored.
 * @retval RETURN_ERR - The port configuration was not reported. This interface
 * defines one failure value and does not enumerate the conditions that produce
 * it, so the value reports that the call did not succeed and never why; the
 * conditions this call requires are stated on its parameters and preconditions
 * above, and a caller cannot tell from the value which of them was not met.
 *
 * @note Error handling: as for `CcspHalEthSwGetPortStatus()` - re-check the
 * arguments, then retry or report.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT CcspHalEthSwGetPortCfg (
    CCSP_HAL_ETHSW_PORT PortId, 
    PCCSP_HAL_ETHSW_LINK_RATE pLinkRate, 
    PCCSP_HAL_ETHSW_DUPLEX_MODE pDuplexMode
);

/**
 * @brief Sets the link rate and duplex mode of one switch port.
 *
 * Both values are applied together; this interface offers no way to change one
 * and leave the other untouched, so a caller that wants to preserve one reads
 * the current pair with `CcspHalEthSwGetPortCfg()` first and passes it back.
 * Applying a configuration may drop and re-establish the port's link.
 *
 * @param[in] PortId - Port to configure. Valid values are the enumerators of
 * ::CCSP_HAL_ETHSW_PORT except ::CCSP_HAL_ETHSW_PortMax.
 * @param[in] LinkRate - Rate to configure; any enumerator of
 * ::CCSP_HAL_ETHSW_LINK_RATE, subject to what the port supports.
 * ::CCSP_HAL_ETHSW_LINK_NULL is a reported state rather than a rate to set.
 * @param[in] DuplexMode - Duplex mode to configure; any enumerator of
 * ::CCSP_HAL_ETHSW_DUPLEX_MODE.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` the port is configured for the requested rate and duplex
 * mode. The link itself may still be down, and
 * `CcspHalEthSwGetPortStatus()` may report a different negotiated result. On
 * `RETURN_ERR` this interface does not define whether the port kept its previous
 * configuration, so a caller reads it back before relying on it.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The configuration was accepted and applied.
 * @retval RETURN_ERR - The call did not succeed. This interface defines one
 * failure value and does not enumerate the conditions that produce it, so the
 * value reports that the call did not succeed and never why, and it establishes
 * nothing about whether any part of the requested change took effect: a caller
 * that needs to know reads the pair back with `CcspHalEthSwGetPortCfg()`. The
 * conditions this call requires are stated on its parameters and preconditions
 * above, and a caller cannot tell from the value which of them was not met.
 *
 * @note Error handling: a caller re-reads the configuration with
 * `CcspHalEthSwGetPortCfg()` to learn the state it is actually in, and falls back
 * to ::CCSP_HAL_ETHSW_LINK_Auto and ::CCSP_HAL_ETHSW_DUPLEX_Auto if a specific
 * combination is refused.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT CcspHalEthSwSetPortCfg(
    CCSP_HAL_ETHSW_PORT PortId,
    CCSP_HAL_ETHSW_LINK_RATE LinkRate,
    CCSP_HAL_ETHSW_DUPLEX_MODE DuplexMode
);

/**
 * @brief Reads whether a switch port is administratively enabled.
 *
 * Reports the state an operator has requested, which is independent of whether
 * a link is up; `CcspHalEthSwGetPortStatus()` reports the latter.
 *
 * @param[in] PortId - Port to query. Valid values are the enumerators of
 * ::CCSP_HAL_ETHSW_PORT except ::CCSP_HAL_ETHSW_PortMax.
 * @param[out] pAdminStatus - Caller-allocated ::CCSP_HAL_ETHSW_ADMIN_STATUS the
 * administrative state is written to. Must not be NULL.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` `*pAdminStatus` has been written; on `RETURN_ERR` it is
 * undefined and a caller treats it as unset.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The administrative state was read and stored.
 * @retval RETURN_ERR - The administrative status was not reported. This
 * interface defines one failure value and does not enumerate the conditions
 * that produce it, so the value reports that the call did not succeed and
 * never why; the conditions this call requires are stated on its parameters
 * and preconditions above, and a caller cannot tell from the value which of
 * them was not met.
 *
 * @note Error handling: re-check the arguments, then retry or report; the value
 * does not identify the cause.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT CcspHalEthSwGetPortAdminStatus (
    CCSP_HAL_ETHSW_PORT PortId, 
    PCCSP_HAL_ETHSW_ADMIN_STATUS pAdminStatus
);

/**
 * @brief Administratively enables, disables or test-mode-switches one port.
 *
 * Disabling a port takes its link down and stops it forwarding traffic;
 * enabling it permits a link to be established again.
 *
 * @param[in] PortId - Port to change. Valid values are the enumerators of
 * ::CCSP_HAL_ETHSW_PORT except ::CCSP_HAL_ETHSW_PortMax.
 * @param[in] AdminStatus - State to apply: ::CCSP_HAL_ETHSW_AdminUp,
 * ::CCSP_HAL_ETHSW_AdminDown or ::CCSP_HAL_ETHSW_AdminTest. Pass the enumerator
 * itself rather than a numeric literal, because the enumeration's ordering is
 * `AdminUp` first, which is the reverse of the up-is-one convention a caller
 * might otherwise assume.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` the port is in the requested administrative state, and a
 * subsequent `CcspHalEthSwGetPortAdminStatus()` reports it. On `RETURN_ERR` this
 * interface does not define whether the previous state was retained.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The state was applied.
 * @retval RETURN_ERR - The call did not succeed. This interface defines one
 * failure value and does not enumerate the conditions that produce it, so the
 * value reports that the call did not succeed and never why, and it establishes
 * nothing about whether the port's administrative state changed: a caller that
 * needs to know reads it back with `CcspHalEthSwGetPortAdminStatus()`. The
 * conditions this call requires are stated on its parameters and preconditions
 * above, and a caller cannot tell from the value which of them was not met.
 *
 * @note Error handling: a caller reads the state back with
 * `CcspHalEthSwGetPortAdminStatus()` rather than assuming the port is unchanged.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT CcspHalEthSwSetPortAdminStatus(
    CCSP_HAL_ETHSW_PORT PortId,
    CCSP_HAL_ETHSW_ADMIN_STATUS AdminStatus
); 

/**
 * @brief Sets how quickly a port ages MAC addresses out of the forwarding
 * table.
 *
 * The aging speed governs how long a learned MAC address survives in the
 * switch's forwarding table without further traffic from it. A shorter lifetime
 * makes the table follow a moving client sooner; a longer one reduces flooding.
 *
 * @param[in] PortId - Port to configure. Valid values are the enumerators of
 * ::CCSP_HAL_ETHSW_PORT except ::CCSP_HAL_ETHSW_PortMax.
 * @param[in] AgingSpeed - Aging speed to apply. This interface does not specify
 * the unit, the valid range or the meaning of zero or a negative value: the
 * quantity is vendor-defined, so a caller must obtain an acceptable value from
 * the vendor implementation rather than assume seconds, and must handle the call
 * being rejected.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` the port uses the requested aging speed. This interface
 * provides no way to read the value back, so the applied setting cannot be
 * confirmed through it.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The aging speed was accepted and applied.
 * @retval RETURN_ERR - The call did not succeed. This interface defines one
 * failure value and does not enumerate the conditions that produce it, so the
 * value reports that the call did not succeed and never why, and it establishes
 * nothing about whether the port's aging speed changed. This interface declares
 * no call that reports the configured aging speed, so the setting cannot be read
 * back through it either, and a caller treats the port's aging speed as unknown
 * after a failure. The conditions this call requires are stated on its
 * parameters and preconditions above, and a caller cannot tell from the value
 * which of them was not met.
 *
 * @note Error handling: because the accepted range is not part of this
 * interface, a caller cannot tell from the return value whether the speed it
 * passed was refused or the change failed for some other reason; it reports
 * the failure and does not retry the same value.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT CcspHalEthSwSetAgingSpeed(CCSP_HAL_ETHSW_PORT PortId, INT AgingSpeed); 

/**
 * @brief Finds which port a given MAC address was learned on.
 *
 * Searches the switch's MoCA and Ethernet MAC address tables for the supplied
 * address and reports the port it is associated with. Only an address the switch
 * has learned can be found, so an idle or newly attached client may be absent.
 *
 * @param[in] mac - Pointer to exactly 6 bytes holding the MAC address as binary
 * octets, not as a printable string. The callee reads the buffer and does not
 * modify it; this interface does not state whether the callee retains the
 * pointer, so a caller keeps the buffer valid for the duration of the call and
 * must not assume it may be freed earlier. The interface does not specify the
 * octet order, so a caller must confirm it against the vendor implementation
 * rather than assume a byte reversal either way. Must not be NULL.
 * @param[out] port - Caller-allocated `INT` the port number is written to. Must
 * not be NULL.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` `*port` holds the port the address was found on; on
 * `RETURN_ERR` it is undefined and a caller treats it as unset.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The address was found and the port number was stored.
 * @retval RETURN_ERR - No port was reported for the address. This interface
 * defines one failure value and does not enumerate the conditions that produce
 * it, so the value reports that the call did not succeed and never why; the
 * conditions this call requires are stated on its parameters and preconditions
 * above, and a caller cannot tell from the value which of them was not met.
 *
 * @warning The number written to `*port` is not a ::CCSP_HAL_ETHSW_PORT
 * enumerator. This interface does not define the numbering it uses, does not
 * define which value denotes a MoCA rather than an Ethernet port, and defines no
 * conversion to ::CCSP_HAL_ETHSW_PORT. A caller must therefore not pass the
 * value to a port-scoped function of this interface, and must establish its
 * meaning with the vendor implementation before acting on it.
 *
 * @note Error handling: `RETURN_ERR` does not identify the cause, and in
 * particular does not tell a caller whether the address is simply absent from
 * the switch's tables, so a caller that needs that distinction re-reads the
 * device list with `CcspHalExtSw_getAssociatedDevice()` instead of inferring it
 * here.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT CcspHalEthSwLocatePortByMacAddress(unsigned char *mac, INT *port);

/**
 * @}
 */


/**
 * @addtogroup ETHSW_HAL_TYPES
 * @{
 */

/**
 * @brief One device observed on an Ethernet switch port.
 *
 * Returned in the array `CcspHalExtSw_getAssociatedDevice()` produces, and
 * passed by pointer to a ::CcspHalExtSw_ethAssociatedDevice_callback when a
 * device associates or disassociates. Every member is an observation reported by
 * the implementation; writing to a structure a caller has received changes
 * nothing in the switch, because this interface defines no function that accepts
 * one.
 */
typedef struct _eth_device {
    UCHAR eth_devMacAddress[6];  /*!< MAC address of the device as 6 binary octets, not a printable string. The interface does not specify the octet order. */
    INT eth_port;               /*!< External port the device is attached to. This is not a ::CCSP_HAL_ETHSW_PORT enumerator: this interface defines neither the numbering base nor the upper bound of this index, and defines no conversion to ::CCSP_HAL_ETHSW_PORT, so a caller must not pass it to a port-scoped function of this interface. */
    INT eth_vlanid;             /*!< VLAN identifier the port is tagged with, in the range 1 to 4094. Reported for information only; this interface declares no function that creates, deletes or modifies a VLAN. */
    INT eth_devTxRate;          /*!< Transmit rate observed for the device. The unit is not specified by this interface and is vendor-defined, so a caller must not assume Mbit/s or any other scale, and must not compare the value across implementations. */
    INT eth_devRxRate;          /*!< Receive rate observed for the device. The unit is not specified by this interface and is vendor-defined, on the same terms as `eth_devTxRate`. */
    BOOLEAN eth_Active;         /*!< `TRUE` while the device is present on the port, `FALSE` once it is no longer seen. In a callback delivery this member is what distinguishes an association from a disassociation. */
} eth_device_t;

/**
 * @}
 */


/**
 * @addtogroup ETHSW_HAL_APIS
 * @{
 */

/**
 * @brief Enumerates the devices currently seen on the switch's Ethernet ports.
 *
 * Produces a snapshot: an array of ::eth_device_t and its element count. The
 * contents are accurate only as of the call, so a caller that needs to track
 * arrivals and departures registers a callback with
 * `CcspHalExtSw_ethAssociatedDevice_callback_register()` rather than polling this
 * function.
 *
 * @param[out] output_array_size - Caller-allocated `ULONG` the number of
 * elements written to the array is stored in. Must not be NULL. A successful call
 * may report zero, which means no device is currently associated.
 * @param[out] output_struct - Caller-allocated pointer variable. On success the
 * implementation stores in it the address of an array of `*output_array_size`
 * ::eth_device_t elements that the implementation itself allocated. Must not be
 * NULL.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` `*output_array_size` holds the element count and
 * `*output_struct` points to an array the caller now owns. On `RETURN_ERR`
 * neither output is defined: a caller must not read either of them and in
 * particular must not release `*output_struct`.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The snapshot was produced and both outputs were written.
 * @retval RETURN_ERR - The snapshot was not produced. This interface defines
 * one failure value and does not enumerate the conditions that produce it, so
 * the value reports that the call did not succeed and never why; the
 * conditions this call requires are stated on its parameters and preconditions
 * above, and a caller cannot tell from the value which of them was not met.
 *
 * @warning Ownership is inverted here relative to the rest of this interface.
 * The implementation allocates the array and the caller is responsible for
 * releasing it once it has finished with it, otherwise every call leaks.
 * This interface does not state which allocator was used and provides no release
 * function of its own, so the matching deallocator is not established by this
 * contract and a caller must confirm it with the vendor implementation before
 * freeing. [the HAL specification, "Memory Model" and
 * "Module Responsibilities"]
 *
 * @note Error handling: on `RETURN_ERR` a caller retries or reports, and must not
 * attempt to release memory it was never given.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls. The
 * returned array is private to the caller that received it, so no locking is
 * needed to read it afterwards. [the HAL specification, "Threading Model"]
 */
INT CcspHalExtSw_getAssociatedDevice(ULONG *output_array_size, eth_device_t **output_struct);
/** @} */
/** @addtogroup ETHSW_HAL_TYPES
 *  @{ */
/**
 * @brief Notification a caller implements to learn that an Ethernet device
 * associated with or disassociated from a switch port.
 *
 * The implementation invokes the function a caller installed with
 * `CcspHalExtSw_ethAssociatedDevice_callback_register()` each time a device
 * appears on or leaves a port; `eth_dev->eth_Active` distinguishes the two
 * cases. A caller implements this function; it does not call it.
 *
 * @param[in] eth_dev - Details of the device the event concerns. The pointer and
 * the structure it addresses belong to the implementation and are valid only for
 * the duration of the call, so an implementation of this callback must copy any
 * field it needs to keep rather than store the pointer, and must not release it.
 *
 * @returns Status the callback reports back to the implementation.
 * @retval RETURN_OK - The event was processed.
 * @retval RETURN_ERR - The callback did not accept the event. This interface
 * defines one such value, does not enumerate the conditions in which a
 * callback returns it, and does not state what the implementation does with
 * it, so it reports only that the event was not accepted.
 *
 * @warning This interface does not specify the thread or process context in
 * which the callback runs, nor whether deliveries are serialised, nor whether
 * calling back into this HAL from inside it is permitted. An implementation must
 * therefore protect its own state, must not assume it runs on the registering
 * thread, and should return promptly, deferring real work rather than blocking.
 * @see CcspHalExtSw_ethAssociatedDevice_callback_register
 */
typedef INT (*CcspHalExtSw_ethAssociatedDevice_callback)(eth_device_t *eth_dev);
/** @} */
/** @addtogroup ETHSW_HAL_APIS
 *  @{ */
/**
 * @brief Installs the callback that reports Ethernet device association and
 * disassociation.
 *
 * This is the interface's asynchronous notification path: once a callback is
 * installed, the implementation reports device arrivals and departures as they
 * happen instead of the caller polling `CcspHalExtSw_getAssociatedDevice()`.
 *
 * @param[in] callback_proc - Function to install, of type
 * ::CcspHalExtSw_ethAssociatedDevice_callback. It must remain callable for as
 * long as notifications are wanted, so it must not be a function whose
 * containing module may be unloaded. This interface defines no way to remove a
 * previously installed callback and does not state whether a second call
 * replaces the first or adds to it, so a caller registers once. It also does not
 * state whether passing NULL is a valid way to disable delivery, so a caller must
 * not rely on that.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post Association and disassociation events are delivered to `callback_proc`.
 *
 * @execution callback
 *
 * @warning This function reports nothing: it returns no value, so a caller
 * cannot tell from it whether registration succeeded. Confirmation is only
 * available indirectly, by observing that notifications arrive.
 *
 * @note Blocking: must complete synchronously without blocking the calling
 * thread; it installs a pointer rather than waiting for an event.
 * [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls, and in
 * particular must not register from one thread while another is calling this
 * interface. [the HAL specification, "Threading Model"]
 *
 * @see CcspHalExtSw_ethAssociatedDevice_callback
 */
void CcspHalExtSw_ethAssociatedDevice_callback_register(CcspHalExtSw_ethAssociatedDevice_callback callback_proc);

#ifdef FEATURE_RDKB_WAN_MANAGER
#ifdef FEATURE_RDKB_AUTO_PORT_SWITCH

/**
 * @brief Puts one named Ethernet interface into or out of WAN mode.
 *
 * Where `CcspHalExtSw_setEthWanPort()` selects the WAN port by index, this
 * function acts on an interface by name, which is how the automatic port-switch
 * feature moves the WAN role between interfaces.
 *
 * @param[in] ifname - Name of the interface to configure, for example "eth0".
 * The parameter is a bare `char *` with no length beside it, and this interface
 * states no representation for it: it does not establish that the name is
 * NUL-terminated, or that the callee reads a terminator at all. A caller
 * supplies a conventional C string, because that is the only form a callee has
 * any declared means of reading, and treats doing so as its own obligation
 * rather than as a property of this interface. The parameter is declared
 * `char *` rather than `const char *`, so this interface does not establish that
 * the callee leaves the buffer unmodified either, and it does not state whether
 * the callee retains the pointer, so a caller keeps the buffer valid for the
 * duration of the call and must not pass a pointer to storage it is about to
 * release. The interface specifies neither a maximum length nor the set of
 * acceptable names. Must not be NULL.
 * @param[in] WanMode - `TRUE` to make the interface a WAN interface, `FALSE` to
 * return it to LAN use.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` the named interface is in the requested mode. Changing the
 * mode of an interface that is carrying traffic interrupts that traffic.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The interface was reconfigured.
 * @retval RETURN_ERR - The call did not succeed. This interface defines one
 * failure value and does not enumerate the conditions that produce it, so the
 * value reports that the call did not succeed and never why, and it establishes
 * nothing about whether the interface was left in its previous mode, moved to
 * the requested one or partially reconfigured: a caller that needs to know reads
 * the WAN state back with `CcspHalExtSw_getEthWanEnable()` and
 * `CcspHalExtSw_getEthWanPort()`. The conditions this call requires are stated
 * on its parameters and preconditions above, and a caller cannot tell from the
 * value which of them was not met.
 *
 * @warning This declaration is compiled only when both `FEATURE_RDKB_WAN_MANAGER`
 * and `FEATURE_RDKB_AUTO_PORT_SWITCH` are defined, so it is absent from a build
 * that lacks either. Code calling it must be guarded by the same two macros; the
 * interface offers no runtime way to discover whether it is present, and no
 * "not supported" result, so an unguarded call is a build failure rather than a
 * handled error.
 *
 * @note Error handling: a caller reports the failure and leaves the WAN
 * assignment as it was; the return value does not say whether the interface was
 * partially reconfigured, so a caller re-reads the WAN state with
 * `CcspHalExtSw_getEthWanEnable()` and `CcspHalExtSw_getEthWanPort()`.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
int CcspHalExtSw_ethPortConfigure(char *ifname, BOOLEAN WanMode);

#endif // FEATURE_RDKB_AUTO_PORT_SWITCH
#endif // FEATURE_RDKB_WAN_MANAGER

/**
 * @brief Reads whether the Ethernet WAN feature is enabled.
 *
 * Reports the feature's administrative setting. It says nothing about whether
 * the WAN link is up; `GWP_GetEthWanLinkStatus()` reports that.
 *
 * @param[out] pFlag - Caller-allocated `BOOLEAN` the setting is written to:
 * `TRUE` if Ethernet WAN is enabled, `FALSE` if it is disabled. Must not be
 * NULL.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` `*pFlag` has been written; on `RETURN_ERR` it is undefined
 * and a caller treats it as unset rather than as `FALSE`.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The setting was read and stored.
 * @retval RETURN_ERR - The setting was not reported. This interface defines
 * one failure value and does not enumerate the conditions that produce it, so
 * the value reports that the call did not succeed and never why; the
 * conditions this call requires are stated on its parameters and preconditions
 * above, and a caller cannot tell from the value which of them was not met.
 *
 * @note Error handling: a caller must not treat `RETURN_ERR` as "disabled",
 * because the two are different states; it retries or reports instead.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT CcspHalExtSw_getEthWanEnable(BOOLEAN *pFlag);

/**
 * @brief Enables or disables the Ethernet WAN feature.
 *
 * Enabling makes the selected Ethernet port the WAN interface; disabling
 * returns it to LAN use. The port acted on is the one
 * `CcspHalExtSw_getEthWanPort()` reports, so a caller that wants a specific port
 * sets it with `CcspHalExtSw_setEthWanPort()` first.
 *
 * @param[in] Flag - `TRUE` to enable Ethernet WAN, `FALSE` to disable it. Pass
 * `TRUE` or `FALSE`; this interface does not define the meaning of any other
 * value.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` the feature is in the requested state and
 * `CcspHalExtSw_getEthWanEnable()` reports it. Changing the state interrupts
 * traffic on the affected port. This interface does not state whether the setting
 * survives a restart, so a caller re-applies it after re-initialisation rather
 * than assuming it persisted. [the HAL specification, "Persistence Model"]
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The requested state was applied.
 * @retval RETURN_ERR - The call did not succeed. This interface defines one
 * failure value and does not enumerate the conditions that produce it, so the
 * value reports that the call did not succeed and never why, and it establishes
 * nothing about whether the Ethernet WAN feature changed state: a caller that
 * needs to know reads it back with `CcspHalExtSw_getEthWanEnable()`. The
 * conditions this call requires are stated on its parameters and preconditions
 * above, and a caller cannot tell from the value which of them was not met.
 *
 * @note Error handling: a caller reads the state back with
 * `CcspHalExtSw_getEthWanEnable()` rather than assuming the previous state
 * survived, then reports the failure.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT CcspHalExtSw_setEthWanEnable(BOOLEAN Flag);

#ifdef FEATURE_RDKB_AUTO_PORT_SWITCH
/**
 * @brief Reports whether the hardware is currently wired for WAN or for LAN
 * use.
 *
 * Answers which of the two hardware configurations the automatic port-switch
 * feature has the device in. It is a hardware-level view, distinct from the
 * Ethernet WAN feature setting that `CcspHalExtSw_getEthWanEnable()` reports.
 *
 * @return `TRUE` if the hardware is configured for WAN, `FALSE` if it is
 * configured for LAN. These are the only two values this function reports.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post The hardware configuration is as the call found it: this is a query, it
 * stores no setting and it changes nothing else in this interface. The returned
 * value is the call's only output - there is no output parameter and no status
 * value - so on `TRUE` a caller holds the hardware's WAN configuration as a
 * fact, and on `FALSE` it holds only the value, for the reason the warning below
 * gives.
 *
 * @warning This function has no error channel. Its signature admits no status
 * value and no output parameter, so a failed query is indistinguishable from a
 * successful query that found the LAN configuration: both surface as `FALSE`. A
 * caller that must not act on a false negative has no way to detect one through
 * this interface, and this interface defines no alternative convention for
 * signalling failure here.
 *
 * @warning This declaration is compiled only when `FEATURE_RDKB_AUTO_PORT_SWITCH`
 * is defined, so it is absent from a build without that macro. Code calling it
 * must be guarded by the same macro; the interface provides no runtime presence
 * check and no "not supported" result.
 *
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
BOOLEAN CcspHalExtSw_getCurrentWanHWConf();
#endif

/**
 * @brief Reads which Ethernet port is selected for WAN use.
 *
 * Reports the port index the Ethernet WAN feature is bound to, whether or not
 * the feature is currently enabled.
 *
 * @param[out] pPort - Caller-allocated `UINT` the port index is written to. Must
 * not be NULL. The index is the 0-based Ethernet WAN numbering that
 * `ETHWAN_DEF_INTF_NUM` uses, not a ::CCSP_HAL_ETHSW_PORT enumerator. This
 * interface does not define the upper bound of the index - no symbol here states
 * how many external Ethernet ports a product has - so a caller must not derive
 * one from ::CCSP_HAL_ETHSW_PORT and must obtain the port count from the
 * platform instead.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` `*pPort` has been written; on `RETURN_ERR` it is undefined
 * and a caller treats it as unset rather than as port 0.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The port index was read and stored.
 * @retval RETURN_ERR - The port number was not reported. This interface
 * defines one failure value and does not enumerate the conditions that produce
 * it, so the value reports that the call did not succeed and never why; the
 * conditions this call requires are stated on its parameters and preconditions
 * above, and a caller cannot tell from the value which of them was not met.
 *
 * @note Error handling: a caller must not fall back to `ETHWAN_DEF_INTF_NUM` on
 * failure and present it as the live selection, because the default and the
 * current selection are different facts; it retries or reports instead.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT CcspHalExtSw_getEthWanPort(UINT *pPort);

/**
 * @brief Selects which Ethernet port is used for WAN.
 *
 * Binds the Ethernet WAN feature to a physical port. Whether WAN is actually
 * active on it is governed separately by `CcspHalExtSw_setEthWanEnable()`.
 *
 * @param[in] Port - Port index to select, in the 0-based Ethernet WAN numbering
 * that `ETHWAN_DEF_INTF_NUM` uses. It is not a ::CCSP_HAL_ETHSW_PORT enumerator,
 * and passing one is a numbering error that this interface cannot detect for the
 * caller. This interface does not define the upper bound of the index, so a
 * caller obtains the port count from the platform and must handle this function
 * rejecting an index the hardware does not have.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` the feature is bound to `Port` and
 * `CcspHalExtSw_getEthWanPort()` reports it. Moving the WAN role away from a port
 * that is carrying WAN traffic interrupts that traffic. This interface does not
 * state whether the selection survives a restart, so a caller re-applies it after
 * re-initialisation. [the HAL specification, "Persistence Model"]
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The port was selected.
 * @retval RETURN_ERR - The call did not succeed. This interface defines one
 * failure value and does not enumerate the conditions that produce it, so the
 * value reports that the call did not succeed and never why, and it establishes
 * nothing about which port the feature is now bound to: a caller that needs to
 * know reads the selection back with `CcspHalExtSw_getEthWanPort()`. The
 * conditions this call requires are stated on its parameters and preconditions
 * above, and a caller cannot tell from the value which of them was not met.
 *
 * @note Error handling: a caller reads the selection back with
 * `CcspHalExtSw_getEthWanPort()` rather than assuming the previous value
 * survived, and does not retry the same index after a failure.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT CcspHalExtSw_setEthWanPort(UINT Port);

/**
 * @brief Reads the traffic and error counters of one switch port.
 *
 * Fills a caller-supplied ::CCSP_HAL_ETH_STATS with the port's counters.
 *
 * @param[in]  PortId - Port to query. Valid values are the enumerators of
 * ::CCSP_HAL_ETHSW_PORT except ::CCSP_HAL_ETHSW_PortMax.
 * @param[out] pStats - Pointer to a ::CCSP_HAL_ETH_STATS the caller allocated and
 * continues to own; the implementation writes into it. Whether it retains the
 * pointer beyond the call is not specified by this interface, so a caller
 * keeps the structure allocated and unmoved rather than assuming a lifetime
 * that ends with the call. Must not be NULL. Every member is written on
 * success, so the caller need not pre-clear the structure.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` the structure holds the port's counters as of the call. On
 * `RETURN_ERR` this interface does not define whether any member was written, so
 * a caller discards the whole structure rather than reading part of it.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The counters were read and stored.
 * @retval RETURN_ERR - The statistics were not reported. This interface
 * defines one failure value and does not enumerate the conditions that produce
 * it, so the value reports that the call did not succeed and never why; the
 * conditions this call requires are stated on its parameters and preconditions
 * above, and a caller cannot tell from the value which of them was not met.
 *
 * @note The counters carry no unit, window or wrap semantics in this interface
 * (see ::CCSP_HAL_ETH_STATS), and it defines no way to reset them, so a caller
 * computing a rate differences two samples over its own elapsed time and copes
 * with a counter that wraps or restarts.
 * @note Error handling: re-check the arguments, then retry or report; a failed
 * sample is skipped rather than treated as zero, which would otherwise appear as
 * a large negative delta.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT CcspHalEthSwGetEthPortStats(CCSP_HAL_ETHSW_PORT PortId, PCCSP_HAL_ETH_STATS pStats);

/**
 * @}
 */



/**
 * @addtogroup ETHSW_HAL_TYPES
 * @{
 */

/**
 * @brief Notification a caller implements to learn that the Ethernet WAN link
 * has come up.
 *
 * A caller supplies this function in the `pGWP_act_EthWanLinkUP` slot of an
 * ::appCallBack and installs it with `GWP_RegisterEthWan_Callback()`; the
 * provisioning abstraction layer invokes it when that provisioning event occurs.
 * It takes no argument and returns nothing, so the notification carries no detail
 * beyond the fact of the event: a caller that needs the port, the interface name
 * or the current state records that the event arrived and reads them with
 * `CcspHalExtSw_getEthWanPort()`, `GWP_GetEthWanInterfaceName()` or
 * `GWP_GetEthWanLinkStatus()` from its own context, serialised with its other
 * calls to this interface, rather than from inside the callback body.
 *
 * @warning This interface is not thread safe and does not specify the thread or
 * process context the callback runs on, whether deliveries are serialised, or
 * whether re-entering this HAL from inside it is permitted. A body must protect
 * its own state, assume no particular thread, and return without blocking.
 *
 * @see GWP_RegisterEthWan_Callback
 */
typedef void (*fpEthWanLink_Up)(); 

/**
 * @brief Notification a caller implements to learn that the Ethernet WAN link
 * has gone down.
 *
 * The counterpart of ::fpEthWanLink_Up, supplied in the
 * `pGWP_act_EthWanLinkDown` slot of an ::appCallBack and installed by
 * `GWP_RegisterEthWan_Callback()`. It carries no argument and no result, and the
 * same unspecified execution context applies.
 *
 * @warning As for ::fpEthWanLink_Up, this interface does not specify the calling
 * context, the serialisation of deliveries or the legality of re-entering this
 * HAL from inside the callback.
 *
 * @see GWP_RegisterEthWan_Callback
 */
typedef void (*fpEthWanLink_Down)();

/**
 * @brief The pair of Ethernet WAN link callbacks a caller installs.
 *
 * A caller fills in both members and passes the structure's address to
 * `GWP_RegisterEthWan_Callback()`. This interface does not state whether the
 * implementation copies the structure or retains the pointer, so a caller keeps
 * the object alive for as long as notifications are wanted rather than passing a
 * short-lived one. It likewise does not state whether a member may be left NULL
 * to decline one of the two events, so a caller supplies both.
 */
typedef struct __appCallBack {
    fpEthWanLink_Up pGWP_act_EthWanLinkUP; /*!< Function invoked when the Ethernet WAN link comes up. */
    fpEthWanLink_Down pGWP_act_EthWanLinkDown; /*!< Function invoked when the Ethernet WAN link goes down. */
} appCallBack;

/**
 * @}
 */

/**
 * @addtogroup ETHSW_HAL_APIS
 * @{
 */

/**
 * @brief Installs the Ethernet WAN link up and link down callbacks.
 *
 * Together with
 * `CcspHalExtSw_ethAssociatedDevice_callback_register()`, this is the
 * interface's asynchronous notification path: once installed, the provisioning
 * abstraction layer reports Ethernet WAN link transitions as they happen instead
 * of the caller polling `GWP_GetEthWanLinkStatus()`.
 *
 * @param[in] obj - Address of a caller-owned ::appCallBack holding both callback
 * functions. Must not be NULL. The caller keeps ownership and must keep the
 * object valid for as long as notifications are wanted, because this interface
 * does not state whether the implementation copies it. It defines no way to
 * remove a registration and does not state whether a second call replaces the
 * first, so a caller registers once.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post Ethernet WAN link transitions are delivered to the two functions in
 * `obj`.
 *
 * @execution callback
 *
 * @warning This function reports nothing: it returns no value, so a caller
 * cannot tell from it whether registration succeeded, and a NULL or partially
 * populated `obj` is not reported back. Confirmation is only available
 * indirectly, by observing that notifications arrive.
 *
 * @note Blocking: must complete synchronously without blocking the calling
 * thread; it installs pointers rather than waiting for an event.
 * [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls, and in
 * particular must not register from one thread while another is calling this
 * interface. [the HAL specification, "Threading Model"]
 *
 * @see fpEthWanLink_Up
 * @see fpEthWanLink_Down
 */
void GWP_RegisterEthWan_Callback(appCallBack *obj);

/**
 * @brief Reports whether the Ethernet WAN link is currently up.
 *
 * Gives the live link state of the Ethernet WAN interface, as distinct from the
 * feature setting `CcspHalExtSw_getEthWanEnable()` reports: the feature can be
 * enabled while the link is down.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post The link is as the call found it: this is a query, it stores no setting
 * and it changes nothing else in this interface. The returned value is the
 * call's only output - there is no output parameter - so on 1 or 0 the caller
 * holds the link state, and on a negative value it holds none, because the
 * state was not determined.
 *
 * @return 1 when the Ethernet WAN link is up, 0 when it is down, and a negative
 * value when the state could not be determined. Only 1 and 0 are meaningful link
 * states; a caller tests for a negative value first and must not treat it as
 * "down", because a failed query and a down link are different facts. The
 * interface does not define which negative value is used or distinguish causes
 * among them, and it does not state that any global error indicator is set, so a
 * caller must not consult one.
 *
 * @note Error handling: on a negative result a caller reports the failure and
 * retries, and leaves any state that depends on the link unchanged rather than
 * driving it to "down".
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT GWP_GetEthWanLinkStatus();

/**
 * @brief Reads the name of the interface currently used for Ethernet WAN.
 *
 * Writes the interface name into a caller-supplied buffer, so that a caller can
 * address the WAN interface by the name the platform actually uses instead of
 * assuming one.
 *
 * @param[out] Interface - Buffer the caller allocated and continues to own; the
 * implementation writes the name into it. Whether it retains the pointer
 * beyond the call is not specified by this interface, so a caller keeps the
 * buffer allocated and unmoved rather than assuming a lifetime that ends with
 * the call. Must not be NULL. Size it to `ETHWAN_INTERFACE_NAME_MAX_LENGTH`,
 * which is the only bound this interface declares, and pass that same size as
 * `maxSize`. This interface does not state whether the written name is NUL-
 * terminated, so a caller zero-fills the buffer before the call and does not rely
 * on a terminator being added.
 * @param[in] maxSize - Number of bytes available in `Interface`, so that the
 * implementation does not write past its end. Pass the buffer's real size.
 *
 * @pre `CcspHalEthSwInit()` has returned `RETURN_OK`.
 * [the HAL specification, "Initialization and Startup"]
 * @post On `RETURN_OK` `Interface` holds the interface name. On `RETURN_ERR` this
 * interface does not define whether the buffer was partially written, so a
 * caller discards its contents.
 *
 * @returns Status of the operation.
 * @retval RETURN_OK - The name was written into the buffer.
 * @retval RETURN_ERR - The call did not succeed. This interface defines one
 * failure value and does not enumerate the conditions that produce it, so the
 * value reports that the call did not succeed and never why, and it establishes
 * nothing about how much of the buffer was written: a caller discards the
 * buffer's contents rather than reading part of it as a name, as the
 * post-condition above requires. The conditions this call requires are stated on
 * its parameters and preconditions above, and a caller cannot tell from the
 * value which of them was not met.
 *
 * @warning The interface is not self-consistent about the size of this buffer,
 * and a caller must not assume the statements agree.
 * `ETHWAN_INTERFACE_NAME_MAX_LENGTH` declares 32 bytes, while this function has
 * also been described as requiring a buffer of at least 64 bytes and a `maxSize`
 * confined to an 11-to-262-byte range. Nothing in this interface reconciles the
 * three figures, so no vendor-independent minimum or maximum can be derived from
 * them. Sizing to `ETHWAN_INTERFACE_NAME_MAX_LENGTH` and passing that size is the
 * only option grounded in a declaration in this header; a caller with a longer
 * name to accommodate must confirm the real bound with the vendor
 * implementation.
 *
 * @note Error handling: `RETURN_ERR` does not identify the cause, so a caller
 * cannot resize and retry on the strength of the return value alone; it reports
 * the failure and does not treat the buffer as holding a name.
 * @note Blocking: must complete synchronously without blocking the calling
 * thread. [the HAL specification, "Blocking calls"]
 * @note Thread safety: not thread safe; a caller serialises its calls.
 * [the HAL specification, "Threading Model"]
 */
INT GWP_GetEthWanInterfaceName(unsigned char *Interface, ULONG maxSize);

/**
 * @}
 */

#endif /* __CCSP_HAL_ETHSW_H__ */

