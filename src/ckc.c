
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


int main(int argc, char *const *argv)
{
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
    ckc_prompt_username(&t->username);
    ckc_prompt_password(&t->password);
    ckc_transport_list_accounts(t, NULL);
    ckc_transport_free(t);
    return 0;
}


