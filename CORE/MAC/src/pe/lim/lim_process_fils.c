/*
 * Copyright (c) 2017 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include <palTypes.h>
#include <lim_fils_defs.h>
#include <lim_process_fils.h>
#include <limSendMessages.h>
#include <limTypes.h>
#include <limUtils.h>
#include <limPropExtsUtils.h>
#include <limAssocUtils.h>
#include <limSession.h>
#include <ieee80211_defines.h>
#include <qdf_crypto.h>
#include <limSerDesUtils.h>
#ifdef WLAN_FEATURE_FILS_SK

#define SHA256_DIGEST_SIZE 32
#define SHA384_DIGEST_SIZE 48

#define HMAC_SHA256_CRYPTO_TYPE "hmac(sha256)"
#define HMAC_SHA386_CRYPTO_TYPE "hmac(sha384)"

int alloc_tfm(uint8_t *type);

#define WLAN_FILS_DEBUG

#ifdef WLAN_FILS_DEBUG
/**
 * lim_fils_data_dump()- dump fils data
 * @type: Data name
 * @data: pointer to data buffer
 * @len: data len
 *
 * Return: None
 */
void lim_fils_data_dump(char *type, uint8_t *data, uint32_t len)
{

    VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_INFO,
         ("%s : length %d"), type, len);
    vos_trace_hex_dump(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_INFO, data, len);
}
#else
static void lim_fils_data_dump(char *type, uint8_t *data, uint32_t len)
{ }
#endif

/**
 * lim_get_crypto_digest_len()- This API return hash length based on crypto type
 * @type: Crypto type
 *
 * Return: hash length
 */
static int lim_get_crypto_digest_len(uint8_t *type)
{
    if (!strcmp(type, HMAC_SHA386_CRYPTO_TYPE))
        return SHA384_DIGEST_SIZE;
    else if (!strcmp(type, HMAC_SHA256_CRYPTO_TYPE))
        return SHA256_DIGEST_SIZE;
    return -EINVAL;
}

/**
 * lim_get_auth_tag_len()- This API return auth tag len based on crypto suit
 * used to encrypt erp packet.
 * @crypto_suite: Crtpto suit
 *
 * Return: tag length
 */
static uint8_t lim_get_auth_tag_len(enum fils_erp_cryptosuite crypto_suite)
{
    switch (crypto_suite) {
    case HMAC_SHA256_64:
        return -EINVAL;
    case HMAC_SHA256_128:
        return FILS_SHA256_128_AUTH_TAG;
    case HMAC_SHA256_256:
        return FILS_SHA256_256_AUTH_TAG;
    default:
        return -EINVAL;
    }
}

/**
 * lim_copy_u16_be()- This API reads u16 value from network byte order buffer
 * @ptr: pointer to buffer
 *
 * Return: 16bit value
 */
static uint16_t limGetU16_be(uint8_t *buf)
{
    return (buf[0] << 8) | buf[1];
}
/**
 * lim_get_crypto_type()- This API returns crypto type based on akm suite used
 * @akm: akm used for authentication
 *
 * Return: Crypto type
 */
static uint8_t *lim_get_crypto_type(uint8_t akm)
{
    switch (akm) {
    case eCSR_AUTH_TYPE_FILS_SHA384:
    case eCSR_AUTH_TYPE_FT_FILS_SHA384:
        return FILS_SHA384_CRYPTO_TYPE;
    case eCSR_AUTH_TYPE_FILS_SHA256:
    case eCSR_AUTH_TYPE_FT_FILS_SHA256:
    default:
        return FILS_SHA256_CRYPTO_TYPE;
    }
}

/**
 * lim_get_pmk_length()- This API returns pmk length based on akm used
 * @akm: akm used for authentication
 *
 * Return: PMK length
 */
static uint8_t lim_get_pmk_length(int akm_type)
{
    switch (akm_type) {
    case eCSR_AUTH_TYPE_FILS_SHA256:
    case eCSR_AUTH_TYPE_FT_FILS_SHA256:
        return FILS_SHA256_PKM_LEN;
    case eCSR_AUTH_TYPE_FILS_SHA384:
    case eCSR_AUTH_TYPE_FT_FILS_SHA384:
        return FILS_SHA384_PKM_LEN;
    default:
        return FILS_SHA256_PKM_LEN;
    }
}

/**
 * lim_get_kek_len()- This API returns kek length based on akm used
 * @akm: akm used for authentication
 *
 * Return: KEK length
 */
static uint8_t lim_get_kek_len(uint8_t akm)
{
    switch (akm) {
    case eCSR_AUTH_TYPE_FILS_SHA384:
    case eCSR_AUTH_TYPE_FT_FILS_SHA384:
        return FILS_SHA384_KEK_LEN;
    case eCSR_AUTH_TYPE_FILS_SHA256:
    case eCSR_AUTH_TYPE_FT_FILS_SHA256:
        return FILS_SHA256_KEK_LEN;
    default:
        return FILS_SHA256_KEK_LEN;
    }
}

/**
 * lim_get_tk_len()- This API returns tk length based on cypher used
 * @akm: cypher used
 *
 * Return: TK length
 */
static uint8_t lim_get_tk_len(int cypher_suite)
{
    switch (cypher_suite) {
    case eSIR_ED_TKIP:
        return TK_LEN_TKIP;
    case eSIR_ED_CCMP:
        return TK_LEN_CCMP;
    case eSIR_ED_AES_128_CMAC:
        return TK_LEN_AES_128_CMAC;
    default:
        return 0;
    }
}

/**
 * lim_get_ick_len()- This API returns ick length based on akm used
 * @akm: akm used for authentication
 *
 * Return: ICK length
 */
static int lim_get_ick_len(uint8_t akm)
{
    switch (akm) {
    case eCSR_AUTH_TYPE_FILS_SHA384:
    case eCSR_AUTH_TYPE_FT_FILS_SHA384:
        return FILS_SHA384_ICK_LEN;
    case eCSR_AUTH_TYPE_FILS_SHA256:
    case eCSR_AUTH_TYPE_FT_FILS_SHA256:
    default:
        return FILS_SHA256_ICK_LEN;
    }
}

/**
 * lim_get_key_from_prf()- This API returns key data using PRF-X as defined in
 * 11.6.1.7.2 ieee-80211-2012.
 * @type: crypto type needs to be used
 * @secret: key which needs to be used in crypto
 * @secret_len: key_len of secret
 * @label: PRF label
 * @optional_data: Data used for hash
 * @optional_data_len: data length
 * @key: key data output
 * @keylen: key data length
 *
 * Return: VOS_STATUS
 */
static VOS_STATUS lim_get_key_from_prf(uint8_t *type, uint8_t *secret,
        uint32_t secret_len, uint8_t *label, uint8_t *optional_data,
        uint32_t optional_data_len, uint8_t *key, uint32_t keylen)
{
    uint8_t count[2];
    uint8_t *addr[4];
    uint32_t len[4];
    uint16_t key_bit_length = keylen * 8;
    uint8_t key_length[2];
    uint32_t i = 0, remain_len;
    uint16_t interation;
    uint8_t crypto_digest_len = lim_get_crypto_digest_len(type);
    uint8_t tmp_hash[SHA384_DIGEST_SIZE] = {0};

    addr[0] = count;
    len[0] = sizeof(count);

    addr[1] = label;
    len[1] = strlen(label);

    addr[2] = optional_data;
    len[2] = optional_data_len;

    vos_mem_copy(key_length, &key_bit_length, sizeof(key_bit_length));
    addr[3] = key_length;
    len[3] = sizeof(key_length);

    for (interation = 1; i < keylen; interation++) {
        remain_len = keylen - i;
        vos_mem_copy(count, &interation, sizeof(interation));

        if (remain_len >= crypto_digest_len)
            remain_len = crypto_digest_len;

        if(!alloc_tfm(type))
        {
            if (qdf_get_hmac_hash(type, secret, secret_len, 4,
                    addr, len, tmp_hash) < 0) {
                VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
                    FL("qdf_get_hmac_hash failed"));
                return VOS_STATUS_E_FAILURE;
            }
        }
        vos_mem_copy(&key[i], tmp_hash, remain_len);
        i += crypto_digest_len;
    }
    return VOS_STATUS_SUCCESS;
}

#define MAX_PRF_INTERATIONS_COUNT 255

/**
 * lim_default_hmac_sha256_kdf()- This API calculates key data using default kdf
 * defined in RFC4306.
 * @secret: key which needs to be used in crypto
 * @secret_len: key_len of secret
 * @label: PRF label
 * @optional_data: Data used for hash
 * @optional_data_len: data length
 * @key: key data output
 * @keylen: key data length
 *
 * This API creates default KDF as defined in RFC4306
 * PRF+ (K,S) = T1 | T2 | T3 | T4 | ...
 * T1 = PRF (K, S | 0x01)
 * T2 = PRF (K, T1 | S | 0x02)
 * T3 = PRF (K, T2 | S | 0x03)
 * T4 = PRF (K, T3 | S | 0x04)
 *
 * for every iteration its creates 32 bit of hash
 *
 * Return: VOS_STATUS
 */
static VOS_STATUS
lim_default_hmac_sha256_kdf(uint8_t *secret, uint32_t secret_len,
        uint8_t *label, uint8_t *optional_data,
        uint32_t optional_data_len, uint8_t *key, uint32_t keylen)
{
    uint8_t tmp_hash[SHA256_DIGEST_SIZE] = {0};
    uint8_t count = 1;
    uint8_t *addr[4];
    uint32_t len[4];
    uint32_t current_position = 0, remaining_data = SHA256_DIGEST_SIZE;

    addr[0] = tmp_hash;
    len[0] = SHA256_DIGEST_SIZE;
    addr[1] = label;
    len[1] = strlen(label) + 1;
    addr[2] = optional_data;
    len[2] = optional_data_len;
    addr[3] = &count;
    len[3] = 1;

    if (keylen == 0 ||
       (keylen > (MAX_PRF_INTERATIONS_COUNT * SHA256_DIGEST_SIZE))) {
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
            FL("invalid key length %d"), keylen);
        return VOS_STATUS_E_FAILURE;
    }

    if(!alloc_tfm(FILS_SHA256_CRYPTO_TYPE))
    {
        /* Create T1 */
        if (qdf_get_hmac_hash(FILS_SHA256_CRYPTO_TYPE, secret, secret_len, 3,
                &addr[1], &len[1], tmp_hash) < 0) {
            VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
                FL("failed to get hmac hash"));
            return VOS_STATUS_E_FAILURE;
        }
    }
    /* Update hash from tmp_hash */
    vos_mem_copy(key + current_position, tmp_hash, remaining_data);
    current_position += remaining_data;

    for (count = 2; current_position < keylen; count++) {
        remaining_data = keylen - current_position;
        if (remaining_data > SHA256_DIGEST_SIZE)
            remaining_data = SHA256_DIGEST_SIZE;

        if(!alloc_tfm(FILS_SHA256_CRYPTO_TYPE))
        {
            /* Create T-n */
            if (qdf_get_hmac_hash(FILS_SHA256_CRYPTO_TYPE, secret,
                        secret_len, 4, addr, len, tmp_hash) < 0) {
                VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
                        FL("failed to get hmac hash"));
                return VOS_STATUS_E_FAILURE;
            }
        }
        /* Update hash from tmp_hash */
        vos_mem_copy(key + current_position, tmp_hash, remaining_data);
        current_position += remaining_data;
    }
    return VOS_STATUS_SUCCESS;
}

/**
 * lim_process_fils_eap_tlv()- This API process eap tlv available in auth resp
 * and returns remaining length.
 * @pe_session: PE session
 * @wrapped_data: wrapped data
 * @data_len: length of tlv
 *
 * Return: remaining length
 */
static uint32_t lim_process_fils_eap_tlv(tpPESession pe_session,
                uint8_t *wrapped_data, uint32_t data_len)
{
    struct fils_eap_tlv *tlv;
    struct fils_auth_rsp_info *auth_info;
    uint8_t auth_tag_len;

    auth_info = &pe_session->fils_info->auth_info;
    /* Minimum */
    auth_tag_len = lim_get_auth_tag_len(HMAC_SHA256_128);

    while (data_len > (auth_tag_len + 1)) {
        tlv = (struct fils_eap_tlv *) wrapped_data;

        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_INFO,
            FL("tlv type %x len %u total %u"),
            tlv->type, tlv->length, data_len);
        switch (tlv->type) {
        case SIR_FILS_EAP_TLV_KEYNAME_NAI:
            auth_info->keyname = vos_mem_malloc(tlv->length);
            if (!auth_info->keyname) {
                VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_INFO,
                                    FL("failed to alloc memory"));
                return 0;
            }
            vos_mem_copy(auth_info->keyname,
                     tlv->data, tlv->length);
            auth_info->keylength = tlv->length;
            data_len -= (tlv->length + 2);
            wrapped_data += (tlv->length + 2);
            break;
        case SIR_FILS_EAP_TLV_R_RK_LIFETIME:
            /* TODO check this */
            auth_info->r_rk_lifetime = limGetU32(tlv->data);
            data_len -= (tlv->length + 2);
            wrapped_data += (tlv->length + 2);
            break;
        case SIR_FILS_EAP_TLV_R_MSK_LIFETIME:
            /* TODO check this */
            auth_info->r_msk_lifetime = limGetU32(tlv->data);
            data_len -= (tlv->length + 2);
            wrapped_data += (tlv->length + 2);
            break;
        case SIR_FILS_EAP_TLV_DOMAIN_NAME:
            auth_info->domain_name = vos_mem_malloc(tlv->length);
            if (!auth_info->domain_name) {
                VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_INFO,
                                    FL("failed to alloc memory"));
                return 0;
            }
            vos_mem_copy(auth_info->domain_name,
                     tlv->data, tlv->length);
            auth_info->domain_len = tlv->length;
            data_len -= (tlv->length + 2);
            wrapped_data += (tlv->length + 2);
            break;
            /* TODO process these now */
        case SIR_FILS_EAP_TLV_CRYPTO_LIST:
        case SIR_FILS_EAP_TLV_AUTH_INDICATION:
            data_len -= (tlv->length + 2);
            wrapped_data += (tlv->length + 2);
            break;
        default:
            VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
                FL("Unknown type"));
            return data_len;
        }
    }
    return data_len;
}

/**
 * lim_generate_key_data()- This API generates key data using prf
 * FILS-Key-Data = KDF-X(PMK, "FILS PTK Derivation", SPA||AA||SNonce||ANonce)
 * @fils_info: fils session info
 * @key_label: label used
 * @data: data buffer
 * @data_len: data buffer len
 * @key_data: hash data needs to be generated
 * @key_data_len: hash data len
 *
 * Return: VOS_STATUS
 */
static VOS_STATUS lim_generate_key_data(struct pe_fils_session *fils_info,
            uint8_t *key_label, uint8_t *data, uint32_t data_len,
            uint8_t *key_data, uint32_t key_data_len)
{
    VOS_STATUS status;

    if (!fils_info)
        return VOS_STATUS_E_FAILURE;

    status = lim_get_key_from_prf(lim_get_crypto_type(fils_info->akm),
            fils_info->fils_pmk,
            fils_info->fils_pmk_len,
            key_label, data, data_len, key_data, key_data_len);
    if (status != VOS_STATUS_SUCCESS)
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
                        FL("failed to generate keydata"));
    return status;
}

/**
 * lim_generate_ap_key_auth()- This API generates ap auth data which needs to be
 * verified in assoc response.
 * @pe_session: pe session pointer
 *
 * Return: None
 */
static void lim_generate_ap_key_auth(tpPESession pe_session)
{
    uint8_t *buf, *addr[1];
    uint32_t len;
    struct pe_fils_session *fils_info = pe_session->fils_info;
    uint8_t data[SIR_FILS_NONCE_LENGTH + SIR_FILS_NONCE_LENGTH
            + QDF_MAC_ADDR_SIZE + QDF_MAC_ADDR_SIZE] = {0};

    if (!fils_info)
        return;
    len = SIR_FILS_NONCE_LENGTH + SIR_FILS_NONCE_LENGTH +
            QDF_MAC_ADDR_SIZE +  QDF_MAC_ADDR_SIZE;
    addr[0] = data;
    buf = data;
    vos_mem_copy(buf, fils_info->auth_info.fils_nonce,
            SIR_FILS_NONCE_LENGTH);
    buf += SIR_FILS_NONCE_LENGTH;
    vos_mem_copy(buf, fils_info->fils_nonce, SIR_FILS_NONCE_LENGTH);
    buf += SIR_FILS_NONCE_LENGTH;
    vos_mem_copy(buf, pe_session->bssId, QDF_MAC_ADDR_SIZE);
    buf += QDF_MAC_ADDR_SIZE;
    vos_mem_copy(buf, pe_session->selfMacAddr, QDF_MAC_ADDR_SIZE);
    buf += QDF_MAC_ADDR_SIZE;

    if(!alloc_tfm(lim_get_crypto_type(fils_info->akm)))
    {
        if (qdf_get_hmac_hash(lim_get_crypto_type(fils_info->akm),
                    fils_info->ick, fils_info->ick_len, 1, &addr[0],
                    &len, fils_info->ap_key_auth_data) < 0)
            VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
                FL("failed to generate PMK id"));
    }
    fils_info->ap_key_auth_len = lim_get_crypto_digest_len(
                    lim_get_crypto_type(fils_info->akm));
    lim_fils_data_dump("AP Key Auth", fils_info->ap_key_auth_data,
        fils_info->ap_key_auth_len);
}

/**
 * lim_generate_key_auth()- This API generates sta auth data which needs to be
 * send to AP in assoc request, AP will generate the same the verify it.
 * @pe_session: pe session pointer
 *
 * Return: None
 */
static void lim_generate_key_auth(tpPESession pe_session)
{
    uint8_t *buf, *addr[1];
    uint32_t len;
    struct pe_fils_session *fils_info = pe_session->fils_info;
    uint8_t data[SIR_FILS_NONCE_LENGTH + SIR_FILS_NONCE_LENGTH
            + QDF_MAC_ADDR_SIZE + QDF_MAC_ADDR_SIZE] = {0};

    if (!fils_info)
        return;

    len = SIR_FILS_NONCE_LENGTH + SIR_FILS_NONCE_LENGTH +
            QDF_MAC_ADDR_SIZE +  QDF_MAC_ADDR_SIZE;
    addr[0] = data;
    buf = data;
    vos_mem_copy(buf, fils_info->fils_nonce, SIR_FILS_NONCE_LENGTH);
    buf += SIR_FILS_NONCE_LENGTH;
    vos_mem_copy(buf, fils_info->auth_info.fils_nonce,
            SIR_FILS_NONCE_LENGTH);
    buf += SIR_FILS_NONCE_LENGTH;
    vos_mem_copy(buf, pe_session->selfMacAddr, QDF_MAC_ADDR_SIZE);
    buf += QDF_MAC_ADDR_SIZE;
    vos_mem_copy(buf, pe_session->bssId, QDF_MAC_ADDR_SIZE);
    buf += QDF_MAC_ADDR_SIZE;

    if(!alloc_tfm(lim_get_crypto_type(fils_info->akm)))
    {
        if (qdf_get_hmac_hash(lim_get_crypto_type(fils_info->akm),
                    fils_info->ick, fils_info->ick_len, 1,
                    &addr[0], &len, fils_info->key_auth) < 0)
            VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
                FL("failed to generate key auth"));
    }
    fils_info->key_auth_len = lim_get_crypto_digest_len(
                lim_get_crypto_type(fils_info->akm));
    lim_fils_data_dump("STA Key Auth",
            fils_info->key_auth, fils_info->key_auth_len);
}

/**
 * lim_get_keys()- This API generates keys keydata which is generated after
 * parsing of auth response.
 * KCK = L(FILS-Key-Data, 0, KCK_bits)
 * KEK = L(FILS-Key-Data, KCK_bits, KEK_bits)
 * TK = L(FILS-Key-Data, KCK_bits + KEK_bits, TK_bits)
 * FILS-FT = L(FILS-Key-Data, KCK_bits + KEK_bits + TK_bits, FILS-FT_bits)
 * @pe_session: pe session pointer
 *
 * Return: None
 */
static void lim_get_keys(tpPESession pe_session)
{
    uint8_t key_label[] = PTK_KEY_LABEL;
    uint8_t *data;
    uint8_t data_len;
    struct pe_fils_session *fils_info = pe_session->fils_info;
    uint8_t key_data[MAX_ICK_LEN + MAX_KEK_LEN + MAX_TK_LEN] = {0};
    uint8_t key_data_len;
    uint8_t ick_len = lim_get_ick_len(fils_info->akm);
    uint8_t kek_len = lim_get_kek_len(fils_info->akm);
    uint8_t tk_len = lim_get_tk_len(pe_session->encryptType);
    uint8_t *buf;

    if (!fils_info)
        return;
    key_data_len = ick_len + kek_len + tk_len;

    data_len = 2 * SIR_FILS_NONCE_LENGTH + 2 * QDF_MAC_ADDR_SIZE;
    data = vos_mem_malloc(data_len);
    if (!data) {
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
            FL("failed to alloc memory"));
            return;
    }

    /* Update data */
    buf = data;
    vos_mem_copy(buf, pe_session->selfMacAddr, QDF_MAC_ADDR_SIZE);
    buf += QDF_MAC_ADDR_SIZE;
    vos_mem_copy(buf, pe_session->bssId, QDF_MAC_ADDR_SIZE);
    buf += QDF_MAC_ADDR_SIZE;
    vos_mem_copy(buf, fils_info->fils_nonce, SIR_FILS_NONCE_LENGTH);
    buf += SIR_FILS_NONCE_LENGTH;
    vos_mem_copy(buf, fils_info->auth_info.fils_nonce,
            SIR_FILS_NONCE_LENGTH);
    lim_generate_key_data(fils_info, key_label, data, data_len,
                key_data, key_data_len);
    buf = key_data;
    vos_mem_copy(fils_info->ick, buf, ick_len);
    fils_info->ick_len = ick_len;
    buf += ick_len;
    vos_mem_copy(fils_info->kek, buf, kek_len);
    fils_info->kek_len = kek_len;
    buf += kek_len;
    vos_mem_copy(fils_info->tk, buf, tk_len);
    fils_info->tk_len = tk_len;
    lim_fils_data_dump("Key Data", key_data, key_data_len);
    lim_fils_data_dump("ICK", fils_info->ick, ick_len);
    lim_fils_data_dump("KEK", fils_info->kek, kek_len);
    lim_fils_data_dump("TK", fils_info->tk, tk_len);
    vos_mem_free(data);
}

/**
 * lim_generate_pmkid()- This API generates PMKID using hash of erp auth packet
 * parsing of auth response.
 * PMKID = Truncate-128(Hash(EAP-Initiate/Reauth))
 * @pe_session: pe session pointer
 *
 * Return: None
 */
static void lim_generate_pmkid(tpPESession pe_session)
{
    uint8_t hash[SHA384_DIGEST_SIZE];
    struct pe_fils_session *fils_info = pe_session->fils_info;

    if (!fils_info)
        return;

    if(!alloc_tfm(lim_get_crypto_type(fils_info->akm)))
    {
        qdf_get_hash(lim_get_crypto_type(fils_info->akm), 1,
            &fils_info->fils_erp_reauth_pkt,
            &fils_info->fils_erp_reauth_pkt_len, hash);
        vos_mem_copy(fils_info->fils_pmkid, hash, PMKID_LEN);
        lim_fils_data_dump("PMKID", fils_info->fils_pmkid, PMKID_LEN);
    }
}

/**
 * lim_generate_pmk()- This API generates PMK using hmac hash of rmsk data
 * anonce, snonce will be used as key for this
 * PMK = HMAC-Hash(SNonce || ANonce, rMSK [ || DHss ])
 * @pe_session: pe session pointer
 *
 * Return: None
 */
static void lim_generate_pmk(tpPESession pe_session)
{
    uint8_t nounce[2 * SIR_FILS_NONCE_LENGTH] = {0};
    uint8_t nounce_len = 2 * SIR_FILS_NONCE_LENGTH;
    uint8_t *addr[1];
    uint32_t len[1];
    struct pe_fils_session *fils_info = pe_session->fils_info;

    /* Snounce */
    vos_mem_copy(nounce, fils_info->fils_nonce,
            SIR_FILS_NONCE_LENGTH);
    /* anounce */
    vos_mem_copy(nounce + SIR_FILS_NONCE_LENGTH,
            fils_info->auth_info.fils_nonce,
            SIR_FILS_NONCE_LENGTH);
    fils_info->fils_pmk_len = lim_get_pmk_length(fils_info->akm);

    if (fils_info->fils_pmk)
        vos_mem_free(fils_info->fils_pmk);

    fils_info->fils_pmk = vos_mem_malloc(fils_info->fils_pmk_len);
    if (!fils_info->fils_pmk) {
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
            FL("failed to alloc memory"));
        return;
    }
    addr[0] = fils_info->fils_rmsk;
    len[0] = fils_info->fils_rmsk_len;
    lim_fils_data_dump("Nonce", nounce, nounce_len);

    if(!alloc_tfm(lim_get_crypto_type(fils_info->akm)))
    {
        if (qdf_get_hmac_hash(lim_get_crypto_type(fils_info->akm), nounce,
                    nounce_len, 1,
                    &addr[0], &len[0], fils_info->fils_pmk) < 0)
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
            FL("failed to generate PMK"));
    }
    lim_fils_data_dump("PMK", fils_info->fils_pmk, fils_info->fils_pmk_len);
}

/**
 * lim_generate_rmsk_data()- This API generates RMSK data using
 * default kdf as defined in RFC4306.
 * @pe_session: pe session pointer
 *
 * Return: None
 */
static void lim_generate_rmsk_data(tpPESession pe_session)
{
    uint8_t optional_data[4] = {0};
    uint8_t rmsk_label[] = RMSK_LABEL;
    struct pe_fils_session *fils_info = pe_session->fils_info;
    struct fils_auth_rsp_info *auth_info;

    if (!fils_info)
        return;
    auth_info = &(pe_session->fils_info->auth_info);
    fils_info->fils_rmsk_len = fils_info->fils_r_rk_len;
    fils_info->fils_rmsk = vos_mem_malloc(fils_info->fils_r_rk_len);
    if (!fils_info->fils_rmsk) {
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
            FL("failed to alloc memory"));
            return;
    }
    /*
     * Sequence number sent in EAP-INIT packet,
     * it should be in network byte order
     */
    lim_copy_u16_be(&optional_data[0], fils_info->sequence_number);
    lim_copy_u16_be(&optional_data[2], fils_info->fils_r_rk_len);
    lim_default_hmac_sha256_kdf(fils_info->fils_r_rk,
            fils_info->fils_r_rk_len, rmsk_label,
            optional_data, sizeof(optional_data),
            fils_info->fils_rmsk, fils_info->fils_rmsk_len);
    lim_fils_data_dump("RMSK", fils_info->fils_rmsk,
                fils_info->fils_rmsk_len);
}

/**
 * lim_process_auth_wrapped_data()- This API process wrapped data element
 * of auth response.
 * @pe_session: pe session pointer
 * @wrapped_data: wrapped data pointer
 * @data_len: wrapped data len
 *
 * Return: None
 */
static VOS_STATUS lim_process_auth_wrapped_data(tpPESession pe_session,
            uint8_t *wrapped_data, uint32_t data_len)
{
    uint8_t code;
    uint8_t identifier;
    uint16_t length;
    uint8_t type;
    unsigned long flags;
    struct pe_fils_session *fils_info;
    uint8_t hash[32], crypto;
    uint32_t remaining_len = data_len, new_len;
    uint8_t *input_data[1];
    uint32_t input_len[1];
    uint8_t auth_tag_len;

    fils_info = pe_session->fils_info;
    if (!fils_info)
        return VOS_STATUS_E_FAILURE;
    input_data[0] = wrapped_data;
    input_len[0] = data_len;

    VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_INFO,
        FL("trying to process the wrappped data"));

    code = *wrapped_data;
    wrapped_data++;
    remaining_len--;
    identifier = *wrapped_data;
    wrapped_data++;
    remaining_len--;

    length = limGetU16(wrapped_data);
    wrapped_data += sizeof(uint16_t);
    remaining_len -= sizeof(uint16_t);

    type = *wrapped_data; /* val should be 2 here */
    wrapped_data++;
    remaining_len--;

    flags = *wrapped_data;
    if (test_bit(7, &flags)) {
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
            FL("R bit is set in flag, error"));
        return VOS_STATUS_E_FAILURE;
    }
    wrapped_data++;
    remaining_len--;

    fils_info->auth_info.sequence = limGetU16_be(wrapped_data);
    wrapped_data += sizeof(uint16_t);
    remaining_len -= sizeof(uint16_t);
    /* Validate Auth sequence number */
    if (fils_info->auth_info.sequence < fils_info->sequence_number) {
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
            FL("sequence EAP-finish:%d is less than EAP-init:%d"),
            fils_info->auth_info.sequence,
            fils_info->sequence_number);
        return VOS_STATUS_E_FAILURE;
    }

    /* Parse attached TLVs */
    new_len = lim_process_fils_eap_tlv(pe_session,
                wrapped_data, remaining_len);
    VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
            FL("new len=%d remain=%d"), new_len, remaining_len);

    wrapped_data += remaining_len - new_len;
    remaining_len = new_len;
    /* Remove cryptosuite */
    crypto = *wrapped_data;
    wrapped_data++;
    remaining_len--;

    auth_tag_len = lim_get_auth_tag_len(crypto);
    input_len[0] -= auth_tag_len;
    /* if we have auth tag remaining */
    if (remaining_len == auth_tag_len) {

        if(!alloc_tfm(FILS_SHA256_CRYPTO_TYPE))
        {
            qdf_get_hmac_hash(FILS_SHA256_CRYPTO_TYPE, fils_info->fils_r_ik,
                    fils_info->fils_r_ik_len, 1,
                    input_data, input_len, hash);
        }
    } else {
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
            FL("invalid remaining len %d"),
            remaining_len);
    }
    if (!vos_mem_compare(wrapped_data, hash, auth_tag_len)) {
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
            FL("integratity check failed for auth, crypto %d"),
            crypto);
        return VOS_STATUS_E_FAILURE;
    }

    lim_generate_rmsk_data(pe_session);
    lim_generate_pmk(pe_session);
    lim_generate_pmkid(pe_session);
    return VOS_STATUS_SUCCESS;
}

/**
 * lim_is_valid_fils_auth_frame()- This API check whether auth frame is a
 * valid frame.
 * @mac_ctx: mac context
 * @pe_session: pe session pointer
 * @rx_auth_frm_body: pointer to autherntication frame
 *
 * Return: true if frame is valid or fils is disable, false otherwise
 */
bool lim_is_valid_fils_auth_frame(tpAniSirGlobal mac_ctx,
        tpPESession pe_session,
        tSirMacAuthFrameBody *rx_auth_frm_body)
{
    if (!pe_session->fils_info)
        return true;
    if (pe_session->fils_info->is_fils_connection == false)
        return true;

    if (!vos_mem_compare(rx_auth_frm_body->session,
                pe_session->fils_info->fils_session,
                SIR_FILS_SESSION_LENGTH)) {
        lim_fils_data_dump("Current FILS session",
                pe_session->fils_info->fils_session,
                SIR_FILS_SESSION_LENGTH);
        lim_fils_data_dump("FILS Session in pkt",
                rx_auth_frm_body->session,
                SIR_FILS_SESSION_LENGTH);
        return false;
    }
    vos_mem_copy(pe_session->fils_info->auth_info.fils_nonce,
            rx_auth_frm_body->nonce, SIR_FILS_NONCE_LENGTH);
    pe_session->fils_info->auth_info.assoc_delay =
            rx_auth_frm_body->assoc_delay_info;
    return true;
}

/**
 * lim_create_fils_r_ik()- This API create rik using rrk coming from
 * supplicant.
 * rIK = KDF (K, S), where
 * K = rRK and
 * S = rIK Label + "\0" + cryptosuite + length
 * The rIK Label is the 8-bit ASCII string:
 * Re-authentication Integrity Key@ietf.org
 * @fils_info: fils session info
 *
 * Return: None
 */
static VOS_STATUS lim_create_fils_r_ik(struct pe_fils_session *fils_info)
{
    uint8_t optional_data[SIR_FILS_OPTIONAL_DATA_LEN];
    uint8_t label[] = SIR_FILS_RIK_LABEL;

    if (!fils_info)
        return VOS_STATUS_E_FAILURE;
    optional_data[0] = HMAC_SHA256_128;
    /* basic validation */
    if (fils_info->fils_r_rk_len <= 0) {
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_INFO,
            FL("invalid r_rk length %d"), fils_info->fils_r_rk_len);
        return VOS_STATUS_E_FAILURE;
    }
    lim_copy_u16_be(&optional_data[1], fils_info->fils_r_rk_len);
    fils_info->fils_r_ik = vos_mem_malloc(fils_info->fils_r_rk_len);
    if (!fils_info->fils_r_ik) {
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_INFO,
                FL("failed to alloc memory"));
        return VOS_STATUS_E_FAILURE;
    }
    if (lim_default_hmac_sha256_kdf(fils_info->fils_r_rk,
                fils_info->fils_r_rk_len, label,
                optional_data, sizeof(optional_data),
                fils_info->fils_r_ik, fils_info->fils_r_rk_len)
            != VOS_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_INFO,
                FL("failed to create r_ik"));
        return VOS_STATUS_E_FAILURE;
    }
    fils_info->fils_r_ik_len = fils_info->fils_r_rk_len;
    lim_fils_data_dump("rRk", fils_info->fils_r_rk,
                fils_info->fils_r_rk_len);
    lim_fils_data_dump("rIk", fils_info->fils_r_ik,
                fils_info->fils_r_rk_len);

    return VOS_STATUS_SUCCESS;
}

/**
 * lim_create_fils_wrapper_data()- This API create warpped data which will be
 * sent in auth request.
 * @fils_info: fils session info
 *
 * Return: None
 */
static int lim_create_fils_wrapper_data(struct pe_fils_session *fils_info)
{
    uint8_t *buf;
    uint8_t auth_tag[FILS_AUTH_TAG_MAX_LENGTH] = {0};
    uint32_t length = 0;
    int buf_len =
        /* code + identifier */
        sizeof(uint8_t) * 2 +
        /* length */
        sizeof(uint16_t) +
        /* type + flags */
        sizeof(uint8_t) * 2 +
        /* sequence */
        sizeof(uint16_t) +
        /* tlv : type, length, data */
        sizeof(uint8_t) * 2 + fils_info->keyname_nai_length +
        /* cryptosuite + auth_tag */
        sizeof(uint8_t) + lim_get_auth_tag_len(HMAC_SHA256_128);

    if (!fils_info)
        return 0;
    fils_info->fils_erp_reauth_pkt = vos_mem_malloc(buf_len);
    if (!fils_info->fils_erp_reauth_pkt) {
        VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_ERROR,
                FL("failed to allocate memory"));
        return -EINVAL;
    }
    buf = fils_info->fils_erp_reauth_pkt;
    *buf = 5;
    buf++;
    /* Identifier */
    *buf = 0;
    buf++;
    /* Length */
    lim_copy_u16_be(buf, buf_len);
    buf += sizeof(uint16_t);
    /* type */
    *buf = SIR_FILS_EAP_INIT_PACKET_TYPE;
    buf++;

    /**
     *  flag
     *  0 1 2  <-- 5 -->
     * ----------------
     * |R|B|L| Reserved|
     * -----------------
     */
    *buf = 0x20; /* l=1, b=0, r=0 */
    buf++;
    /* sequence */
    lim_copy_u16_be(buf, fils_info->sequence_number);
    buf += sizeof(uint16_t);

    /* tlv */
    /* Type */
    *buf = SIR_FILS_EAP_TLV_KEYNAME_NAI;
    buf++;
    /* NAI Length */
    *buf = fils_info->keyname_nai_length;
    buf++;
    /* NAI Data */
    vos_mem_copy(buf, fils_info->keyname_nai_data,
            fils_info->keyname_nai_length);
    buf += fils_info->keyname_nai_length;

    /* cryptosuite */
    *buf = HMAC_SHA256_128;
    buf++;

    /*
     * This should be moved to just after sending probe to save time
     * lim_process_switch_channel_join_req ??
     */
    lim_create_fils_r_ik(fils_info);
    fils_info->fils_erp_reauth_pkt_len = buf_len;
    length = fils_info->fils_erp_reauth_pkt_len -
            lim_get_auth_tag_len(HMAC_SHA256_128);
    if(!alloc_tfm(FILS_SHA256_CRYPTO_TYPE))
    {
        qdf_get_hmac_hash(FILS_SHA256_CRYPTO_TYPE,
                fils_info->fils_r_ik, fils_info->fils_r_ik_len, 1,
                &fils_info->fils_erp_reauth_pkt, &length, auth_tag);
    }
    lim_fils_data_dump("Auth tag", auth_tag,
            lim_get_auth_tag_len(HMAC_SHA256_128));
    lim_fils_data_dump("EAP init pkt", fils_info->fils_erp_reauth_pkt,
            fils_info->fils_erp_reauth_pkt_len);

    vos_mem_copy(buf, auth_tag, lim_get_auth_tag_len(HMAC_SHA256_128));
    buf += lim_get_auth_tag_len(HMAC_SHA256_128);

    return buf_len;
}

/**
 * lim_add_fils_data_to_auth_frame()- This API add fils data to auth frame.
 * Following will be added in this.
 * RSNIE
 * SNonce
 * Session
 * Wrapped data
 * @session: PE session
 * @body: pointer to auth frame where data needs to be added
 *
 * Return: None
 */
void lim_add_fils_data_to_auth_frame(tpPESession session,
        uint8_t *body)
{
    struct pe_fils_session *fils_info;

    fils_info = session->fils_info;
    if (!fils_info)
        return;
    /* RSN IE */
    vos_mem_copy(body, fils_info->rsn_ie, fils_info->rsn_ie_len);
    body += fils_info->rsn_ie_len;
    lim_fils_data_dump("FILS RSN", fils_info->rsn_ie,
            fils_info->rsn_ie_len);

    /* ***Nounce*** */
    /* Add element id */
    *body = SIR_MAX_ELEMENT_ID;
    body++;
    /* Add nounce length + 1 for ext element id */
    *body = SIR_FILS_NONCE_LENGTH + 1;
    body++;
    /* Add ext element */
    *body = SIR_FILS_NONCE_EXT_EID;
    body++;
    /* Add data */
    vos_rand_get_bytes(0, fils_info->fils_nonce, SIR_FILS_NONCE_LENGTH);
    vos_mem_copy(body, fils_info->fils_nonce, SIR_FILS_NONCE_LENGTH);
    body = body + SIR_FILS_NONCE_LENGTH;
    /* Dump data */
    lim_fils_data_dump("fils anonce", fils_info->fils_nonce,
            SIR_FILS_NONCE_LENGTH);

    /*   *** Session ***  */
    /* Add element id */
    *body = SIR_MAX_ELEMENT_ID;
    body++;
    /* Add nounce length + 1 for ext element id */
    *body = SIR_FILS_SESSION_LENGTH + 1;
    body++;
    /* Add ext element */
    *body = SIR_FILS_SESSION_EXT_EID;
    body++;
    /* Add data */
    vos_rand_get_bytes(0, fils_info->fils_session, SIR_FILS_SESSION_LENGTH);
    vos_mem_copy(body, fils_info->fils_session, SIR_FILS_SESSION_LENGTH);
    body = body + SIR_FILS_SESSION_LENGTH;
    /* dump data */
    lim_fils_data_dump("Fils Session",
        fils_info->fils_session, SIR_FILS_SESSION_LENGTH);

    /*  ERP Packet  */
    /* Add element id */
    *body = SIR_MAX_ELEMENT_ID;
    body++;
    /* Add packet length + 1 for ext element id */
    *body = fils_info->fils_erp_reauth_pkt_len + 1;
    body++;
    /* Add ext element */
    *body = SIR_FILS_WRAPPED_DATA_EXT_EID;
    body++;
    /* Copy data */
    vos_mem_copy(body, fils_info->fils_erp_reauth_pkt,
            fils_info->fils_erp_reauth_pkt_len);
    lim_fils_data_dump("Fils ERP reauth Pkt",
            fils_info->fils_erp_reauth_pkt,
            fils_info->fils_erp_reauth_pkt_len);
    body = body + fils_info->fils_erp_reauth_pkt_len;
}

/**
 * lim_process_fils_auth_frame2()- This API process fils data from auth response
 * @mac_ctx: mac context
 * @session: PE session
 * @rx_auth_frm_body: pointer to auth frame
 *
 * Return: true if fils data needs to be processed else false
 */
bool lim_process_fils_auth_frame2(tpAniSirGlobal mac_ctx,
        tpPESession pe_session,
        tSirMacAuthFrameBody *rx_auth_frm_body)
{
    bool pmkid_found = false;
    int i;
    tDot11fIERSN dot11f_ie_rsn = {0};

    if (rx_auth_frm_body->authAlgoNumber != eSIR_FILS_SK_WITHOUT_PFS)
        return false;

    if (!pe_session->fils_info)
        return false;
    dot11fUnpackIeRSN(mac_ctx,
                &rx_auth_frm_body->rsn_ie.info[0],
                rx_auth_frm_body->rsn_ie.length,
                &dot11f_ie_rsn);

    for (i = 0; i < dot11f_ie_rsn.pmkid_count; i++) {
        if (vos_mem_compare(dot11f_ie_rsn.pmkid[i],
            pe_session->fils_info->fils_pmkid,
            IEEE80211_PMKID_LEN)) {
            pmkid_found = true;
            VOS_TRACE(VOS_MODULE_ID_PE,
                VOS_TRACE_LEVEL_DEBUG,
                FL("pmkid match in rsn ie total_count %d"),
                dot11f_ie_rsn.pmkid_count);
            break;
        }
    }
    if (!pmkid_found) {
        if (VOS_STATUS_SUCCESS !=
            lim_process_auth_wrapped_data(pe_session,
            rx_auth_frm_body->wrapped_data,
            rx_auth_frm_body->wrapped_data_len))
            return false;
    } else {
        lim_fils_data_dump("PMK",
            pe_session->fils_info->fils_pmk,
            pe_session->fils_info->fils_pmk_len);
    }
    lim_get_keys(pe_session);
    lim_generate_key_auth(pe_session);
    lim_generate_ap_key_auth(pe_session);
    return true;
}

/**
 * lim_update_fils_config()- This API update fils session info to csr config
 * from join request.
 * @session: PE session
 * @sme_join_req: pointer to join request
 *
 * Return: None
 */
void lim_update_fils_config(tpPESession session,
        tpSirSmeJoinReq sme_join_req)
{
    struct pe_fils_session *csr_fils_info;
    struct cds_fils_connection_info *fils_config_info;

    fils_config_info = &sme_join_req->fils_con_info;
    csr_fils_info = session->fils_info;
    if (!csr_fils_info){
        VOS_TRACE(VOS_MODULE_ID_PE,
                VOS_TRACE_LEVEL_DEBUG,
                FL(" csr fils info not present exiting....."));
        return;
    }
    if (fils_config_info->is_fils_connection == false){
        VOS_TRACE(VOS_MODULE_ID_PE,
                VOS_TRACE_LEVEL_DEBUG,
                FL(" is_fils_connection is false....."));
        return;
    }

    csr_fils_info->is_fils_connection =
        fils_config_info->is_fils_connection;
    csr_fils_info->keyname_nai_length =
        fils_config_info->key_nai_length;
    csr_fils_info->fils_r_rk_len =
        fils_config_info->r_rk_length;
    csr_fils_info->akm = fils_config_info->akm_type;
    csr_fils_info->auth = fils_config_info->auth_type;
    csr_fils_info->sequence_number = fils_config_info->sequence_number;
    csr_fils_info->keyname_nai_data =
        vos_mem_malloc(fils_config_info->key_nai_length);
    if (!csr_fils_info->keyname_nai_data) {
        VOS_TRACE(VOS_MODULE_ID_PE,
                VOS_TRACE_LEVEL_DEBUG,
                FL("failed to alloc memory"));
                return;
    }
    vos_mem_copy(csr_fils_info->keyname_nai_data,
            fils_config_info->keyname_nai,
            fils_config_info->key_nai_length);
    csr_fils_info->fils_r_rk =
        vos_mem_malloc(fils_config_info->r_rk_length);
    if (!csr_fils_info->fils_r_rk) {
        VOS_TRACE(VOS_MODULE_ID_PE,
                VOS_TRACE_LEVEL_DEBUG,
                FL("failed to alloc memory"));
        vos_mem_free(csr_fils_info->keyname_nai_data);
        return;
    }
    vos_mem_copy(csr_fils_info->fils_r_rk,
            fils_config_info->r_rk,
            fils_config_info->r_rk_length);

    vos_mem_copy(csr_fils_info->fils_pmkid,
            fils_config_info->pmkid, PMKID_LEN);

    csr_fils_info->rsn_ie_len = sme_join_req->rsnIE.length;
    vos_mem_copy(csr_fils_info->rsn_ie,
            sme_join_req->rsnIE.rsnIEdata,
            sme_join_req->rsnIE.length);

    csr_fils_info->fils_pmk_len = fils_config_info->pmk_len;
    if (fils_config_info->pmk_len)
    {
        csr_fils_info->fils_pmk =
            vos_mem_malloc(fils_config_info->pmk_len);
        if (!csr_fils_info->fils_pmk) {
            vos_mem_free(csr_fils_info->keyname_nai_data);
            vos_mem_free(csr_fils_info->fils_r_rk);
            VOS_TRACE(VOS_MODULE_ID_PE,
                VOS_TRACE_LEVEL_DEBUG,
                FL("failed to alloc memory"));
            return;
        }
    }
    vos_mem_copy(csr_fils_info->fils_pmk, fils_config_info->pmk,
            fils_config_info->pmk_len);

    VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_INFO,
        FL("fils=%d nai-len=%d rrk_len=%d akm=%d auth=%d pmk_len=%d"),
        fils_config_info->is_fils_connection,
        fils_config_info->key_nai_length,
        fils_config_info->r_rk_length,
        fils_config_info->akm_type,
        fils_config_info->auth_type,
        fils_config_info->pmk_len);
}

#define EXTENDED_IE_HEADER_LEN 3
/**
 * lim_create_fils_auth_data()- This API creates the fils auth data
 * which needs to be sent in auth req.
 * @mac_ctx: mac context
 * @auth_frame: pointer to auth frame
 * @session: PE session
 *
 * Return: length of fils data
 */
uint32_t lim_create_fils_auth_data(tpAniSirGlobal mac_ctx,
        tpSirMacAuthFrameBody auth_frame,
        tpPESession session)
{
    uint32_t frame_len = 0;
    uint32_t wrapped_data_len;

    if (!session->fils_info)
        return 0;
    if (auth_frame->authAlgoNumber == eSIR_FILS_SK_WITHOUT_PFS) {
        frame_len += session->fils_info->rsn_ie_len;
        /* FILS nounce */
        frame_len += SIR_FILS_NONCE_LENGTH + EXTENDED_IE_HEADER_LEN;
        /* FILS Session */
        frame_len += SIR_FILS_SESSION_LENGTH + EXTENDED_IE_HEADER_LEN;
        /* Calculate data/length for FILS Wrapped Data */
        wrapped_data_len =
            lim_create_fils_wrapper_data(session->fils_info);
        if (wrapped_data_len < 0) {
            VOS_TRACE(VOS_MODULE_ID_PE, VOS_TRACE_LEVEL_INFO,
            FL("failed to create warpped data"));
            return 0;
        }
        frame_len += wrapped_data_len + EXTENDED_IE_HEADER_LEN;
    }
    return frame_len;
}
#endif
