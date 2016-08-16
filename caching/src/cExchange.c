
#include "cExchange.h"

extern u_int32_t 	g_my_ip;
extern cacheTable	*ctable;
extern int		ct_len;
extern int KVAL;
extern int BUF_SIZE;
extern u_int32_t	cPrefetch;
exchTable *extable;
rwlock_t extable_lock = RW_LOCK_UNLOCKED;

void convert_exreq_to_host(exchReq *tmp_exreq)
{
    tmp_exreq -> src_ip = ntohl(tmp_exreq -> src_ip);
    tmp_exreq -> dst_ip = ntohl(tmp_exreq -> dst_ip);
    tmp_exreq -> file_id = ntohl(tmp_exreq -> file_id);
    tmp_exreq -> frag_id = ntohl(tmp_exreq -> frag_id);
}

void convert_exrep_to_host(exchRep *tmp_exrep)
{
    tmp_exrep -> src_ip = ntohl(tmp_exrep -> src_ip);
    tmp_exrep -> dst_ip = ntohl(tmp_exrep -> dst_ip);
    tmp_exrep -> file_id = ntohl(tmp_exrep -> file_id);
    tmp_exrep -> frag_id = ntohl(tmp_exrep -> frag_id);
    tmp_exrep -> score = ntohl(tmp_exrep -> score);
}

void convert_addcache_to_host(addCache *tmp_addcache)
{
    tmp_addcache -> src_ip = ntohl(tmp_addcache -> src_ip);
    tmp_addcache -> dst_ip = ntohl(tmp_addcache -> dst_ip);
    tmp_addcache -> file_id = ntohl(tmp_addcache -> file_id);
    tmp_addcache -> frag_id = ntohl(tmp_addcache -> frag_id);
}

int recv_exreq(cTask *tmp_task) {
    exchReq *tmp_exreq = tmp_task -> data;
    convert_exreq_to_host(tmp_exreq);

    u_int32_t fileId = tmp_exreq -> file_id;
    u_int32_t fragId = tmp_exreq -> frag_id;
    u_int32_t neigh_ip = tmp_exreq -> src_ip;
    u_int32_t dst_ip = tmp_exreq -> dst_ip;
    
    u_int32_t score = getScore(fileId, fragId);
    gen_exchRep(g_my_ip, dst_ip, neigh_ip, fileId, fragId, score);

    return 0;
}

int recv_exrep(cTask *tmp_task) {
    exchRep *tmp_exrep = tmp_task -> data;
    convert_exrep_to_host(tmp_exrep);

    create_extable(tmp_task);
    return 0;
}

int recv_addcache(cTask *tmp_task) {
    addCache *tmp_addcache = tmp_task -> data;
    convert_addcache_to_host(tmp_addcache);

    u_int32_t fileId = tmp_addcache -> file_id;
    u_int32_t fragId = tmp_addcache -> frag_id;
    u_int32_t dst_ip = tmp_addcache -> dst_ip;

    retrieve_frag(dst_ip, fileId, fragId);
    return 0;
}

int select_agent(cTask *tmp_task) {
    u_int32_t fileId = tmp_task -> file_id;
    u_int32_t fragId = tmp_task -> frag_id;

    u_int32_t max_score = getScore(fileId, fragId);
    u_int32_t dst_ip = g_my_ip;

    exchTable *tmp_extable = find_extable(extable, fileId, fragId);
    while (tmp_extable != NULL) {
	if (tmp_extable -> score > max_score) {
	    max_score = tmp_extable -> score;
	    dst_ip = tmp_extable -> neigh_ip;
	}
	tmp_extable -> expired = 1;
	tmp_extable = find_extable(tmp_extable -> next, fileId, fragId);
    }

    if (dst_ip != g_my_ip) {
	gen_addCache(dst_ip, tmp_task -> dst_ip, fileId, fragId);
    }
    else retrieve_frag(tmp_task -> dst_ip, fileId, fragId);

    return 0;    
}

int gen_exchReq(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t file_id, u_int32_t frag_id) {
    exchReq *tmp_exreq;
    
    if ((tmp_exreq = (exchReq *) kmalloc(sizeof(exchReq), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: Can't allocate new exchange request\n");
        return 0;
    }

    tmp_exreq -> type = EXCHANGE_REQ_MESSAGE;
    tmp_exreq -> reserved1 = 0;
    tmp_exreq -> reserved2 = 0;
    tmp_exreq -> reserved3 = 0;
    tmp_exreq -> src_ip = htonl(src_ip);
    tmp_exreq -> dst_ip = htonl(dst_ip);
    tmp_exreq -> file_id = htonl(file_id);
    tmp_exreq -> frag_id = htonl(frag_id);

    local_broadcast(1, tmp_exreq, sizeof(exchReq));
    return 0;
}

int gen_exchRep(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t neigh_ip, u_int32_t file_id, u_int32_t frag_id, u_int32_t score) {
    exchRep *tmp_exrep;
    
    if ((tmp_exrep = (exchRep *) kmalloc(sizeof(exchRep), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: Can't allocate new exchange reply\n");
        return 0;
    }

    tmp_exrep -> type = EXCHANGE_REP_MESSAGE;
    tmp_exrep -> reserved1 = 0;
    tmp_exrep -> reserved2 = 0;
    tmp_exrep -> reserved3 = 0;
    tmp_exrep -> src_ip = htonl(src_ip);
    tmp_exrep -> dst_ip = htonl(dst_ip);
    tmp_exrep -> file_id = htonl(file_id);
    tmp_exrep -> frag_id = htonl(frag_id);
    tmp_exrep -> score = htonl(score);

    send_message(neigh_ip, NET_DIAMETER, tmp_exrep, sizeof(exchRep));
    return 0;
}

u_int32_t getScore(u_int32_t file_id, u_int32_t frag_id) {
    int count = 0;
    cacheTable *tmp_cache = find_cache_by_fr(file_id, frag_id);
    if (tmp_cache != NULL) {
	return 0;
    }

    tmp_cache = find_cache_by_fl(ctable, file_id);
    while (tmp_cache != NULL) {
	count++;
	tmp_cache = find_cache_by_fl(tmp_cache -> next, file_id);
    }
    
    if (count >= KVAL - 1) {
	return 0;
    }

    return (BUF_SIZE - ct_len) + 1;
}

int gen_addCache(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t file_id, u_int32_t frag_id) {
    addCache *tmp_addcache;
    
    if ((tmp_addcache = (addCache *) kmalloc(sizeof(addCache), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: Can't allocate new add cache request\n");
        return 0;
    }

    tmp_addcache -> type = ADD_CACHE_MESSAGE;
    tmp_addcache -> reserved1 = 0;
    tmp_addcache -> reserved2 = 0;
    tmp_addcache -> reserved3 = 0;
    tmp_addcache -> src_ip = htonl(src_ip);
    tmp_addcache -> dst_ip = htonl(dst_ip);
    tmp_addcache -> file_id = htonl(file_id);
    tmp_addcache -> frag_id = htonl(frag_id);

    send_message(src_ip, NET_DIAMETER, tmp_addcache, sizeof(addCache));
    return 0;
}

int remove_file(char *filename)
{
    // supposed to remove the fragment under /myfs directory due to buffer constraint. Currently not implemented...

    /*int ret = 0;
    struct nameidata nd;
    struct dentry *dentry;

    ret = path_lookup(filename, LOOKUP_PARENT, &nd);
    if (ret != 0)
    {
        return -ENOENT;
    }
    dentry = lookup_one_len(nd.last.name, nd.dentry, strlen(nd.last.name));
    if (IS_ERR(dentry))
    {
        path_release(&nd);
        return -EACCES;
    }
    vfs_unlink(nd.dentry -> d_inode, dentry);

    dput(dentry);
    path_release(&nd);*/

    return 0; 
}

int retrieve_frag(u_int32_t dst_ip, u_int32_t file_id, u_int32_t frag_id) {
    // supposed to retrive fragment from dst_ip by TCP. But here we just ...
    //char name[20];
    //char file[10];
    //char frag[10];
    int count = 0;
    cacheTable *tmp_cache = find_cache_by_fl(ctable, file_id);
    while (count < KVAL && tmp_cache != NULL) {
	count++;
	tmp_cache = find_cache_by_fl(tmp_cache -> next, file_id);	
    }
    if (count >= KVAL - 1) return 1;

    cPrefetch++;
    if (ct_len >= BUF_SIZE) {
	cacheTable *min_cache, *tmp_cache;
	bool replace = false;
	u_int32_t min_ref;

	ctable_read_lock();
	tmp_cache = ctable;
	while (tmp_cache != NULL && tmp_cache -> isServiceCenter == 1) {
	    tmp_cache = tmp_cache -> next;
	}

	if (tmp_cache == NULL) {
	    printk("all service centers\n");
	    ctable_read_unlock();
	    return 1;
	}
	min_cache = tmp_cache;
	min_ref = min_cache -> refs;
	tmp_cache = tmp_cache -> next;
	while (tmp_cache != NULL) {
	    if (tmp_cache -> refs < min_ref && tmp_cache -> isServiceCenter == 0) {
		min_cache = tmp_cache;
		min_ref = tmp_cache -> refs;
	    }
	    tmp_cache = tmp_cache -> next;
	}
	ctable_read_unlock();

	//u_int32_t fileId = min_cache -> file_id;
	//u_int32_t fragId = min_cache -> frag_id;

	/*sprintf(name, "/myfs/");
	sprintf(file, "%u", fileId);
	sprintf(frag, "%u", fragId);
	strcat(name, file);
	strcat(name, "-");
	strcat(name, frag);*/

	//remove_file(name);
	remove_cache(min_cache);
    }
    
    /*struct file *new_file;
    sprintf(name, "/myfs/");
    sprintf(file, "%u", file_id);
    sprintf(frag, "%u", frag_id);
    strcat(name, file);
    strcat(name, "-");
    strcat(name, frag);

    new_file = filp_open(name, O_CREAT | O_RDWR, 0);
    if(IS_ERR(new_file)) return -1;
    filp_close(new_file, 0);*/

    create_cache(file_id, frag_id, 0);
    return 0;
}

/*int read_prefetch_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data)
{
    static char *my_buffer;
    int len;

    my_buffer = buffer;
    sprintf(my_buffer, "%d\n", cPrefetch);

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

   operations related to exchTable

****************************************************/

inline void extable_read_lock()
{
    read_lock_bh(&extable_lock);
}

inline void extable_read_unlock()
{
    read_unlock_bh(&extable_lock);
}

inline void extable_write_lock()
{
    write_lock_bh(&extable_lock);
}

inline void extable_write_unlock()
{	
    write_unlock_bh(&extable_lock);
}

void init_extable() {
    extable = NULL;
}

exchTable *find_extable(exchTable *head, u_int32_t fileId, u_int32_t fragId) {
    extable_read_lock();
    exchTable *tmp_extable = head;
    while (tmp_extable != NULL && (tmp_extable -> expired == 1 || tmp_extable -> file_id != fileId || tmp_extable -> frag_id != fragId)) {
	tmp_extable = tmp_extable -> next;
    }
    extable_read_unlock();
    return tmp_extable;  
}

void remove_extable(exchTable *dead_extable) {
    extable_write_lock();

    if (extable == dead_extable) {
        extable = dead_extable -> next;
    }
    if (dead_extable -> prev != NULL) {
        dead_extable -> prev -> next = dead_extable -> next;
    }
    if (dead_extable -> next != NULL) {
        dead_extable -> next -> prev = dead_extable -> prev;
    }

    extable_write_unlock();
    kfree(dead_extable);
}

void flush_extable() {
    exchTable *dead_extable, *tmp_extable;
    extable_read_lock();
    
    tmp_extable = extable;
    while (tmp_extable != NULL) {
	if (tmp_extable -> expired == 1) {
	    dead_extable = tmp_extable;
	    tmp_extable = tmp_extable -> next;
	    extable_read_unlock();
	    remove_extable(dead_extable);
	    extable_read_lock();
	}
	else tmp_extable = tmp_extable -> next;
    }

    extable_read_unlock();
}

int cleanup_extable() {
    exchTable *dead_extable, *tmp_extable;
    extable_write_lock();

    tmp_extable = extable;
    while (tmp_extable != NULL) 
    {
	dead_extable = tmp_extable;
        tmp_extable = tmp_extable -> next;
	kfree(dead_extable);
    }
    extable = NULL;

    extable_write_unlock();
    return 0;
}

void insert_extable(exchTable *new_extable) {
    extable_write_lock();

    if (extable == NULL) {
	extable = new_extable;
	extable_write_unlock();
	return;
    }

    new_extable -> next = extable;
    extable -> prev = new_extable;
    extable = extable -> prev;
 
    extable_write_unlock();
}

exchTable *create_extable(cTask *tmp_task) {
    exchRep *tmp_exrep = tmp_task -> data;
    exchTable *tmp_extable;

    /* Allocate memory for new entry */

    if ((tmp_extable = (exchTable *) kmalloc(sizeof(exchTable), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: Error getting memory for new extable\n");
        return NULL;
    }

    tmp_extable -> file_id = tmp_exrep -> file_id;
    tmp_extable -> frag_id = tmp_exrep -> frag_id;
    tmp_extable -> dst_ip = tmp_exrep -> dst_ip;
    tmp_extable -> neigh_ip = tmp_exrep -> src_ip;
    tmp_extable -> score = tmp_exrep -> score;
    tmp_extable -> expired = 0;

    tmp_extable -> prev = NULL;
    tmp_extable -> next = NULL;

    insert_extable(tmp_extable);

    return tmp_extable;
}


