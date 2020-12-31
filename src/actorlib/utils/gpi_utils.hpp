#ifndef ACTORGPI_UTILS_HPP
#define ACTORGPI_UTILS_HPP

#include <GASPI.h>
#include <GASPI_Ext.h>
#include <stdlib.h>
#include <cstdint>
#include <stdexcept>
#include <inttypes.h>

namespace gpi_util
{
	namespace
	{
		static gaspi_rank_t local_rank = 65535;
		static gaspi_rank_t total_ranks = 65535;
	}

	static void success_or_exit(const char* file, const int line, const gaspi_return_t ec)
	{
		if(ec!=GASPI_SUCCESS)
		{
			gaspi_printf("Assertions failed in %s[%i]:%d\n",file,line,ec);
			gaspi_string_t error = gaspi_error_str(ec);
			gaspi_printf("Error string: %s\n", error);
			exit(EXIT_FAILURE);
		}
	}
	static gaspi_rank_t get_local_rank()
	{
		if(local_rank == 65535)
			success_or_exit(__FILE__, __LINE__, gaspi_proc_rank(&local_rank));
		return local_rank;
	}
	static gaspi_rank_t get_total_ranks()
	{
		if(total_ranks == 65535)
			success_or_exit(__FILE__, __LINE__, gaspi_proc_num(&total_ranks));
		return total_ranks;
	}
	static gaspi_pointer_t create_segment_return_ptr(int segmentID, uint64_t segmentSize)
	{
		const gaspi_segment_id_t tempID = segmentID;
		const gaspi_size_t tempSize = segmentSize;
		// gaspi_printf("Creating segment ID %d of size %" PRIu64 "\n", segmentID, segmentSize);
		success_or_exit(__FILE__, __LINE__, gaspi_segment_create(tempID, tempSize
								, GASPI_GROUP_ALL, GASPI_BLOCK
								, GASPI_ALLOC_DEFAULT
								)
			);
		gaspi_pointer_t gasptr_locSeg;
		success_or_exit(__FILE__,__LINE__,gaspi_segment_ptr (tempID, &gasptr_locSeg));
		return gasptr_locSeg;
	}
	static void wait_for_flush_queues()
	{
		gaspi_number_t queue_num;
		success_or_exit(__FILE__, __LINE__, gaspi_queue_num(&queue_num));
		gaspi_queue_id_t queue = 0;
		while(queue < queue_num)
		{
			// gaspi_printf("flushing queue %d\n",queue);
			success_or_exit(__FILE__, __LINE__, gaspi_wait(queue, GASPI_BLOCK));
			++queue;
		}
	}
	static void wait_for_queue_entries(gaspi_queue_id_t* queue, int wanted_entries)
	{
		gaspi_number_t queue_size_max, queue_size, queue_num;
		success_or_exit(__FILE__, __LINE__, gaspi_queue_size_max(&queue_size_max));
		success_or_exit(__FILE__, __LINE__, gaspi_queue_size(*queue, &queue_size));

		if(!(queue_size + wanted_entries <= queue_size_max))
		{
			*queue = (*queue + 1) % queue_num;
			success_or_exit(__FILE__, __LINE__, gaspi_wait(*queue, GASPI_BLOCK));
		}
	}
	static void wait_if_queue_full ( const gaspi_queue_id_t queue_id
                        , const gaspi_number_t request_size
                        )
	{
	  gaspi_number_t queue_size_max;
	  gaspi_number_t queue_size;
	 
	  success_or_exit (__FILE__, __LINE__, gaspi_queue_size_max (&queue_size_max));
	  success_or_exit (__FILE__, __LINE__, gaspi_queue_size (queue_id, &queue_size));
	 
	  if (queue_size + request_size >= queue_size_max)
	    {
	      success_or_exit (__FILE__, __LINE__, gaspi_wait (queue_id, GASPI_BLOCK));
	    }
	}
	static bool test_notif_or_exit ( gaspi_segment_id_t segment_id,
									gaspi_notification_id_t notification_id,
									gaspi_notification_t expected)
	{
		//gaspi_printf("Entered");
		gaspi_notification_id_t id;
		gaspi_return_t ret;
		if((ret = gaspi_notify_waitsome(segment_id, notification_id, 1, &id, GASPI_TEST)) == GASPI_SUCCESS)
		{
			if(id != notification_id)
				throw std::runtime_error("ID not equal to notification ID");
			
			gaspi_notification_t value;
			success_or_exit(__FILE__, __LINE__, gaspi_notify_reset(segment_id, id, &value));
			if(value != expected)
				throw std::runtime_error("Notification unexpected value");

			return 1;
		}
		else
		{
			if(ret == GASPI_ERROR)
				throw std::runtime_error("Error while waiting for notification");
			
			return 0;
		}
		
	}

}



#endif
