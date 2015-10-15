#include <par.h>
#include <string.h>
#include "internal.h"

static sds _exedir = 0;

int par_file_is_http(const char* locator)
{
	return strstr(locator, "http://") == locator ||
		strstr(locator, "https://") == locator;
}

#if EMSCRIPTEN

const char* par_file_whereami()
{
    if (!_exedir) {
        _exedir = sdsnew("web/");
    }
    return _exedir;
}

int par_file_is_local(const char* fullpath) { return 1; }

int par_file_local_to_memory(const char* filepath, par_byte** buffer, int* len)
{
    return 0;
}

int par_file_http_to_memory(const char* url, par_byte** buffer, int* len)
{
    return 0;
}

int par_file_http_to_local(const char* srcurl, const char* dstpath)
{
    return 0;
}

#else

#include "whereami.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

void* kopen(const char* fn, int* _fd);
int kclose(void* a);

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

int par_file_is_local(const char* fullpath)
{
    return access(fullpath, F_OK) != -1;
}

int par_file_local_to_memory(const char* filepath, par_byte** buffer, int* len)
{
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    par_byte* content = malloc(fsize + 1);
    fread(content, fsize, 1, f);
    fclose(f);
    content[fsize] = 0;
    *buffer = content;
    *len = (int) fsize;
    return 1;
}

int par_file_http_to_local(const char* srcurl, const char* dstpath)
{
    int source = 0;
    printf("Downloading %s...\n", srcurl);
    void* kdfile = kopen(srcurl, &source);
    if (!source) {
printf("%s:%d\n", __FILE__, __LINE__);
        return 0;
    }
    int dest = open(dstpath, O_WRONLY | O_CREAT, 0644);
    char buf[BUFSIZ];
    size_t size;
    size_t total = 0;
    size_t elapsed = 0;
    while ((size = read(source, buf, BUFSIZ)) > 0) {
        if (size > BUFSIZ) {
            close(dest);
            kclose(kdfile);
            remove(dstpath);
printf("%s:%d %zu\n", __FILE__, __LINE__, size);
            return 0;
        }
        write(dest, buf, size);
        total += size;
        elapsed += size;
        if (elapsed > 1024 * 1024) {
            printf("\t%zu bytes so far...\n", total);
            elapsed = 0;
        }
    }
    printf("\t%zu bytes total.\n", total);
    close(dest);
    kclose(kdfile);
    return 0;
}

int par_file_http_to_memory(const char* url, par_byte** buffer, int* len)
{
	if (!par_file_http_to_local(url, "tmp")) {
		return 0;
	}
	int success = par_file_local_to_memory("tmp", buffer, len);
	remove("tmp");
	return success;

/*
    int source = 0;
    printf("Downloading %s...\n", url);
    void* kdfile = kopen(url, &source);
    if (!source) {
        return 0;
    }
	size_t total = lseek(source, 0, SEEK_END);
	if (!total) {
		return 0;
	}
    printf("\t%zu bytes total.\n", total);
	lseek(source, 0, SEEK_SET);
	*buffer = malloc(total);
	par_byte* writeptr = *buffer;
    size_t size;
    size_t elapsed = 0;
    while ((size = read(source, writeptr, BUFSIZ)) > 0) {
        if (size > BUFSIZ) {
            kclose(kdfile);
            return 0;
        }
		writeptr += size;
        elapsed += size;
        if (elapsed > 1024 * 1024) {
            printf("\t%zu bytes so far...\n", total);
            elapsed = 0;
        }
    }
    kclose(kdfile);
    return 0;
*/
}

#endif
