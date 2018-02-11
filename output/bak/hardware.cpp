#include "hardware.h"

void (QOPENGLF_APIENTRYP MultiTexCoord2f)(GLenum target, GLfloat s, GLfloat t);
void (QOPENGLF_APIENTRYP WindowPos2i)(GLuint x, GLuint y);
GLvoid *(QOPENGLF_APIENTRYP MapBuffer)(GLenum target, GLenum access);
GLboolean (QOPENGLF_APIENTRYP UnmapBuffer)(GLenum target);

hardware::hardware():
    QOpenGLFunctions()
{
    MultiTexCoord2f = 0;
    WindowPos2i = 0;
    MapBuffer = 0;
    UnmapBuffer = 0;
}
void hardware::initialize()
{
    QOpenGLFunctions::initializeOpenGLFunctions();
}
static void xglKillCrlf(char* str)
{
    size_t l = strlen(str);
    if (l > 0 && str[l - 1] == '\n')
        str[--l] = '\0';
    if (l > 0 && str[l - 1] == '\r')
        str[l - 1] = '\0';
}

uint hardware::texture_id(const QSize &size,
                          uint iformat, uint format , uint type,
                          int min_filter, int mag_filter,
                          uchar *data)
{
    GLuint id;
    ::glEnable(GL_TEXTURE_2D);
    ::glGenTextures(1, &id);
    ::glBindTexture(GL_TEXTURE_2D, id);

    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (size.width() < 3 )
    {
        ::glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        ::glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    ::glTexImage2D(GL_TEXTURE_2D, 0, iformat,
               size.width(),
               size.height(), 0,
               format,
               type, data);

    //::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ::glBindTexture(GL_TEXTURE_2D, 0);
    ::glDisable(GL_TEXTURE_2D);
        return id;
}
void hardware::texture_up(uint id, const QSize &size, uint format, uchar *data)
{
    ::glEnable(GL_TEXTURE_2D);
    ::glBindTexture(GL_TEXTURE_2D, id);
    ::glTexSubImage2D(GL_TEXTURE_2D, 0 , 0, 0, size.width(), size.height(),
                      format, GL_UNSIGNED_BYTE, data);
    ::glBindTexture(GL_TEXTURE_2D, 0);
    ::glDisable(GL_TEXTURE_2D);
}
void hardware::texture_del(uint id)
{
    ::glDeleteTextures(1, &id);
}

bool hardware::check_error(const QString& where)
{
    GLenum e = glGetError();
    if (e != GL_NO_ERROR)
    {
        QString pfx = (where.length() > 0 ? where + ": " : "");
        throw pfx + QString("OpenGL error 0x")+ QString::number(static_cast<unsigned int>(e), 16);
        return false;
    }
    return true;
}
bool hardware::check_FBO(const QString& where)
{
    GLenum e = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (e != GL_FRAMEBUFFER_COMPLETE)
    {
        QString pfx = (where.length() > 0 ? where + ": " : "");
        throw pfx + QString("OpenGL Framebuffer status error 0x")+ QString::number(static_cast<unsigned int>(e), 16);
        return false;
    }
    return true;
}
GLuint hardware::shader_compile(const QString& name, GLenum type, const QString& src)
{
    //msg::dbg("Compiling %s shader %s.", type == GL_VERTEX_SHADER ? "vertex" : "fragment", name.toLatin1().constData());

    GLuint shader = glCreateShader(type);
    const GLchar* glsrc = src.toLatin1().constData();
    glShaderSource(shader, 1, &glsrc, NULL);
    glCompileShader(shader);

    QString log;
    GLint e, l;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &e);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &l);
    if (l > 0)
    {
        char* tmplog = new char[l];
        glGetShaderInfoLog(shader, l, NULL, tmplog);
        xglKillCrlf(tmplog);
        log = QString(tmplog);
        delete[] tmplog;
    }
    else
    {
        log = QString("");
    }

    if (e == GL_TRUE && log.length() > 0)
    {
        /*msg::wrn(_("OpenGL %s '%s': compiler warning:"),
                type == GL_VERTEX_SHADER ? _("vertex shader") : _("fragment shader"),
                name.c_str());
        msg::wrn_txt("%s", log.c_str());*/
    }
    else if (e != GL_TRUE)
    {
        QString when = QString("OpenGL %1 '%2': compilation failed.")
                .arg(type == GL_VERTEX_SHADER ? ("vertex shader") : ("fragment shader")).arg(name.toLatin1().constData());
        QString what = QString("\n%1")
                 .arg(log.length() > 0 ? log.toLatin1().constData() : ("unknown error"));

        throw when+ ": "+what;
    }
    return shader;
}
GLuint hardware::program_create(GLuint vshader, GLuint fshader)
{
    Q_ASSERT(vshader != 0 || fshader != 0);
    GLuint program = glCreateProgram();
    if (vshader != 0)
        glAttachShader(program, vshader);
    if (fshader != 0)
        glAttachShader(program, fshader);
    return program;
}
GLuint hardware::program_create(const QString& name,
        const QString& vshader_src, const QString& fshader_src)
{
    GLuint vshader = 0, fshader = 0;
    if (vshader_src.length() > 0)
        vshader = shader_compile(name, GL_VERTEX_SHADER, vshader_src);
    if (fshader_src.length() > 0)
        fshader = shader_compile(name, GL_FRAGMENT_SHADER, fshader_src);
    return program_create(vshader, fshader);
}
void hardware::program_link(const QString& name, const GLuint prg)
{
    //msg::dbg("Linking OpenGL program %s.", name.c_str());

    glLinkProgram(prg);

    QString log;
    GLint e, l;
    glGetProgramiv(prg, GL_LINK_STATUS, &e);
    glGetProgramiv(prg, GL_INFO_LOG_LENGTH, &l);
    if (l > 0)
    {
        char *tmplog = new char[l];
        glGetProgramInfoLog(prg, l, NULL, tmplog);
        xglKillCrlf(tmplog);
        log = QString(tmplog);
        delete[] tmplog;
    }
    else
    {
        log = QString("");
    }

    if (e == GL_TRUE && log.length() > 0)
    {
        /*msg::wrn(_("OpenGL program '%s': linker warning:"), name.c_str());
        msg::wrn_txt("%s", log.c_str());*/
    }
    else if (e != GL_TRUE)
    {
        QString when = QString("OpenGL program '%1': linking failed.").arg( name.toLatin1().constData());
        QString what = QString("\n%1").arg( log.length() > 0 ? log.toLatin1().constData() : ("unknown error"));
        throw when + ": " + what;
    }
}
void hardware::program_delete(GLuint program)
{
    if (glIsProgram(program))
    {
        GLint shader_count;
        glGetProgramiv(program, GL_ATTACHED_SHADERS, &shader_count);
        GLuint* shaders = new GLuint[shader_count];
        glGetAttachedShaders(program, shader_count, NULL, shaders);
        for (int i = 0; i < shader_count; i++)
            glDeleteShader(shaders[i]);
        delete[] shaders;
        glDeleteProgram(program);
    }
}
bool hardware::srgb8_textures_color_renderable()
{
    bool retval = true;
    GLuint fbo;
    GLuint tex;

    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, 2, 2, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    GLint framebuffer_bak;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &framebuffer_bak);
    glBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER_EXT,
            GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex, 0);
    GLenum e = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
    if (e != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        retval = false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER_EXT, framebuffer_bak);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
    return retval;
}

void hardware::glMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t)
{
    if (!MultiTexCoord2f)
    {
        MultiTexCoord2f = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLfloat , GLfloat )>
                (QOpenGLContext::currentContext()->getProcAddress("glMultiTexCoord2fARB"));
    }

    MultiTexCoord2f(target, s, t);
}
void hardware::glWindowPos2i(uint x, uint y)
{
    if (!WindowPos2i)
    {
        WindowPos2i = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLuint)>
                (QOpenGLContext::currentContext()->getProcAddress("glWindowPos2iARB"));
    }

    WindowPos2i(x, y);
}
GLvoid *hardware::glMapBuffer(GLenum target, GLenum access)
{
    if (!MapBuffer)
    {
        MapBuffer = reinterpret_cast<GLvoid *(QOPENGLF_APIENTRYP)(GLenum , GLenum)>
                (QOpenGLContext::currentContext()->getProcAddress("glMapBufferARB"));
    }

    MapBuffer(target, access);
}
GLboolean hardware::glUnmapBuffer(GLenum target)
{
    if (!UnmapBuffer)
    {
        UnmapBuffer = reinterpret_cast<GLboolean (QOPENGLF_APIENTRYP)(GLenum)>
                (QOpenGLContext::currentContext()->getProcAddress("glUnmapBufferARB"));
    }

    UnmapBuffer(target);
}
