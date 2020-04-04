#ifdef HOST_H
#define HOST_H

const int REQ_COUNT = 10;

struct ping_result
{
	bool not_lost;
	int latency;
};

struct Host {
	Host();
	string ip	;
	bool is_available;
	int min_latency ;
	int max_latency ;
	int mean_latency;
	int jitter;
	int per_loss;
	void create(string s, ping_result res[]);
	std::string host_to_string();
};
	

#endif
