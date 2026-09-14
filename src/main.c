#include "main.h"

struct bar_manager g_bar_manager;
pid_t g_pid;
static char g_lock_file[256];

static void signal_handler(int sig) {
    (void)sig;
    omabar_cleanup();
    exit(0);
}

static int acquire_lockfile(void) {
    snprintf(g_lock_file, sizeof(g_lock_file),
             "/tmp/omabar_%s.lock", getenv("USER"));

    int fd = open(g_lock_file, O_CREAT | O_RDWR, 0600);
    if (fd < 0) return -1;

    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

void omabar_init(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    CGSetLocalEventsSuppressionInterval(0.0);
    g_pid = getpid();

    bar_manager_init(&g_bar_manager);
}

void omabar_begin(void) {
    bar_manager_begin(&g_bar_manager);
}

void omabar_cleanup(void) {
    bar_manager_destroy(&g_bar_manager);
}

int main(int argc, char *argv[]) {
    (void)argc;

    /* reject root */
    if (getuid() == 0) {
        fprintf(stderr, "omabar: refusing to run as root\n");
        return 1;
    }

    /* acquire lock */
    if (acquire_lockfile() < 0) {
        fprintf(stderr, "omabar: another instance is already running\n");
        return 1;
    }

    /* bootstrap */
    printf("omabar\n");
    omabar_init();
    omabar_begin();

    /* run loop (restored when the event loop module is implemented) */
    omabar_cleanup();
    return 0;
}
