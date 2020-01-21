#pragma once
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <atomic>
#include <future>
#include <iostream>



using namespace std;

class ThreadPool
{
public:

	/** Constructor. */
	ThreadPool();

	/** Destructor. */
	~ThreadPool();

	/**
	* Gets the number of threads in the pool.
	* @return Number of threads.
	*/
	size_t size() const;

	/** Stops the thread pool from processing or accepting any further jobs. */
	void shutdown();

	void threadFunc();

	

	vector<thread> m_threads;         /**< The threads */
	queue<function<void()>> m_queue;  /**< The queue of pending tasks */
	mutex m_mutex;                    /**< The mutex used to access the task queue */
	condition_variable m_conditionVar;/**< The condition variable used to notify
									  waiting threads of new work items */
	atomic<bool> m_shutdown = false;  /**< Flag to immediately shutdown threads */
	
	/**
	* Adds a work job onto the end of the queue.
	* @param func New task function.
	*/
	void enqueueFunc(function<void()> func);

	/**
	* Adds a work job onto the end of the queue.
	* @param func New task function.
	* @param arg  The argument passed to the function.
	* @return A future used to receive the input functions result.
	*/
	future<int> enqueueFunc(function<int(int)> func, int arg);


	/**
	* Adds a work job onto the end of the queue.
	* @param func New task function.
	* @param args Variable arguments for the associated function.
	* @return A future used to retrieve the work items returned data (if any).
	*/
	/*template<class F, class... Args>
	auto enqueue(F&& f, Args&&... args)->future<result_of_t<F(Args ...)>>*/

	template<class F, class... Args>
	auto enqueue(F&& f, Args&&... args) ->future<result_of_t<F(Args ...)>>
	{
		//Determine the return type of the task
		using ReturnType = result_of_t<F(Args ...)>;
		//Create a packaged task by binding the input task together
		// with its input arguments
		auto task = make_shared<packaged_task<ReturnType()>>(bind(forward<F>(f),
			forward<Args>(args)...));
		//Get the packaged_task's future
		future<ReturnType> ret = task->get_future();
		//Convert the packaged task to a void() function that can be passed to
		// to the original enqueueFunc
		enqueueFunc([task]() {
			(*task)();
		});
		return ret;
	}

};
