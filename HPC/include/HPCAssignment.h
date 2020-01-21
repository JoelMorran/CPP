/**
 * @file HPCAssignment.h
 *
 * @section LICENSE
 *
 * This code is not available for commercial use. This code is provided as is
 * and the author claims no responsibility for any issues, damages or any other
 * ill effects resulting from use of this code.
 *
 * @section DESCRIPTION
 *
 * This declares the base HPCAssignment class. This is provided to abstract some of the
 * more complex aspects of completing the HPC Assignment.
 */
#ifndef HPCASSIGNMENT_H
#define HPCASSIGNMENT_H
#include <cmath>
#include <vector>
#include "Vector3_SSE.h"
#include "ThreadPool.h"
using namespace std;


class HPCAssignment
{
public:
    /** Default constructor. */
    HPCAssignment() = default;

    /** Destructor. */
	~HPCAssignment() noexcept {};

    /**
     * Copy constructor.
     * @param other The other HPCAssignment.
     */
    HPCAssignment(const HPCAssignment& other) = delete;

    /**
     * Move constructor.
     * @param [in,out] other The other HPCAssignment.
     */
    HPCAssignment(HPCAssignment&& other) noexcept = delete;

    /**
     * Assignment operator.
     * @param other The other HPCAssignment.
     * @return A shallow copy of this HPCAssignment.
     */
    HPCAssignment& operator=(const HPCAssignment& other) = delete;

    /**
     * Move assignment operator.
     * @param [in,out] other The other HPCAssignment.
     * @return A shallow copy of this HPCAssignment.
     */
    HPCAssignment& operator=(HPCAssignment&& other) noexcept = delete;

    /**
     * Loads any required data needed for operation.
     * @return True if it succeeds, false if it fails.
     */
    bool load() noexcept;

    /**
     * Run the assignment state.
     * @param      elapsedTime The elapsed time since last frame.
     * @param [in] gravity     The gravity vector (cast to desired Vector3 type to use).
     * @param      addBalls    True if more balls should be added.
     */
    void run(float elapsedTime, float* gravity, bool addBalls) noexcept;

    /** Unloads any data created during load() */
    void unload() noexcept;

private:
    /* Add any required member variables here */
	vector<Vector3> myballz;
	vector<Vector3> myvelocityz;

	vector<Vector3> myballz2;
	vector<Vector3> myvelocityz2;

	ThreadPool threads;
	

	void addBalls();
	void doSomeBallStuff(uint32_t start, uint32_t end, const float elapsedTime, const Vector3* gravityVec);
};
#endif
