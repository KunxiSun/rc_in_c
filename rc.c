
#include "rc.h"

//struct strong_ref* list[200];
struct strong_ref** list;
int cur_sz = 0;
int ctr = 0;

int match_ptr(void* ptr) {
  int match = -1;
  for (int i = 0; i < ctr; i++) {
    if (list[i] != NULL) {
      if (ptr == list[i]->ptr) {
        match = i;
        break;
      }
    }
  }
  return match;
}

int match_ref(struct strong_ref* ref) {
  int match = -1;
  for (int i = 0; i < ctr; i++) {
    if (ref == list[i]) {
      match = i;
      break;
    }
  }
  return match;
}

struct rc_entry init_entry(size_t count, size_t* dep_list, size_t n_deeps,
                           size_t dep_capacity) {
  struct rc_entry entry;
  entry.dep_list = dep_list;
  entry.count = count;
  entry.dep_capacity = dep_capacity;
  entry.n_deps = n_deeps;
  return entry;
}

/**
 * Returns an allocation of n bytes and creates an internal rc entry.
 *
 * If the ptr argument is NULL and deps is NULL, it will return a new
 * allocation
 *
 * If the ptr argument is not NULL and an entry exists, it will increment
 *  the reference count of the allocation and return a strong_ref pointer
 *
 * If the ptr argument is NULL and deps is not NULL, it will return
 * a new allocation but the count will correlate to the dependency
 * if the dependency is deallocated the reference count on the object will
 * decrement
 *
 * If the ptr argument is not NULL and an entry exists and dep is not
 * NULL, it will increment the count of the strong reference but the count
 * will be related to the dependency, if the dependency is deallocated the
 * reference count on the object will decrement
 */
struct strong_ref* rc_alloc(void* ptr, size_t n, struct strong_ref* dep) {

	if(ctr==0){
		list = (struct strong_ref**)malloc(RC_INIT_SZ*sizeof(struct strong_ref*));
		cur_sz=RC_INIT_SZ;
	}
  
  if(ctr==cur_sz){
	  list = realloc(list,cur_sz*RC_GROWTH_RT*sizeof(struct strong_ref*));
	  cur_sz*=RC_GROWTH_RT;
   }

  int match = -1;
  if (ptr == NULL && dep == NULL) {
    size_t* dep_list = (size_t*)malloc(RC_INIT_SZ * sizeof(size_t));
    struct rc_entry entry = init_entry(1, dep_list, 0, 0);
    struct strong_ref* new =
        (struct strong_ref*)malloc(sizeof(struct strong_ref));
    new->ptr = (void*)malloc(n);
    new->entry = entry;
    list[ctr] = new;
    ctr++;
    return new;

  } else if (ptr != NULL && (match = match_ptr(ptr)) != -1) {
    struct strong_ref* new = list[match];
    new->ptr = list[match]->ptr;
    list[match]->entry.count += 1;
    list[ctr] = new;
    ctr++;
    return new;
  } else if (ptr == NULL && dep != NULL) {
    struct strong_ref* new =
        (struct strong_ref*)malloc(sizeof(struct strong_ref));
    new->ptr = (void*)malloc(n);
    new->entry.n_deps = 0;
    new->entry.dep_capacity = 0;
    new->entry.dep_list = (size_t*)malloc(RC_INIT_SZ * sizeof(size_t));
    new->entry.count = dep->entry.count;
    dep->entry.dep_list[dep->entry.n_deps] = ctr;
    dep->entry.n_deps += 1;

    list[ctr] = new;
    ctr++;
    return new;
  } else if(ptr !=NULL && dep!=NULL && list[match_ptr(ptr)]->entry.count!=0){

    }
  return NULL;
}

/**
 * Downgrades a strong reference to a weak reference, this will decrement the
 * reference count by 1
 * If ref is NULL, the function will return an invalid weak ref object
 * If ref is a value that does not exist in the reference graph, it will return
 * an weak_ref object that is invalid
 *
 * If ref is a value that does exist in the reference graph, it will return
 *    a valid weak_ref object
 *
 * An invalid weak_ref object is where its entry_id field is set to
 *   0xFFFFFFFFFFFFFFFF
 *
 * @param strong_ref* ref (reference to allocation)
 * @return weak_ref (reference with an entry id)
 */
struct weak_ref rc_downgrade(struct strong_ref* ref) {
  struct weak_ref r;
  if (ref == NULL || ref->entry.count == 0 || match_ref(ref) == -1) {
    r.entry_id = RC_INVALID_REF;
    return r;
  }

  int match = match_ptr(ref->ptr);

  // decrement
  if (ref->entry.count > 1) {
    for (size_t i = 0; i < ref->entry.n_deps; i++) {
      list[ref->entry.dep_list[i]]->entry.count -= 1;
    }
    ref->entry.count -= 1;
    r.entry_id = match;
    return r;
  } else {
    // deallocate
    for (size_t i = 0; i < ref->entry.n_deps; i++) {
      list[ref->entry.dep_list[i]]->entry.count -= 1;
    }
    ref->entry.count -= 1;
    r.entry_id = RC_INVALID_REF;
    return r;
  }
}

/**
 * Upgrdes a weak reference to a strong reference.
 * The weak reference should check that the entry id is valid (bounds check)
 * If a strong reference no longer exists or has been deallocated, the return
 *   result should be null.
 */
struct strong_ref* rc_upgrade(struct weak_ref ref) {
  if (ref.entry_id >= 0 && ref.entry_id <= (size_t)ctr) {
    if (list[ref.entry_id] == NULL) {
      return NULL;
    } else if (list[ref.entry_id]->entry.count == 0) {
      return NULL;
    } else {
	  list[ref.entry_id]->entry.count+=1;
      return list[ref.entry_id];
    }
  }
  return NULL;
}

/**
 * Cleans up the reference counting graph.
 */
void rc_cleanup() {
  for (int i = 0; i < ctr; i++) {
    if (list[i]->ptr != NULL) {
      free(list[i]->ptr);
	  free(list[i]->entry.dep_list);
	  free(list[i]);
      list[i]->ptr = NULL;
    }
	list[i] = NULL;
  }
}
