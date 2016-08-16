
#include "cFileRep.h"

extern u_int32_t   g_my_ip;
extern cacheTable *ctable;
extern u_int32_t	cEnergy;
extern u_int32_t	cSuccess;
extern char g_act[8];
extern int KVAL;
fileRepInfo *rinfo;
rwlock_t rinfo_lock = RW_LOCK_UNLOCKED;

int recv_flrep(cTask *tmp_task) {
    //printk("entering recv_flrep\n");
    if (tmp_task -> dst_ip != g_my_ip) {
	return 1;
    }

    fileRep *tmp_flrep = (fileReq *)tmp_task -> data;
    u_int32_t fileId = tmp_flrep -> file_id;
    //printk("fileId = %d\n", fileId);
    fileRepInfo *new_rinfo = find_rinfo(rinfo, fileId);

    if (new_rinfo == NULL) {
	insert_timer(TASK_FRAG_REQ_SEND, WAIT_FILE_REP_TIME, g_my_ip, fileId, 0);
	update_timer_queue();
    }
    
    create_rinfo(tmp_task);
}

int gen_flrep(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t file_id, u_int32_t dist) {
    fileRep *tmp_flrep;
    u_int32_t *frags;
    u_int32_t buf[KVAL];
    int count = 0;
    int i;
    cacheTable *tmp_cache = find_cache_by_fl(ctable, file_id);
    while (count < KVAL && tmp_cache != NULL) {
	buf[count] = tmp_cache -> frag_id;
	//printk("buf[%d] = %d\n", count, buf[count]);
	count++;
	tmp_cache = find_cache_by_fl(tmp_cache -> next, file_id);
    }

    if (count == 0) {
	return 1;
    }

    //printk("count = %d\n", count);

    if ((tmp_flrep = (fileRep *) kmalloc((sizeof(fileRep) + sizeof(u_int32_t) * count), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: Can't allocate new file reply\n");
        return 0;
    }

    tmp_flrep -> type = FILE_REP_MESSAGE;
    tmp_flrep -> reserved = 0;
    tmp_flrep -> frag_count = count;
    tmp_flrep -> src_ip = htonl(src_ip);
    tmp_flrep -> dst_ip = htonl(dst_ip);
    tmp_flrep -> file_id = htonl(file_id);
    tmp_flrep -> dist = htonl(dist);
    //printk("dist = %d\n", tmp_flrep -> dist);
    
    frags = (u_int32_t *)((void*)tmp_flrep + sizeof(fileRep));
    for (i = 0; i < count; i++) {
	frags[i] = htonl(buf[i]);
	//printk("buf[%d] = %d, frags[%d] = %d\n", i, buf[i], i, frags[i]);
    }

    //printk("dst_ip = %s\n", inet_ntoa(dst_ip));
    send_message(dst_ip, NET_DIAMETER, tmp_flrep, (sizeof(fileRep) + sizeof(u_int32_t) * count));
    return 0;
}

int process_flrep(cTask *tmp_task) {
    u_int32_t fileId = tmp_task -> file_id;
    //printk("fileId in process_flrep: %d\n", fileId);
    u_int32_t buf[KVAL];
    int count = 0;
    int i, j;
    int energy = 0;

    cacheTable *tmp_cache = find_cache_by_fl(ctable, fileId);
    while (count < KVAL && tmp_cache != NULL) {
	if (strcmp(g_act, "on") == 0) update_ref(fileId, tmp_cache -> frag_id);
	buf[count] = tmp_cache -> frag_id;
	count++;
	tmp_cache = find_cache_by_fl(tmp_cache -> next, fileId);	
    }
    //printk("count = %d\n", count);

    int retrieve_num = KVAL - count;
    int need = retrieve_num;
    u_int32_t dst[need];
    u_int32_t frag[need];
    bool repeat;
    while (need > 0) {
	fileRepInfo *tmp_rinfo = find_rinfo(rinfo, fileId);
	fileRepInfo *min_rinfo;
	u_int32_t min_dist;
	if (tmp_rinfo == NULL) {
	    printk(KERN_INFO "Cache Daemon: don't receive enough file replies\n");
	    return 1;
	}
	min_rinfo = tmp_rinfo;
	min_dist = min_rinfo -> dist;
	tmp_rinfo = find_rinfo(tmp_rinfo -> next, fileId);
	while (tmp_rinfo != NULL) {
	    if (tmp_rinfo -> dist < min_dist) {
		min_rinfo = tmp_rinfo;
		min_dist = tmp_rinfo -> dist;
	    }
	    tmp_rinfo = find_rinfo(tmp_rinfo -> next, fileId);
	}
	u_int32_t *min_map = min_rinfo -> map;
	//printk("frag_count: %d\n", min_rinfo -> frag_count);
	for (i = 0; i < min_rinfo -> frag_count && count < KVAL; i++) {
	//printk("min_map: %d\n", min_map[i]);
	repeat = false;
	    for (j = 0; j < count; j++) {
		if (min_map[i] == buf[j]) {
		    repeat = true;
		    break;
		}
	    }
	    if (!repeat) {
		energy += min_dist;
		buf[count++] = min_map[i];
		dst[need - 1] = min_rinfo -> dst_ip;
		frag[need - 1] = min_map[i];
		need--;
	    }	    
	}
	min_rinfo -> expired = 1;
    }

    reqList *tmp_creq;
    cEnergy += energy;
    cSuccess += 1;
    for (i = 0; i < retrieve_num; i++) {
	//printk("dst[%d] = %d, frag[%d] = %d\n", i, dst[i], i, frag[i]);
	gen_frreq(g_my_ip, dst[i], fileId, frag[i]);

	if (strcmp(g_act, "on") == 0) {
	    tmp_creq = find_creq(fileId, frag[i]);
	    if (tmp_creq == NULL) {
		//printk("receiving a new fragment request\n");
		tmp_creq = create_creq(fileId, frag[i]);
		update_count(tmp_creq);
	    }
	    else if (reachThreshold(tmp_creq) == 1) {
	    //printk("reaching threshold\n");
		flush_count(tmp_creq);
		//gen_frreq_by_task(tmp_task);
		gen_exchReq(g_my_ip, dst[i], fileId, frag[i]);
		insert_timer(TASK_ADD_CACHE_SEND, WAIT_EXCH_REP_TIME, dst[i], fileId, frag[i]);
		update_timer_queue();
	    }
	    else update_count(tmp_creq);
	}
    }

    fileRepInfo *tmp_rinfo = find_rinfo(rinfo, fileId);
    while (tmp_rinfo != NULL) {
	tmp_rinfo -> expired = 1;
	tmp_rinfo = find_rinfo(tmp_rinfo -> next, fileId);
    }
    //printk("end of process_flrep\n");

    return 0;
}

/*int read_energy_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data)
{
    static char *my_buffer;
    int len;

    my_buffer = buffer;
    sprintf(my_buffer, "%d\n", cEnergy);

    len = strlen(my_buffer);
    *buffer_location = my_buffer + offset;
    len -= offset;
    if (len > buffer_length)
        len = buffer_length;
    else if (len < 0)
        len = 0;
    return len;
}*/

/*int read_success_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data)
{
    static char *my_buffer;
    int len;

    my_buffer = buffer;
    sprintf(my_buffer, "%d\n", cSuccess);

    len = strlen(my_buffer);
    *buffer_location = my_buffer + offset;
    len -= offset;
    if (len > buffer_length)
        len = buffer_length;
    else if (len < 0)
        len = 0;
    return len;
}*/

/****************************************************

   operations related to fileRepInfo

****************************************************/

inline void rinfo_read_lock()
{
    read_lock_bh(&rinfo_lock);
}

inline void rinfo_read_unlock()
{
    read_unlock_bh(&rinfo_lock);
}

inline void rinfo_write_lock()
{
    write_lock_bh(&rinfo_lock);
}

inline void rinfo_write_unlock()
{	
    write_unlock_bh(&rinfo_lock);
}

void init_rinfo() {
    rinfo = NULL;
}

fileRepInfo *find_rinfo(fileRepInfo *head, u_int32_t fileId) {
    rinfo_read_lock();
    fileRepInfo *tmp_rinfo = head;
    while (tmp_rinfo != NULL && (tmp_rinfo -> expired == 1 || tmp_rinfo -> file_id != fileId)) {
	tmp_rinfo = tmp_rinfo -> next;
    }
    rinfo_read_unlock();
    return tmp_rinfo;  
}

void remove_rinfo(fileRepInfo *dead_rinfo) {
    rinfo_write_lock();

    if (rinfo == dead_rinfo) {
        rinfo = dead_rinfo -> next;
    }
    if (dead_rinfo -> prev != NULL) {
        dead_rinfo -> prev -> next = dead_rinfo -> next;
    }
    if (dead_rinfo -> next != NULL) {
        dead_rinfo -> next -> prev = dead_rinfo -> prev;
    }

    rinfo_write_unlock();
    kfree(dead_rinfo);
}

void flush_rinfo() {
    fileRepInfo *dead_rinfo, *tmp_rinfo;
    rinfo_read_lock();
    
    tmp_rinfo = rinfo;
    while (tmp_rinfo != NULL) {
	if (tmp_rinfo -> expired == 1) {
	    dead_rinfo = tmp_rinfo;
	    tmp_rinfo = tmp_rinfo -> next;
	    rinfo_read_unlock();
	    remove_rinfo(dead_rinfo);
	    printk("removing one expired rinfo\n");
	    rinfo_read_lock();
	}
	else tmp_rinfo = tmp_rinfo -> next;
    }

    rinfo_read_unlock();
}

int cleanup_rinfo() {
    fileRepInfo *dead_rinfo, *tmp_rinfo;
    rinfo_write_lock();

    tmp_rinfo = rinfo;
    while (tmp_rinfo != NULL) 
    {
	dead_rinfo = tmp_rinfo;
        tmp_rinfo = tmp_rinfo -> next;
	kfree(dead_rinfo);
    }
    rinfo = NULL;

    rinfo_write_unlock();
    return 0;
}

void insert_rinfo(fileRepInfo *new_rinfo) {
    rinfo_write_lock();

    if (rinfo == NULL) {
	rinfo = new_rinfo;
	rinfo_write_unlock();
	return;
    }

    new_rinfo -> next = rinfo;
    rinfo -> prev = new_rinfo;
    rinfo = rinfo -> prev;
 
    rinfo_write_unlock();
}

fileRepInfo *create_rinfo(cTask *tmp_task) {
    fileRepInfo *tmp_rinfo;
    fileRep *tmp_flrep;
    u_int32_t *frags;
    int frag_count, i;

    tmp_flrep = (fileRep *)tmp_task -> data;
    frags = (u_int32_t *)((void*)tmp_task -> data + sizeof(fileRep));
    frag_count = ntohl(tmp_flrep -> frag_count);

    /* Allocate memory for new entry */

    if ((tmp_rinfo = (fileRepInfo *) kmalloc(sizeof(fileRepInfo), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: Error getting memory for new rinfo\n");
        return NULL;
    }

    tmp_rinfo -> file_id = ntohl(tmp_flrep -> file_id);
    tmp_rinfo -> dst_ip = ntohl(tmp_flrep -> src_ip);
    tmp_rinfo -> dist = ntohl(tmp_flrep -> dist);
    tmp_rinfo -> frag_count = ntohl(tmp_flrep -> frag_count);
    tmp_rinfo -> expired = 0;

    for (i = 0; i < frag_count; i++) {
	tmp_rinfo -> map[i] = ntohl(frags[i]);
	//printk("frags[%d] = %d, map[%d] = %d\n", i, frags[i], i, tmp_rinfo -> map[i]);
    }

    tmp_rinfo -> prev = NULL;
    tmp_rinfo -> next = NULL;

    insert_rinfo(tmp_rinfo);

    return tmp_rinfo;
}

/****************************************************

   Checking the routing table

****************************************************/

/*int getDist(u_int32_t dst_ip) {
    struct file *rt_file;
    char buf[200];
    mm_segment_t fs;
    loff_t pos;
    char *outer_ptr;
    char *inner_ptr;
    char *token1;
    char *token2;

    rt_file = filp_open("/proc/aodv/routes", O_RDONLY, 0);
    if (IS_ERR(rt_file)) {
        printk(KERN_INFO "open file error\n");
        return -1;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);
    pos = 0;
    vfs_read(rt_file, buf, sizeof(buf), &pos);
    filp_close(rt_file, NULL);
    set_fs(fs);

    outer_ptr = NULL;
    inner_ptr = NULL;
    token1 = strtok_r(buf,"\n", &outer_ptr);
    while (token1) {
	token2 = strtok_r(token1, "\t", &inner_ptr);
	if (strcmp(token2, inet_ntoa(dst_ip)) == 0) {
	    //printk("dst_ip = %s\n", token2);
	    token2 = strtok_r(NULL, "\t", &inner_ptr);
	    //printk("hop_count = %s\n", token2);
	    return atoi(token2);
	}
	token1 = strtok_r(NULL, "\n", &outer_ptr);
    }
    
    printk(KERN_INFO "Cannot find dst_ip %s in the routing table\n", inet_ntoa(dst_ip));
    return 0;
}*/



