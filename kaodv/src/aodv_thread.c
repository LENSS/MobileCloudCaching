/***************************************************************************
                          aodv_thread.c  -  description
                             -------------------
    begin                : Tue Aug 12 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/
#include "aodv_thread.h"

extern struct socket *event_socket;

static int aodv_pid;
wait_queue_head_t aodv_wait;

static atomic_t kill_thread;
static atomic_t aodv_is_dead;

void kick_aodv()
{
    //We are trying to wake up AODV!!!
    //AODV thread is an interupptible sleep state.... this interupts it
    wake_up_interruptible(&aodv_wait);
}

void kill_aodv()
{
    wait_queue_head_t queue;

    init_waitqueue_head(&queue);

    //sets a flag letting the thread know it should die
    //wait for the thread to set flag saying it is dead

    //lower semaphore for the thread
    atomic_set(&kill_thread, 1);
    wake_up_interruptible(&aodv_wait);
    interruptible_sleep_on_timeout(&queue, HZ);
}

void aodv()
{
    //The queue holding all the events to be dealt with
    task *tmp_task;

    //Initalize the variables
    init_waitqueue_head(&aodv_wait);
    atomic_set(&kill_thread, 0);
    atomic_set(&aodv_is_dead, 0);


    //Name our thread
    //lock_kernel();

    //sprintf(current->comm, "kernel-aodv");

    //exit_mm(current);

    //unlock_kernel();

    //add_wait_queue_exclusive(event_socket->sk->sleep,&(aodv_wait));
    //add_wait_queue(&(aodv_wait),event_socket->sk->sleep);
    
    
    //why would I ever want to stop ? :)
    for (;;)
    {
        //should the thread exit?
        if (atomic_read(&kill_thread))
        {
            goto exit;
        }
        //goto sleep until we recieve an interupt
        interruptible_sleep_on(&aodv_wait);

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
                //RREQ
                case TASK_RREQ:
                    recv_rreq(tmp_task);
                    kfree(tmp_task->data);
                    break;

                //RREP
                case TASK_RREP:
            	    recv_rrep(tmp_task);
                    kfree(tmp_task->data);
                    break;

                /*case TASK_RREP_ACK:
                    recv_rrep_ack(tmp_task);
	            kfree(tmp_task->data);
                    break;*/

                //RERR
                case TASK_RERR:
                    recv_rerr(tmp_task);
                    kfree(tmp_task->data);
                    break;

                //Cleanup  the Route Table and Flood ID queue
                case TASK_CLEANUP:
                    //printk(KERN_INFO "a task TASK_CLEANUP\n");
                    flush_flood_id_queue();
               	    flush_aodv_route_table();
                    break;

                case TASK_HELLO:
                    //printk(KERN_INFO "a task TASK_HELLO\n");
            	    send_hello();
              	#ifdef AODV_SIGNAL
	            get_wireless_stats();
	        #endif
		    break;

		case TASK_NEIGHBOR:
		    delete_aodv_neigh(tmp_task->src_ip);
		    break;

		case TASK_ROUTE_CLEANUP:
		    flush_aodv_route_table();
		    break;

		case TASK_RESEND_RREQ:
		    resend_rreq(tmp_task);
		    break;
		        
                default:
                    break;
            }

            kfree(tmp_task);

        }

    }

exit:   
    //Set the flag that shows you are dead
    atomic_set(&aodv_is_dead, 1);
}


void startup_aodv()
{
    aodv_pid = kernel_thread((void *) &aodv, NULL, 0);
}


