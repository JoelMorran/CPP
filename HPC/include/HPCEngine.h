/**
 * @file HPCEngine.h
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
 * This declares the base HPCEngine class. This is provided to abstract some of the
 * more complex aspects of completing the HPC Assignment.
 */
#ifndef HPCENGINE_H
#define HPCENGINE_H
#include "HPCAssignment.h"
#include <cstdint>
#include <map>
#include <string>
class HPCVec3;

class HPCEngine
{
public:
    /** Default constructor. */
    HPCEngine() noexcept;

    /** Default destructor. */
    ~HPCEngine() noexcept;

    /**
     * Copy constructor
     * @param other The other.
     */
    HPCEngine(const HPCEngine& other) = delete;

    /**
     * Move constructor
     * @param [in,out] other The other.
     */
    HPCEngine(HPCEngine&& other) noexcept = delete;

    /**
     * Assignment operator
     * @param other The other.
     * @return A shallow copy of this HPCEngine.
     */
    HPCEngine& operator=(const HPCEngine& other) = delete;

    /**
     * Move assignment operator
     * @param [in,out] other The other.
     * @return A shallow copy of this HPCEngine.
     */
    HPCEngine& operator=(HPCEngine&& other) noexcept = delete;

    /**
     * Runs the engine.
     * @note Starts the internal engine loop. The engine will continue to run until it receives a quit command.
     * All internal processing is done within this function.
     * @return Boolean signalling if the engine was able to run or not.
     */
    static bool run() noexcept;

    /**
     * Shuts down the engine.
     * @note Tells the engine to shut down. This will cause the engine to terminate once
     * the current loop has finished executing.
     */
    static void shutdown() noexcept;

    /**
     * Outputs a string to a program log file.
     * @param message The string to output to the log file. For proper formatting output strings should be terminated
     * with an end line where appropriate.
     */
    static void logMessage(const std::string& message) noexcept;

    /** Basic structure used to enforce conformance of input data. */
    struct RenderData
    {
        float m_positionX, m_positionY, m_positionZ;
        float m_radius;    // Must be either 0.5, 1.0 or 1.5
    };

    /**
     * Updates internal render function with required data.
     * @note This must be used to inform the renderer of required render items. After this
     * function is called the renderer will then render numRenderItems of spheres using the
     * information passed in through renderData. The input pointer does not have to be to an array
     * of RenderData items but it must be a pointer to a list of data that conforms with the layout
     * assumed by RenderData. This function runs asynchronously so renderData must remain valid
     * until the next frame.
     * @param renderData     Pointer to array of data holding new updated values.
     * @param numRenderItems The number of render items in the update array.
     */
    static void updateRenderData(const RenderData* renderData, uint32_t numRenderItems) noexcept;

private:
    bool m_shutdown = false;      /**< Variable indicating if engine should terminate */
    float m_rotationAngle = 0.0f; /**< The current gravity rotation angle */
    float m_rotationSign = 1.0f;  /**< The current gravity rotation angle direction of change */
    bool m_updateGravity = true;  /**< Variable indicating if gravity should be rotated */
    HPCAssignment m_assignment;   /**< The assignment state */
    const RenderData* m_renderData = nullptr; /**< Pointer to external sphere position list */
    uint32_t m_numSpheres = 0;    /**< Number of spheres to be rendered */
    uint32_t m_frameNumber = 0;   /**< Number of frames since last FPS update */

    // Data required for frame rate calculations
    float m_renderTime = 0.0f;  /**< Elapsed time since last render update */
    float m_frameTime = 0.0f;   /**< Elapsed time since last FPS update */

    // Data required for camera update
    uint32_t m_windowWidth;     /**< Width of the window */
    uint32_t m_windowHeight;    /**< Height of the window */
    uint32_t m_cameraUBO = 0;   /**< Uniform Buffer object for camera data */

    // Data required for sphere rendering
    uint32_t m_sphereProgram = 0; /**< Sphere shader program */
    uint32_t m_sphereVAO = 0;     /**< Sphere Vertex Array object*/
    uint32_t m_sphereIndices = 0; /**< Number of indices in sphere Index Buffer */

    // Data required for text rendering
    char m_overlayString[30] = "FPS:        \nNum Balls:      ";

    struct GlyphData
    {
        int32_t m_offsetX;         /**< Offset till start of char glyph along X axis */
        int32_t m_offsetY;         /**< Offset till start of char glyph along Y axis */
        int32_t m_sizeX;           /**< Size of char glyph along X axis */
        int32_t m_sizeY;           /**< Size of char glyph along Y axis */
        int32_t m_texturePosition; /**< Position within texture array that this char is located */
    };

    std::map<char, GlyphData> m_glyphData;  /**< Character map used to store Glyph information for text rendering */
    uint32_t m_textProgram = 0;             /**< Shader program for text rendering */
    uint32_t m_textVAO = 0;                 /**< Text Vertex Array object */
    uint32_t m_textChars = 0;               /**< The number of characters in the text buffer */

    // Remaining data required for sphere rendering
    uint32_t m_sphereVBO = 0;   /**< Sphere OpenGL VBO index */
    uint32_t m_sphereIBO = 0;   /**< Sphere OpenGL EBO index */
    uint32_t m_sphereABO = 0;   /**< Array Buffer for instanced sphere rendering */

    // Remaining data required for text rendering
    uint32_t m_textVBO = 0;     /**< OpenGL Text VBO index */
    uint32_t m_textABO = 0;     /**< Array Buffer for instanced text rendering */
    uint32_t m_charTexture = 0; /**< Character texture used for font rendering */

    // Data required for text rendering
    struct TextData
    {
        float m_globalOffsetX;
        float m_globalOffsetY;
        float m_width;
        float m_height;
        float m_textureWidth;
        float m_textureHeight;
        float m_textureInstance;
    };

    /**
     * Initialise required OpenGL data.
     * @return True if it succeeds, false if it fails.
     */
    bool glInit() noexcept;

    /**
     * Initialise required OpenGL data for sphere rendering.
     * @return True if it succeeds, false if it fails.
     */
    bool glInitSphere() noexcept;

    /**
     * Initialise required OpenGL data for camera movement.
     * @return True if it succeeds, false if it fails.
     */
    bool glInitCamera() noexcept;

    /**
     * Initialise required OpenGL data for text overlay rendering.
     * @return True if it succeeds, false if it fails.
     */
    bool glInitText() noexcept;

    /**
     * Updates the render camera to a specific view rotation.
     * @note This is used to control the render cameras position and view direction.
     * @param rot1 The first component of the rotation transform.
     * @param rot2 The second component of the rotation transform.
     */
    void glUpdateCamera(const HPCVec3& rot1, const HPCVec3& rot2) const noexcept;

    /**
     * Updates the text data for text overlay rendering.
     * @param fps The fps value to display.
     */
    void glUpdateText(float fps);

    /** Perform required OpenGL rendering operations. */
    void glRender() noexcept;

    /** Perform required OpenGL operations to render the spheres. */
    void glRenderSpheres() const noexcept;

    /** Perform required OpenGL operations to render the text overlay. */
    void glRenderText() noexcept;

    /** Free up all OpenGL data. */
    void glQuit() noexcept;

    /** Free up all OpenGL data used for sphere rendering. */
    void glQuitSphere() noexcept;

    /** Free up all OpenGL data used for camera movement. */
    void glQuitCamera() noexcept;

    /** Free up all OpenGL data used for text overlay rendering. */
    void glQuitText() noexcept;
};
#endif
