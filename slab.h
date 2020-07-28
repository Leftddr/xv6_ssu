#define NSLAB 9 // {8, 16, 32, 64, 128, 256, 512, 1024, 2048}  
#define MAX_PAGES_PER_SLAB 100

struct slab {
	int size;
	int num_pages;
	int num_free_objects;
	int num_used_objects;
	int num_objects_per_page;
	char *bitmap;
	char *page[MAX_PAGES_PER_SLAB];
};

//memory-> 4kb one page
//slab cache(8)-> one page -> 512 -> one page ->  
//slab cache(16) -> one page -> 256
//slab cache(2048) -> 2

// 8 8 8 8 8 8 8 8 
// 1 0 1 0 0 0 0 0

// 8 8 8 8 8 8 8 8 
// 1 0 1 0 0 0 0 0
// 1 0 1 0  1  0 0
// 1 0 1 0    1