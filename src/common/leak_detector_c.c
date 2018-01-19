/**
 * leak_detector_c.c
 *   http://www.codeproject.com/Articles/19361/Memory-Leak-Detection-in-C
 */
#include  "leak_detector_c.h"

#undef    malloc
#undef    calloc
#undef    free

static MEM_LEAK * ptr_start = NULL;
static MEM_LEAK * ptr_next =  NULL;

/**
 * adds allocated memory info. into the list
 *
 */
void add(MEM_INFO alloc_info) {
  MEM_LEAK *leak = NULL;
  leak = (MEM_LEAK*) malloc (sizeof(MEM_LEAK));
  leak->mem_info.addr = alloc_info.addr;
  leak->mem_info.size = alloc_info.size;
  strcpy(leak->mem_info.fname, alloc_info.fname); 
  leak->mem_info.line = alloc_info.line;
  leak->next = NULL;

  if (ptr_start == NULL) {
    ptr_start = leak;
    ptr_next = ptr_start;
  }
  else {
    ptr_next->next = leak;
    ptr_next = ptr_next->next;        
  }
}

/**
 * erases memory info. from the list
 *
 */
void erase(uint32_t pos) {
  uint32_t index = 0;
  MEM_LEAK * alloc_info, * temp;
  
  if(pos == 0) {
    MEM_LEAK * temp = ptr_start;
    ptr_start = ptr_start->next;
    free(temp);
  }
  else {
    for (index = 0, alloc_info = ptr_start; index < pos; 
         alloc_info = alloc_info->next, ++index) {
      if(pos == index + 1) {
        temp = alloc_info->next;
        alloc_info->next =  temp->next;
        free(temp);
        break;
      }
    }
  }
}

/**
 * deletes all the elements from the list
 */
void clear() {
  MEM_LEAK * temp = ptr_start;
  MEM_LEAK * alloc_info = ptr_start;

  while(alloc_info != NULL) {
    alloc_info = alloc_info->next;
    free(temp);
    temp = alloc_info;
  }
}

/**
 * replacement of malloc
 */
void * xmalloc(uint32_t size, const char * file, uint32_t line) {
  void * ptr = malloc (size);
  if (ptr != NULL) {
    add_mem_info(ptr, size, file, line);
  }
  return ptr;
}

/**
 * replacement of calloc
 */
void* xcalloc (uint32_t elements, uint32_t size, const char * file, uint32_t line) {
  uint32_t total_size;
  void * ptr = calloc(elements , size);
  if(ptr != NULL) {
    total_size = elements * size;
    add_mem_info (ptr, total_size, file, line);
  }
  return ptr;
}

/**
 * replacement of free
 */
void xfree(void * mem_ref) {
  remove_mem_info(mem_ref);
  free(mem_ref);
}

/**
 * gets the allocated memory info and adds it to a list
 *
 */
void add_mem_info (void *mem_ref, uint32_t size,  const char *file, uint32_t line) {
  MEM_INFO mem_alloc_info;

  /* fill up the structure with all info */
  memset( &mem_alloc_info, 0, sizeof ( mem_alloc_info ) );
  mem_alloc_info.addr   = mem_ref;
  mem_alloc_info.size = size;
  strncpy(mem_alloc_info.fname, file, LEAK_FNAME_SIZE);
  mem_alloc_info.line = line;

  /* add the above info to a list */
  add(mem_alloc_info);
}

/**
 * if the allocated memory info is part of the list, removes it
 *
 */
void remove_mem_info (void * mem_ref) {
  uint32_t index;
  MEM_LEAK  *leak_info = ptr_start;

  /* check if allocate memory is in our list */
  for (index = 0; leak_info != NULL; ++index, leak_info = leak_info->next) {
    if (leak_info->mem_info.addr == mem_ref) {
      erase(index);
      break;
    }
  }
}

/**
 * writes all info of the unallocated memory into a file
 */
void report_mem_leak_file(void) {
  MEM_LEAK *leak_info;
  char buf[1024];
  time_t dt;

  FILE *fpw = fopen(LEAK_REPORT_FILE, "at+");
  time(&dt);

  if(fpw != NULL) {
    sprintf(buf, "==============================================\n");
    fwrite(buf, (strlen(buf) + 1) , 1, fpw);
    sprintf(buf, "              Memory Leak Report\n %s", ctime(&dt));
    fwrite(buf, (strlen(buf) + 1) , 1, fpw);
    sprintf(buf, "----------------------------------------------\n");  
    fwrite(buf, (strlen(buf) + 1) , 1, fpw);
    
    for(leak_info = ptr_start; leak_info != NULL; leak_info = leak_info->next) {
      sprintf(buf, "addr : 0x%x\n", (unsigned int)leak_info->mem_info.addr);
      fwrite(buf, (strlen(buf) + 1) , 1, fpw);
      sprintf(buf, "size : %d bytes\n", leak_info->mem_info.size);      
      fwrite(buf, (strlen(buf) + 1) , 1, fpw);
      sprintf(buf, "file : %s\n", leak_info->mem_info.fname);
      fwrite(buf, (strlen(buf) + 1) , 1, fpw);
      sprintf(buf, "line : %d\n\n", leak_info->mem_info.line);
      fwrite(buf, (strlen(buf) + 1) , 1, fpw);
    }
  }

  fclose(fpw);
  clear();
}

/**
 * writes all info of the unallocated memory into stdout
 */
void report_mem_leak_stdout(void) {
  MEM_LEAK *leak_info;
  time_t dt;
  time(&dt);
  fprintf(stdout, "==============================================\n");
  fprintf(stdout, "              Memory Leak Report\n %s", ctime(&dt));
  fprintf(stdout, "----------------------------------------------\n");
  for(leak_info = ptr_start; leak_info != NULL; leak_info = leak_info->next) {
    fprintf(stdout, "addr : 0x%x\n", (unsigned int)leak_info->mem_info.addr);
    fprintf(stdout, "size : %d bytes\n", leak_info->mem_info.size);      
    fprintf(stdout, "file : %s\n", leak_info->mem_info.fname);
    fprintf(stdout, "line : %d\n\n", leak_info->mem_info.line);
  }
  clear();
}
