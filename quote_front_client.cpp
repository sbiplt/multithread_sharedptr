#include <stdio.h>
#include <signal.h>
#include <memory.h>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
//#include <asio.hpp>
//#include <asio/io_context.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/circular_buffer.hpp>
#include <set>

#define MAX_SOCKET 1
#define BUF_SIZE 8000
#define CONNECT_PER_LOOP 200

unsigned int max_fds = MAX_SOCKET;
std::string sub_msg{ "sub_msg" };
std::string unsub_msg{ "unsub_msg" };
std::string heartbeat_msg{ "heartbeat_msg" };

int log_fd = -1;
char svr_ip[32] = "127.0.0.1";
int port = 11700;

boost::lockfree::spsc_queue<boost::shared_ptr<void>,
	boost::lockfree::capacity<MAX_SOCKET>,
	boost::lockfree::fixed_sized<true> > socket_del_queue;

class tcp_channel : public boost::enable_shared_from_this<tcp_channel>
{
public:
	tcp_channel(boost::asio::io_context &io_):ioc(io_),socket_(io_),ep(boost::asio::ip::address::from_string(svr_ip), port),timer(io_)
	{
	}
	~tcp_channel()
	{
		socket_.close();
		std::cout << "channel release ..." << std::endl;
	}
	
	void release()
	{
		timer.cancel();
		auto self(shared_from_this());
		ioc.post([self] {socket_del_queue.push(self); });
	}

	void send_beat(boost::system::error_code ec)
	{
		if (!ec)
		{
			if(socket_.is_open())
			{
				write_buf_queue.push_back(heartbeat_msg);
			}
			if (!is_writing)
			{
				is_writing = true;
				do_write();
			}
			timer.expires_at(timer.expiry() + boost::asio::chrono::seconds(2));
			auto self(shared_from_this());
			timer.async_wait(boost::bind(&tcp_channel::send_beat, self, std::placeholders::_1));
		}
		else
		{
			printf("time error: Errcode=%d,ErrMsg=%s\n", ec.value(), ec.message().c_str());
			release();
		}
	}

	void do_connect()
	{
		auto self(shared_from_this());
		socket_.async_connect(ep,
			[this, self](boost::system::error_code ec)
		{
			if (!ec)
			{
				do_read();
				write_buf_queue.push_back(sub_msg);
				do_write();

				timer.expires_at(std::chrono::steady_clock::now() + std::chrono::seconds(2));
				timer.async_wait(boost::bind(&tcp_channel::send_beat, self, std::placeholders::_1));
			}
			else
			{
				printf("do_accept failure: Errcode=%d,ErrMsg=%s\n", ec.value(), ec.message().c_str());
				release();
			}
		});
	}

	void do_write()
	{
		auto self(shared_from_this());
		boost::asio::async_write(socket_,
			boost::asio::buffer(write_buf_queue.front()),
			[this, self](boost::system::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				write_buf_queue.pop_front();
				if (write_buf_queue.size() > 0)
				{
					do_write();
				}
				else
				{
					is_writing = false;
				}
			}
			else
			{
				printf("do_write Disconnect:PeerPort=%d,Errcode=%d,ErrMsg=%s\n", socket_.remote_endpoint().port(), ec.value(), ec.message().c_str());
				release();
			}
		});
	}

	void do_read()
	{
		auto self(shared_from_this());
		socket_.async_read_some(boost::asio::buffer(read_buf, BUF_SIZE),
			[this, self](boost::system::error_code ec, std::size_t bytes)
		{
			if (!ec)
			{
				do_read();
			}
			else
			{
				printf("do_read Disconnect:PeerPort=%d,Errcode=%d,ErrMsg=%s\n", socket_.remote_endpoint().port(), ec.value(), ec.message().c_str());
				release();
			}
		});
	}

	char read_buf[BUF_SIZE + 1];
	boost::circular_buffer<std::string> write_buf_queue{ 100 };

	boost::asio::io_context &ioc;
	boost::asio::ip::tcp::socket socket_;
	boost::asio::ip::tcp::endpoint ep;

	bool is_writing{false};

    boost::asio::steady_timer timer; 
};

class event_handler 
{
public:
	void start()
	{
		boost::thread th1([this]
		{
			boost::asio::io_context::work worker(context);
			context.run();
		});

		boost::thread th2([this] {
			boost::shared_ptr<void> ptr;
			while (is_running)
			{
				while (socket_del_queue.pop(ptr))
				{
					channel_set.erase(ptr);
				}
				if (channel_set.size() < max_fds)
				{
					auto ptr = boost::make_shared<tcp_channel>(context);
					channel_set.insert(ptr);
					ptr->do_connect();
				}
				else
				{
					boost::this_thread::sleep_for(boost::chrono::seconds(1));
				}
			}
		});
		th2.join();
	}
    
	void stop()
	{
		is_running = false;
	}

private:
	bool is_running{true};
	boost::asio::io_context context;
	std::set<boost::shared_ptr<void>> channel_set;
};

int main(int argc, char *argv[]) {

    if(argc > 1)
    {
        max_fds = std::stoi(argv[1]);
    }
    std::cout << "max_fds: " << max_fds << std::endl;

    if(argc > 2)
    {
        strncpy(svr_ip, argv[2], sizeof(svr_ip) - 1);
    }
    std::cout << "svr_ip: " << svr_ip << std::endl;

    if(argc > 3)
    {
        port = std::stoi(argv[3]);
    }
    std::cout << "port: " << port << std::endl;

	event_handler eh;

	signal(SIGINT, [](int signum)
	{
		printf("receiv sig int");
		exit(0);
	});

	eh.start();
    return 0;
}

