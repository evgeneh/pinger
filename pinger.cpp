#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <istream>
#include <iostream>
#include <ostream>

#include "icmp_header.hpp"
#include "ipv4_header.hpp"


using boost::asio::ip::icmp;
using boost::asio::deadline_timer;
namespace posix_time = boost::posix_time;
 
const int REQ_COUNT = 10;

struct ping_result
{
	bool not_lost;
	int latency;
};

struct Host {
	Host(): max_latency(0), min_latency(9000), is_available(false)
	{} 
	std::string ip	;
	bool is_available;
	int min_latency ;
	int max_latency ;
	int mean_latency;
	int jitter;
	int per_loss;
	void create(std::string s, ping_result res[])
	{
		ip = s;
		long total_time = 0;
		int total_count = 0;		
		for (int i = 0; i < REQ_COUNT; i++)
		if (res[i].not_lost)
		{		
			is_available = true;
			total_time += res[i].latency;
			total_count++; 
			if (res[i].latency > max_latency)
				max_latency = res[i].latency;
			if (res[i].latency < min_latency)
				min_latency = res[i].latency;
		}
		mean_latency = total_time / total_count;
		per_loss = 100 * (REQ_COUNT - total_count) / REQ_COUNT;
		jitter = max_latency - min_latency;   
		
	}
	std::string host_to_string()
	{ 
		if (!is_available) 
			return "host is not available";
		return "latency: " + std::to_string(mean_latency) + "ms, jitter: " + std::to_string(jitter) + ", loss:" + std::to_string(per_loss) + "%";
	}
};
	

ping_result ping_results[REQ_COUNT];

class pinger
{
public:
  pinger(boost::asio::io_service& io_service, const char* destination)
    : resolver_(io_service), socket_(io_service, icmp::v4()),
      timer_(io_service), sequence_number_(0), num_replies_(0), counter(0)
  {
	std::cout << "Pinger created " << destination << std::endl;	
	
    icmp::resolver::query query(icmp::v4(), destination, "");
    destination_ = *resolver_.resolve(query);

    start_send();
    start_receive();

    return;
  }

private:
  //	
  int counter;
  void start_send()
  {
    std::string body("\"Hello!\" from Asio ping.");

    // Create an ICMP header for an echo request.
    icmp_header echo_request;
    echo_request.type(icmp_header::echo_request);
    echo_request.code(0);
    echo_request.identifier(get_identifier());
    echo_request.sequence_number(++sequence_number_);
    compute_checksum(echo_request, body.begin(), body.end());

    // Encode the request packet.
    boost::asio::streambuf request_buffer;
    std::ostream os(&request_buffer);
    os << echo_request << body;

    // Send the request.
    time_sent_ = posix_time::microsec_clock::universal_time();
    socket_.send_to(request_buffer.data(), destination_);

    // Wait up to five seconds for a reply.
    num_replies_ = 0;

    if (counter++ >= REQ_COUNT)
    {	
 	return;
    }

    timer_.expires_at(time_sent_ + posix_time::seconds(5));
    timer_.async_wait(boost::bind(&pinger::handle_timeout, this));
  }

  void handle_timeout()
  {
    if (num_replies_ == 0)
      std::cout << "Request timed out" << std::endl;

    // Requests must be sent no less than one second apart.
    timer_.expires_at(time_sent_ + posix_time::seconds(1));
    timer_.async_wait(boost::bind(&pinger::start_send, this));
  }

  void start_receive()
  {
    if (counter >= REQ_COUNT)
    {	
	return;
    }
	
    // Discard any data already in the buffer.
    reply_buffer_.consume(reply_buffer_.size());

    // Wait for a reply. We prepare the buffer to receive up to 64KB.
    socket_.async_receive(reply_buffer_.prepare(65536),
        boost::bind(&pinger::handle_receive, this, _2));
  }

  void handle_receive(std::size_t length)
  {
    // The actual number of bytes received is committed to the buffer so that we
    // can extract it using a std::istream object.
    reply_buffer_.commit(length);

    // Decode the reply packet.
    std::istream is(&reply_buffer_);
    ipv4_header ipv4_hdr;
    icmp_header icmp_hdr;
    is >> ipv4_hdr >> icmp_hdr;
    
    // filter out only the echo replies that match the our identifier and
    // expected sequence number.
    if (is && icmp_hdr.type() == icmp_header::echo_reply
          && icmp_hdr.identifier() == get_identifier()
          && icmp_hdr.sequence_number() == sequence_number_)
    {
      // If this is the first reply, interrupt the five second timeout.
      if (num_replies_++ == 0)
        timer_.cancel();
	
      posix_time::ptime now = posix_time::microsec_clock::universal_time();
      long latency = (now - time_sent_).total_milliseconds();		
      ping_results[counter-1] = {true, (int)latency};
      // Print out some information about the reply packet.
      
      std::cout << "icmp_seq=" << icmp_hdr.sequence_number() <<" c:"<< counter
                << ", time=" << latency << "ms" << std::endl;
    }
		
    start_receive();
  }

  static unsigned short get_identifier()
  {
    return static_cast<unsigned short>(::getpid());
  }

  icmp::resolver resolver_;
  icmp::endpoint destination_;
  icmp::socket socket_;
  deadline_timer timer_;
  unsigned short sequence_number_;
  posix_time::ptime time_sent_;
  boost::asio::streambuf reply_buffer_;
  std::size_t num_replies_;
};
 
int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: ping <host>" << std::endl;

      return 1;
    }

    boost::asio::io_service io_service;
    pinger p(io_service, argv[1]);
    io_service.run();
	Host host;
	host.create(std::string(argv[1]), ping_results);
	
		
	std::cout << host.host_to_string() << std::endl;
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}

