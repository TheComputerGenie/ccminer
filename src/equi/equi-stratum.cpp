/**
 * Equihash specific stratum protocol
 * tpruvot@github - 2017 - Part under GPLv3 Licence
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <miner.h>

#include "equihash.h"

extern struct stratum_ctx stratum;
extern pthread_mutex_t stratum_work_lock;

/**
 * ZEC uses a different scale to compute diff... 
 * sample targets to diff (stored in the reverse byte order in work->target)
 * 0007fff800000000000000000000000000000000000000000000000000000000 is stratum diff 32
 * 003fffc000000000000000000000000000000000000000000000000000000000 is stratum diff 4
 * 00ffff0000000000000000000000000000000000000000000000000000000000 is stratum diff 1
 */
/**
 * Converts a target to difficulty for Equihash.
 *
 * @param target The target value.
 * @return The difficulty.
 */
double target_to_diff_equi(uint32_t* target)
{
    // Extract significant bytes from the target
    uchar* tgt = (uchar*) target;
    uint64_t m =
        (uint64_t)tgt[30] << 24 |
        (uint64_t)tgt[29] << 16 |
        (uint64_t)tgt[28] << 8  |
        (uint64_t)tgt[27] << 0;

    // Calculate and return difficulty
    if (!m)
        return 0.;
    else
        return (double)0xffff0000UL/m;
}

/**
 * Converts difficulty to target for Equihash.
 *
 * @param target The target value (output).
 * @param diff The difficulty.
 */
void diff_to_target_equi(uint32_t *target, double diff)
{
    uint64_t m;
    int k;

    // Scale down the difficulty to fit within range
    for (k = 6; k > 0 && diff > 1.0; k--)
        diff /= 4294967296.0;

    // Calculate the target value
    m = (uint64_t)(4294901760.0 / diff);
    if (m == 0 && k == 6)
        memset(target, 0xff, 32);
    else {
        memset(target, 0, 32);
        target[k + 1] = (uint32_t)(m >> 8);
        target[k + 2] = (uint32_t)(m >> 40);
        // Ensure leading zero bytes are set
        for (k = 0; k < 28 && ((uint8_t*)target)[k] == 0; k++)
            ((uint8_t*)target)[k] = 0xff;
    }
}

/**
 * Computes the network difficulty using nbits.
 *
 * @param work The work structure.
 * @return The network difficulty.
 */
double equi_network_diff(struct work *work)
{
    // Extract the nbits field from the work data
    uint32_t nbits = work->data[26];
    
    // Extract and shift bits to calculate target
    uint32_t bits = (nbits & 0xffffff);
    int16_t shift = (swab32(nbits) & 0xff);
    shift = (31 - shift) * 8; 
    uint64_t tgt64 = swab32(bits);
    tgt64 = tgt64 << shift;

    // Copy target to an array
    uint8_t net_target[32] = { 0 };
    for (int b=0; b<8; b++)
        net_target[31-b] = ((uint8_t*)&tgt64)[b];
    
    // Calculate and return difficulty
    double d = target_to_diff_equi((uint32_t*)net_target);
    return d;
}

/**
 * Sets the target for the given work based on difficulty.
 *
 * @param work The work structure.
 * @param diff The difficulty.
 */
void equi_work_set_target(struct work* work, double diff)
{
    // Convert difficulty to target and store it in work
    diff_to_target_equi(work->target, diff);
    work->targetdiff = diff;
}

/**
 * Sets the target for the stratum context from JSON parameters.
 *
 * @param sctx The stratum context.
 * @param params The JSON parameters.
 * @return True if successful, false otherwise.
 */
bool equi_stratum_set_target(struct stratum_ctx *sctx, json_t *params)
{
    uint8_t target_bin[32], target_be[32];

    // Extract target hex string from JSON parameters
    const char *target_hex = json_string_value(json_array_get(params, 0));
    if (!target_hex || strlen(target_hex) == 0)
        return false;

    // Convert hex string to binary
    hex2bin(target_bin, target_hex, 32);
    memset(target_be, 0x00, 32);
    int filled = 0;
    for (int i=0; i<32; i++) {
        if (filled == 8) break;
        target_be[31-i] = target_bin[i];
        if (target_bin[i]) filled++;
    }
    memcpy(sctx->job.extra, target_be, 32);

    // Calculate difficulty from target and store it in stratum context
    pthread_mutex_lock(&stratum_work_lock);
    sctx->next_diff = target_to_diff_equi((uint32_t*) &target_be);
    pthread_mutex_unlock(&stratum_work_lock);

    return true;
}

/**
 * Handles stratum notify messages.
 *
 * @param sctx The stratum context.
 * @param params The JSON parameters.
 * @return True if successful, false otherwise.
 */
bool equi_stratum_notify(struct stratum_ctx *sctx, json_t *params)
{
    const char *job_id, *version, *prevhash, *coinb1, *coinb2, *nbits, *stime, *solution = NULL;
    size_t coinb1_size, coinb2_size;
    bool clean, ret = false;
    int ntime, i, p=0;

    // Parse JSON elements
    job_id = json_string_value(json_array_get(params, p++));
    version = json_string_value(json_array_get(params, p++));
    prevhash = json_string_value(json_array_get(params, p++));
    coinb1 = json_string_value(json_array_get(params, p++));
    coinb2 = json_string_value(json_array_get(params, p++));
    stime = json_string_value(json_array_get(params, p++));
    nbits = json_string_value(json_array_get(params, p++));
    clean = json_is_true(json_array_get(params, p)); p++;
    solution = json_string_value(json_array_get(params, p++));

    // Validate received parameters
    if (!job_id || !prevhash || !coinb1 || !coinb2 || !version || !nbits || !stime ||
        strlen(prevhash) != 64 || strlen(version) != 8 ||
        strlen(coinb1) != 64 || strlen(coinb2) != 64 ||
        strlen(nbits) != 8 || strlen(stime) != 8) {
        applog(LOG_ERR, "Stratum notify: invalid parameters");
        goto out;
    }

    // Convert and store solution
    hex2bin(&sctx->job.solution, solution, 1344);

    // Store server time difference
    hex2bin((uchar *)&ntime, stime, 4);
    ntime = ntime - (int) time(0);
    if (ntime > sctx->srvtime_diff) {
        sctx->srvtime_diff = ntime;
        if (opt_protocol && ntime > 20)
            applog(LOG_DEBUG, "stratum time is at least %ds in the future", ntime);
    }

    // Store parsed job elements in stratum context
    pthread_mutex_lock(&stratum_work_lock);
    hex2bin(sctx->job.version, version, 4);
    hex2bin(sctx->job.prevhash, prevhash, 32);

    coinb1_size = strlen(coinb1) / 2;
    coinb2_size = strlen(coinb2) / 2;
    sctx->job.coinbase_size = coinb1_size + coinb2_size + 
        sctx->xnonce1_size + sctx->xnonce2_size;

    sctx->job.coinbase = (uchar*) realloc(sctx->job.coinbase, sctx->job.coinbase_size);
    hex2bin(sctx->job.coinbase, coinb1, coinb1_size);
    hex2bin(sctx->job.coinbase + coinb1_size, coinb2, coinb2_size);

    sctx->job.xnonce2 = sctx->job.coinbase + coinb1_size + coinb2_size + sctx->xnonce1_size;
    if (!sctx->job.job_id || strcmp(sctx->job.job_id, job_id))
        memset(sctx->job.xnonce2, 0, sctx->xnonce2_size);
    memcpy(sctx->job.coinbase + coinb1_size + coinb2_size, sctx->xnonce1, sctx->xnonce1_size);

    // Free previous merkle data and store new job ID
    for (i = 0; i < sctx->job.merkle_count; i++)
        free(sctx->job.merkle[i]);
    free(sctx->job.merkle);
    sctx->job.merkle = NULL;
    sctx->job.merkle_count = 0;

    free(sctx->job.job_id);
    sctx->job.job_id = strdup(job_id);

    // Store nbits and ntime
    hex2bin(sctx->job.nbits, nbits, 4);
    hex2bin(sctx->job.ntime, stime, 4);
    sctx->job.clean = clean;

    // Set job difficulty
    sctx->job.diff = sctx->next_diff;
    pthread_mutex_unlock(&stratum_work_lock);

    ret = true;

out:
    return ret;
}

/**
 * Handles stratum show message protocol for Equihash (use client.show_message to pass block height).
 *
 * @param sctx The stratum context.
 * @param id The JSON id.
 * @param params The JSON parameters.
 * @return True if successful, false otherwise.
 */
bool equi_stratum_show_message(struct stratum_ctx *sctx, json_t *id, json_t *params)
{
    char *s;
    json_t *val;
    bool ret;

    // Parse the first parameter
    val = json_array_get(params, 0);
    if (val) {
        const char* data = json_string_value(val);
        if (data && strlen(data)) {
            char symbol[32] = { 0 };
            uint32_t height = 0;
            int ss = sscanf(data, "equihash %s block %u", symbol, &height);
            if (height && ss > 1) sctx->job.height = height;
            if (opt_debug && ss > 1) applog(LOG_DEBUG, "%s", data);
        }
    }

    // Respond to the show message if id is provided
    if (!id || json_is_null(id))
        return true;

    val = json_object();
    json_object_set(val, "id", id);
    json_object_set_new(val, "error", json_null());
    json_object_set_new(val, "result", json_true());
    s = json_dumps(val, 0);
    ret = stratum_send_line(sctx, s);
    json_decref(val);
    free(s);

    return ret;
}

/**
 * Stores the work solution for Equihash.
 *
 * @param work The work structure.
 * @param hash The hash value.
 * @param sol_data The solution data.
 */
void equi_store_work_solution(struct work* work, uint32_t* hash, void* sol_data)
{
    int nonce = work->valid_nonces-1;
    // Copy solution data to extra field in work
    memcpy(work->extra, sol_data, 1347);
    // Store hash target ratio
    bn_store_hash_target_ratio(hash, work->target, work, nonce);
}

/**
 * Submits the work solution to the mining pool via stratum protocol (called by submit_upstream_work()).
 *
 * @param pool The pool information.
 * @param work The work structure.
 * @return True if successful, false otherwise.
 */
#define JSON_SUBMIT_BUF_LEN (4*1024)
bool equi_stratum_submit(struct pool_infos *pool, struct work *work)
{
    char _ALIGN(64) s[JSON_SUBMIT_BUF_LEN];
    char _ALIGN(64) timehex[16] = { 0 };
    char *jobid, *noncestr, *solhex;
    int idnonce = work->submit_nonce_id;

    // Prepare nonce for submission
    work->data[EQNONCE_OFFSET] = work->nonces[idnonce];
    unsigned char * nonce = (unsigned char*) (&work->data[27]);
    size_t nonce_len = 32 - stratum.xnonce1_size;
    noncestr = bin2hex(&nonce[stratum.xnonce1_size], nonce_len);

    // Prepare solution for submission
    solhex = (char*) calloc(1, 1344*2 + 64);
    if (!solhex || !noncestr) {
        applog(LOG_ERR, "unable to alloc share memory");
        return false;
    }
    cbin2hex(solhex, (const char*) work->extra, 1347);

    char* solHexRestore = (char*)calloc(128, 1);
    cbin2hex(solHexRestore, (const char*)&work->solution[8], 64);
    memcpy(&solhex[6 + 16], solHexRestore, 128);

    jobid = work->job_id + 8;
    sprintf(timehex, "%08x", swab32(work->data[25]));

    // Format JSON submission string
    snprintf(s, sizeof(s), "{\"method\":\"mining.submit\",\"params\":"
        "[\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"], \"id\":%u}",
        pool->user, jobid, timehex, noncestr, solhex,
        stratum.job.shares_count + 10);
        
    free(solHexRestore);
    free(solhex);
    free(noncestr);

    // Send submission to the stratum server
    gettimeofday(&stratum.tv_submit, NULL);

    if(!stratum_send_line(&stratum, s)) {
        applog(LOG_ERR, "%s stratum_send_line failed", __func__);
        return false;
    }

    // Update sharediff and share count
    stratum.sharediff = work->sharediff[idnonce];
    stratum.job.shares_count++;

    return true;
}