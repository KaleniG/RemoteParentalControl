#pragma once

#include "net_common.h"

namespace rpc
{
	template<typename T>
	class tsdeque
	{
	public:
		tsdeque() = default;
		tsdeque(const tsdeque<T>&) = delete;
		virtual ~tsdeque() { clear(); }

	public:
		const T& front()
		{
			std::scoped_lock lock(m_DequeMutex);
			return m_Deque.front();
		}

		const T& back()
		{
			std::scoped_lock lock(m_DequeMutex);
			return m_Deque.back();
		}

		T pop_front()
		{
			std::scoped_lock lock(m_DequeMutex);
			auto t = std::move(m_Deque.front());
			m_Deque.pop_front();
			return t;
		}

		T pop_back()
		{
			std::scoped_lock lock(m_DequeMutex);
			auto t = std::move(m_Deque.back());
			m_Deque.pop_back();
			return t;
		}

		void push_back(const T& item)
		{
			std::scoped_lock lock(m_DequeMutex);
			m_Deque.emplace_back(std::move(item));

			std::unique_lock<std::mutex> ul(m_BlockingMutex);
			m_BlockingCondionVariable.notify_one();
		}

		void push_front(const T& item)
		{
			std::scoped_lock lock(m_DequeMutex);
			m_Deque.emplace_front(std::move(item));

			std::unique_lock<std::mutex> ul(m_BlockingMutex);
			m_BlockingCondionVariable.notify_one();
		}

		bool empty()
		{
			std::scoped_lock lock(m_DequeMutex);
			return m_Deque.empty();
		}
		size_t count()
		{
			std::scoped_lock lock(m_DequeMutex);
			return m_Deque.size();
		}

		void clear()
		{
			std::scoped_lock lock(m_DequeMutex);
			m_Deque.clear();
		}

		void wait()
		{
			while (empty())
			{
				std::unique_lock<std::mutex> ul(m_BlockingMutex);
				m_BlockingCondionVariable.wait(ul);
			}
		}

	private:
		std::mutex m_DequeMutex;
		std::deque<T> m_Deque;
		std::condition_variable m_BlockingCondionVariable;
		std::mutex m_BlockingMutex;
	};
}