#ifdef MAK_THREADS

#include "makalu_internal.h"
#include "makalu_thread_local.h"

STATIC pthread_mutex_t MAK_global_ml = PTHREAD_MUTEX_INITIALIZER;
STATIC pthread_mutex_t MAK_granule_ml[TINY_FREELISTS];

STATIC pthread_mutex_t mark_mutex = PTHREAD_MUTEX_INITIALIZER;
STATIC pthread_cond_t mark_cv = PTHREAD_COND_INITIALIZER;

MAK_INNER long MAK_n_markers = MAK_N_MARKERS;
MAK_INNER MAK_bool MAK_parallel_mark = FALSE;

MAK_INNER void MAK_lock_gran(word gran)
{
    int g = gran >= TINY_FREELISTS ? 0 : gran;
    pthread_mutex_lock(&(MAK_granule_ml[g]));
}

MAK_INNER void MAK_unlock_gran(word gran){
    int g = gran >= TINY_FREELISTS ? 0 : gran;
    pthread_mutex_unlock(&(MAK_granule_ml[g]));
}

MAK_INNER void MAK_acquire_mark_lock(void)
{
    pthread_mutex_lock(&(mark_mutex));
}

MAK_INNER void MAK_release_mark_lock(void)
{
    if (pthread_mutex_unlock(&mark_mutex) != 0) {
        MAK_abort("pthread_mutex_unlock failed");
    }
}

MAK_INNER void MAK_wait_marker(void)
{
    if (pthread_cond_wait(&mark_cv, &mark_mutex) != 0) {
        MAK_abort("pthread_cond_wait failed");
    }
}

MAK_INNER void MAK_notify_all_marker(void)
{
    if (pthread_cond_broadcast(&mark_cv) != 0) {
        MAK_abort("pthread_cond_broadcast failed");
    }
}

STATIC pthread_t MAK_mark_threads[MAK_N_MARKERS];

STATIC void * MAK_mark_thread(void * id)
{
    word my_mark_no = 0;
    int cancel_state;
    if ((word)id == (word)-1) return 0; /* to make compiler happy */
    DISABLE_CANCEL(cancel_state);
    for (;;) {
        MAK_help_marker();
    }
}


MAK_INNER void MAK_start_mark_threads(void)
{

    if ( MAK_n_markers <= 1) {
        MAK_parallel_mark = FALSE;
        return;
    } else {
        MAK_parallel_mark = TRUE;
    }

    int i;
    pthread_attr_t attr;
    if (0 != pthread_attr_init(&attr)) 
        ABORT("pthread_attr_init failed");
    
    if (0 != pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
        ABORT("pthread_attr_setdetachstate failed");
    
    for (i = 0; i < MAK_n_markers - 1; ++i) {
        if (0 != REAL_FUNC(pthread_create)(MAK_mark_threads + i, &attr,
                              MAK_mark_thread, (void *)(word)i)) {
            WARN("Marker thread creation failed, errno = %" GC_PRIdPTR "\n",
               errno);
            /* Don't try to create other marker threads.    */
            MAK_n_markers = i + 1;
            if (i == 0) MAK_parallel_mark = FALSE;
            break;
        }
    }
    pthread_attr_destroy(&attr);
}


MAK_INNER void MAK_thr_init(void)
{
    int i;
    for (i=0; i < TINY_FREELISTS; i++)
    {
        pthread_mutex_init(MAK_granule_ml + i, NULL);
    }
    
    MAK_init_thread_local();    
    MAK_set_my_thread_local();

}


            
#endif //MAK_THREADS
