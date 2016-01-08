#include <parg.h>
#include <parwin.h>
#include <sds.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <par/par_shapes.h>

#define TOKEN_TABLE(F)          \
    F(PLATONIC, "platonic")     \
    F(P_TEXTURE, "p_texture")   \
    F(P_SIMPLE, "p_simple")     \
    F(A_POSITION, "a_position") \
    F(A_TEXCOORD, "a_texcoord") \
    F(A_NORMAL, "a_normal")     \
    F(U_MVP, "u_mvp")           \
    F(U_IMV, "u_imv")           \
    F(S_SIMPLE, "shapes.glsl")
TOKEN_TABLE(PARG_TOKEN_DECLARE);

const int NSTATES = 7;

Matrix4 projection;
Matrix4 model;
Matrix4 view;
parg_mesh* mesh;
parg_texture* aotex = 0;
int state = 6;
int dirty = 1;
parg_token scene = 0;

static void create_platonic_scene(char const* name)
{
    sds objpath = sdscatprintf(sdsempty(), "build/%s.obj", name);
    sds pngpath = sdscatprintf(sdsempty(), "build/%s.png", name);
    sds cmd = sdscatprintf(sdsempty(),
            "../aobaker/build/aobaker %s "
            "--outmesh %s "
            "--atlas %s "
            "--nsamples %d ",
            objpath, objpath, pngpath, 1024);

    // Generate the scene and export an OBJ.
    int slices = 32;
    float radius = 20;
    float normal[3] = {0, 1, 0};
    float center[3] = {0, 0, 0};
    par_shapes_mesh *a, *b;
    a = par_shapes_create_disk(radius, slices, center, normal);
    b = par_shapes_create_dodecahedron();
    par_shapes_unweld(b, true);
    par_shapes_translate(b, 0, 0.934, 0);
    par_shapes_merge(a, b);
    par_shapes_free_mesh(b);
    par_shapes_export(a, objpath);
    par_shapes_free_mesh(a);

    // Bake ambient occlusion; generate a new OBJ and a PNG.
    system(cmd);

    // Load up the new OBJ.
    mesh = parg_mesh_from_file(objpath);
    parg_mesh_compute_normals(mesh);
    parg_mesh_send_to_gpu(mesh);

    // Load up the new PNG.
    parg_buffer* aobuf = parg_buffer_from_file(pngpath);
    aotex = parg_texture_from_buffer(aobuf);
    parg_buffer_free(aobuf);

    // Cleanup.
    sdsfree(objpath);
    sdsfree(pngpath);
    sdsfree(cmd);
}

static void create_mesh()
{
    parg_mesh_free(mesh);
    if (scene == PLATONIC) {
        create_platonic_scene("platonic");
        return;
    }

    par_shapes_mesh* shape;
    if (state == 0) {
        shape = par_shapes_create_icosahedron();
        par_shapes_unweld(shape, true);
        par_shapes_compute_normals(shape);
    } else if (state == 1) {
        shape = par_shapes_create_subdivided_sphere(3);
    } else if (state == 2) {
        shape = par_shapes_create_parametric_sphere(10, 10);
    } else if (state == 3) {
        shape = par_shapes_create_rock(1, 3);
        par_shapes_compute_normals(shape);
    } else if (state == 4) {
        shape = par_shapes_create_rock(2, 3);
        par_shapes_compute_normals(shape);
    } else if (state == 5) {
        shape = par_shapes_create_trefoil_knot(20, 100, 0.1);
    } else if (state == 6) {
        shape = par_shapes_create_klein_bottle(20, 30);
        par_shapes_scale(shape, 0.1, 0.1, 0.1);
        float axis[3] = {1, 0, 0};
        par_shapes_rotate(shape, -PARG_PI * 0.5, axis);
    }
    mesh = parg_mesh_from_shape(shape);
    par_shapes_free_mesh(shape);
}

void init(float winwidth, float winheight, float pixratio)
{
    parg_state_clearcolor((Vector4){0.5, 0.5, 0.5, 0.0});
    parg_state_cullfaces(1);
    parg_state_depthtest(1);
    parg_shader_load_from_asset(S_SIMPLE);
    const float h = 1.0f;
    const float w = h * winwidth / winheight;
    const float znear = 4;
    const float zfar = 20;
    projection = M4MakeFrustum(-w, w, -h, h, znear, zfar);
    Point3 eye = {0, 2.2, 10};
    Point3 target = {0, 0, 0};
    Vector3 up = {0, 1, 0};
    view = M4MakeLookAt(eye, target, up);
    model = M4MakeIdentity();
}

void draw()
{
    if (dirty) {
        create_mesh();
        dirty = 0;
    }

    Matrix4 vp = M4Mul(projection, view);
    Matrix4 modelview = M4Mul(view, model);
    Matrix3 invmodelview = M4GetUpper3x3(modelview);
    Matrix4 mvp = M4Mul(projection, modelview);
    parg_draw_clear();
    parg_varray_bind(parg_mesh_index(mesh));
    parg_varray_enable(parg_mesh_coord(mesh), A_POSITION, 3, PARG_FLOAT, 0, 0);
    parg_varray_enable(parg_mesh_norml(mesh), A_NORMAL, 3, PARG_FLOAT, 0, 0);

    if (aotex) {
        assert(parg_mesh_uv(mesh));
        parg_varray_enable(parg_mesh_uv(mesh), A_TEXCOORD, 2, PARG_FLOAT, 0, 0);
        parg_texture_bind(aotex, 0);
        parg_shader_bind(P_TEXTURE);
    } else {
        parg_shader_bind(P_SIMPLE);
    }

    parg_uniform_matrix4f(U_MVP, &mvp);
    parg_uniform_matrix3f(U_IMV, &invmodelview);
    parg_uniform_matrix4f(U_MVP, &mvp);

    parg_draw_triangles_u16(0, parg_mesh_ntriangles(mesh));
    parg_varray_disable(A_NORMAL);
    parg_varray_disable(A_TEXCOORD);
}

void dispose()
{
    parg_shader_free(P_TEXTURE);
    parg_shader_free(P_SIMPLE);
    parg_mesh_free(mesh);
    parg_texture_free(aotex);
}

void input(parg_event evt, float code, float unused0, float unused1)
{
    int key = (char) code;
    if ((evt == PARG_EVENT_KEYPRESS && key == ' ') || evt == PARG_EVENT_UP) {
        state = (state + 1) % NSTATES;
        dirty = 1;
    }
}

int main(int argc, char* argv[])
{
    TOKEN_TABLE(PARG_TOKEN_DEFINE);

    if (argc > 1) {
        scene = parg_token_from_string(argv[1]);
    } else {
        printf("Spacebar to cycle the shape.\n");
    }

    parg_asset_preload(S_SIMPLE);
    parg_window_setargs(argc, argv);
    parg_window_oninit(init);
    parg_window_oninput(input);
    parg_window_ondraw(draw);
    parg_window_onexit(dispose);
    return parg_window_exec(512, 512, 1, 1);
}