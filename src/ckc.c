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

static void show_version()
{
    fprintf(stdout, "cloudkick-config - %d.%d.%d\n",
            CKC_VERSION_MAJOR, CKC_VERSION_MINOR, CKC_VERSION_PATCH);
    exit(EXIT_SUCCESS);
}

static void show_help()
{
    fprintf(stdout, "cloudkick-config - Generates /etc/cloudkick.conf\n");
    fprintf(stdout, "  Usage:  \n");
    fprintf(stdout, "    ckl [-h|-V]\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "     -h          Show Help message\n");
    fprintf(stdout, "     -V          Show Version number\n");
    exit(EXIT_SUCCESS);
}

void ckc_error_out(const char *msg)
{
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(EXIT_FAILURE);
}

static void write_config(FILE *fp, const char *key, const char *secret)
{
    fprintf(fp, "#\n");
    fprintf(fp, "# Cloudkick Congfiguration\n");
    fprintf(fp, "#\n");
    fprintf(fp, "# See the following URL for the most up to date documentation:\n");
    fprintf(fp, "#   https://support.cloudkick.com/Agent/Cloudkick.conf\n");
    fprintf(fp, "#\n");
    fprintf(fp, "# The keys in cloudkick.conf are tied to your entire account,\n");
    fprintf(fp, "# so you can deploy the same file across all of your machines.\n");
    fprintf(fp, "#\n");
    fprintf(fp, "# oAuth consumer key\n");
    fprintf(fp, "oauth_key %s\n", key);
    fprintf(fp, "# oAuth consumer secret\n");
    fprintf(fp, "oauth_secret %s\n", secret);
    fprintf(fp, "# Path to a directory containing custom agent plugins\n");
    fprintf(fp, "local_plugins_path /usr/lib/cloudkick-agent/plugins/\n");
}

int main(int argc, char *const *argv)
{
    int happy = 1;
    const char *account = NULL;
    const char *password = NULL;
    const char *username = NULL;
    const char *mfa_ssid = NULL;
    int rv;
    ckc_transport_t *t;
    int c;

    curl_global_init(CURL_GLOBAL_ALL);

    while ((c = getopt(argc, argv, "hVslm:d:")) != -1) {
        switch (c) {
            case 'V':
                show_version();
                break;
            case 'h':
                show_help();
                break;
            case '?':
                ckc_error_out("See -h for correct options");
                break;
        }
    }

    curl_global_cleanup();

    t = calloc(1, sizeof(ckc_transport_t));

    ckc_transport_init(t);

    fprintf(stdout, "Welcome to the Cloudkick configuration utility. This will securely \n");
    fprintf(stdout, "use your Cloudkick username and password to generate an API key \n");
    fprintf(stdout, "for the Cloudkick Agent.\n\n");

    rv = ckc_prompt_username(&username);
    if (rv < 0) {
        ckc_error_out("error reading username");
    }

    rv = ckc_prompt_password(&password, "Password");
    if (rv < 0) {
        ckc_error_out("error reading password");
    }

    t->username = username;
    t->password = password;
    ckc_accounts_t *a;

    rv = ckc_transport_list_accounts(t, &a, &mfa_ssid);

    if (rv < 0) {
        ckc_error_out("error listing accounts");
    }

    account = a->head->s;
    ckc_ll_t *l;

    if (a->count > 1) {
        int i = 1;
        int t = 0;
        //ckc_error_out("multple accounts");
        fprintf(stdout, "Please select which Cloudkick Account to use:\n");
        for (l = a->head; l != NULL; l = l->next, i++) {
            fprintf(stdout, "  [%d]  %s\n", i, l->s);
        }
        ckc_prompt_number(&t, 1, i-1);
        for (l = a->head, i = 0; l != NULL; l = l->next, i++) {
            if (i == t-1) {
                account = l->s;
                break;
            }
        }
    }

    ckc_transport_free(t);

    t = calloc(1, sizeof(ckc_transport_t));

    ckc_transport_init(t);

    l = NULL;
    t->username = username;
    t->password = password;

    if (mfa_ssid != NULL) {
        t->mfa_ssid = mfa_ssid;
    }

    rv = ckc_transport_get_consumer(t, account, &l);

    if (rv < 0) {
        ckc_error_out("error getting api key");
    }

    const char *secret = l->s;
    const char *key = l->next->s;

#define CONF "/etc/cloudkick.conf"
    FILE *fp = stdout;
    if (access(CONF,  F_OK) == 0) {
        fprintf(stdout, "/etc/cloudkick.conf already exists!\n");
        fprintf(stdout, "Overwrite [y/n] ?: ");
        rv = ckc_prompt_yn();
        if (rv < 0 || rv == 1) {
            ckc_error_out("Config file already exists");
        }
    }

    fp = fopen(CONF, "w");
    if (fp == NULL) {
        fprintf(stderr, "Unable to open "CONF" for writing!\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Would of written:\n");
        fp = stdout;
        happy = 0;
    }
    else {
        fprintf(stdout, "Writing configuration to "CONF"\n");
    }

    write_config(fp, key, secret);

    if (happy) {
        fprintf(stdout, "All done!\n");
    }
    fprintf(stdout, "\n");

    ckc_transport_free(t);

    return 0;
}
