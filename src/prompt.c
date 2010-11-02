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

static void ckc_nuke_newlines(char *p)
{
    size_t i;
    size_t l = strlen(p);
    for (i = 0; i < l; i++) {
        if (p[i] == '\n') {
            p[i] = '\0';
        }
        if (p[i] == '\r') {
            p[i] = '\0';
        }
    }
}

int ckc_prompt_username(const char **username)
{
    char buf[256] = {0};
    fprintf(stdout, "Username: ");
    fflush(stdout);
    char *p = fgets(&buf[0], sizeof(buf), stdin);

    if (p == NULL) {
        return -1;
    }

    ckc_nuke_newlines(p);

    if (*p == '\0') {
      return -1;
    }

    *username = strdup(p);

    return 0;
}

int ckc_prompt_password(const char **password, const char *prompt)
{
	char string[256] = {0};
	snprintf(string, sizeof(string), "%s: ", prompt);
    char *p = getpass(string);

    if (p == NULL || *p == '\0') {
        return -1;
    }

    *password = strdup(p);

    return 0;
}

int ckc_prompt_number(int *num, int min, int max)
{
    char buf[256] = {0};
    fprintf(stdout, "Select %d to %d: ", min, max);
    fflush(stdout);
    char *p = fgets(&buf[0], sizeof(buf), stdin);
    if (p == NULL) {
        return -1;
    }
    *num = atoi(p);
    if (*num > max) {
        return ckc_prompt_number(num, min, max);
    }
    if (*num < min) {
        return ckc_prompt_number(num, min, max);
    }
    return 0;
}

int ckc_prompt_yn()
{
    char buf[256] = {0};
    fflush(stdout);
    char *p = fgets(&buf[0], sizeof(buf), stdin);
    if (p == NULL) {
        return -1;
    }

    ckc_nuke_newlines(p);

    if (strcasecmp(p, "y") == 0 ||
        strcasecmp(p, "yes") == 0 ||
        strcasecmp(p, "fuckyeah") == 0) { /* easteregg for soemone who reads commit mails */
        return 0;
    }

    return 1;
}

