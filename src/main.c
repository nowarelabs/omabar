#include "main.h"

struct bar_manager g_bar_manager;
struct env_vars g_env_vars;
pid_t g_pid;

static int g_lock_fd = -1;
static char g_lock_file[256];

static void daemonize(void) {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) _exit(0);

    setsid();
    chdir("/");

    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if (devnull > STDERR_FILENO) close(devnull);
    }
}

static void signal_handler(int sig) {
    (void)sig;
    omabar_cleanup();
    exit(0);
}

static void setup_signal_handlers(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGPIPE, SIG_IGN);
}

static int acquire_lockfile(void) {
    snprintf(g_lock_file, sizeof(g_lock_file),
             OMABAR_LOCKFILE "%s" OMABAR_LOCKFILE_SUFFIX, getenv("USER"));

    g_lock_fd = open(g_lock_file, O_CREAT | O_RDWR, 0600);
    if (g_lock_fd < 0) return -1;

    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0,
    };

    if (fcntl(g_lock_fd, F_SETLK, &fl) != 0) {
        close(g_lock_fd);
        g_lock_fd = -1;
        return -1;
    }
    return 0;
}

void omabar_init(void) {
    setup_signal_handlers();
    env_vars_init(&g_env_vars);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    CGSetLocalEventsSuppressionInterval(0.0);
#pragma clang diagnostic pop

    g_pid = getpid();

    /* obtain the CGS connection used for all SLS calls */
    window_connection();

    bar_manager_init(&g_bar_manager);
}

void omabar_begin(void) {
    bar_manager_begin(&g_bar_manager);
}

void omabar_cleanup(void) {
    bar_manager_destroy(&g_bar_manager);
    env_vars_destroy(&g_env_vars);
    if (g_lock_fd >= 0) close(g_lock_fd);
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv; /* Nix-only configuration: no CLI argument parsing */

    /* reject root */
    if (is_root()) {
        fprintf(stderr, "omabar: refusing to run as root\n");
        return 1;
    }

    /* fork into the background */
    daemonize();

    /* acquire lock file so only one daemon can run.
       NOTE: must happen in the daemon process — POSIX fcntl record
       locks are not inherited across fork(), so acquiring before
       daemonize() would release the lock when the parent exits. */
    if (acquire_lockfile() < 0)
        return 1;

    /* bootstrap */
    omabar_init();

    omabar_begin();

    /* run loop */
    CFRunLoopRun();

    omabar_cleanup();
    return 0;
}