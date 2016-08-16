
#include "cThread.h"

static int cache_pid;
wait_queue_head_t cache_wait;

static atomic_t kill_thread;
static atomic_t cache_is_dead;

void kick_cache()
{
    //We are trying to wake up Cache Daemon!!!
    //Cache thread is an interupptible sleep state.... this interupts it
    wake_up_interruptible(&cache_wait);
}

void kill_cache()
{
    wait_queue_head_t queue;

    init_waitqueue_head(&queue);

    //sets a flag letting the thread know it should die
    //wait for the thread to set flag saying it is dead

    //lower semaphore for the thread
    atomic_set(&kill_thread, 1);
    wake_up_interruptible(&cache_wait);
    interruptible_sleep_on_timeout(&queue, HZ);
}

void cache()
{
    //The queue holding all the events to be dealt with
    cTask *tmp_task;

    //Initalize the variables
    init_waitqueue_head(&cache_wait);
    atomic_set(&kill_thread, 0);
    atomic_set(&cache_is_dead, 0);
    
    //why would I ever want to stop ? :)
    for (;;)
    {
        //should the thread exit?
        if (atomic_read(&kill_thread))
        {
            goto exit;
        }
        //goto sleep until we recieve an interupt
        interruptible_sleep_on(&cache_wait);

        //should the thread exit?
        if (atomic_read(&kill_thread))
        {
            goto exit;
        }
        //While the buffer is not empty
        while ((tmp_task = get_task()) != NULL)
        {
            //takes a different action depending on what type of event is recieved
            switch (tmp_task->type)
            {
                case TASK_FILE_REQ_PROCESS:
                    recv_flreq(tmp_task);
                    kfree(tmp_task->data);
                    break;

                case TASK_FILE_REP_PROCESS:
            	    recv_flrep(tmp_task);
                    kfree(tmp_task->data);
                    break;

                case TASK_FRAG_REQ_UPDATE:
                    recv_frreq_ori(tmp_task);
                    kfree(tmp_task->data);
                    break;

                case TASK_FRAG_REQ_FLUSH:
		    recv_frreq_fwd(tmp_task);
                    kfree(tmp_task->data);
                    break;

                case TASK_EXCH_SEND:
		    recv_exreq(tmp_task);
                    kfree(tmp_task->data);
		    break;

		case TASK_EXCH_UPDATE:
		    recv_exrep(tmp_task);
                    kfree(tmp_task->data);
		    break;

		case TASK_ADD_CACHE_PROCESS:
		    recv_addcache(tmp_task);
		    kfree(tmp_task->data);
		    break;

		case TASK_FRAG_REQ_SEND:
                    process_flrep(tmp_task);
                    break;
		        
		case TASK_ADD_CACHE_SEND:
		    select_agent(tmp_task);
		    break;

		case TASK_CLEAN_UP:
		    flush_flood_id_queue();
		    flush_rinfo();
		    flush_extable();
		    break;

                default:
                    break;
            }

            kfree(tmp_task);

        }

    }

exit:   
    //Set the flag that shows you are dead
    atomic_set(&cache_is_dead, 1);
}


void startup_cache()
{
    cache_pid = kernel_thread((void *) &cache, NULL, 0);
}


