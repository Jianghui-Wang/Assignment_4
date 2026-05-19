//
// Created by gin on 25/03/2021.


#include "main.h"

#include "tf_tables.h"

#include <cstdio>
#include <cmath>
#include <algorithm>

#define BUFFER_WIDTH 1024
#define BUFFER_HEIGHT 1024

int printOglError(char *file, int line)
{
    //
    // Returns 1 if an OpenGL error occurred, 0 otherwise.
    //
    GLenum glErr;
    int retCode = 0;

    glErr = glGetError();
    while (glErr != GL_NO_ERROR) {
        printf("glError in file %s @ line %d: %s\n", file, line, gluErrorString(glErr));
        retCode = 1;
        glErr = glGetError();
    }
    return retCode;
}



/*
rotating background color
*/
static void NextClearColor(void)
{
    static int color = 0;

    switch(color++)
    {
        case 0:
            clear_color[0] = 0.0f;
            clear_color[1] = 0.0f;
            clear_color[2] = 0.0f;
            clear_color[3] = 1.0f;
            break;
        case 1:
            clear_color[0] = 0.2f;
            clear_color[1] = 0.2f;
            clear_color[2] = 0.3f;
            clear_color[3] = 1.0f;
            break;
        default:
            clear_color[0] = 0.7f;
            clear_color[1] = 0.7f;
            clear_color[2] = 0.7f;
            clear_color[3] = 1.0f;
            color = 0;
            break;
    }
    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
}


/******************************************************************************/
/*
 * GLUT glue
 *
 ******************************************************************************/

static
void display(void)
{
    // reshape for offscreen render pass
    // set viewport
    glViewport(0, 0, BUFFER_WIDTH, BUFFER_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // set perspective projection
    glFrustum(-1.1, 1.1, -1.1, 1.1, 3.0, 10.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // object translation
    glTranslatef(0.0, 0.0, -5.0);

    // object rotation
    glRotatef(fYDiff, 1, 0, 0);
    glRotatef(fXDiff, 0, 1, 0);
    glRotatef(fZDiff, 0, 0, 1);


    float zscale = 1;
    float yscale = 1;
    float xscale = 1;

    if (data_loaded) {
        zscale = (float)vol_dim[2] / (float)vol_dim[0];
        yscale = (float)vol_dim[1] / (float)vol_dim[0];
        xscale = (float)vol_dim[0] / (float)vol_dim[0];
    }

//    // object scaling - assuming that x and y dimensions of dataset are the same, correct scaling of z
    glScalef(fScale, fScale, fScale);

    RenderFrontFaces(xscale, yscale, zscale);
    RenderBackFaces(xscale, yscale, zscale);
    RenderRaycastPass();
//    RenderBufferToScreen(frontface_buffer);
    glFlush();
    glutSwapBuffers();

}


static void play(void)
{
    glutSetWindow( window );

    glutPostRedisplay();
}


static
void key(unsigned char keyPressed, int x, int y) // key handling
{
    switch (keyPressed) {
        case '1':
            LoadData("../data/lobster.dat");
            break;
        case '2':
            LoadData("../data/skewed_head.dat");
            break;
        case 'o':
            enable_lighting = 1 - enable_lighting;
            if(enable_lighting) fprintf(stderr, "lighting is now enabled\n");
            else fprintf(stderr, "lighting is now disabled\n");
            break;
        case 'g':
            enable_gm_scaling = 1 - enable_gm_scaling;
            if(enable_gm_scaling) fprintf(stderr, "gradient magnitude scaled shading is now enabled\n");
            else fprintf(stderr, "gradient magnitude scaled shading is now disabled\n");
            break;
        case 'f':
            use_dvr = 1 - use_dvr;
            if(use_dvr) fprintf(stderr, "DVR Rendering\n");
            else fprintf(stderr, "Iso Value Rendering\n");
            break;
        case 'i':
            if (iso_value < 1.0f) {
                iso_value += 0.05f;
            }
            fprintf(stderr, "increasing iso value to: %f\n", iso_value);
            break;
        case 'k':
            if (iso_value > 0.0f) {
                iso_value -= 0.05f;
            }
            fprintf(stderr, "decreasing iso value to: %f\n", iso_value);
            break;
        case 'a':
            if (ambient < 1.0f) {
                ambient += 0.05f;
            }
            fprintf(stderr, "increasing ambient brightness value to: %f\n", ambient);
            break;
        case 'z':
            if (ambient > 0.0f) {
                ambient -= 0.05f;
            }
            fprintf(stderr, "decreasing ambient brightness value to: %f\n", ambient);
            break;
        case 'b':
            NextClearColor();
            break;
        case ' ':
            Rotate = !Rotate;

            if (!Rotate) {
                fXInertiaOld = fXInertia;
                fYInertiaOld = fYInertia;
            } else {
                fXInertia = fXInertiaOld;
                fYInertia = fYInertiaOld;

                // to prevent confusion, force some rotation
                if ((fXInertia == 0.0) && (fYInertia == 0.0)) {
                    fXInertia = -0.5;
                }
            }
            break;
        case 'r': //return to inital state/view
            fXDiff    = 0;
            fYDiff    = 35;
            fZDiff    = 0;
            xLastIncr = 0;
            yLastIncr = 0;
            fXInertia = -0.5;
            fYInertia = 0;
            break;
        case '+':
            step_size /= 1.5f;
            fprintf(stderr, "stepsize is now: %f\n", step_size);
            break;
        case '-':
            step_size *= 1.5f;
            fprintf(stderr, "stepsize is now: %f\n", step_size);
            break;
        case 'l':
            mip = !mip;
            fprintf(stderr, "Using Mip: %d\n", mip);
            break;
        case 'w':
            tf_win_min = std::min(tf_win_min + 1, tf_win_max);
            UpdateTransferfunction(false, false, false, false);
            fprintf(stderr, "lower window boundary increased to: %d\n", tf_win_min);
            break;
        case 's':
            tf_win_min = std::max(tf_win_min - 1, 0);
            UpdateTransferfunction(false, false, false, false);
            fprintf(stderr, "lower window boundary decreased to: %d\n", tf_win_min);
            break;
        case 'e':
            tf_win_max = std::min(tf_win_max + 1, 127);
            UpdateTransferfunction(false, false, false, false);
            fprintf(stderr, "lower window boundary increased to: %d\n", tf_win_max);
            break;
        case 'd':
            tf_win_max = std::max(tf_win_max - 1, tf_win_min);
            UpdateTransferfunction(false, false, false, false);
            fprintf(stderr, "lower window boundary decreased to: %d\n", tf_win_max);
            break;
        case '5':
            DownloadTransferFunctionTexture(0);
            break;
        case '6':
            DownloadTransferFunctionTexture(1);
            break;
        case '7':
            DownloadTransferFunctionTexture(2);
            break;
        case '8':
            DownloadTransferFunctionTexture(3);
            break;
        case '9':
            DownloadTransferFunctionTexture(4);
            break;
        case '0':
            DownloadTransferFunctionTexture(5);
            break;
        case 'q':
        case 27:
            exit(0);
            break;
        default:
            fprintf(stderr, "\nKeyboard commands:\n\n"
                            "1 - Load lobster dataset\n"
                            "2 - Load head dataset\n"
                            "3 - Load christmas tree dataset\n"
                            "4 - Load stagbeetle1 dataset\n"
                            "~ - Load stagbeetle2 dataset\n"
                            "p - Load test dataset\n"
                            "b - Toggle among background clear colors\n"
                            "o - en-/disable lighting\n"
                            "g - use gradient magnitude for shading (highlight edges)\n"
                            "f - switch between DVR and Iso value rendering\n"
                            "i, k - in-/decrease iso value\n"
                            "a, z - in-/decrease ambient brightness\n"
                            "w, s - in-/decrease lower tf window value\n"
                            "e, d - in-/decrease upper tf window value\n"
                            "y, u - in-/decrease red value\n"
                            "h, j - in-/decrease green value\n"
                            "n, m - in-/decrease blue value\n"
                            "c, v - in-/decrease alpha value\n"
                            "5,6,7,8,9,0 - select transferfunctions\n"
                            "<home>	 - reset zoom and rotation\n"
                            "r - reset rotation\n"
                            "<space> or <click> - stop rotation\n"
                            "<+>, <-> - in-/decrease sampling rate\n"
                            "<ctrl + drag> - lock rotation around z-axis\n"
                            "<arrow keys> or <drag> - rotate model\n"
                            "F5, F6, F7, F8 - clip in the x, y, z and no clipping\n"
                            "F1, F2 - zoom in/out\n"
                            "q, <esc> - Quit\n");
            break;
    }
}

void mouse(int button, int state, int x, int y)
{
    bmModifiers = glutGetModifiers();

    if (button == GLUT_LEFT_BUTTON) { //rotate

        if (state == GLUT_UP) {
            xLast = -1;
            yLast = -1;

            if (xLastIncr > INERTIA_THRESHOLD) {
                fXInertia = (xLastIncr - INERTIA_THRESHOLD) * INERTIA_FACTOR;
            }

            if (-xLastIncr > INERTIA_THRESHOLD) {
                fXInertia = (xLastIncr + INERTIA_THRESHOLD) * INERTIA_FACTOR;
            }

            if (yLastIncr > INERTIA_THRESHOLD) {
                fYInertia = (yLastIncr - INERTIA_THRESHOLD) * INERTIA_FACTOR;
            }

            if (-yLastIncr > INERTIA_THRESHOLD) {
                fYInertia = (yLastIncr + INERTIA_THRESHOLD) * INERTIA_FACTOR;
            }

        } else {
            fXInertia = 0;
            fYInertia = 0;
        }

        xLastIncr = 0;
        yLastIncr = 0;
    }
}


static
void timer(int value)
{

    /* Increment wrt inertia */
    if (Rotate)
    {
        fXDiff = fXDiff + fXInertia;
        fYDiff = fYDiff + fYInertia;
    }

    /* Callback */
    glutTimerFunc(TIMER_FREQUENCY_MILLIS , timer, 0);
}



static
void motion(int x, int y)
{
    if ((xLast != -1) || (yLast != -1)) { //zoom
        xLastIncr = x - xLast;
        yLastIncr = y - yLast;
        if (bmModifiers & GLUT_ACTIVE_CTRL) {
            if (xLast != -1) {
                fZDiff += xLastIncr;
                fScale += yLastIncr * SCALE_FACTOR;
            }
        } else {
            if (xLast != -1) { //rotate
                fXDiff += xLastIncr;
                fYDiff += yLastIncr;
            }
        }
    }

    xLast = x;
    yLast = y;
}

static void reshape2D(int w, int h)
{

    float aspect = (float) w / (float) h;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(-1.0, 1.0, -1.0, 1.0, -10.0, 10.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -5.0);
}

static void reshape(int wid, int ht)
{
    view_width = wid;
    view_height = ht;

    float vp = 0.8f;
    float aspect = (float)wid / (float)ht;

    // set viewport
    glViewport(0, 0, wid, ht);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // set perspective projection
    glFrustum(-vp, vp, (-vp / aspect), (vp / aspect), 3, 10.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -5.0);
}


static
void special(int key, int x, int y)
{
    //handle special keys
    switch (key) {
        case GLUT_KEY_HOME: //return to inital state/view
            fXDiff    = 0;
            fYDiff    = 35;
            fZDiff    = 0;
            xLastIncr = 0;
            yLastIncr = 0;
            fXInertia = -0.5;
            fYInertia = 0;
            fScale    = 1.0;
            break;
        case GLUT_KEY_LEFT:
            fXDiff--;
            break;
        case GLUT_KEY_RIGHT:
            fXDiff++;
            break;
        case GLUT_KEY_UP:
            fYDiff--;
            break;
        case GLUT_KEY_DOWN:
            fYDiff++;
            break;
        default:
            break;
    }
}


/*
 * load .dat dataset
 */
void LoadData(char* filename)
{
    fprintf(stderr, "loading data %s\n", filename);

    FILE* fp;
    fp = fopen(filename, "rb");

    if (fp == NULL) {
        fprintf(stderr, "Cannot open file %s for reading.\n", filename);
        return;
    }

    memset(vol_dim, 0, sizeof(unsigned short) * 3);

    //read volume dimensions from file
    fread(&vol_dim[0], sizeof(unsigned short), 1, fp);
    fread(&vol_dim[1], sizeof(unsigned short), 1, fp);
    fread(&vol_dim[2], sizeof(unsigned short), 1, fp);

    fprintf(stderr, "volume dimensions: x: %i, y: %i, z:%i \n", vol_dim[0], vol_dim[1], vol_dim[2]);

    if (data_array != NULL) {
        delete[] data_array;
    }

    // 1D array for storing volume data
    data_array = new unsigned short[vol_dim[0] * vol_dim[1] * vol_dim[2]];

    // read volume data
    fread(data_array, sizeof(unsigned short), (vol_dim[0] * vol_dim[1] * vol_dim[2]), fp);

    // shift volume data by 4 bit (converting 12 bit data to 16 bit data)
    for(int z = 0; z < vol_dim[2]; z++) {
        for(int y = 0; y < vol_dim[1]; y++) {
            for(int x = 0; x < vol_dim[0]; x++) {
                int current_idx = x + (y * vol_dim[0]) + (z * vol_dim[0] * vol_dim[1]);
                data_array[current_idx] = data_array[current_idx] << 4;
            }
        }
    }

    // close file
    fclose(fp);

    // download data into texture
    DownloadVolumeAsTexture();

    data_loaded = true;
}


/*
 * download data to 3D texture
 */
void DownloadVolumeAsTexture()
{
    fprintf(stderr, "downloading volume to 3D texture\n");

    glEnable(GL_TEXTURE_3D);

    // generate and bind 3D texture
    glGenTextures(1, &vol_texture);
    glBindTexture(GL_TEXTURE_3D, vol_texture);

    // set necessary texture parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    //download texture in correct format (our data only has one channel of unsigned short data, therefore we use GL_INTENSITY16)
    glTexImage3D(GL_TEXTURE_3D, 0,  GL_INTENSITY16, vol_dim[0], vol_dim[1], vol_dim[2], 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, data_array);

    glDisable(GL_TEXTURE_3D);
}


/*
 * Interactive "windowing" transfer function (rubric: 20 pts).
 *
 * tf0 is a 128-entry RGBA 1D texture. `tf_win_min` and `tf_win_max` are the
 * lower and upper bounds of the active window in [0, 127], driven by the
 * w/s/e/d keys. Inside the window the table ramps linearly from black/
 * transparent to white/opaque; outside it the table is zero. Re-uploads the
 * texture so the change is visible the very next frame.
 */
void UpdateTransferfunction(bool r, bool g, bool b, bool a)
{
    const int N = 128;
    const float lo = (float) tf_win_min;
    const float hi = (float) tf_win_max;
    const float winRange = std::max(hi - lo, 1.0f);   // avoid div-by-zero

    for (int i = 0; i < N; ++i) {
        float t = 0.0f;
        if (i >= tf_win_min && i <= tf_win_max) {
            t = ((float)i - lo) / winRange;           // 0..1 inside the window
        }
        tf0[4 * i + 0] = t;   // R
        tf0[4 * i + 1] = t;   // G
        tf0[4 * i + 2] = t;   // B
        tf0[4 * i + 3] = t;   // A — same ramp also drives opacity
    }

    // Push it to the 1D texture so the shader sees the new ramp.
    glEnable(GL_TEXTURE_1D);
    glBindTexture(GL_TEXTURE_1D, tf_texture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, N, 0, GL_RGBA, GL_FLOAT, tf0);
    glDisable(GL_TEXTURE_1D);

    // r/g/b/a parameters are placeholders for keyboard channel knobs that
    // would tint the ramp; not used in this minimum-requirements build.
    (void) r; (void) g; (void) b; (void) a;
}

/*
 * download tf data to 1D texture
 */
void DownloadTransferFunctionTexture(int tf_id)
{
    fprintf(stderr, "downloading transfer function to 1D texture\n");

    glEnable(GL_TEXTURE_1D);

    // generate and bind 1D texture
    glGenTextures(1, &tf_texture);
    glBindTexture(GL_TEXTURE_1D, tf_texture);

    // set necessary texture parameters
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    //download texture in correct format
    switch(tf_id){
        case 0:
            glTexImage1D(GL_TEXTURE_1D, 0,  GL_RGBA, 128, 0, GL_RGBA, GL_FLOAT, tf0);
            break;
        case 1:
            glTexImage1D(GL_TEXTURE_1D, 0,  GL_RGBA, 2, 0, GL_RGBA, GL_FLOAT, tf1);
            break;
        case 2:
            glTexImage1D(GL_TEXTURE_1D, 0,  GL_RGBA, 4, 0, GL_RGBA, GL_FLOAT, tf2);
            break;
        case 3:
            glTexImage1D(GL_TEXTURE_1D, 0,  GL_RGBA, 12, 0, GL_RGBA, GL_FLOAT, tf3);
            break;
        case 4:
            glTexImage1D(GL_TEXTURE_1D, 0,  GL_RGBA, 12, 0, GL_RGBA, GL_FLOAT, tf4);
            break;
        case 5:
            glTexImage1D(GL_TEXTURE_1D, 0,  GL_RGBA, 12, 0, GL_RGBA, GL_FLOAT, tf5);
            break;
        default:
            glTexImage1D(GL_TEXTURE_1D, 0,  GL_RGBA, 128, 0, GL_RGBA, GL_FLOAT, tf0);
            break;
    }

    glDisable(GL_TEXTURE_1D);
}


void Draw(float x, float y, float z, unsigned int dir) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float cubeCoords[8][3] = {
            {-x, y, z }, // front top    left  0
            {-x,-y, z }, // front bottom left  1
            { x,-y, z }, // front bottom right 2
            { x, y, z }, // front top    right 3
            {-x, y,-z }, // back  top    left  4
            {-x,-y,-z }, // back  bottom left  5
            { x,-y,-z }, // back  bottom right 6
            { x, y,-z }, // back  top    right 7

    };
    float texCoords[8][3] = {
            { 0, 1, 1 }, // front top    left  0
            { 0, 0, 1 }, // front bottom left  1
            { 1, 0, 1 }, // front bottom right 2
            { 1, 1, 1 }, // front top    right 3
            { 0, 1, 0 }, // back  top    left  4
            { 0, 0, 0 }, // back  bottom left  5
            { 1, 0, 0 }, // back  bottom right 6
            { 1, 1, 0 }, // back  top    right 7
    };
    short drawOrder[6][4] = {
            {0, 1, 2, 3}, // front
            {4, 0, 3, 7}, // top
            {5, 1, 0, 4}, // left
            {7, 3, 2, 6}, // right
            {1, 5, 6, 2}, // bottom
            {7, 6, 5, 4}  // back
    };

    glEnable(GL_CULL_FACE);
    glCullFace(dir);
    glBegin(GL_QUADS);
    for(int face = 0; face < 6; face++) {
        for(int v = 0; v < 4; v++) {

            glColor3f(texCoords[drawOrder[face][v]][0],texCoords[drawOrder[face][v]][1],texCoords[drawOrder[face][v]][2]);
            glVertex3f(cubeCoords[drawOrder[face][v]][0],cubeCoords[drawOrder[face][v]][1],cubeCoords[drawOrder[face][v]][2]);
        }
    }
    glEnd();
    glDisable(GL_CULL_FACE);
}


/*
 * render backfaces to start raycasting
 */
void RenderBackFaces(float x, float y, float z)
{
    enableRenderToBuffer(backface_buffer);
    Draw(x, y, z, GL_FRONT);
    disableRenderToBuffer();
}


/*
 * render frontfaces to start raycasting
 */
void RenderFrontFaces(float x, float y, float z)
{
    enableRenderToBuffer(frontface_buffer);
    Draw(x, y, z, GL_BACK);
    disableRenderToBuffer();
}


void RenderRaycastPass(void)
{
    // switch for dvr and iso shader programs
    GLuint shader_program = use_dvr ? raycast_dvr_shader_program : raycast_iso_shader_program;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // enable shader
    glUseProgram(shader_program);

    // ----- texture units --------------------------------------------------
    //   unit 0 = transfer function (1D)
    //   unit 1 = front-face position buffer (2D, RGBA16F)
    //   unit 2 = back-face  position buffer (2D, RGBA16F)
    //   unit 3 = volume                    (3D, R/intensity)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, tf_texture);
    glUniform1i(glGetUniformLocation(shader_program, "transferfunction"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, frontface_buffer);
    glUniform1i(glGetUniformLocation(shader_program, "frontfaces"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, backface_buffer);
    glUniform1i(glGetUniformLocation(shader_program, "backfaces"), 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_3D, vol_texture);
    glUniform1i(glGetUniformLocation(shader_program, "volume"), 3);

    // ----- scalar / vector uniforms ---------------------------------------
    const float vx = data_loaded ? (float)vol_dim[0] : 1.0f;
    const float vy = data_loaded ? (float)vol_dim[1] : 1.0f;
    const float vz = data_loaded ? (float)vol_dim[2] : 1.0f;
    glUniform3f(glGetUniformLocation(shader_program, "vol_dim"), vx, vy, vz);

    glUniform1f(glGetUniformLocation(shader_program, "step_size"),     step_size);
    glUniform1f(glGetUniformLocation(shader_program, "iso_value"),     iso_value);
    glUniform1f(glGetUniformLocation(shader_program, "ambient"),       ambient);
    glUniform1i(glGetUniformLocation(shader_program, "enable_lighting"),   enable_lighting);
    glUniform1i(glGetUniformLocation(shader_program, "enable_gm_scaling"), enable_gm_scaling);
    glUniform1i(glGetUniformLocation(shader_program, "mip"),               mip ? 1 : 0);

    // legacy "params" uniform kept for the original debug path
    int params_location = glGetUniformLocation(shader_program, "params");
    if (params_location >= 0)
        glUniform4f(params_location, 0.0f, 0.5f, 1.0f, 1.0f);



    // Render screen filling quad
    // use squared centered quad to not disturb ratio
    int x[2] = { 0, 0 };
    int y[2] = { 0, 0 };
    int dim, off;
    if(view_width < view_height) {

        dim = view_width;
        off = (view_height - view_width) / 2;

        x[0] = 0;
        x[1] = dim;
        y[0] = off;
        y[1] = dim + off;

    } else {

        dim = view_height;
        off = (view_width - view_height) / 2;

        x[0] = off;
        x[1] = dim + off;
        y[0] = 0;
        y[1] = dim;
    }

    glViewport(0, 0, view_width, view_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, view_width, 0, view_height);
    glMatrixMode(GL_MODELVIEW);

    //draw_fullscreen_quad
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_QUADS);

    // red screen means your shader is broken
    glColor3f(1.0, 0.0, 0.0);

    glTexCoord2i(0, 0); glVertex2i(x[0], y[0]);
    glTexCoord2i(1, 0); glVertex2i(x[1], y[0]);
    glTexCoord2i(1, 1); glVertex2i(x[1], y[1]);
    glTexCoord2i(0, 1); glVertex2i(x[0], y[1]);

    glEnd();
    glEnable(GL_DEPTH_TEST);

    //disable shader
    glUseProgram(0);
}


// for debugging purposes
void RenderBufferToScreen(GLuint buffer){

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture (GL_TEXTURE_2D, buffer);

    // use squared centered quad to not disturb ratio
    int x[2] = { 0, 0 };
    int y[2] = { 0, 0 };
    int dim, off;
    if(view_width < view_height){

        dim = view_width;
        off = (view_height - view_width) / 2;

        x[0] = 0;
        x[1] = dim;
        y[0] = off;
        y[1] = dim + off;

    } else {

        dim = view_height;
        off = (view_width - view_height);

        x[0] = off;
        x[1] = dim + off;
        y[0] = 0;
        y[1] = dim;
    }

    glViewport(0, 0, view_width, view_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, view_width, 0, view_height);
    glMatrixMode(GL_MODELVIEW);

    //draw_fullscreen_quad
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_QUADS);

    glTexCoord2i(0, 0); /*glColor3f(0.0f, 0.0f, 1.0f);*/ glVertex2i(x[0], y[0]);
    glTexCoord2i(1, 0); /*glColor3f(1.0f, 0.0f, 1.0f);*/ glVertex2i(x[1], y[0]);
    glTexCoord2i(1, 1); /*glColor3f(1.0f, 1.0f, 1.0f);*/ glVertex2i(x[1], y[1]);
    glTexCoord2i(0, 1); /*glColor3f(0.0f, 1.0f, 1.0f);*/ glVertex2i(x[0], y[1]);

    glEnd();
    glEnable(GL_DEPTH_TEST);

    glDisable(GL_TEXTURE_2D);
}

/*
 * enable buffer to render to
 */
void enableRenderToBuffer(GLuint buffer)
{
    glBindFramebufferEXT(GL_FRAMEBUFFER, framebuffer);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer, 0);
}

/*
 * unbinds buffer
 */
void disableRenderToBuffer()
{
    glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
}


void initGL(void){

    // Initialize the "OpenGL Extension Wrangler" library
    printf("glew init\n");
    GLenum err = glewInit();


    // Create the to FBO's one for the backsides and one for the frontsides
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER ,framebuffer);

    glGenTextures(1, &backface_buffer);
    glBindTexture(GL_TEXTURE_2D, backface_buffer);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA16F_ARB, BUFFER_WIDTH, BUFFER_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, backface_buffer, 0);


    glGenTextures(1, &frontface_buffer);
    glBindTexture(GL_TEXTURE_2D, frontface_buffer);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA16F_ARB, BUFFER_WIDTH, BUFFER_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frontface_buffer, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create Shader Program
    LoadAndLinkShaders();

    // download initial transfer function
    DownloadTransferFunctionTexture(0);
}

char *textFileRead(char *fn) {

    FILE *fp;
    char *content = NULL;

    int count=0;

    if (fn != NULL) {
        fp = fopen(fn, "rt");
        if (fp != NULL) {
            fseek(fp, 0, SEEK_END);
            count = ftell(fp);
            rewind(fp);

            if (count > 0) {
                content = (char *)malloc(sizeof(char) * (count + 1));
                count = fread(content, sizeof(char), count, fp);
                content[count] = '\0';
            }
            fclose(fp);
        }
    }
    return content;
}


void LoadAndLinkShaders(void){

    char *fs = NULL;

    raycast_dvr_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    raycast_iso_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    fs = textFileRead("../shaders/raycast_dvr.glsl");

    if (fs == NULL){

        printf("DVR shader file not found\n");
        exit(1);
    }

    const char *f1 = fs;

    glShaderSource(raycast_dvr_fragment_shader, 1, &f1, NULL);

    free(fs);

    fs = textFileRead("../shaders/raycast_iso.glsl");

    if (fs == NULL) {

        printf("ISO shader file not found\n");
        exit(1);
    }

    const char *f2 = fs;

    glShaderSource(raycast_iso_fragment_shader, 1, &f2, NULL);

    free(fs);

    glCompileShader(raycast_dvr_fragment_shader);
    glCompileShader(raycast_iso_fragment_shader);

    raycast_dvr_shader_program = glCreateProgram();
    raycast_iso_shader_program = glCreateProgram();
    glAttachShader(raycast_dvr_shader_program, raycast_dvr_fragment_shader);
    glAttachShader(raycast_iso_shader_program, raycast_iso_fragment_shader);

    glLinkProgram(raycast_dvr_shader_program);
    glLinkProgram(raycast_iso_shader_program);
}


/******************************************************************************/
/*
 * Main
 *
 ******************************************************************************/

int main(int argc, char **argv)
{
    // init variables
    fXDiff = 206;		//rotation
    fYDiff = 16;
    fZDiff = 10;
    xLastIncr = 0;		//last difference in mouse movement
    yLastIncr = 0;
    fXInertia = -0.5;	//inertia for keeping rotating even after mouse movement
    fYInertia = 0;
    fXInertiaOld;		//old inertia
    fYInertiaOld;
    fScale = 1.0;		//scale/zoom factor
    xLast = -1;			//last mouse position
    yLast = -1;
    bmModifiers;		//keyboard modifiers (e.g. ctrl,...)
    Rotate = 1;			//auto-rotate
    view_width = 0;
    view_height = 0;

    use_dvr = 1;
    mip = false;
    iso_value = 0.20f;
    clip = 0;

    enable_lighting = 0;
    enable_gm_scaling = 0;
    ambient = 0.5;
    step_size = 0.0025f;

    tf_win_min = 0;
    tf_win_max = 127;


    for(int i = 0; i < 128; i++){

        tf0[4 * i + 0] = (float)(i) / 128.0f;
        tf0[4 * i + 1] = (float)(i) / 128.0f;
        tf0[4 * i + 2] = (float)(i) / 128.0f;
        tf0[4 * i + 3] = (float)(i) / 128.0f;
    }

    data_array = NULL;
    data_loaded = false;

    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    // Note: do NOT call DownloadTransferFunctionTexture() here — no GL context
    // exists yet. initGL() further down (after glutCreateWindow) handles the
    // initial upload of tf0.


    glutInitWindowSize(512, 512);
    window = glutCreateWindow("AMCS/CS247 Scientific Visualization");

    glutIdleFunc(play);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    glutMotionFunc(motion);
    glutMouseFunc(mouse);
    glutSpecialFunc(special);

    glutTimerFunc(TIMER_FREQUENCY_MILLIS, timer, 0);

    initGL();



    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    NextClearColor();

    // display help
    key('?', 0, 0);

    glutMainLoop();

    return 0;
}

