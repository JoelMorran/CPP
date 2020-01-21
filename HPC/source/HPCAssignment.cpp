/**
 * @file HPCAssignment.cpp
 *
 * @section LICENSE
 *
 * This code is not available for commercial use. This code is provided as is
 * and the author claims no responsibility for any issues, damages or any other
 * ill effects resulting from use of this code.
 *
 * @section DESCRIPTION
 *
 * This defines the base HPCAssignment class. This is provided to abstract some of the
 * more complex aspects of completing the HPC Assignment.
 */
#include "HPCAssignment.h"
#include "HPCEngine.h"
#include <cstdint>
#include <thread>
using namespace std;

void HPCAssignment::addBalls()
{
	//blue ballz
		for (float x = -38.0f; x < 38.0f; x += 4.5f) {
			for (float z = -38.0f; z < 38.0f; z += 4.0f) {
				myballz.push_back(Vector3(x, 38.0f, z, 1.5f));
				myvelocityz.push_back(Vector3(0.0f));
			}
		}
	//green
		for (float x = -38.0f; x < 38.5f; x += 4.5f) {
			for (float z = -38.0f; z < 38.5f; z += 4.5f) {
				myballz.push_back(Vector3(x, 35.3f, z, 1.0f));
				myvelocityz.push_back(Vector3(0.0f));
			}
		}
	//red

		for (float x = -38.0f; x < 38.5f; x += 4.3f) {
			for (float z = -38.0f; z < 38.5f; z += 3.5f) {
				myballz.push_back(Vector3(x, 32.0f, z, 0.5f));
				myvelocityz.push_back(Vector3(0.0f));
			}
		}
	myballz2.reserve(myballz.size());
	myballz2.resize(myballz.size());
	myvelocityz2.reserve(myvelocityz.size());
	myvelocityz2.resize(myvelocityz.size());
}



void HPCAssignment::doSomeBallStuff(uint32_t start, uint32_t end, const float elapsedTime, const Vector3* gravityVec)
{
	Vector3 kw = Vector3(-500);
	Vector3 bw = Vector3(10);
	Vector3 kb = Vector3(-300);
	Vector3 bb = Vector3(5);

	for (uint32_t current = start; current < end; current++)
	{
		
			Vector3 pointp = myballz[current];
			Vector3 radius = pointp.getR();
			Vector3 pointv = myvelocityz[current];
			//d = pa - pb ??????
			Vector3 force = Vector3(0);

			Vector3 four = Vector3(40.0);
			Vector3 xp = (pointp + radius) - four;
			Vector3 match = Vector3().lessThan(xp);//cmplt
			Vector3 force2 = (((kw * xp) - (bw * pointv)));
			force2 = force2 & match;
			force += force2;

			Vector3 xn = four + (pointp - radius);
			Vector3 match2 = xn.lessThan(Vector3());
			Vector3 force3 = (((kw * xn) - (bw * pointv)));
			force3 = force3 & match2;
			force += force3;

			
			for (uint32_t current2 = 0; current2 < myballz.size(); current2++) {

				if (current != current2)
				{
					Vector3 pointp2 = myballz[current2];
					Vector3 d = pointp - pointp2;
					Vector3 length = d.length();
					Vector3 radius2 = pointp2.getR();
						if(length < (radius + radius2))
						{
							Vector3 pointv2 = myvelocityz[current2];
							Vector3 nor = d / length;
							Vector3 x = length - (radius + radius2);
							Vector3 vs = (pointv - pointv2).dot3(nor);
							//normalise = d / d.length

							force += nor * ((kb * x) - (bb * vs));
						}

						
				}
			}

			Vector3 accleration = (force / (radius + radius)) + *gravityVec;

			Vector3 newpos = pointp + ((pointv + (accleration * elapsedTime)) * elapsedTime);
			newpos.setR(radius);
			myballz2[current] = newpos;
			//calculate velocity
			Vector3 newvelocity = (newpos - pointp) / elapsedTime;
			myvelocityz2[current] = newvelocity;
	}


}

bool HPCAssignment::load() noexcept
{
    /* Add required start up code here */
	addBalls();
    return true;
}

void HPCAssignment::run(const float elapsedTime, float* gravity, const bool addBall) noexcept
{
    /* Add required code here */
    /* Note gravity can be converted to whatever Vector 3 type you are using through a simple cast
       e.g. Vector3 gravityVec = *reinterpret_cast<Vector3*>(gravity);
    */
	Vector3 gravityVec = *reinterpret_cast<Vector3*>(gravity);
	if (addBall == true) {
		addBalls();
	}
	//thread pool of doSomeBallStuff
	int numBalls = myballz.size() / (threads.size() * 2);
	int numThreads = myballz.size() / numBalls;
	vector<std::future<void>> waits;
	for (int i = 0; i < numThreads-1; i++) {
		waits.emplace_back(threads.enqueue(&HPCAssignment::doSomeBallStuff, this, i*numBalls, (i+1)*numBalls, elapsedTime, &gravityVec));
	}
	waits.emplace_back(threads.enqueue(&HPCAssignment::doSomeBallStuff, this, (numThreads - 1 )*numBalls, myballz.size(), elapsedTime, &gravityVec));

	for (auto& w : waits) {
		w.get();
	}

	std::swap(myballz, myballz2);
	std::swap(myvelocityz, myvelocityz2);

	HPCEngine::updateRenderData((HPCEngine::RenderData*)myballz.data(), myballz.size());
}

void HPCAssignment::unload() noexcept
{
    /* Add required shut down code here */
}


