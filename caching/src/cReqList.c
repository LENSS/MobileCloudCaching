
#include "cReqList.h"

extern int THRESHOLD;
reqList *crlist;
rwlock_t crlist_lock = RW_LOCK_UNLOCKED;

inline void crlist_read_lock()
{
    read_lock_bh(&crlist_lock);
}

inline void crlist_read_unlock()
{
    read_unlock_bh(&crlist_lock);
}

inline void crlist_write_lock()
{
    write_lock_bh(&crlist_lock);
}

inline void crlist_write_unlock()
{	
    write_unlock_bh(&crlist_lock);
}

void init_req_list()
{
    crlist = NULL;
}

reqList *find_creq(u_int32_t fileId, u_int32_t fragId) {
    crlist_read_lock();
    reqList *tmp_creq = crlist;
    while (tmp_creq != NULL && (tmp_creq -> file_id != fileId || tmp_creq -> frag_id != fragId)) {
	tmp_creq = tmp_creq -> next;
    }
    crlist_read_unlock();
    return tmp_creq;
}

int reachThreshold(reqList *tmp_creq) {
    
    crlist_read_lock();

    if (tmp_creq == NULL || tmp_creq -> count < (THRESHOLD - 1)) {
	crlist_read_unlock();
	return 0;
    }

    crlist_read_unlock();
    return 1;
}

int update_count(reqList *tmp_creq) {
    
    crlist_write_lock();

    tmp_creq -> count = tmp_creq -> count + 1;
    //printk("count for this frag request: %d\n", tmp_creq -> count);

    crlist_write_unlock();
    return 1;
}

int flush_count(reqList *tmp_creq) {
    
    crlist_write_lock();

    if (tmp_creq != NULL) {
	tmp_creq -> count = 0;
    }
    
    crlist_write_unlock();
    return 1;
}

void remove_creq(reqList *dead_creq) {
    crlist_write_lock();

    if (crlist == dead_creq) {
        crlist = dead_creq -> next;
    }
    if (dead_creq -> prev != NULL) {
        dead_creq -> prev -> next = dead_creq -> next;
    }
    if (dead_creq -> next != NULL) {
        dead_creq -> next -> prev = dead_creq -> prev;
    }

    crlist_write_unlock();
    kfree(dead_creq);
}

int delete_creq(u_int32_t fileId, u_int32_t fragId) {
    reqList *dead_creq = find_creq(fileId, fragId);
    crlist_read_lock();

    if (dead_creq != NULL) {
	crlist_read_unlock();
	remove_creq(dead_creq);
	return 1;
    }

    crlist_read_unlock();
    return 0;
}

int cleanup_req_list() {
    reqList *dead_creq, *tmp_creq;
    crlist_write_lock();

    tmp_creq = crlist;
    while (tmp_creq != NULL) 
    {
	dead_creq = tmp_creq;
        tmp_creq = tmp_creq -> next;
	kfree(dead_creq);
    }
    crlist = NULL;

    crlist_write_unlock();
    return 0;
}

void insert_creq(reqList *new_creq) {
    crlist_write_lock();

    if (crlist == NULL) {
	crlist = new_creq;
	crlist_write_unlock();
	return;
    }

    new_creq -> next = crlist;
    crlist -> prev = new_creq;
    crlist = crlist -> prev;
 
    crlist_write_unlock();
}

reqList *create_creq(u_int32_t fileId, u_int32_t fragId) {
    reqList *tmp_creq;

    /* Allocate memory for new entry */

    if ((tmp_creq = (reqList *) kmalloc(sizeof(reqList), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: Error getting memory for new creq\n");
        return NULL;
    }

    tmp_creq -> file_id = fileId;
    tmp_creq -> frag_id = fragId;
    tmp_creq -> count = 0;
    tmp_creq -> prev = NULL;
    tmp_creq -> next = NULL;

    insert_creq(tmp_creq);

    return tmp_creq;
}

int read_creq_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length,int *eof,void *data)
{
    static char *my_buffer;
    char temp_buffer[200];
    char temp[100];
    reqList *tmp_entry;
    int len, i;
    
    /*lock table*/
    crlist_read_lock();

    tmp_entry = crlist;

    my_buffer = buffer;

    sprintf(my_buffer, "\nFragment Request List \n----------------------------------------------------------\n");
    sprintf(temp_buffer, "        fileId        |      fragId      |     Count    \n");
    strcat(my_buffer, temp_buffer);
    sprintf(temp_buffer, "----------------------------------------------------------\n");
    strcat(my_buffer, temp_buffer);

    while (tmp_entry != NULL)
    {
        sprintf(temp_buffer, "        %6d                %6d            %5d    \n", tmp_entry -> file_id, tmp_entry -> frag_id, tmp_entry -> count);
        strcat(my_buffer,temp_buffer);
	
        tmp_entry = tmp_entry -> next;
    }

    /*unlock table*/
    crlist_read_unlock();

    strcat(my_buffer,"----------------------------------------------------------\n\n");

    len = strlen(my_buffer);
    *buffer_location = my_buffer + offset;
    len -= offset;
    if (len > buffer_length)
        len = buffer_length;
    else if (len < 0)
        len = 0;
    return len;
}
	

