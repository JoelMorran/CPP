#include <mutex>
#include <thread>
#include "ThreadPool.h"

using namespace std;

ThreadPool::ThreadPool()
{
	const uint32_t numThreads = max(thread::hardware_concurrency(), 1U);
	for (uint32_t i = 0; i < numThreads; ++i) {
		m_threads.emplace_back(&ThreadPool::threadFunc, this);
	}
}

ThreadPool::~ThreadPool()
{
	shutdown();
	for (auto& worker : m_threads) {
		worker.join();
	}
}

size_t ThreadPool::size() const
{
	return m_threads.size();;
}

void ThreadPool::shutdown()
{
	lock_guard<mutex> lock(m_mutex);
	m_shutdown = true;
	m_conditionVar.notify_all();
}

void ThreadPool::threadFunc()
{
	while (true) {
		function<void()> task;
		{
			//Try and get the queue lock. Wait on the condition variable 
			// if the lock could not be immediately acquired
			unique_lock<mutex> lock(m_mutex);
			m_conditionVar.wait(lock, [this] {
				return m_shutdown || !m_queue.empty();
			});
			//Check that the pool has not been shut down and exit if it has
			if (m_shutdown) {
				return;
			}
			//Move the queued task from the queue
			task = move(m_queue.front());
			m_queue.pop();
		}
		//Execute the new task
		task();
	}
}




void ThreadPool::enqueueFunc(function<void()> func)
{
	lock_guard<mutex> lock(m_mutex);
	if (m_shutdown.load()) {
		throw runtime_error("enqueue on stopped ThreadPool");
	}
	m_queue.push(move(func));
	m_conditionVar.notify_one();
}

future<int> ThreadPool::enqueueFunc(function<int(int)> func, int arg)
{
	//Create a packaged task by binding the input task together with 
	//its input argument
	auto task = make_shared<packaged_task<int()>>(bind(func, arg));
	//Get the packaged_task's future
	future<int> ret = task->get_future();
	//Convert the packaged task to a void() function that can be passed
	// to the original enqueueFunc
	enqueueFunc([task]() {
		(*task)();
	});
	return ret;
}



