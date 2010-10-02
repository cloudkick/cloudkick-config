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

#include "ckc.h"
#include "ckc_version.h"

typedef struct ckc_buf_t {
    char *data;
    size_t len;
} ckc_buf_t;

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *baton)
{
    ckc_buf_t *buf = (ckc_buf_t*)baton;
    size_t s = size * nmemb;
    buf->data = realloc(buf->data, buf->len + s +1);
    memcpy((void*)&buf->data[buf->len], ptr, s);
    buf->len += s;
    buf->data[buf->len] = '\0';
    return s;
}

static int to_post_data(ckc_transport_t *t, const char *account)
{
    curl_formadd(&t->formpost,
                 &t->lastptr,
                 CURLFORM_COPYNAME, "user",
                 CURLFORM_COPYCONTENTS, t->username,
                 CURLFORM_END);

    curl_formadd(&t->formpost,
                 &t->lastptr,
                 CURLFORM_COPYNAME, "password",
                 CURLFORM_COPYCONTENTS, t->password,
                 CURLFORM_END);

    if (account != NULL) {
        curl_formadd(&t->formpost,
                     &t->lastptr,
                     CURLFORM_COPYNAME, "account",
                     CURLFORM_COPYCONTENTS, account,
                     CURLFORM_END);
    }

    curl_easy_setopt(t->curl, CURLOPT_HTTPPOST, t->formpost);

    return 0;
}

static ckc_ll_t* split_by_lines(const char *str)
{
    ckc_ll_t *head = NULL;
    ckc_ll_t *tmp;
    char *s = strdup(str);
    char *p = NULL;
    p = strtok(s, "\n");

    while (p != NULL)
    {
        tmp = malloc(sizeof(ckc_ll_t));
        tmp->s = strdup(p);
        tmp->next = head;
        head = tmp;
        p = strtok(NULL, "\n");
    }

    return head;
}

static int ckc_transport_run(ckc_transport_t *t, ckc_buf_t *buf)
{
    long httprc = -1;
    CURLcode res;

    curl_easy_setopt(t->curl, CURLOPT_WRITEDATA, (void *)buf);
    curl_easy_setopt(t->curl, CURLOPT_WRITEFUNCTION, write_cb);
    res = curl_easy_perform(t->curl);

    if (res != 0) {
        fprintf(stderr, "Failed talking to endpoint: (%d) %s\n\n",
                res, curl_easy_strerror(res));
        return -1;
    }

    curl_easy_getinfo(t->curl, CURLINFO_RESPONSE_CODE, &httprc);

    if (httprc >299 || httprc <= 199) {
        fprintf(stderr, "Endpoint returned HTTP %d\n",
                (int)httprc);
        fprintf(stderr, "%s\n\n", buf->data);
        return -1;
    }

    return 0;
}

int ckc_transport_list_accounts(ckc_transport_t *t, ckc_accounts_t **acct)
{
    ckc_accounts_t *a;
    ckc_ll_t *l;
    ckc_buf_t buf = {0};

    int rv = to_post_data(t, NULL);

    if (rv < 0) {
        return rv;
    }

    curl_easy_setopt(t->curl, CURLOPT_URL, "https://www.cloudkick.com/oauth/list_accounts/");
    
    rv = ckc_transport_run(t, &buf);

    if (rv < 0) {
        return rv;
    }

    l = split_by_lines(buf.data);
    if (l == NULL) {
        return -1;
    }
    
    a = calloc(1, sizeof(ckc_accounts_t));
    a->head = l;
    for (l = a->head; l != NULL; l = l->next) {
        a->count++;
    }

    *acct = a;

    return 0;
}

int ckc_transport_get_consumer(ckc_transport_t *t, const char *account, ckc_ll_t **xl)
{
    ckc_ll_t *l;
    ckc_buf_t buf = {0};
    int rv = to_post_data(t, account);

    if (rv < 0) {
        return rv;
    }

    curl_easy_setopt(t->curl, CURLOPT_URL, "https://www.cloudkick.com/oauth/create_consumer/");

    rv = ckc_transport_run(t, &buf);

    if (rv < 0) {
        return rv;
    }

    l = split_by_lines(buf.data);
    if (l == NULL) {
        return -1;
    }

    *xl = l;

    return 0;
}

int ckc_transport_init(ckc_transport_t *t)
{
    char uabuf[255];
    static const char buf[] = "Expect:";
    
    t->curl = curl_easy_init();
    
    snprintf(uabuf, sizeof(uabuf), "cloudkick-config/%d.%d.%d",
             CKC_VERSION_MAJOR, CKC_VERSION_MINOR, CKC_VERSION_PATCH);
    
    curl_easy_setopt(t->curl, CURLOPT_USERAGENT, uabuf);
//#define CKC_DEBUG
#ifdef CKC_DEBUG
    curl_easy_setopt(t->curl, CURLOPT_VERBOSE, 1);
#endif

    /* TODO: this is less than optimal */
    curl_easy_setopt(t->curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(t->curl, CURLOPT_SSL_VERIFYHOST, 0L);

    t->headerlist = curl_slist_append(t->headerlist, buf);
    curl_easy_setopt(t->curl, CURLOPT_HTTPHEADER, t->headerlist);

    return 0;
}

void ckc_transport_free(ckc_transport_t *t)
{
    curl_easy_cleanup(t->curl);
    curl_formfree(t->formpost);
    curl_slist_free_all(t->headerlist);
    free(t);
}

