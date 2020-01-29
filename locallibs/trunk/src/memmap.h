#ifndef _MEMMAP_H
#define _MEMMAP_H

/**
 * Calculates free memory size & printout its map
 * @param TRACE
 * @return free memory size in Kb */
size_t map_of_free_mem(int l,const char *nickname, const char *filename, int line);

/**
 * Calculates allocated memory size
 * @param 
 * @return allocated memory size in Kb */
size_t size_of_allocated_mem();

#endif /* _MEMMAP_H */
