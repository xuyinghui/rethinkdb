#include "cpu_context.hpp"
#include "event_queue.hpp"

template<class metablock_t>
naive_metablock_manager_t<metablock_t>::naive_metablock_manager_t(extent_manager_t *em)
    : extent_manager(em), state(state_unstarted), dbfd(INVALID_FD) {
    
    // Reserve the beginning of the file for the metablock
    extent_manager->reserve_extent(0);
    
    assert(sizeof(metablock_t) <= DEVICE_BLOCK_SIZE);
    mb_buffer = (metablock_t *)malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    assert(mb_buffer);
#ifndef NDEBUG
    memset(mb_buffer, 0xBD, DEVICE_BLOCK_SIZE);   // Happify Valgrind
#endif
    mb_buffer_in_use = false;
}

template<class metablock_t>
naive_metablock_manager_t<metablock_t>::~naive_metablock_manager_t() {

    assert(state == state_unstarted || state == state_shut_down);
    
    assert(!mb_buffer_in_use);
    free(mb_buffer);
}

template<class metablock_t>
bool naive_metablock_manager_t<metablock_t>::start(fd_t fd, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb) {
    
    assert(state == state_unstarted);
    dbfd = fd;
    assert(dbfd != INVALID_FD);
    
    // Determine if we are creating a new database
    off64_t dbsize = lseek64(dbfd, 0, SEEK_END);
    check("Could not determine database file size", dbsize == -1);
    off64_t res = lseek64(dbfd, 0, SEEK_SET);
    check("Could not reset database file position", res == -1);
    bool is_new_database = (dbsize == 0);
    
    if (is_new_database) {
        
        *mb_found = false;
        state = state_ready;
        return true;
    
    } else {
        
        *mb_found = true;
        this->mb_out = mb_out;
        
        assert(!mb_buffer_in_use);
        mb_buffer_in_use = true;
        
        event_queue_t *queue = get_cpu_context()->event_queue;
        queue->iosys.schedule_aio_read(dbfd, 0, DEVICE_BLOCK_SIZE, mb_buffer, queue, this);
        
        read_callback = cb;
        state = state_reading;
        return false;
    }
}

template<class metablock_t>
bool naive_metablock_manager_t<metablock_t>::write_metablock(metablock_t *mb, metablock_write_callback_t *cb) {
    
    assert(state == state_ready);
    
    assert(!mb_buffer_in_use);
    memcpy(mb_buffer, mb, sizeof(metablock_t));
    mb_buffer_in_use = true;
    
    event_queue_t *queue = get_cpu_context()->event_queue;
    queue->iosys.schedule_aio_write(
        dbfd,
        0,                  // Offset of beginning of write
        DEVICE_BLOCK_SIZE,  // Length of write
        mb_buffer,
        queue,
        this);
    
    state = state_writing;
    write_callback = cb;
    return false;
}

template<class metablock_t>
void naive_metablock_manager_t<metablock_t>::shutdown() {
    
    assert(state == state_ready);
    state = state_shut_down;
}

template<class metablock_t>
void naive_metablock_manager_t<metablock_t>::on_io_complete(event_t *e) {

    switch(state) {
    
    case state_reading:
        state = state_ready;
        memcpy(mb_out, mb_buffer, sizeof(metablock_t));
        mb_buffer_in_use = false;
        if (read_callback) read_callback->on_metablock_read();
        read_callback = NULL;
        break;
        
    case state_writing:
        state = state_ready;
        mb_buffer_in_use = false;
        if (write_callback) write_callback->on_metablock_write();
        write_callback = NULL;
        break;
        
    default:
        fail("Unexpected state.");
    }
}
