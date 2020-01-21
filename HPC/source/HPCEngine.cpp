/**
 * @file HPCEngine.cpp
 * @author Matthew Oliver
 *
 * @section LICENSE
 *
 * This code is not available for commercial use. This code is provided as is
 * and the author claims no responsibility for any issues, damages or any other
 * ill effects resulting from use of this code.
 *
 * @section DESCRIPTION
 *
 * This defines the base HPCEngine class. This is provided to abstract some of the
 * more complex aspects of completing the HPC Assignment.
 */
#include "HPCEngine.h"
#include <cmath>
#include <chrono>
#include <ft2build.h>
#include FT_FREETYPE_H
#define REQ_GLVERSION_MAJOR 3
#define REQ_GLVERSION_MINOR 3
#include <GL/glew.h>
#include <SDL.h>
#include <xmmintrin.h>
#ifndef WINDOWFULLSCREEN
#   define WINDOWFULLSCREEN false
#endif
#ifndef WINDOWWIDTH
#   define WINDOWWIDTH 1600
#endif
#ifndef WINDWINDOWHEIGHTOWWIDTH
#   define WINDOWHEIGHT 900
#endif
#define FONTSIZE 32

// forward declarations
char g_charHPCRenderShaderVertex[];
char g_charHPCRenderShaderFragment[];
char g_charHPCTextShaderVertex[];
char g_charHPCTextShaderFragment[];

extern bool glLoadShader(GLuint& shader, GLenum shaderType, const GLchar* shaderString) noexcept;

extern bool glLoadShaders(GLuint& shader, GLuint vertexShader, GLuint fragmentShader) noexcept;

extern GLsizei glGenerateSphere(uint32_t tessU, uint32_t tessV, GLuint indexVBO, GLuint indexIBO) noexcept;

// Create default engine instance
HPCEngine g_hpc;

HPCEngine::HPCEngine() noexcept
{
    // Configure the CPU for DAZ and FLZ Mode operations
    _mm_setcsr((_mm_getcsr() & ~0x8800UL) | 0x8800UL);
    _mm_setcsr((_mm_getcsr() & ~0x0140UL) | 0x0140UL);
}

HPCEngine::~HPCEngine() noexcept
{
    // Turn DAZ and FLZ Mode operations off again (assumes they were disabled to begin with)
    _mm_setcsr((_mm_getcsr() & ~0x8800UL) | 0x0000UL);
    _mm_setcsr((_mm_getcsr() & ~0x0140UL) | 0x0100UL);
}

bool HPCEngine::glInit() noexcept
{
    // Initialise GLEW
    glewExperimental = GL_TRUE;    // Allow experimental or pre-release drivers to return all supported extensions
    const GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialise GLEW: %s\n", glewGetErrorString(glewError));
        return false;
    }

    // Set up initial GL attributes
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    // Set the cleared back buffer to black
    glCullFace(GL_BACK);                     // Set back-face culling
    glEnable(GL_CULL_FACE);                  // Enable use of back/front face culling
    glEnable(GL_DEPTH_TEST);                 // Enable use of depth testing
    glDisable(GL_STENCIL_TEST);              // Disable stencil test for speed
    // Initialise sphere rendering
    if (!glInitSphere()) {
        return false;
    }

    // Initialise camera
    if (!glInitCamera()) {
        return false;
    }

    // Initialise text rendering
    if (!glInitText()) {
        return false;
    }

    // Print out debug information
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Render System Information:\n");
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Renderer: %s\n", glGetString(GL_RENDERER));
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Vendor: %s\n", glGetString(GL_VENDOR));
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Supported GL Version: %s\n", glGetString(GL_VERSION));
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Supported GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    int32_t glVersionMajor, glVersionMinor;
    glGetIntegerv(GL_MAJOR_VERSION, &glVersionMajor);
    glGetIntegerv(GL_MINOR_VERSION, &glVersionMinor);
    char buffer[33];
    std::string sTemp = SDL_itoa(glVersionMajor, buffer, 10);
    sTemp += ".";
    sTemp += SDL_itoa(glVersionMinor, buffer, 10);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Using GL Version: %s\n", sTemp.c_str());

    // Initialise the assignment state
    return g_hpc.m_assignment.load();
}

bool HPCEngine::glInitSphere() noexcept
{
    // Create vertex shader
    GLuint vertexShader;
    if (!glLoadShader(vertexShader, GL_VERTEX_SHADER, g_charHPCRenderShaderVertex)) {
        return false;
    }

    // Create fragment shader
    GLuint fragmentShader;
    if (!glLoadShader(fragmentShader, GL_FRAGMENT_SHADER, g_charHPCRenderShaderFragment)) {
        return false;
    }

    // Create program
    if (!glLoadShaders(m_sphereProgram, vertexShader, fragmentShader)) {
        return false;
    }

    // Clean up unneeded shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Specify program to use
    glUseProgram(m_sphereProgram);

    // Create Vertex Array Objects
    glGenVertexArrays(1, &m_sphereVAO);

    // Create VBO and IBOs
    glGenBuffers(1, &m_sphereVBO);
    glGenBuffers(1, &m_sphereIBO);

    // Bind the Cube VAO
    glBindVertexArray(m_sphereVAO);

    // Create Sphere VBO and IBO data
    m_sphereIndices = glGenerateSphere(12, 6, m_sphereVBO, m_sphereIBO);

    // Create another buffer for instanced positions
    glGenBuffers(1, &m_sphereABO);
    glBindBuffer(GL_ARRAY_BUFFER, m_sphereABO);    // bind the buffer as an array
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RenderData),
        nullptr);    // Pass location 0 as we bound new buffer before hand
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);    // Set attrib 1 as an instanced attrib.
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(RenderData),
        reinterpret_cast<const GLvoid*>(offsetof(RenderData, m_radius)));    // Pass offset to 4th element
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);    // Set attrib 2 as an instanced attrib.
    return true;
}

bool HPCEngine::glInitCamera() noexcept
{
    /* Load the camera data */
    glGenBuffers(1, &m_cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_cameraUBO);

    // Link the camera buffer
    const uint32_t blockIndex = glGetUniformBlockIndex(m_sphereProgram, "CameraData");
    glUniformBlockBinding(m_sphereProgram, blockIndex, 0);

    // This binds the camera UBO as the current rendering camera
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_cameraUBO);
    return true;
}

bool HPCEngine::glInitText() noexcept
{
    // Create vertex shader
    GLuint vertexShader;
    if (!glLoadShader(vertexShader, GL_VERTEX_SHADER, g_charHPCTextShaderVertex)) {
        return false;
    }

    // Create fragment shader
    GLuint fragmentShader;
    if (!glLoadShader(fragmentShader, GL_FRAGMENT_SHADER, g_charHPCTextShaderFragment)) {
        return false;
    }

    // Create program
    if (!glLoadShaders(m_textProgram, vertexShader, fragmentShader)) {
        return false;
    }

    // Clean up unneeded shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    const int32_t uniformIndex = glGetUniformLocation(m_textProgram, "textArray");
    glUniform1i(uniformIndex, 0);

    /* Create full screen quad for text rendering */
    glGenVertexArrays(1, &m_textVAO);
    // Generate VBO buffer
    glGenBuffers(1, &m_textVBO);
    glBindVertexArray(m_textVAO);
    // 2 floats for screen position and 2 for tex coords
    static const float quadBufferData[] = {0.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadBufferData), quadBufferData, GL_STATIC_DRAW);
    // Specify location of data within buffer
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    // Create another buffer for instanced positions
    glGenBuffers(1, &m_textABO);
    glBindBuffer(GL_ARRAY_BUFFER, m_textABO);    // bind the buffer as an array
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(TextData),
        nullptr);    // Pass location 0 as we bound new buffer before hand
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);    // Set attrib 1 as an instanced attrib.
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(TextData),
        reinterpret_cast<const GLvoid*>(offsetof(TextData, m_textureWidth)
        ));    // Pass location 0 as we bound new buffer before hand
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);    // Set attrib 1 as an instanced attrib.
    /* Load in the font that we are going to use for the text rendering */
    const std::string usedCharacters = "FPSNumBals:0123456789";

    // Initialise FreeType2
    FT_Library ft2Library;
    if (FT_Init_FreeType(&ft2Library) != 0) {
        logMessage("Error: Could not initialise FreeType2 library\n");
        return false;
    }

    // Freetype open font file
    FT_Face ft2FontFace;
#ifdef _WIN32
    if (FT_New_Face(ft2Library, "C:/Windows/Fonts/times.ttf", 0, &ft2FontFace) != 0) {
#else
    if (FT_New_Face(FT2Library, "/usr/share/fonts/truetype/times.ttf", 0, &FT2FontFace) != 0) {
#endif
        logMessage("Error: Failed opening font file\n");
        return false;
    }

    // Set the font size
    FT_Set_Pixel_Sizes(ft2FontFace, 0, FONTSIZE);

    // Make texture that will hold all desired chars
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &m_charTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_charTexture);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // All chars get allocated fixed amount of texture based on size of desired font
    // We allocate for a full RGB texture as there are compatibility issues with only using an Alpha map in a
    // texture array
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, FONTSIZE, FONTSIZE, static_cast<uint32_t>(usedCharacters.length()), 0,
        GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // Generate working cache for converted Glyph data
    struct GlyphColour
    {
        uint8_t m_colours[3] = {};

        GlyphColour() noexcept = default;

        enum class Colour
        {
            White,
            Red,
            Green
        };

        GlyphColour(const Colour glyphColour, const uint8_t a) noexcept
        {
            switch (glyphColour) {
                case Colour::White:
                    m_colours[0] = (255 * a) / 255;
                    m_colours[1] = (255 * a) / 255;
                    m_colours[2] = (255 * a) / 255;
                    break;
                case Colour::Red:
                    m_colours[0] = (255 * a) / 255;
                    m_colours[1] = 0;
                    m_colours[2] = 0;
                    break;
                case Colour::Green:
                    m_colours[0] = 0;
                    m_colours[1] = (255 * a) / 255;
                    m_colours[2] = 0;
                    break;
                default:
                    break;
            }
        }
    };
    GlyphColour rgbaCache[FONTSIZE * FONTSIZE];

    // Loop through each desired char
    uint32_t texturePosition = 0;
    for (const auto& currChar : usedCharacters) {
        // Give the alphabetic characters some colour
        const GlyphColour::Colour glyphcolour = (texturePosition < 3) ? GlyphColour::Colour::Red
            : (texturePosition < 10) ? GlyphColour::Colour::Green : GlyphColour::Colour::White;

        // Retrieve glyph index from character code
        const FT_UInt ft2GlyphIndex = FT_Get_Char_Index(ft2FontFace, currChar);

        // load glyph image into the slot (erase previous one)
        if (FT_Load_Glyph(ft2FontFace, ft2GlyphIndex, FT_LOAD_DEFAULT) != 0) {
            logMessage("Error: could not load desired glyph (Falling back to default)\n");
            // Continue anyway as it will just use a fallback glyph instead
            // return false;
        }

        // Convert to an anti-aliased bitmap
        if (FT_Render_Glyph(ft2FontFace->glyph, FT_RENDER_MODE_NORMAL) != 0) {
            logMessage("Error: failed to render glyph\n");
            return false;
        }

        // Copy char glyph into texture at desired location
        //(Note: Glyph is only 8bit alpha channel and so cant be directly used for standard texture
        //mapping)
        for (uint32_t j = 0; j < ft2FontFace->glyph->bitmap.rows; j++) {
            for (uint32_t i = 0; i < ft2FontFace->glyph->bitmap.width; i++) {
                rgbaCache[i + (j * FONTSIZE)] = GlyphColour(glyphcolour,
                    ft2FontFace->glyph->bitmap.buffer[i + (j * ft2FontFace->glyph->bitmap.width)]);
            }
            // We don't fill in the remaining texture values as they will never be accessed anyway
        }
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, texturePosition, FONTSIZE, FONTSIZE, 1, GL_RGB, GL_UNSIGNED_BYTE,
            rgbaCache);

        // Add glyph texture coordinates, texture size, render offset, kerning to stored array of glyph
        // data
        /*
         * FreeType2 Glyph metrics:
         * --------------
         *
         *                       xmin                     xmax
         *                        |                         |
         *                        |<-------- width -------->|
         *                        |                         |
         *              |         +-------------------------+----------------- ymax
         *              |         |    ggggggggg   ggggg    |     ^        ^
         *              |         |   g:::::::::ggg::::g    |     |        |
         *              |         |  g:::::::::::::::::g    |     |        |
         *              |         | g::::::ggggg::::::gg    |     |        |
         *              |         | g:::::g     g:::::g     |     |        |
         *    offset_x -|-------->| g:::::g     g:::::g     |  offset_y    |
         *              |         | g:::::g     g:::::g     |     |        |
         *              |         | g::::::g    g:::::g     |     |        |
         *              |         | g:::::::ggggg:::::g     |     |        |
         *              |         |  g::::::::::::::::g     |     |      height
         *              |         |   gg::::::::::::::g     |     |        |
         *  baseline ---*---------|---- gggggggg::::::g-----*--------      |
         *            / |         |             g:::::g     |              |
         *     origin   |         | gggggg      g:::::g     |              |
         *              |         | g:::::gg   gg:::::g     |              |
         *              |         |  g::::::ggg:::::::g     |              |
         *              |         |   gg:::::::::::::g      |              |
         *              |         |     ggg::::::ggg        |              |
         *              |         |         gggggg          |              v
         *              |         +-------------------------+----------------- ymin
         *              |                                   |
         *              |------------- advance_x ---------->|
         */
        // We define the text metrics using a different system
        // We define the text based on a baseline starting at the top left corner and then all offsets
        // are relative to that (i.e. y offset is now in downward direction) We assume that the max
        // distance from baseline to top of a character is the FontPixelSize
        const int32_t baselineOffset = FONTSIZE;
        GlyphData currentGlyphData = {
            currentGlyphData.m_offsetX = ft2FontFace->glyph->metrics.horiBearingX / 64,
            currentGlyphData.m_offsetY = -baselineOffset + (ft2FontFace->glyph->metrics.horiBearingY / 64),
            currentGlyphData.m_sizeX = ft2FontFace->glyph->metrics.width / 64,    // The pixel width of the glyph
            currentGlyphData.m_sizeY = ft2FontFace->glyph->metrics.height / 64,    // The pixel height of the glyph
            currentGlyphData.m_texturePosition = texturePosition
        };
        m_glyphData[currChar] = currentGlyphData;
        ++texturePosition;
    }

    //Initialise the overlay
    glUpdateText(0);
    return true;
}

class HPCInVec
{
public:
    __m128 m_vec;

    explicit HPCInVec(const __m128& vec)
        : m_vec(vec)
    {}
};

class HPCVec3
{
public:
    __m128 m_vec3;

    explicit HPCVec3(const __m128& vec)
        : m_vec3(vec)
    {}

    HPCVec3(const float x, const float y, const float z)
        : m_vec3(_mm_set_ps(1.0f, z, y, x))
    {}

    HPCInVec getX() const
    {
        return HPCInVec(_mm_shuffle_ps(m_vec3, m_vec3, _MM_SHUFFLE(0, 0, 0, 0)));
    }

    HPCInVec getY() const
    {
        return HPCInVec(_mm_shuffle_ps(m_vec3, m_vec3, _MM_SHUFFLE(1, 1, 1, 1)));
    }

    HPCInVec getZ() const
    {
        return HPCInVec(_mm_shuffle_ps(m_vec3, m_vec3, _MM_SHUFFLE(2, 2, 2, 2)));
    }

    HPCVec3 operator+(const HPCVec3& vec3) const
    {
        return HPCVec3(_mm_add_ps(m_vec3, vec3.m_vec3));
    }

    HPCVec3 operator*(const HPCInVec& vec) const
    {
        return HPCVec3(_mm_mul_ps(m_vec3, vec.m_vec));
    }

    HPCVec3 operator-() const
    {
        return HPCVec3(_mm_sub_ps(_mm_setzero_ps(), m_vec3));
    }

    HPCVec3 normalise() const
    {
        return HPCVec3(_mm_mul_ps(m_vec3, _mm_rsqrt_ps(_mm_dp_ps(m_vec3, m_vec3, 0xFF))));
    }

    HPCVec3 cross3(const HPCVec3& vec3) const
    {
        return HPCVec3(_mm_permute_ps(
            _mm_sub_ps(_mm_mul_ps(_mm_permute_ps(vec3.m_vec3, _MM_SHUFFLE(3, 0, 2, 1)), m_vec3),
                _mm_mul_ps(_mm_permute_ps(m_vec3, _MM_SHUFFLE(3, 0, 2, 1)), vec3.m_vec3)), _MM_SHUFFLE(3, 0, 2, 1)));
    }

    static void transpose(const HPCVec3& vec0, const HPCVec3& vec1, const HPCVec3& vec2, HPCVec3& vecT0, HPCVec3& vecT1,
        HPCVec3& vecT2)
    {
        const __m128 temp1 = _mm_unpacklo_ps(vec0.m_vec3, vec1.m_vec3);
        const __m128 temp2 = _mm_permute_ps(vec2.m_vec3, _MM_SHUFFLE(3, 1, 3, 0));    //(0,x,1,x))
        const __m128 temp3 = _mm_unpackhi_ps(vec0.m_vec3, vec1.m_vec3);
        vecT0.m_vec3 = _mm_movelh_ps(temp1, temp2);
        vecT1.m_vec3 = _mm_movehl_ps(temp2, temp1);
        vecT2.m_vec3 = _mm_blend_ps(temp3, vec2.m_vec3, 0xC);
    }
};

void HPCEngine::glUpdateCamera(const HPCVec3& rot1, const HPCVec3& rot2) const noexcept
{
    // Use initial values
    HPCVec3 cameraPosition(0.0f, 50.0f, 300.0f);
    HPCVec3 cameraUp(0.0f, 1.0f, 0.0f);

    // Rotate the positions
    HPCVec3 temp = (rot1 * cameraPosition.getY()) + (rot2 * cameraPosition.getX());
    cameraPosition = HPCVec3(_mm_blend_ps(temp.m_vec3, _mm_add_ps(temp.m_vec3, cameraPosition.m_vec3), 0x4));
    temp = (rot1 * cameraUp.getY()) + (rot2 * cameraUp.getX());
    cameraUp = HPCVec3(_mm_blend_ps(temp.m_vec3, _mm_add_ps(temp.m_vec3, cameraUp.m_vec3), 0x4));

    // Generate view direction
    HPCVec3 backward(cameraPosition);
    // Generate proper horizontal direction
    HPCVec3 right = cameraUp.cross3(backward);
    // Generate proper up direction
    cameraUp = backward.cross3(right);

    // Normalise just in case
    backward = backward.normalise();
    right = right.normalise();
    cameraUp = cameraUp.normalise();
    HPCVec3::transpose(right, cameraUp, backward, right, cameraUp, backward);

    // Transform the translational component
    cameraPosition = -cameraPosition;
    const HPCVec3 origin(
        (backward * cameraPosition.getZ()) + (cameraUp * cameraPosition.getY()) + (right * cameraPosition.getX()));
    class HPCVec4
    {
    public:
        __m128 m_vec4;

        explicit HPCVec4(const __m128& vec)
            : m_vec4(vec)
        {}

        HPCVec4(const float x, const float y, const float z, const float w)
            : m_vec4(_mm_set_ps(w, z, y, x))
        {}

        explicit HPCVec4(const HPCVec3& vec3)
            : m_vec4(_mm_shuffle_ps(vec3.m_vec3, vec3.m_vec3, _MM_SHUFFLE(2, 2, 1, 0)))
        {}

        HPCVec4 operator*(const HPCVec4& vec4) const
        {
            return HPCVec4(_mm_mul_ps(m_vec4, vec4.m_vec4));
        }

        HPCVec4& addZ(const float z)
        {
            __m128 vec = _mm_shuffle_ps(m_vec4, m_vec4, _MM_SHUFFLE(3, 0, 1, 2));
            vec = _mm_add_ss(vec, _mm_set_ss(z));
            m_vec4 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(3, 0, 1, 2));
            return *this;
        }
    };
    // Set up projective 4th element
    HPCVec4 right4(right);
    HPCVec4 up4(cameraUp);
    HPCVec4 backward4(backward);
    HPCVec4 origin4(origin);

    // Get the size of the screen in the X and Y screen direction
    const float startRange = 1.0f;
    const float endRange = 1000.0f;
    const float size = 1.0f / (tanf((40.0f * 3.1415926535897932384626433832795f) / 360.0f) * startRange);
    const float sizeY = size * (static_cast<float>(g_hpc.m_windowWidth) / static_cast<float>(g_hpc.m_windowHeight));

    // Determine projection matrix components
    const float abDenom(startRange - endRange);
    const float b = (endRange * startRange * 2.0f) / abDenom;
    const float a = (endRange + startRange) / abDenom;

    // Multiply view by projection
    const HPCVec4 multer(size, sizeY, a, -1.0f);
    right4 = right4 * multer;
    up4 = up4 * multer;
    backward4 = backward4 * multer;
    origin4 = origin4 * multer;

    // Add z offset to origin.z
    origin4.addZ(b);
    const HPCVec4 matrix[] = {
        right4, up4, backward4, origin4,
    };
    glBindBuffer(GL_UNIFORM_BUFFER, g_hpc.m_cameraUBO);
    // Make new buffer and drop old one allowing for asynch buffer update
    glBufferData(GL_UNIFORM_BUFFER, 16 * sizeof(float), matrix, GL_STREAM_DRAW);
}

void HPCEngine::glUpdateText(const float fps)
{
    // Update frame counter display text
    strncpy(&m_overlayString[5], "       ", 6);    // clear in case new number has fewer digits than the last
    SDL_itoa(static_cast<int32_t>(fps), &m_overlayString[5], 10);
    // Update number of balls
    strncpy(&m_overlayString[24], "     ", 5);
    SDL_uitoa(m_numSpheres, &m_overlayString[24], 10);

    // Create instance data
    TextData buffer[27];
    // Offset by 1.0 to get the start offset in the top left corner of the screen
    // Then we offset by 1 character (screen size normalised).
    // Note: The offset will require additional offset when in windowed mode due to differences between
    // requested resolution and actual resolution (due to window border)
    const auto halfWindowWidth = static_cast<float>(m_windowWidth / 2UL);
    const auto halfWindowHeight = static_cast<float>(m_windowHeight / 2UL);
    const auto fontSize = static_cast<float>(FONTSIZE);
    float globalTextAdvanceX = -1.0f + (fontSize / halfWindowWidth);
    float globalTextAdvanceY = 1.0f - (fontSize / halfWindowHeight);
    uint32_t index = 0;
    for (const auto& cbuff : m_overlayString) {
        // Check if this is special char (i.e. space or newline)
        if (cbuff == ' ') {
            // Increment global x advance by space length
            globalTextAdvanceX += (fontSize / 2.0f) / halfWindowWidth;
            continue;
        }
        if (cbuff == '\n') {
            // Increment global y advance by char size
            globalTextAdvanceY -= fontSize / halfWindowHeight;
            globalTextAdvanceX = -1.0f + (fontSize / halfWindowWidth);
            continue;
        }

        // Update the array instance data for current char
        const GlyphData currentGlyphData = m_glyphData[cbuff];
        buffer[index] = {
            (static_cast<float>(currentGlyphData.m_offsetX) / halfWindowWidth) + globalTextAdvanceX,
            (static_cast<float>(currentGlyphData.m_offsetY) / halfWindowHeight) + globalTextAdvanceY,
            static_cast<float>(currentGlyphData.m_sizeX) / halfWindowWidth,
            static_cast<float>(currentGlyphData.m_sizeY) / halfWindowHeight,
            static_cast<float>(currentGlyphData.m_sizeX) / fontSize,
            -static_cast<float>(currentGlyphData.m_sizeY) / fontSize,
            static_cast<float>(currentGlyphData.m_texturePosition)
        };
        // Increment the text cursor position
        globalTextAdvanceX = buffer[index].m_globalOffsetX + buffer[index].m_width;
        ++index;
    }
    m_textChars = index;

    // Bind the required text instanced data
    glBindBuffer(GL_ARRAY_BUFFER, m_textABO);

    // Allocate the array object
    glBufferData(GL_ARRAY_BUFFER, m_textChars * sizeof(TextData), buffer, GL_DYNAMIC_READ);
}

void HPCEngine::glRender() noexcept
{
    // Clear the render output and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glRenderSpheres();
    glRenderText();
}

void HPCEngine::glRenderSpheres() const noexcept
{
    // Bind the Instanced buffer
    glBindBuffer(GL_ARRAY_BUFFER, g_hpc.m_sphereABO);

    // Allocate the array object and copy from existing positions
    glBufferData(GL_ARRAY_BUFFER, g_hpc.m_numSpheres * sizeof(RenderData), g_hpc.m_renderData, GL_DYNAMIC_READ);

    // Draw the instanced spheres
    glUseProgram(m_sphereProgram);
    glBindVertexArray(m_sphereVAO);
    glDrawElementsInstanced(GL_TRIANGLES, m_sphereIndices, GL_UNSIGNED_INT, nullptr, m_numSpheres);
}

void HPCEngine::glRenderText() noexcept
{
    // Determine required text string
    if (m_frameTime >= 1.0f) {
        // Get second accurate frame rate
        const float fps = static_cast<float>(m_frameNumber) / m_frameTime;

        // Reset time to overflow time
        do {
            m_frameTime -= 1.0f;
        } while (m_frameTime >= 1.0f);

        // Reset counter
        m_frameNumber = 0;

        //Update the overlay
        glUpdateText(fps);
    }
    // Draw the text
    glDisable(GL_DEPTH_TEST);    // Disable use of depth testing
    glUseProgram(m_textProgram);
    glBindVertexArray(m_textVAO);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, m_textChars);
    glEnable(GL_DEPTH_TEST);    // Enable use of depth testing
}

void HPCEngine::glQuit() noexcept
{
    // Bind default VAO
    glBindVertexArray(0);

    // Delete OpenGL data
    glQuitSphere();
    glQuitCamera();
    glQuitText();

    // Unload the assignment state
    g_hpc.m_assignment.unload();
}

void HPCEngine::glQuitSphere() noexcept
{
    // Release the shader program
    if (m_sphereProgram != 0) {
        glDeleteProgram(m_sphereProgram);
        m_sphereProgram = 0;
    }

    // Delete VBOs/IBOs and VAOs
    if (m_sphereVBO != 0) {
        glDeleteBuffers(1, &m_sphereVBO);
        m_sphereVBO = 0;
    }
    if (m_sphereIBO != 0) {
        glDeleteBuffers(1, &m_sphereIBO);
        m_sphereIBO = 0;
    }
    if (m_sphereVAO != 0) {
        glDeleteVertexArrays(1, &m_sphereVAO);
        m_sphereVAO = 0;
    }
}

void HPCEngine::glQuitCamera() noexcept
{
    // Delete camera UBO
    if (m_cameraUBO != 0) {
        glDeleteBuffers(1, &m_cameraUBO);
        m_cameraUBO = 0;
    }
}

void HPCEngine::glQuitText() noexcept
{
    // Delete used Text Geometry Buffers
    if (m_textABO != 0) {
        glDeleteBuffers(1, &m_textABO);
        m_textABO = 0;
    }
    if (m_textVBO != 0) {
        glDeleteBuffers(1, &m_textVBO);
        m_textVBO = 0;
    }
    // Release the Text VAO
    if (m_textVAO != 0) {
        glDeleteVertexArrays(1, &m_textVAO);
        m_textVAO = 0;
    }
    // Release the text shader program
    if (m_textProgram != 0) {
        glDeleteProgram(m_textProgram);
        m_textProgram = 0;
    }

    // Release text texture
    if (m_charTexture != 0) {
        glDeleteTextures(1, &m_charTexture);
        m_charTexture = 0;
    }
}

bool HPCEngine::run() noexcept
{
    // Initialise SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialise SDL: %s\n", SDL_GetError());
        return false;
    }
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_VERBOSE);
    // Use OpenGL 3.3 core profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Turn on double buffering with a 24bit Z buffer
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Get desktop resolution
    SDL_DisplayMode currentDisplay;
    bool windowFullscreen = WINDOWFULLSCREEN;
    windowFullscreen &= (SDL_GetCurrentDisplayMode(0, &currentDisplay) == 0);

    // Update screen resolution
    g_hpc.m_windowWidth = (windowFullscreen) ? currentDisplay.w : WINDOWWIDTH;
    g_hpc.m_windowHeight = (windowFullscreen) ? currentDisplay.h : WINDOWHEIGHT;

    // Create a SDL window
    SDL_Window* window = SDL_CreateWindow("HPC Assignment", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        g_hpc.m_windowWidth, g_hpc.m_windowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | (static_cast<uint32_t>(windowFullscreen) * SDL_WINDOW_FULLSCREEN));
    if (window == nullptr) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to create OpenGL window: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    // Create a OpenGL Context
    const SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to create OpenGL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // Disable VSync with the OpenGL context
    SDL_GL_SetSwapInterval(0);

    // Initialise OpenGL
    if (g_hpc.glInit()) {
        // Initialise elapsed time
        using clock_type = std::conditional<std::chrono::high_resolution_clock::is_steady, std::chrono::
            high_resolution_clock, std::chrono::steady_clock>::type;
        auto currentTime = clock_type::now();
        // Start the program message pump
        SDL_Event event;
        while (!g_hpc.m_shutdown) {
            // Poll SDL for buffered events
            bool addBalls = false;
            while (SDL_PollEvent(&event) != 0) {
                if (event.type == SDL_QUIT) {
                    g_hpc.m_shutdown = true;
                } else if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        g_hpc.m_shutdown = true;
                    } else if (event.key.keysym.sym == SDLK_SPACE) {
                        addBalls = true;
                    } else if (event.key.keysym.sym == SDLK_p) {
                        g_hpc.m_updateGravity = !g_hpc.m_updateGravity;
                    }
                }
            }
            // Update elapsed frame time
            const auto oldTime = currentTime;
            currentTime = clock_type::now();
            auto elapsed = currentTime - oldTime;

            // Enforce fixed max frame rate to prevent elapsed time getting to small and causing precision errors
            while (elapsed < std::chrono::microseconds(500)) {
                currentTime = clock_type::now();
                elapsed = currentTime - oldTime;
            }
            const float elapsedTime = std::chrono::duration_cast<std::chrono::duration<float>>(elapsed).count();

            // Calculate new rotation
            if (g_hpc.m_updateGravity) {
                if (fabs(g_hpc.m_rotationAngle) > 1.0f) {
                    g_hpc.m_rotationSign = (g_hpc.m_rotationAngle < 0.0f) ? 1.0f : -1.0f;
                }
                g_hpc.m_rotationAngle += (g_hpc.m_rotationSign * elapsedTime * 0.2f) * (1.1f - (g_hpc.m_rotationAngle *
                    g_hpc.m_rotationAngle));
            }

            // Calculate rotation transform
            const float sinAngle = sinf(g_hpc.m_rotationAngle);
            const float cosAngle = cosf(g_hpc.m_rotationAngle);
            const HPCVec3 rot1(-sinAngle, cosAngle, 0.0f);
            const HPCVec3 rot2(cosAngle, sinAngle, 0.0f);

            // Calculate the returned gravity using the rotation matrix
            HPCVec3 gravity(0.0f, -9.81f, 0.0f);
            const HPCVec3 temp = (rot1 * gravity.getY()) + (rot2 * gravity.getX());
            gravity = HPCVec3(_mm_blend_ps(temp.m_vec3, _mm_add_ps(temp.m_vec3, gravity.m_vec3), 0x4));

            // Run the state function
            g_hpc.m_assignment.run(elapsedTime, reinterpret_cast<float*>(&gravity), addBalls);

            //Only render the scene at max 60fps
            g_hpc.m_renderTime += elapsedTime;
            g_hpc.m_frameTime += elapsedTime;
            constexpr float desiredFrameTime = (1.0 / 60.0);
            if (g_hpc.m_renderTime >= desiredFrameTime) {
                // Update camera
                g_hpc.glUpdateCamera(rot1, rot2);

                // Render the scene
                g_hpc.glRender();

                // Swap the back-buffer and present the render
                SDL_GL_SwapWindow(window);

                // Reset time to overflow time
                do {
                    g_hpc.m_renderTime -= desiredFrameTime;
                } while (g_hpc.m_renderTime >= desiredFrameTime);
            }
        }

        // Delete any created GL resources
        g_hpc.glQuit();
    }

    // Delete the OpenGL context, SDL window and shutdown SDL
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return g_hpc.m_shutdown;
}

void HPCEngine::shutdown() noexcept
{
    g_hpc.m_shutdown = true;
}

void HPCEngine::logMessage(const std::string& message) noexcept
{
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, message.c_str());
}

void HPCEngine::updateRenderData(const RenderData* renderData, const uint32_t numRenderItems) noexcept
{
    //Update internal pointer (requires pointer to remain valid till next frame)
    g_hpc.m_renderData = renderData;

    // Update number of render items
    g_hpc.m_numSpheres = numRenderItems;

    //Update frame counter
    g_hpc.m_frameNumber++;
}

char g_charHPCRenderShaderVertex[] =
    "#version 330\n\
\n\
uniform CameraData\n\
{\n\
    mat4 m4ViewProjection;\n\
};\n\
\n\
layout(location = 0) in vec3 vVertex;\n\
layout(location = 1) in vec3 vPosition;\n\
layout(location = 2) in float fRadius;\n\
\n\
smooth out vec3 vVertexPass;\n\
flat out float fRadiusPass;\n\
\n\
void main(void)\n\
{\n\
    vVertexPass = vVertex;\n\
    fRadiusPass = fRadius;\n\
    gl_Position = m4ViewProjection * vec4((vVertex * fRadius) + vPosition, 1.0f);\n\
}\n\
";
char g_charHPCRenderShaderFragment[] =
    "#version 330\n\
\n\
uniform CameraData\n\
{\n\
    mat4 m4ViewProjection;\n\
};\n\
\n\
smooth in vec3 vVertexPass;\n\
flat in float fRadiusPass;\n\
\n\
layout(location = 0) out vec4 FragColor;\n\
\n\
void main(void)\n\
{\n\
    //Transform normal into view space and then do dot product with view vector (0, 0, 1)\n\
    //  This does Goroud shading in view space\n\
    vec4 vNormal = m4ViewProjection * vec4(vVertexPass, 0.0f);\n\
    vec4 vColour;\n\
    vColour.x = (fRadiusPass == 0.5f)? -vNormal.z : 0.0f;\n\
    vColour.y = (fRadiusPass == 1.0f)? -vNormal.z : 0.0f;\n\
    vColour.z = (fRadiusPass == 1.5f)? -vNormal.z : 0.0f;\n\
    vColour.w = 1.0f;\n\
    FragColor = vColour;\n\
}\n\
";
char g_charHPCTextShaderVertex[] =
    "#version 330\n\
\n\
layout(location = 0) in vec2 vVertex;\n\
layout(location = 1) in vec4 vTextData;\n\
layout(location = 2) in vec3 vTextData2;\n\
\n\
smooth out vec3 vTextureCoords;\n\
\n\
void main(void)\n\
{\n\
    vec2 vVertex2 = vVertex;\n\
    vVertex2 *= vTextData.zw;\n\
    vVertex2 += vTextData.xy;\n\
    gl_Position = vec4(vVertex2, -1.0f, 1.0f);\n\
    vTextureCoords = vec3(vVertex, 1.0f) * vTextData2;\n\
}\n\
";
char g_charHPCTextShaderFragment[] =
    "#version 330\n\
\n\
uniform sampler2DArray textArray;\n\
\n\
smooth in vec3 vTextureCoords;\n\
\n\
layout(location = 0) out vec4 FragColor;\n\
\n\
void main(void)\n\
{\n\
    FragColor = texture(textArray, vTextureCoords);\n\
}\n\
";