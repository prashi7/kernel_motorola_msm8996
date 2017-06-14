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

#include <wma.h>
#include <csrApi.h>
#include <limGlobal.h>
#include <aniGlobal.h>
#include <limSerDesUtils.h>

#ifdef WLAN_FEATURE_FILS_SK

/**
 * lim_process_fils_auth_frame2()- This API processes fils data from auth resp
 * @mac_ctx: mac context
 * @session: PE session
 * @rx_auth_frm_body: pointer to auth frame
 *
 * Return: true if fils data needs to be processed else false
 */
bool lim_process_fils_auth_frame2(tpAniSirGlobal mac_ctx,
				tpPESession pe_session,
				tSirMacAuthFrameBody * rx_auth_frm_body);

/**
 * lim_add_fils_data_to_auth_frame()- This API adds fils data to auth frame.
 * Following will be added in this.
 *     1. RSNIE
 *     2. SNonce
 *     3. Session
 *     4. Wrapped data
 * @session: PE session
 * @body: pointer to auth frame where data needs to be added
 *
 * Return: None
 */
void lim_add_fils_data_to_auth_frame(tpPESession session, uint8_t *body);

/**
 * lim_is_valid_fils_auth_frame()- This API checks whether auth frame is a
 * valid frame.
 * @mac_ctx: mac context
 * @pe_session: pe session pointer
 * @rx_auth_frm_body: pointer to autherntication frame
 *
 * Return: true if frame is valid or fils is disable, false otherwise
 */
bool lim_is_valid_fils_auth_frame(tpAniSirGlobal mac_ctx,
	tpPESession pe_session, tSirMacAuthFrameBody *rx_auth_frm_body);

/**
 * lim_update_fils_config()- This API updates fils session info to csr config
 * from join request.
 * @session: PE session
 * @sme_join_req: pointer to join request
 *
 * Return: None
 */
void lim_update_fils_config(tpPESession session, tpSirSmeJoinReq sme_join_req);

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
		tpSirMacAuthFrameBody auth_frame, tpPESession session);

/**
 * lim_increase_fils_sequence_number: this API increases fils sequence number in
 * the event of resending auth packet
 * @session_entry: pointer to PE session
 *
 * Return: None
 */
static inline void lim_increase_fils_sequence_number(tpPESession session_entry)
{
	if (session_entry->fils_info->is_fils_connection)
		session_entry->fils_info->sequence_number++;
}

/**
 * aead_encrypt_assoc_req() - Encrypt FILS IE's in assoc request
 * @mac_ctx: mac context
 * @pe_session: PE session
 * @frame: packed frame buffer
 * @payload: length of @frame
 *
 * This API is used to encrypt the all the IE present after FILS session IE
 * in Association request frame
 *
 * Return: VOS_STATUS
 */
VOS_STATUS aead_encrypt_assoc_req(tpAniSirGlobal mac_ctx,
				  tpPESession pe_session,
				  uint8_t *frame, uint32_t *payload);

/**
 * aead_decrypt_assoc_rsp() - API for AEAD decryption in FILS connection
 * @mac_ctx: MAC context
 * @session: PE session
 * @ar: Assoc response frame structure
 * @p_frame: frame buffer received
 * @n_frame: length of @p_frame
 *
 * This API is used to decrypt the AEAD encrypted part of FILS assoc response
 * and populate the decrypted FILS IE's to Assoc response frame structure(ar)
 *
 * Return: VOS_STATUS
 */
VOS_STATUS aead_decrypt_assoc_rsp(tpAniSirGlobal mac_ctx,
				  tpPESession session,
				  tDot11fAssocResponse *ar,
				  uint8_t *p_frame, uint32_t *n_frame);
/**
 * lim_is_fils_connection() - Check if it is FILS connection
 * @pe_session: PE session
 *
 * This API is used to check if current PE session is FILS connection
 *
 * Return: True if FILS connection, false if not
 */
static inline bool lim_is_fils_connection(tpPESession pe_session)
{

	if (!pe_session->fils_info)
		return false;

	if (pe_session->fils_info->is_fils_connection)
		return true;
	return false;
}

/**
 * lim_verify_fils_params_assoc_rsp() - Verify FILS params in assoc rsp
 * @mac_ctx: Mac context
 * @session_entry: PE session
 * @assoc_rsp: Assoc response received
 * @assoc_cnf: Assoc cnf msg to be sent to MLME
 *
 * This API is used to match FILS params received in Assoc response
 * with Assoc params received/derived at the Authentication stage
 *
 * Return: True, if successfully matches. False, otherwise
 */
bool lim_verify_fils_params_assoc_rsp(tpAniSirGlobal mac_ctx,
				      tpPESession session_entry,
				      tpSirAssocRsp assoc_rsp,
				      tLimMlmAssocCnf *assoc_cnf);
#else
static inline bool lim_process_fils_auth_frame2(tpAniSirGlobal mac_ctx,
		tpPESession pe_session, tSirMacAuthFrameBody *rx_auth_frm_body)
{
	return false;
}

static inline void
lim_increase_fils_sequence_number(tpPESession session_entry)
{ }

static inline void
lim_add_fils_data_to_auth_frame(tpPESession session, uint8_t *body)
{
}

static inline bool lim_is_valid_fils_auth_frame(tpAniSirGlobal mac_ctx,
	tpPESession pe_session, tSirMacAuthFrameBody *rx_auth_frm_body)
{
	return true;
}

static inline void
lim_update_fils_config(tpPESession session, tpSirSmeJoinReq sme_join_req)
{ }

static inline uint32_t lim_create_fils_auth_data(tpAniSirGlobal mac_ctx,
		tpSirMacAuthFrameBody auth_frame, tpPESession session)
{
	return 0;
}

static inline bool lim_is_fils_connection(tpPESession pe_session)
{
	return false;
}

static inline VOS_STATUS aead_encrypt_assoc_req(tpAniSirGlobal mac_ctx,
						tpPESession pe_session,
						uint8_t *frame,
						uint32_t *payload)
{
	return VOS_STATUS_SUCCESS;
}

static inline VOS_STATUS aead_decrypt_assoc_rsp(tpAniSirGlobal mac_ctx,
				  tpPESession session,
				  tDot11fAssocResponse *ar,
				  uint8_t *p_frame, uint32_t *n_frame)
{
	return VOS_STATUS_SUCCESS;
}

static inline bool lim_verify_fils_params_assoc_rsp(tpAniSirGlobal mac_ctx,
			tpPESession session_entry,
			tpSirAssocRsp assoc_rsp,
			tLimMlmAssocCnf *assoc_cnf)

{
	return true;
}
#endif
