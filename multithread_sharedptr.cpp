#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/move/move.hpp>
#include "circle_buff.hpp"

boost::asio::io_context io;

class buff
{
public:
	buff(int index_) :index(index_) {
		//std::cout << "construct " << index << std::endl;
	}
	~buff()
	{
		//std::cout << "destruct " << index << std::endl;
	}
private:
	int index;
};

class ptr_consumer
{
public:
	void deliver(boost::shared_ptr<buff> msg)
	{
#if 1
		if (buff.is_empty())
		{
			if (!buff.push(boost::move(msg)))
			{
				std::cout << "buff is full" << std::endl;
			}
			consumer();
		}
		else
		{
			if (!buff.push(boost::move(msg)))
			{
				std::cout << "buff is full" << std::endl;
			}
		}
#else
		
#endif
	}

	void consumer()
	{
		//io.post([this] {
			while (!buff.is_empty())
			{
				buff.front().use_count();
				buff.pop();
			}
		//});
	}

private:
	circle_buffer<buff, 4> buff;
};


class context_
{
public:
	context_() :th([this] {boost::asio::io_context::work worker(ctx); ctx.run(); }) {}
	~context_() { ctx.stop(); }
	boost::asio::io_context ctx;
	boost::thread th;
};

int main()
{
	boost::thread th([] {boost::asio::io_context::work worker(io); io.run(); });


	context_ ctx;
	ptr_consumer ptr1, ptr2, ptr3, ptr4;

	for (long i = 1; i <= 100000; i++)
	{
		if (i % 10000 == 0) {
			std::cout << i << std::endl;
		}
		boost::shared_ptr<buff> msg = boost::make_shared<buff>(i);
		ctx.ctx.post(boost::bind(&ptr_consumer::deliver, &ptr1, msg));
		ctx.ctx.post(boost::bind(&ptr_consumer::deliver, &ptr2, msg));
		ctx.ctx.post(boost::bind(&ptr_consumer::deliver, &ptr3, msg));
		ctx.ctx.post(boost::bind(&ptr_consumer::deliver, &ptr4, msg));
	}

	th.join();
	return 0;
}