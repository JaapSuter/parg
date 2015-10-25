
// @program p_simple, vertex, fragment

uniform mat4 u_mvp;
uniform vec3 u_eyepos;

-- vertex

attribute vec4 a_position;
attribute float a_vertexid;

const vec4 OUTSIDE_FRUSTUM = vec4(2, 2, 2, 1);

void main()
{
    vec4 p = a_position;
    p.xy -= 0.5;
    p.y *= -1.0;
    gl_Position = u_mvp * p;
    gl_PointSize = 2.0;
    if (a_vertexid > 10000.0 && u_eyepos.z > -1.0) {
        gl_Position = OUTSIDE_FRUSTUM;
    }
}

-- fragment

void main()
{
    gl_FragColor = vec4(0.2, 0.2, 0.2, 1.0);
}
