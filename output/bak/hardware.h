#ifndef HARDWARE_H
#define HARDWARE_H

#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
//#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>

class hardware : public QOpenGLFunctions
{

public:
    hardware();

    void initialize();

    //! 纹理对象
    static uint texture_id(const QSize &size,
                           uint iformat, uint format, uint type,
                           int min_filter, int mag_filter,
                           uchar *data);
    static void texture_up(uint id, const QSize &size, uint format, uchar *data);
    static void texture_del(uint id);

    //! OpenGL扩展函数
    static void glMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t);
    static void glWindowPos2i(uint x, uint y);
    static GLvoid *glMapBuffer(GLenum target, GLenum access);
    static GLboolean glUnmapBuffer(GLenum target);

    //! 检测错误
    static bool check_error(const QString& where = QString()) ;
    bool check_FBO(const QString& where = QString());

    //! 着色器对象
    GLuint shader_compile(const QString& name, GLenum type, const QString& src) ;
    GLuint program_create(GLuint vshader, GLuint fshader) ;
    GLuint program_create(const QString& name,
                            const QString& vshader_src, const QString& fshader_src) ;
    void program_link(const QString& name, const GLuint prg);
    void program_delete(GLuint prg) ;

    //! 是否支持srgb8纹理渲染
    bool srgb8_textures_color_renderable();
};

#endif // HARDWARE_H
