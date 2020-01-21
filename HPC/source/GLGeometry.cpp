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
#include <GL/glew.h>
#include <cmath>
#include <cstdint>
#include <cstdlib>

class Vec3
{
public:
    Vec3() noexcept = default;

    Vec3(const float x, const float y, const float z) noexcept
        : m_x(x)
        , m_y(y)
        , m_z(z)
    {}

    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_z = 0.0f;
};

struct CustomVertex
{
    Vec3 m_position;
};

GLsizei glGenerateSphere(const uint32_t tessU, const uint32_t tessV, const GLuint indexVBO,
    const GLuint indexIBO) noexcept
{
    // Init parameters
    const float dPhi = static_cast<float>(M_PI) / static_cast<float>(tessV);
    const float dTheta = static_cast<float>(M_PI + M_PI) / static_cast<float>(tessU);

    // Determine required parameters
    const uint32_t numVertices = (tessU * (tessV - 1)) + 2;
    const uint32_t numIndices = (tessU * 6) + (tessU * (tessV - 2) * 6);

    // Create the new primitive
    auto* vBuffer = static_cast<CustomVertex *>(malloc(numVertices * sizeof(CustomVertex)));
    auto* iBuffer = static_cast<GLuint *>(malloc(numIndices * sizeof(GLuint)));

    // Set the top and bottom vertex and reuse
    CustomVertex* vertex = vBuffer;
    vertex->m_position = Vec3(0.0f, 1.0f, 0.0f);
    vertex[numVertices - 1].m_position = Vec3(0.0f, -1.0f, 0.0f);
    vertex++;
    float fPhi = dPhi;
    for (uint32_t uiPhi = 0; uiPhi < tessV - 1; uiPhi++) {
        // Calculate initial value
        const float rSinPhi = sinf(fPhi);
        const float rCosPhi = cosf(fPhi);
        const float y = rCosPhi;
        float theta = 0.0f;
        for (uint32_t uiTheta = 0; uiTheta < tessU; uiTheta++) {
            // Calculate positions
            const float cosTheta = cosf(theta);
            const float sinTheta = sinf(theta);

            // Determine position
            const float x = rSinPhi * cosTheta;
            const float z = rSinPhi * sinTheta;

            // Create vertex
            vertex->m_position = Vec3(x, y, z);
            vertex++;
            theta += dTheta;
        }
        fPhi += dPhi;
    }

    // Create top
    GLuint* index = iBuffer;
    for (GLuint j = 1; j <= tessU; j++) {
        // Top triangles all share same vertex point at pos 0
        *index++ = 0;
        // Loop back to start if required
        *index++ = ((j + 1) > tessU) ? 1 : j + 1;
        *index++ = j;
    }

    // Create inner triangles
    for (GLuint i = 0; i < tessV - 2; i++) {
        for (GLuint j = 1; j <= tessU; j++) {
            // Create indexes for each quad face (pair of triangles)
            *index++ = j + (i * tessU);
            // Loop back to start if required
            const GLuint index1 = ((j + 1) > tessU) ? 1 : j + 1;
            *index++ = index1 + (i * tessU);
            *index++ = j + ((i + 1) * tessU);
            *index = *(index - 2);
            index++;
            // Loop back to start if required
            *index++ = index1 + ((i + 1) * tessU);
            *index = *(index - 3);
            index++;
        }
    }

    // Create bottom
    for (GLuint j = 1; j <= tessU; j++) {
        // Bottom triangles all share same vertex at numVertices-1
        *index++ = j + ((tessV - 2) * tessU);
        // Loop back to start if required
        const GLuint index1 = ((j + 1) > tessU) ? 1 : j + 1;
        *index++ = index1 + ((tessV - 2) * tessU);
        *index++ = numVertices - 1;
    }

    // Fill Vertex Buffer Object
    glBindBuffer(GL_ARRAY_BUFFER, indexVBO);
    glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(CustomVertex), vBuffer, GL_STATIC_DRAW);

    // Fill Index Buffer Object
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(GLuint), iBuffer, GL_STATIC_DRAW);

    // Cleanup allocated data
    free(vBuffer);
    free(iBuffer);

    // Specify location of data within buffer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), nullptr);
    glEnableVertexAttribArray(0);
    return numIndices;
}