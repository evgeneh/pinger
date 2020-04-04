#include <iostream>
#include <string>
#include "host.h"

Host::Host(): max_latency(0), min_latency(9000), is_available(false)
	{} 

void Host::create(std::string s, ping_result res[])
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
std::string Host::host_to_string()
	{ 
		if (!is_available) 
			return "host is not available";
		return "latency: " + std::to_string(mean_latency) + "ms, jitter: " + std::to_string(jitter) + ", loss:" + std::to_string(per_loss) + "%";
	}
