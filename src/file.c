#include <par.h>
#include "internal.h"

static sds _exedir = 0;

#if EMSCRIPTEN

const char* par_file_whereami()
{
    if (!_exedir) {
        _exedir = sdsnew("web/");
    }
    return _exedir;
}

int par_file_is_local(const char* fullpath) { return 1; }

#else

#include "whereami.h"
#include <fcntl.h>
#include <unistd.h>

const char* par_file_whereami()
{
    if (!_exedir) {
        int length = wai_getExecutablePath(0, 0, 0);
        _exedir = sdsnewlen("", length);
        int dirlen;
        wai_getExecutablePath(_exedir, length, &dirlen);
        sdsrange(_exedir, 0, dirlen);
    }
    return _exedir;
}

int par_file_is_local(const char* fullpath) { return access(fullpath, F_OK) != -1; }

#endif
