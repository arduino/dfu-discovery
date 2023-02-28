
/*
// The original macros are defined as follows:
# define warnx(...) do {\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, "\n");\ } while (0)
# define warn(...) do {\
    fprintf(stderr, "%s: ", strerror(errno));\
    warnx(__VA_ARGS__); } while (0)
*/
// No-OP: do not report any warning
# define warnx(...)      do { } while(0)
# define warn(...)       do { } while(0)

# define errx(eval, ...) do {\
    warnx(__VA_ARGS__);\
    exit(eval); } while (0)
# define err(eval, ...) do {\
    warn(__VA_ARGS__);\
    exit(eval); } while (0)
