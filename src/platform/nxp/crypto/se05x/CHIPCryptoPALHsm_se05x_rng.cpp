/*
 *
 *    Copyright (c) 2021, 2025 Project CHIP Authors
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

/**
 *    @file
 *      HSM based implementation of CHIP crypto primitives
 *      Based on configurations in CHIPCryptoPALHsm_config.h file,
 *      chip crypto apis use either HSM or rollback to software implementation.
 */

#include "CHIPCryptoPALHsm_se05x_utils.h"
#include <lib/core/CHIPEncoding.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

#if !ENABLE_SE05X_RND_GEN

typedef struct
{
    bool mInitialized;
    bool mDRBGSeeded;
    mbedtls_ctr_drbg_context mDRBGCtxt;
    mbedtls_entropy_context mEntropy;
} EntropyContext_h;

static EntropyContext_h gsEntropyContext_h;

static EntropyContext_h * get_entropy_context()
{
    if (!gsEntropyContext_h.mInitialized)
    {
        mbedtls_entropy_init(&gsEntropyContext_h.mEntropy);
        mbedtls_ctr_drbg_init(&gsEntropyContext_h.mDRBGCtxt);

        gsEntropyContext_h.mInitialized = true;
    }

    return &gsEntropyContext_h;
}

static mbedtls_ctr_drbg_context * get_drbg_context()
{
    EntropyContext_h * const context = get_entropy_context();

    mbedtls_ctr_drbg_context * const drbgCtxt = &context->mDRBGCtxt;

    if (!context->mDRBGSeeded)
    {
        const int status = mbedtls_ctr_drbg_seed(drbgCtxt, mbedtls_entropy_func, &context->mEntropy, nullptr, 0);
        if (status != 0)
        {
            return nullptr;
        }

        context->mDRBGSeeded = true;
    }

    return drbgCtxt;
}

void free_entropy_context_h()
{
    if (gsEntropyContext_h.mInitialized)
    {
        mbedtls_entropy_free(&gsEntropyContext_h.mEntropy);
        mbedtls_ctr_drbg_free(&gsEntropyContext_h.mDRBGCtxt);
        gsEntropyContext_h.mInitialized = false;
    }
}

#endif // #if !ENABLE_SE05X_RND_GEN

namespace chip {
namespace Crypto {

#if !ENABLE_SE05X_RND_GEN
CHIP_ERROR DRBG_get_bytes_h(uint8_t * out_buffer, const size_t out_length)
{
    VerifyOrReturnError(out_buffer != nullptr, CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(out_length > 0, CHIP_ERROR_INVALID_ARGUMENT);

    mbedtls_ctr_drbg_context * const drbg_ctxt = get_drbg_context();
    VerifyOrReturnError(drbg_ctxt != nullptr, CHIP_ERROR_INTERNAL);

    const int result = mbedtls_ctr_drbg_random(drbg_ctxt, Uint8::to_uchar(out_buffer), out_length);
    VerifyOrReturnError(result == 0, CHIP_ERROR_INTERNAL);

    return CHIP_NO_ERROR;
}
#endif //! ENABLE_SE05X_RND_GEN

CHIP_ERROR DRBG_get_bytes(uint8_t * out_buffer, const size_t out_length)
{
#if ENABLE_SE05X_RND_GEN
    sss_status_t status;
    sss_rng_context_t ctx_rng = { 0 };

    VerifyOrReturnError(out_buffer != nullptr, CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(out_length > 0, CHIP_ERROR_INVALID_ARGUMENT);

    ChipLogDetail(Crypto, "se05x::Random number generation using se05x");

    VerifyOrReturnError(se05x_session_open() == CHIP_NO_ERROR, CHIP_ERROR_INTERNAL);

    status = sss_rng_context_init(&ctx_rng, &gex_sss_chip_ctx.session /* Session */);
    VerifyOrReturnError(status == kStatus_SSS_Success, CHIP_ERROR_INTERNAL);

    status = sss_rng_get_random(&ctx_rng, out_buffer, out_length);
    VerifyOrReturnError(status == kStatus_SSS_Success, CHIP_ERROR_INTERNAL);

    sss_rng_context_free(&ctx_rng);

    return CHIP_NO_ERROR;
#else
    return DRBG_get_bytes_h(out_buffer, out_length);
#endif
}

} // namespace Crypto
} // namespace chip
