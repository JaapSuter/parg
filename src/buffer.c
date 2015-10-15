#include <par.h>
#include "internal.h"
#include "pargl.h"
#include <stdlib.h>

struct par_buffer_s {
    char* data;
    int nbytes;
    par_buffer_type memtype;
    GLuint gpuhandle;
    char* gpumapped;
};

int par_buffer_gpu_check(par_buffer* buf)
{
    return buf->memtype == PAR_GPU_ARRAY || buf->memtype == PAR_GPU_ELEMENTS;
}

GLuint par_buffer_gpu_handle(par_buffer* buf) { return buf->gpuhandle; }

par_buffer* par_buffer_alloc(int nbytes, par_buffer_type memtype)
{
    par_buffer* retval = malloc(sizeof(struct par_buffer_s));
    retval->data = (memtype == PAR_CPU) ? malloc(nbytes) : 0;
    retval->nbytes = nbytes;
    retval->memtype = memtype;
    retval->gpuhandle = 0;
    retval->gpumapped = 0;
    if (par_buffer_gpu_check(retval)) {
        glGenBuffers(1, &retval->gpuhandle);
    }
    return retval;
}

void par_buffer_free(par_buffer* buf)
{
    if (!buf) {
        return;
    }
    if (par_buffer_gpu_check(buf)) {
        glDeleteBuffers(1, &buf->gpuhandle);
    } else {
        free(buf->data);
    }
    free(buf);
}

int par_buffer_length(par_buffer* buf)
{
    par_verify(buf, "Null buffer", 0);
    return buf->nbytes;
}

void* par_buffer_lock(par_buffer* buf, par_buffer_mode access)
{
    if (access == PAR_WRITE && par_buffer_gpu_check(buf)) {
        buf->gpumapped = malloc(buf->nbytes);
        return buf->gpumapped;
    }
    return buf->data;
}

void par_buffer_unlock(par_buffer* buf)
{
    if (buf->gpumapped) {
        GLenum target = buf->memtype == PAR_GPU_ARRAY ? GL_ARRAY_BUFFER
            : GL_ELEMENT_ARRAY_BUFFER;
        glBindBuffer(target, buf->gpuhandle);
        glBufferData(target, buf->nbytes, buf->gpumapped, GL_STATIC_DRAW);
        free(buf->gpumapped);
        buf->gpumapped = 0;
    }
}

par_buffer* par_buffer_from_file(const char* filepath)
{
    FILE* f = fopen(filepath, "rb");
    par_verify(f, "Unable to open file", filepath);
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    par_buffer* retval = par_buffer_alloc(fsize + 1, PAR_CPU);
    char* contents = par_buffer_lock(retval, PAR_READ);
    fread(contents, fsize, 1, f);
    fclose(f);
    contents[fsize] = 0;
    par_buffer_unlock(retval);
    return retval;
}

par_buffer* par_buffer_from_asset(par_token id)
{
    return par_asset_to_buffer(id);
}

par_buffer* par_buffer_from_path(const char* filename)
{
#if EMSCRIPTEN
    sds baseurl = par_asset_baseurl();
    sds fullurl = sdscat(sdsdup(baseurl), filename);
    par_buffer* retval = 0;
    printf("TODO: download %s here\n", fullurl);
    sdsfree(fullurl);
#else
    const char* execdir = par_file_whereami();
    sds fullpath = sdscat(sdsnew(execdir), filename);
    if (!par_file_is_local(fullpath)) {
        par_asset_download(filename, fullpath);
    }
    par_buffer* retval = par_buffer_from_file(fullpath);
    sdsfree(fullpath);
#endif
    return retval;
}

void par_buffer_gpu_bind(par_buffer* buf)
{
    par_verify(par_buffer_gpu_check(buf), "GPU buffer required", 0)
    GLenum target = buf->memtype == PAR_GPU_ARRAY ? GL_ARRAY_BUFFER
        : GL_ELEMENT_ARRAY_BUFFER;
    glBindBuffer(target, par_buffer_gpu_handle(buf));
}
