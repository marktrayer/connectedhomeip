/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <credentials/CHIPCert.h>
#include <crypto/CHIPCryptoPAL.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/Span.h>
#include <platform/NetworkCommissioning.h>

#include <vector>

namespace chip {
namespace DeviceLayer {
namespace NetworkCommissioning {

template <typename T>
class LinuxScanResponseIterator : public Iterator<T>
{
public:
    LinuxScanResponseIterator(std::vector<T> * apScanResponse) : mpScanResponse(apScanResponse) {}
    size_t Count() override { return mpScanResponse != nullptr ? mpScanResponse->size() : 0; }
    bool Next(T & item) override
    {
        if (mpScanResponse == nullptr || currentIterating >= mpScanResponse->size())
        {
            return false;
        }
        item = (*mpScanResponse)[currentIterating];
        currentIterating++;
        return true;
    }
    void Release() override
    { /* nothing to do, we don't hold the ownership of the vector, and users is not expected to hold the
         ownership in OnFinished for scan. */
    }

private:
    size_t currentIterating = 0;
    // Note: We cannot post a event in ScheduleLambda since std::vector is not trivial copyable.
    std::vector<T> * mpScanResponse;
};

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
class LinuxWiFiDriver final : public WiFiDriver
{
public:
    class WiFiNetworkIterator final : public NetworkIterator
    {
    public:
        WiFiNetworkIterator(LinuxWiFiDriver * aDriver) : driver(aDriver) {}
        size_t Count() override;
        bool Next(Network & item) override;
        void Release() override { delete this; }
        ~WiFiNetworkIterator() override = default;

    private:
        LinuxWiFiDriver * driver;
        bool exhausted = false;
    };

    void Set5gSupport(bool is5gSupported) { mIs5gSupported = is5gSupported; }

    // BaseDriver
    NetworkIterator * GetNetworks() override { return new WiFiNetworkIterator(this); }
    CHIP_ERROR Init(BaseDriver::NetworkStatusChangeCallback * networkStatusChangeCallback) override;
    void Shutdown() override;

    // WirelessDriver
    uint8_t GetMaxNetworks() override { return 1; }
    uint8_t GetScanNetworkTimeoutSeconds() override { return 10; }
    uint8_t GetConnectNetworkTimeoutSeconds() override { return 20; }

    CHIP_ERROR CommitConfiguration() override;
    CHIP_ERROR RevertConfiguration() override;

    Status RemoveNetwork(ByteSpan networkId, MutableCharSpan & outDebugText, uint8_t & outNetworkIndex) override;
    Status ReorderNetwork(ByteSpan networkId, uint8_t index, MutableCharSpan & outDebugText) override;
    void ConnectNetwork(ByteSpan networkId, ConnectCallback * callback) override;

    // WiFiDriver
    Status AddOrUpdateNetwork(ByteSpan ssid, ByteSpan credentials, MutableCharSpan & outDebugText,
                              uint8_t & outNetworkIndex) override;
    void ScanNetworks(ByteSpan ssid, ScanCallback * callback) override;

    uint32_t GetSupportedWiFiBandsMask() const override
    {
        uint32_t supportedBands = static_cast<uint32_t>(1UL << chip::to_underlying(WiFiBandEnum::k2g4));
        if (mIs5gSupported)
        {
            supportedBands |= static_cast<uint32_t>(1UL << chip::to_underlying(WiFiBandEnum::k5g));
        }
        return supportedBands;
    }

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI_PDC
    bool SupportsPerDeviceCredentials() override { return true; };
    CHIP_ERROR AddOrUpdateNetworkWithPDC(ByteSpan ssid, ByteSpan networkIdentity, Optional<uint8_t> clientIdentityNetworkIndex,
                                         Status & outStatus, MutableCharSpan & outDebugText, MutableByteSpan & outClientIdentity,
                                         uint8_t & outNetworkIndex) override;
    CHIP_ERROR GetNetworkIdentity(uint8_t networkIndex, MutableByteSpan & outNetworkIdentity) override;
    CHIP_ERROR GetClientIdentity(uint8_t networkIndex, MutableByteSpan & outClientIdentity) override;
    CHIP_ERROR SignWithClientIdentity(uint8_t networkIndex, const ByteSpan & message,
                                      Crypto::P256ECDSASignature & outSignature) override;
#endif // CHIP_DEVICE_CONFIG_ENABLE_WIFI_PDC

private:
    struct WiFiNetwork
    {
        bool Empty() const { return ssidLen == 0; }
        bool Matches(ByteSpan aSsid) const { return !Empty() && ByteSpan(ssid, ssidLen).data_equal(aSsid); }

        uint8_t ssid[DeviceLayer::Internal::kMaxWiFiSSIDLength];
        uint8_t ssidLen = 0;
        static_assert(std::numeric_limits<decltype(ssidLen)>::max() >= sizeof(ssid));

        uint8_t credentials[DeviceLayer::Internal::kMaxWiFiKeyLength];
        uint8_t credentialsLen = 0;
        static_assert(std::numeric_limits<decltype(credentialsLen)>::max() >= sizeof(credentials));

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI_PDC
        bool UsingPDC() const { return networkIdentityLen != 0; }

        uint8_t networkIdentity[Credentials::kMaxCHIPCompactNetworkIdentityLength];
        uint8_t networkIdentityLen = 0;
        static_assert(std::numeric_limits<decltype(networkIdentityLen)>::max() >= sizeof(networkIdentity));

        uint8_t clientIdentity[Credentials::kMaxCHIPCompactNetworkIdentityLength];
        uint8_t clientIdentityLen = 0;
        static_assert(std::numeric_limits<decltype(clientIdentityLen)>::max() >= sizeof(clientIdentity));

        Platform::SharedPtr<Crypto::P256Keypair> clientIdentityKeypair;
#endif // CHIP_DEVICE_CONFIG_ENABLE_WIFI_PDC
    };

    WiFiNetwork mSavedNetwork;
    WiFiNetwork mStagingNetwork;
    // Whether 5GHz band is supported, as claimed by callers (`Set5gSupport()`) rather than syscalls.
    bool mIs5gSupported = false;
};
#endif // CHIP_DEVICE_CONFIG_ENABLE_WPA

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
class LinuxThreadDriver final : public ThreadDriver
{
public:
    class ThreadNetworkIterator final : public NetworkIterator
    {
    public:
        ThreadNetworkIterator(LinuxThreadDriver * aDriver) : driver(aDriver) {}
        size_t Count() override;
        bool Next(Network & item) override;
        void Release() override { delete this; }
        ~ThreadNetworkIterator() override = default;

    private:
        LinuxThreadDriver * driver;
        bool exhausted = false;
    };

    // BaseDriver
    NetworkIterator * GetNetworks() override { return new ThreadNetworkIterator(this); }
    CHIP_ERROR Init(BaseDriver::NetworkStatusChangeCallback * networkStatusChangeCallback) override;
    void Shutdown() override;

    // WirelessDriver
    uint8_t GetMaxNetworks() override { return 1; }
    uint8_t GetScanNetworkTimeoutSeconds() override { return 10; }
    uint8_t GetConnectNetworkTimeoutSeconds() override { return 20; }

    CHIP_ERROR CommitConfiguration() override;
    CHIP_ERROR RevertConfiguration() override;

    Status RemoveNetwork(ByteSpan networkId, MutableCharSpan & outDebugText, uint8_t & outNetworkIndex) override;
    Status ReorderNetwork(ByteSpan networkId, uint8_t index, MutableCharSpan & outDebugText) override;
    void ConnectNetwork(ByteSpan networkId, ConnectCallback * callback) override;

    // ThreadDriver
    Status AddOrUpdateNetwork(ByteSpan operationalDataset, MutableCharSpan & outDebugText, uint8_t & outNetworkIndex) override;
    void ScanNetworks(ThreadDriver::ScanCallback * callback) override;
    ThreadCapabilities GetSupportedThreadFeatures() override;
    uint16_t GetThreadVersion() override;

private:
    ThreadNetworkIterator mThreadIterator = ThreadNetworkIterator(this);
    Thread::OperationalDataset mSavedNetwork;
    Thread::OperationalDataset mStagingNetwork;
};

#endif // CHIP_DEVICE_CONFIG_ENABLE_THREAD

class LinuxEthernetDriver final : public EthernetDriver
{
public:
    struct EthernetNetworkIterator final : public NetworkIterator
    {
        EthernetNetworkIterator() = default;
        size_t Count() override { return interfaceNameLen > 0 ? 1 : 0; }
        bool Next(Network & item) override
        {
            if (exhausted)
            {
                return false;
            }
            exhausted = true;
            memcpy(item.networkID, interfaceName, interfaceNameLen);
            item.networkIDLen = interfaceNameLen;
            item.connected    = true;
            return true;
        }
        void Release() override { delete this; }
        ~EthernetNetworkIterator() override = default;

        // Public, but cannot be accessed via NetworkIterator interface.
        uint8_t interfaceName[kMaxNetworkIDLen];
        uint8_t interfaceNameLen = 0;
        bool exhausted           = false;
    };

    uint8_t GetMaxNetworks() override { return 1; };
    NetworkIterator * GetNetworks() override;
    CHIP_ERROR Init(BaseDriver::NetworkStatusChangeCallback * networkStatusChangeCallback) override;
    void Shutdown() override;
};

} // namespace NetworkCommissioning
} // namespace DeviceLayer
} // namespace chip
