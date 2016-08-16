
#include "cacheTable.h"

cacheTable *ctable;
int ct_len;
rwlock_t ctable_lock = RW_LOCK_UNLOCKED;

inline void ctable_read_lock()
{
    read_lock_bh(&ctable_lock);
}

inline void ctable_read_unlock()
{
    read_unlock_bh(&ctable_lock);
}

inline void ctable_write_lock()
{
    write_lock_bh(&ctable_lock);
}

inline void ctable_write_unlock()
{	
    write_unlock_bh(&ctable_lock);
}

void init_cache_table() {
    ctable = NULL;
    ct_len = 0;
    
    struct file *tmp_file = filp_open("/myfs/a", O_RDONLY, 0);
    if (IS_ERR(tmp_file)) return;

    struct list_head *next;
    struct dentry *child_dentry;
    list_for_each(next, &tmp_file -> f_path.dentry -> d_u.d_child) {
        child_dentry = (struct dentry *)list_entry(next, struct dentry, d_u.d_child);
	if (strlen(child_dentry -> d_name.name) < 10) {
	    //printk("file name: %s\n", (char *)child_dentry -> d_name.name);
	    char name[20];
	    char *ptr = NULL;
	    sprintf(name, child_dentry -> d_name.name);

	    char *token = strtok_r(name, "-", &ptr);
	    u_int32_t fileId = (u_int32_t)atoi(token);
	    //printk("fileId: %d\n", fileId);
	    token = strtok_r(NULL, "-", &ptr);
	    u_int32_t fragId = (u_int32_t)atoi(token);
	    //printk("fragId: %d\n", fragId);
	    create_cache(fileId, fragId, 1);
	    //printk("ct_len = %d\n", ct_len);
	}
    }
    filp_close(tmp_file, NULL);
}

cacheTable *find_cache_by_fl(cacheTable *head, u_int32_t fileId) {
    ctable_read_lock();
    cacheTable *tmp_cache = head;
    while (tmp_cache != NULL && tmp_cache -> file_id != fileId) {
	tmp_cache = tmp_cache -> next;
    }
    ctable_read_unlock();
    return tmp_cache;  
}

cacheTable *find_cache_by_fr(u_int32_t fileId, u_int32_t fragId) {
    ctable_read_lock();
    cacheTable *tmp_cache = ctable;
    while (tmp_cache != NULL && (tmp_cache -> file_id != fileId || tmp_cache -> frag_id != fragId)) {
	tmp_cache = tmp_cache -> next;
    }
    ctable_read_unlock();
    return tmp_cache;
}

int update_ref(u_int32_t fileId, u_int32_t fragId) {
    cacheTable *tmp_cache = find_cache_by_fr(fileId, fragId);
    //printk("fileId = %d, fragId = %d\n", tmp_cache -> file_id, tmp_cache -> frag_id);
    ctable_write_lock();

    if (tmp_cache == NULL) {
	printk("Cache Daemon: Failed to find the cache!\n");
	ctable_write_unlock();
	return 1;
    }
    tmp_cache -> refs = tmp_cache -> refs + 1;

    ctable_write_unlock();
    return 0;
}

void remove_cache(cacheTable *dead_cache) {
    ctable_write_lock();

    if (ctable == dead_cache) {
        ctable = dead_cache -> next;
    }
    if (dead_cache -> prev != NULL) {
        dead_cache -> prev -> next = dead_cache -> next;
    }
    if (dead_cache -> next != NULL) {
        dead_cache -> next -> prev = dead_cache -> prev;
    }

    ctable_write_unlock();
    ct_len--;
    kfree(dead_cache);
}

int delete_cache(u_int32_t fileId, u_int32_t fragId) {
    cacheTable *dead_cache = find_cache_by_fr(fileId, fragId);
    ctable_read_lock();

    if (dead_cache != NULL && dead_cache -> isServiceCenter == 0) {
	ctable_read_unlock();
	remove_cache(dead_cache);
	return 1;
    }

    ctable_read_unlock();
    return 0;
}

int cleanup_cache_table() {
    cacheTable *dead_cache, *tmp_cache;
    ctable_read_lock();

    tmp_cache = ctable;
    while (tmp_cache != NULL) {
	if (tmp_cache -> isServiceCenter == 0) {
	    dead_cache = tmp_cache;
	    tmp_cache = tmp_cache -> next;
	    ctable_read_unlock();
	    remove_cache(dead_cache);
	    ctable_read_lock();
	}
	else tmp_cache = tmp_cache -> next;
    }

    ctable_read_unlock();
    return 0;
}

void insert_cache(cacheTable *new_cache) {
    ctable_write_lock();

    if (ctable == NULL) {
	ctable = new_cache;
	ctable_write_unlock();
	ct_len++;
	return;
    }

    new_cache -> next = ctable;
    ctable -> prev = new_cache;
    ctable = ctable -> prev;

    ctable_write_unlock();
    ct_len++;
    return;
}

cacheTable *create_cache(u_int32_t fileId, u_int32_t fragId, u_int32_t flag) {
    cacheTable *tmp_cache;

    /* Allocate memory for new entry */

    if ((tmp_cache = (cacheTable *) kmalloc(sizeof(cacheTable), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: Error getting memory for new cache\n");
        return NULL;
    }

    tmp_cache -> file_id = fileId;
    tmp_cache -> frag_id = fragId;
    tmp_cache -> isServiceCenter = flag;
    tmp_cache -> refs = 0;  
    tmp_cache -> prev = NULL;
    tmp_cache -> next = NULL;

    insert_cache(tmp_cache);

    return tmp_cache;
}

int read_ctable_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length,int *eof,void *data)
{
    static char *my_buffer;
    char temp_buffer[200];
    char temp[100];
    cacheTable *tmp_entry;
    int len, i;
    
    /*lock table*/
    ctable_read_lock();

    tmp_entry = ctable;

    my_buffer = buffer;

    sprintf(my_buffer, "\nCache Table \n---------------------------------------------------------------------------------\n");
    sprintf(temp_buffer, "        fileId        |    fragId    |   refs  |     isServiceCenter  \n");
    strcat(my_buffer, temp_buffer);
    sprintf(temp_buffer, "---------------------------------------------------------------------------------\n");
    strcat(my_buffer, temp_buffer);

    while (tmp_entry != NULL)
    {
        sprintf(temp_buffer, "        %6d             %6d        %4d                 %6d  \n", tmp_entry -> file_id, tmp_entry -> frag_id, tmp_entry -> refs, tmp_entry -> isServiceCenter);
        strcat(my_buffer,temp_buffer);
	
        tmp_entry = tmp_entry -> next;
    }

    /*unlock table*/
    ctable_read_unlock();

    strcat(my_buffer,"---------------------------------------------------------------------------------\n\n");

    len = strlen(my_buffer);
    *buffer_location = my_buffer + offset;
    len -= offset;
    if (len > buffer_length)
        len = buffer_length;
    else if (len < 0)
        len = 0;
    return len;
}

