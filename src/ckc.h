/*
 * Copyright 2010 Cloudkick Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#ifndef _ckc_h_
#define _ckc_h_

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

void ckc_error_out(const char *msg);

typedef struct ckc_transport_t {
    CURL *curl;
    struct curl_slist *headerlist;
    struct curl_httppost *formpost;
    struct curl_httppost *lastptr;
    const char *username;
    const char *password;
} ckc_transport_t;

typedef struct ckc_ll_t ckc_ll_t;

struct ckc_ll_t {
    const char *s;
    ckc_ll_t *next;
};

typedef struct ckc_accounts_t {
    int count;
    ckc_ll_t *head;
} ckc_accounts_t;

int ckc_prompt_username(const char **username);
int ckc_prompt_password(const char **password);
int ckc_prompt_number(int *num, int min, int max);
int ckc_prompt_yn();

int ckc_transport_init(ckc_transport_t *t);
int ckc_transport_list_accounts(ckc_transport_t *t, ckc_accounts_t **accounts);
int ckc_transport_get_consumer(ckc_transport_t *t, const char *account, ckc_ll_t **xl);
void ckc_transport_free(ckc_transport_t *t);

void ckc_accounts_free(ckc_accounts_t *accounts);

#endif

