/*
 * Copyright 2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include "openssl_compat.h"

#if OPENSSL_VERSION_NUMBER < 0x10100000L

#include <string.h>

static void *OPENSSL_zalloc(size_t num)
{
    void *ret = OPENSSL_malloc(num);

    if (ret != NULL)
        memset(ret, 0, num);
    return ret;
}

int RSA_set0_key(RSA *r, BIGNUM *n, BIGNUM *e, BIGNUM *d)
{
    /* If the fields n and e in r are NULL, the corresponding input
    * parameters MUST be non-NULL for n and e.  d may be
    * left NULL (in case only the public key is used).
    */
    if ((r->n == NULL && n == NULL)
        || (r->e == NULL && e == NULL))
        return 0;

    if (n != NULL) {
        BN_free(r->n);
        r->n = n;
    }
    if (e != NULL) {
        BN_free(r->e);
        r->e = e;
    }
    if (d != NULL) {
        BN_free(r->d);
        r->d = d;
    }

    return 1;
}

int RSA_set0_factors(RSA *r, BIGNUM *p, BIGNUM *q)
{
    /* If the fields p and q in r are NULL, the corresponding input
    * parameters MUST be non-NULL.
    */
    if ((r->p == NULL && p == NULL)
        || (r->q == NULL && q == NULL))
        return 0;

    if (p != NULL) {
        BN_free(r->p);
        r->p = p;
    }
    if (q != NULL) {
        BN_free(r->q);
        r->q = q;
    }

    return 1;
}

int RSA_set0_crt_params(RSA *r, BIGNUM *dmp1, BIGNUM *dmq1, BIGNUM *iqmp)
{
    /* If the fields dmp1, dmq1 and iqmp in r are NULL, the corresponding input
    * parameters MUST be non-NULL.
    */
    if ((r->dmp1 == NULL && dmp1 == NULL)
        || (r->dmq1 == NULL && dmq1 == NULL)
        || (r->iqmp == NULL && iqmp == NULL))
        return 0;

    if (dmp1 != NULL) {
        BN_free(r->dmp1);
        r->dmp1 = dmp1;
    }
    if (dmq1 != NULL) {
        BN_free(r->dmq1);
        r->dmq1 = dmq1;
    }
    if (iqmp != NULL) {
        BN_free(r->iqmp);
        r->iqmp = iqmp;
    }

    return 1;
}

void RSA_get0_key(const RSA *r,
                  const BIGNUM **n, const BIGNUM **e, const BIGNUM **d)
{
    if (n != NULL)
        *n = r->n;
    if (e != NULL)
        *e = r->e;
    if (d != NULL)
        *d = r->d;
}

void RSA_get0_factors(const RSA *r, const BIGNUM **p, const BIGNUM **q)
{
    if (p != NULL)
        *p = r->p;
    if (q != NULL)
        *q = r->q;
}

void RSA_get0_crt_params(const RSA *r,
                        const BIGNUM **dmp1, const BIGNUM **dmq1,
                        const BIGNUM **iqmp)
{
    if (dmp1 != NULL)
        *dmp1 = r->dmp1;
    if (dmq1 != NULL)
        *dmq1 = r->dmq1;
    if (iqmp != NULL)
        *iqmp = r->iqmp;
}

int DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g)
{
    /* If the fields p and g in d are NULL, the corresponding input
    * parameters MUST be non-NULL.  q may remain NULL.
    */
    if ((dh->p == NULL && p == NULL)
        || (dh->g == NULL && g == NULL))
        return 0;

    if (p != NULL) {
        BN_free(dh->p);
        dh->p = p;
    }
    if (q != NULL) {
        BN_free(dh->q);
        dh->q = q;
    }
    if (g != NULL) {
        BN_free(dh->g);
        dh->g = g;
    }

    if (q != NULL) {
        dh->length = BN_num_bits(q);
    }

    return 1;
}

void DH_get0_key(const DH *dh, const BIGNUM **pub_key, const BIGNUM **priv_key)
{
    if (pub_key != NULL)
        *pub_key = dh->pub_key;
    if (priv_key != NULL)
        *priv_key = dh->priv_key;
}

int DH_set0_key(DH *dh, BIGNUM *pub_key, BIGNUM *priv_key)
{
    /* If the field pub_key in dh is NULL, the corresponding input
    * parameters MUST be non-NULL.  The priv_key field may
    * be left NULL.
    */
    if (dh->pub_key == NULL && pub_key == NULL)
        return 0;

    if (pub_key != NULL) {
        BN_free(dh->pub_key);
        dh->pub_key = pub_key;
    }
    if (priv_key != NULL) {
        BN_free(dh->priv_key);
        dh->priv_key = priv_key;
    }

    return 1;
}

int DH_set_length(DH *dh, long length)
{
    dh->length = length;
    return 1;
}

HMAC_CTX *HMAC_CTX_new(void)
{
    return OPENSSL_zalloc(sizeof(HMAC_CTX));
}

void HMAC_CTX_free(HMAC_CTX *ctx)
{
    HMAC_CTX_cleanup(ctx);
    OPENSSL_free(ctx);
}

RSA *EVP_PKEY_get0_RSA(EVP_PKEY *pkey)
{
    if (pkey->type != EVP_PKEY_RSA) {
        return NULL;
    }
    return pkey->pkey.rsa;
}


#endif /* OPENSSL_VERSION_NUMBER */
