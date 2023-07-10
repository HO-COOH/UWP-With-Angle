#include "pch.h"

#define GL_GLEXT_PROTOTYPES

// OpenGL ES includes
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

// EGL includes
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

// ANGLE include for Windows Store
#include <angle_windowsstore.h>

#include "MathHelper.h"

using namespace winrt;

using namespace Windows;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::UI::Composition;

//struct App : implements<App, IFrameworkViewSource, IFrameworkView>
//{
//    CompositionTarget m_target{ nullptr };
//    VisualCollection m_visuals{ nullptr };
//    Visual m_selected{ nullptr };
//    float2 m_offset{};
//    CoreApplicationView m_view{ nullptr };
//
//    IFrameworkView CreateView()
//    {
//        return *this;
//    }
//
//    void Initialize(CoreApplicationView const &view)
//    {
//        m_view = view;
//    }
//
//    void Load(hstring const& entryPoint)
//    {
//        OutputDebugString(entryPoint.data());
//    }
//
//    void Uninitialize()
//    {
//    }
//
//    void Run()
//    {
//        CoreWindow window = CoreWindow::GetForCurrentThread();
//        window.Activate();
//        OutputDebugString(std::to_wstring(window == m_view.CoreWindow()).data());
//
//        CoreDispatcher dispatcher = window.Dispatcher();
//        dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
//    }
//
//    void SetWindow(CoreWindow const & window)
//    {
//        Compositor compositor;
//        ContainerVisual root = compositor.CreateContainerVisual();
//        m_target = compositor.CreateTargetForCurrentView();
//        m_target.Root(root);
//        m_visuals = root.Children();
//
//        window.PointerPressed({ this, &App::OnPointerPressed });
//        window.PointerMoved({ this, &App::OnPointerMoved });
//
//        window.PointerReleased([&](auto && ...)
//        {
//            m_selected = nullptr;
//        });
//    }
//
//    void OnPointerPressed(IInspectable const &, PointerEventArgs const & args)
//    {
//        float2 const point = args.CurrentPoint().Position();
//
//        for (Visual visual : m_visuals)
//        {
//            float3 const offset = visual.Offset();
//            float2 const size = visual.Size();
//
//            if (point.x >= offset.x &&
//                point.x < offset.x + size.x &&
//                point.y >= offset.y &&
//                point.y < offset.y + size.y)
//            {
//                m_selected = visual;
//                m_offset.x = offset.x - point.x;
//                m_offset.y = offset.y - point.y;
//            }
//        }
//
//        if (m_selected)
//        {
//            m_visuals.Remove(m_selected);
//            m_visuals.InsertAtTop(m_selected);
//        }
//        else
//        {
//            AddVisual(point);
//        }
//    }
//
//    void OnPointerMoved(IInspectable const &, PointerEventArgs const & args)
//    {
//        if (m_selected)
//        {
//            float2 const point = args.CurrentPoint().Position();
//
//            m_selected.Offset(
//            {
//                point.x + m_offset.x,
//                point.y + m_offset.y,
//                0.0f
//            });
//        }
//    }
//
//    void AddVisual(float2 const point)
//    {
//        Compositor compositor = m_visuals.Compositor();
//        SpriteVisual visual = compositor.CreateSpriteVisual();
//
//        static Color colors[] =
//        {
//            { 0xDC, 0x5B, 0x9B, 0xD5 },
//            { 0xDC, 0xED, 0x7D, 0x31 },
//            { 0xDC, 0x70, 0xAD, 0x47 },
//            { 0xDC, 0xFF, 0xC0, 0x00 }
//        };
//
//        static unsigned last = 0;
//        unsigned const next = ++last % _countof(colors);
//        visual.Brush(compositor.CreateColorBrush(colors[next]));
//
//        float const BlockSize = 100.0f;
//
//        visual.Size(
//        {
//            BlockSize,
//            BlockSize
//        });
//
//        visual.Offset(
//        {
//            point.x - BlockSize / 2.0f,
//            point.y - BlockSize / 2.0f,
//            0.0f,
//        });
//
//        m_visuals.InsertAtTop(visual);
//
//        m_selected = visual;
//        m_offset.x = -BlockSize / 2.0f;
//        m_offset.y = -BlockSize / 2.0f;
//    }
//};

class SimpleRenderer
{
public:
    SimpleRenderer();
    ~SimpleRenderer();
    void Draw();
    void UpdateWindowSize(GLsizei width, GLsizei height);

private:
    GLuint mProgram;
    GLsizei mWindowWidth;
    GLsizei mWindowHeight;

    GLint mPositionAttribLocation;
    GLint mColorAttribLocation;

    GLint mModelUniformLocation;
    GLint mViewUniformLocation;
    GLint mProjUniformLocation;

    GLuint mVertexPositionBuffer;
    GLuint mVertexColorBuffer;
    GLuint mIndexBuffer;

    int mDrawCount;
};

struct App : public winrt::implements<App, IFrameworkViewSource, IFrameworkView>
{
private:
    std::optional<SimpleRenderer> m_cubeRenderer;
    void onActivated(winrt::Windows::ApplicationModel::Core::CoreApplicationView view, winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs const& args)
    {
        view.CoreWindow().Activate();
    }

    void onVisibilityChanged(winrt::Windows::UI::Core::CoreWindow sender, winrt::Windows::UI::Core::VisibilityChangedEventArgs args)
    {
        m_windowVisible = args.Visible();
    }
    void onWindowClosed(winrt::Windows::UI::Core::CoreWindow sender, winrt::Windows::UI::Core::CoreWindowEventArgs args)
    {
        m_windowClosed = true;
    }

    void initializeEgl(winrt::Windows::UI::Core::CoreWindow window)
    {
        const EGLint configAttributes[] =
        {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 8,
            EGL_STENCIL_SIZE, 8,
            EGL_NONE
        };

        const EGLint contextAttributes[] =
        {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };

        const EGLint surfaceAttributes[] =
        {
            // EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER is part of the same optimization as EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER (see above).
            // If you have compilation issues with it then please update your Visual Studio templates.
            EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER, EGL_TRUE,
            EGL_NONE
        };

        const EGLint defaultDisplayAttributes[] =
        {
            // These are the default display attributes, used to request ANGLE's D3D11 renderer.
            // eglInitialize will only succeed with these attributes if the hardware supports D3D11 Feature Level 10_0+.
            EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,

            // EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER is an optimization that can have large performance benefits on mobile devices.
            // Its syntax is subject to change, though. Please update your Visual Studio templates if you experience compilation issues with it.
            EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,

            // EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE is an option that enables ANGLE to automatically call 
            // the IDXGIDevice3::Trim method on behalf of the application when it gets suspended. 
            // Calling IDXGIDevice3::Trim when an application is suspended is a Windows Store application certification requirement.
            EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
            EGL_NONE,
        };

        const EGLint fl9_3DisplayAttributes[] =
        {
            // These can be used to request ANGLE's D3D11 renderer, with D3D11 Feature Level 9_3.
            // These attributes are used if the call to eglInitialize fails with the default display attributes.
            EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
            EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE, 9,
            EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE, 3,
            EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
            EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
            EGL_NONE,
        };

        const EGLint warpDisplayAttributes[] =
        {
            // These attributes can be used to request D3D11 WARP.
            // They are used if eglInitialize fails with both the default display attributes and the 9_3 display attributes.
            EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
            EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE,
            EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
            EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
            EGL_NONE,
        };

        EGLConfig config = NULL;

        // eglGetPlatformDisplayEXT is an alternative to eglGetDisplay. It allows us to pass in display attributes, used to configure D3D11.
        PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
        if (!eglGetPlatformDisplayEXT)
        {
            throw std::runtime_error("Failed to get function eglGetPlatformDisplayEXT");
        }

        //
        // To initialize the display, we make three sets of calls to eglGetPlatformDisplayEXT and eglInitialize, with varying 
        // parameters passed to eglGetPlatformDisplayEXT:
        // 1) The first calls uses "defaultDisplayAttributes" as a parameter. This corresponds to D3D11 Feature Level 10_0+.
        // 2) If eglInitialize fails for step 1 (e.g. because 10_0+ isn't supported by the default GPU), then we try again 
        //    using "fl9_3DisplayAttributes". This corresponds to D3D11 Feature Level 9_3.
        // 3) If eglInitialize fails for step 2 (e.g. because 9_3+ isn't supported by the default GPU), then we try again 
        //    using "warpDisplayAttributes".  This corresponds to D3D11 Feature Level 11_0 on WARP, a D3D11 software rasterizer.
        //

        // This tries to initialize EGL to D3D11 Feature Level 10_0+. See above comment for details.
        m_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, defaultDisplayAttributes);
        if (m_display == EGL_NO_DISPLAY)
        {
            throw std::runtime_error("Failed to get EGL display");
        }

        if (eglInitialize(m_display, NULL, NULL) == EGL_FALSE)
        {
            // This tries to initialize EGL to D3D11 Feature Level 9_3, if 10_0+ is unavailable (e.g. on some mobile devices).
            m_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, fl9_3DisplayAttributes);
            if (m_display == EGL_NO_DISPLAY)
            {
                throw std::runtime_error("Failed to get EGL display");
            }

            if (eglInitialize(m_display, NULL, NULL) == EGL_FALSE)
            {
                // This initializes EGL to D3D11 Feature Level 11_0 on WARP, if 9_3+ is unavailable on the default GPU.
                m_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, warpDisplayAttributes);
                if (m_display == EGL_NO_DISPLAY)
                {
                    throw std::runtime_error("Failed to get EGL display");
                }

                if (eglInitialize(m_display, NULL, NULL) == EGL_FALSE)
                {
                    // If all of the calls to eglInitialize returned EGL_FALSE then an error has occurred.
                    throw std::runtime_error("Failed to initialize EGL");
                }
            }
        }

        EGLint numConfigs = 0;
        if ((eglChooseConfig(m_display, configAttributes, &config, 1, &numConfigs) == EGL_FALSE) || (numConfigs == 0))
        {
            throw std::runtime_error("Failed to choose first EGLConfig");
        }

        // Create a PropertySet and initialize with the EGLNativeWindowType.
        winrt::Windows::Foundation::Collections::PropertySet surfaceCreationProperties;
        surfaceCreationProperties.Insert(EGLNativeWindowTypeProperty, window);

        // You can configure the surface to render at a lower resolution and be scaled up to
        // the full window size. This scaling is often free on mobile hardware.
        //
        // One way to configure the SwapChainPanel is to specify precisely which resolution it should render at.
        // Size customRenderSurfaceSize = Size(800, 600);
        // surfaceCreationProperties->Insert(ref new String(EGLRenderSurfaceSizeProperty), PropertyValue::CreateSize(customRenderSurfaceSize));
        //
        // Another way is to tell the SwapChainPanel to render at a certain scale factor compared to its size.
        // e.g. if the SwapChainPanel is 1920x1280 then setting a factor of 0.5f will make the app render at 960x640
        // float customResolutionScale = 0.5f;
        // surfaceCreationProperties->Insert(ref new String(EGLRenderResolutionScaleProperty), PropertyValue::CreateSingle(customResolutionScale));
        auto abi = winrt::get_abi(surfaceCreationProperties);
        m_surface = eglCreateWindowSurface(m_display, config, reinterpret_cast<::IInspectable*>(abi), surfaceAttributes);
        if (m_surface == EGL_NO_SURFACE)
        {
            throw std::runtime_error("Failed to create EGL fullscreen surface");
        }

        m_context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, contextAttributes);
        if (m_context == EGL_NO_CONTEXT)
        {
            throw std::runtime_error("Failed to create EGL context");
        }

        if (eglMakeCurrent(m_display, m_surface, m_surface, m_context) == EGL_FALSE)
        {
            throw std::runtime_error("Failed to make fullscreen EGLSurface current");
        }
    }

    void recreateRenderer()
    {
        if (!m_cubeRenderer)
            m_cubeRenderer.emplace();
    }

    void cleanupEgl()
    {
        if (m_display != EGL_NO_DISPLAY && m_surface != EGL_NO_SURFACE)
        {
            eglDestroySurface(m_display, m_surface);
            m_surface = EGL_NO_SURFACE;
        }

        if (m_display != EGL_NO_DISPLAY && m_context != EGL_NO_CONTEXT)
        {
            eglDestroyContext(m_display, m_context);
            m_context = EGL_NO_CONTEXT;
        }

        if (m_display != EGL_NO_DISPLAY)
        {
            eglTerminate(m_display);
            m_display = EGL_NO_DISPLAY;
        }
    }
public:
    #pragma region IFrameworkViewSource
    winrt::Windows::ApplicationModel::Core::IFrameworkView CreateView()
    {
        return *this;
    }
    #pragma endregion

#pragma region IFrameworkView
    EGLDisplay m_display;
    EGLContext m_context;
    EGLSurface m_surface;
    bool m_windowClosed;
    bool m_windowVisible;

    void Initialize(winrt::Windows::ApplicationModel::Core::CoreApplicationView const &view)
    {
        view.Activated({ this, &App::onActivated });
    }

    void Load(hstring const& entryPoint)
    {
        recreateRenderer();
    }

    void Uninitialize()
    {

    }

    void Run()
    {
        while (!m_windowClosed)
        {
            if (!m_windowVisible)
                winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(winrt::Windows::UI::Core::CoreProcessEventsOption::ProcessOneAndAllPending);
            else
            {
                winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(winrt::Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent);
                EGLint panelWidth{}, panelHeight{};
                eglQuerySurface(m_display, m_surface, EGL_WIDTH, &panelWidth);
                eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &panelHeight);

                m_cubeRenderer->UpdateWindowSize(panelWidth, panelHeight);
                m_cubeRenderer->Draw();

                if (eglSwapBuffers(m_display, m_surface) != EGL_TRUE)
                {
                    m_cubeRenderer.reset();
                    cleanupEgl();
                }
            }
        }
        cleanupEgl();
    }
    

    void SetWindow(CoreWindow const& window)
    {
        window.VisibilityChanged({ this, &App::onVisibilityChanged });
        window.Closed({ this, &App::onWindowClosed });

        initializeEgl(window);
    }
#pragma endregion
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    CoreApplication::Run(make<App>());
}


GLuint CompileShader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);

    const char* sourceArray[1] = { source.c_str() };
    glShaderSource(shader, 1, sourceArray, NULL);
    glCompileShader(shader);

    GLint compileResult;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);

    if (compileResult == 0)
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<GLchar> infoLog(infoLogLength);
        glGetShaderInfoLog(shader, (GLsizei)infoLog.size(), NULL, infoLog.data());

        std::wstring errorMessage = std::wstring(L"Shader compilation failed: ");
        errorMessage += std::wstring(infoLog.begin(), infoLog.end());

        throw errorMessage;
    }

    return shader;
}

GLuint CompileProgram(const std::string& vsSource, const std::string& fsSource)
{
    GLuint program = glCreateProgram();

    if (program == 0)
    {
        throw std::runtime_error("Program creation failed");
    }

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSource);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSource);

    if (vs == 0 || fs == 0)
    {
        glDeleteShader(fs);
        glDeleteShader(vs);
        glDeleteProgram(program);
        return 0;
    }

    glAttachShader(program, vs);
    glDeleteShader(vs);

    glAttachShader(program, fs);
    glDeleteShader(fs);

    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    if (linkStatus == 0)
    {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<GLchar> infoLog(infoLogLength);
        glGetProgramInfoLog(program, (GLsizei)infoLog.size(), NULL, infoLog.data());

        std::wstring errorMessage = std::wstring(L"Program link failed: ");
        errorMessage += std::wstring(infoLog.begin(), infoLog.end());

        throw errorMessage;
    }

    return program;
}

#define STRING(s) #s

SimpleRenderer::SimpleRenderer() :
    mWindowWidth(0),
    mWindowHeight(0),
    mDrawCount(0)
{
    // Vertex Shader source
    const std::string vs = STRING
    (
        uniform mat4 uModelMatrix;
    uniform mat4 uViewMatrix;
    uniform mat4 uProjMatrix;
    attribute vec4 aPosition;
    attribute vec4 aColor;
    varying vec4 vColor;
    void main()
    {
        gl_Position = uProjMatrix * uViewMatrix * uModelMatrix * aPosition;
        vColor = aColor;
    }
    );

    // Fragment Shader source
    const std::string fs = STRING
    (
        precision mediump float;
    varying vec4 vColor;
    void main()
    {
        gl_FragColor = vColor;
    }
    );

    // Set up the shader and its uniform/attribute locations.
    mProgram = CompileProgram(vs, fs);
    mPositionAttribLocation = glGetAttribLocation(mProgram, "aPosition");
    mColorAttribLocation = glGetAttribLocation(mProgram, "aColor");
    mModelUniformLocation = glGetUniformLocation(mProgram, "uModelMatrix");
    mViewUniformLocation = glGetUniformLocation(mProgram, "uViewMatrix");
    mProjUniformLocation = glGetUniformLocation(mProgram, "uProjMatrix");

    // Then set up the cube geometry.
    GLfloat vertexPositions[] =
    {
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
    };

    glGenBuffers(1, &mVertexPositionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexPositionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);

    GLfloat vertexColors[] =
    {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
    };

    glGenBuffers(1, &mVertexColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);

    short indices[] =
    {
        0, 1, 2, // -x
        1, 3, 2,

        4, 6, 5, // +x
        5, 6, 7,

        0, 5, 1, // -y
        0, 4, 5,

        2, 7, 6, // +y
        2, 3, 7,

        0, 6, 4, // -z
        0, 2, 6,

        1, 7, 3, // +z
        1, 5, 7,
    };

    glGenBuffers(1, &mIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

SimpleRenderer::~SimpleRenderer()
{
    if (mProgram != 0)
    {
        glDeleteProgram(mProgram);
        mProgram = 0;
    }

    if (mVertexPositionBuffer != 0)
    {
        glDeleteBuffers(1, &mVertexPositionBuffer);
        mVertexPositionBuffer = 0;
    }

    if (mVertexColorBuffer != 0)
    {
        glDeleteBuffers(1, &mVertexColorBuffer);
        mVertexColorBuffer = 0;
    }

    if (mIndexBuffer != 0)
    {
        glDeleteBuffers(1, &mIndexBuffer);
        mIndexBuffer = 0;
    }
}

void SimpleRenderer::Draw()
{
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (mProgram == 0)
        return;

    glUseProgram(mProgram);

    glBindBuffer(GL_ARRAY_BUFFER, mVertexPositionBuffer);
    glEnableVertexAttribArray(mPositionAttribLocation);
    glVertexAttribPointer(mPositionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, mVertexColorBuffer);
    glEnableVertexAttribArray(mColorAttribLocation);
    glVertexAttribPointer(mColorAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    MathHelper::Matrix4 modelMatrix = MathHelper::SimpleModelMatrix((float)mDrawCount / 50.0f);
    glUniformMatrix4fv(mModelUniformLocation, 1, GL_FALSE, &(modelMatrix.m[0][0]));

    MathHelper::Matrix4 viewMatrix = MathHelper::SimpleViewMatrix();
    glUniformMatrix4fv(mViewUniformLocation, 1, GL_FALSE, &(viewMatrix.m[0][0]));

    MathHelper::Matrix4 projectionMatrix = MathHelper::SimpleProjectionMatrix(float(mWindowWidth) / float(mWindowHeight));
    glUniformMatrix4fv(mProjUniformLocation, 1, GL_FALSE, &(projectionMatrix.m[0][0]));

    // Draw 36 indices: six faces, two triangles per face, 3 indices per triangle
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    glDrawElements(GL_TRIANGLES, (6 * 2) * 3, GL_UNSIGNED_SHORT, 0);

    mDrawCount += 1;
}

void SimpleRenderer::UpdateWindowSize(GLsizei width, GLsizei height)
{
    glViewport(0, 0, width, height);
    mWindowWidth = width;
    mWindowHeight = height;
}