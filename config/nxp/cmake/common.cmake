#
# Copyright (c) 2024-2025 Project CHIP Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# @file
# CMake for CHIP library configuration common to NXP platforms
#

include(${CHIP_ROOT}/config/nxp/chip-module/generate_factory_data.cmake)

get_filename_component(COMMON_CMAKE_SOURCE_DIR ${CHIP_ROOT}/config/common/cmake REALPATH)

# Get common Cmake sources
include(${COMMON_CMAKE_SOURCE_DIR}/chip_gn_args.cmake)
include(${COMMON_CMAKE_SOURCE_DIR}/chip_gn.cmake)

# ==============================================================================
# Generate configuration for CHIP GN build system
# ==============================================================================
matter_add_gn_arg_bool("chip_logging" CONFIG_LOG)
matter_add_gn_arg_bool("chip_enable_openthread" CONFIG_NET_L2_OPENTHREAD)
matter_add_gn_arg_bool("chip_openthread_ftd" CONFIG_CHIP_OPENTHREAD_FTD)
matter_add_gn_arg_bool("chip_with_lwip" CONFIG_HAVE_LWIP)
matter_add_gn_arg_bool("chip_config_network_layer_ble" CONFIG_BT)
matter_add_gn_arg_bool("chip_inet_config_enable_ipv4" CONFIG_CHIP_IPV4)
matter_add_gn_arg_bool("chip_persist_subscriptions" CONFIG_CHIP_PERSISTENT_SUBSCRIPTIONS)
matter_add_gn_arg_bool("chip_monolithic_tests" CONFIG_CHIP_BUILD_TESTS)
matter_add_gn_arg_bool("chip_inet_config_enable_tcp_endpoint" CONFIG_CHIP_BUILD_TESTS)
matter_add_gn_arg_bool("chip_error_logging" CONFIG_MATTER_LOG_LEVEL GREATER_EQUAL 1)
matter_add_gn_arg_bool("chip_progress_logging" CONFIG_MATTER_LOG_LEVEL GREATER_EQUAL 3)
matter_add_gn_arg_bool("chip_detail_logging" CONFIG_MATTER_LOG_LEVEL GREATER_EQUAL 4)
matter_add_gn_arg_bool("chip_automation_logging" FALSE)
matter_add_gn_arg_bool("chip_malloc_sys_heap" CONFIG_CHIP_MALLOC_SYS_HEAP)
matter_add_gn_arg_bool("chip_enable_wifi" CONFIG_CHIP_WIFI)
matter_add_gn_arg_bool("chip_enable_ethernet" CONFIG_CHIP_ETHERNET)
matter_add_gn_arg_bool("chip_system_config_provide_statistics" CONFIG_CHIP_STATISTICS)
matter_add_gn_arg_bool("chip_enable_icd_server" CONFIG_CHIP_ENABLE_ICD_SUPPORT)
matter_add_gn_arg_bool("chip_enable_icd_lit" CONFIG_CHIP_ICD_LIT_SUPPORT)
matter_add_gn_arg_bool("chip_enable_ota_requestor" CONFIG_CHIP_OTA_REQUESTOR)
matter_add_gn_arg_bool("chip_system_config_use_openthread_inet_endpoints" CONFIG_CHIP_USE_OT_ENDPOINT)

if(CONFIG_DEBUG)
    matter_add_gn_arg_bool("optimize_debug" true)
    if(CONFIG_CHIP_NXP_PLATFORM_MCXW71)
        matter_add_gn_arg_string("optimize_debug_level" "g")
    else()
        matter_add_gn_arg_string("optimize_debug_level" "0")
    endif()
    matter_add_gn_arg_string("symbol_level" "2")
    matter_add_gn_arg_bool("treat_warnings_as_errors" false)
else()
    matter_add_gn_arg_bool("is_debug" false)
    matter_add_gn_arg_bool("optimize_for_size" true)
endif()

if(CONFIG_CHIP_ROTATING_DEVICE_ID)
    matter_add_gn_arg_bool("chip_enable_rotating_device_id" TRUE)
    matter_add_gn_arg_bool("chip_enable_additional_data_advertising" TRUE)
endif()

if(CONFIG_CHIP_CRYPTO_MBEDTLS)
    matter_add_gn_arg_string("chip_crypto" "mbedtls")
elseif(CONFIG_CHIP_CRYPTO_PLATFORM)
    matter_add_gn_arg_string("chip_crypto" "platform")
elseif(CONFIG_CHIP_CRYPTO_PSA)
    matter_add_gn_arg_string("chip_crypto" "psa")
endif()

if(NOT CONFIG_CHIP_DEBUG_SYMBOLS)
    matter_add_gn_arg_string("symbol_level" "0")
endif()

if(CONFIG_CHIP_STRIP_SYMBOLS)
    matter_add_gn_arg_bool("strip_symbols" true)
endif()

if(CONFIG_CHIP_FACTORY_DATA)
    matter_add_gn_arg_bool("chip_use_transitional_commissionable_data_provider" FALSE)
    matter_add_gn_arg_bool("chip_use_transitional_device_instance_info_provider" FALSE)
endif()
