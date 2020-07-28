#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "slab.h"
#include "stdbool.h"

struct {
	struct spinlock lock;
	struct slab slab[NSLAB]; // 0 ~ 8
} stable;

void slabinit(){
	struct slab *tmp_slab;
	int slab_size = 8;
	initlock(&stable.lock, "slab_lock");
	acquire(&stable.lock);
	for(tmp_slab = stable.slab; tmp_slab < &stable.slab[NSLAB]; tmp_slab++){
		tmp_slab->size = slab_size;
		tmp_slab->num_pages = 1;
		tmp_slab->num_free_objects = 4096 / slab_size; 
		tmp_slab->num_used_objects = 0;
		tmp_slab->num_objects_per_page = 4096 / slab_size;
		tmp_slab->bitmap = kalloc();
		tmp_slab->page[0] = kalloc();
		slab_size = slab_size << 1;
	}
	release(&stable.lock);
}
/* bitmap set, get, clear */
/* user -> size */
int userchoice(int size)
{
	int cri = 0x00000001; 
	while(cri < size){
		cri = cri << 1;
 	}
	return cri;
}

bool get_bit(char* size, int i)
{
	char *tmp = size;
	tmp += (i / 8);
	i = i - (i / 8) * 8;
	unsigned char cri = 0x80;
	if((*(tmp + i) & (cri >> i)) != 0) return true; 
	else return false;
	
}

char* set_bit(char* size, int i)
{
	char *tmp = size;
	tmp += (i / 8);
	i = i - (i / 8) * 8;
	unsigned char cri = 0x80;
	*(tmp + i) = (*(tmp + i) | (cri >> i));
	return tmp;
}

char* clear_bit(char* size, int i) {

  	char *tmp = size;
	tmp += (i / 8);
	i = i - (i / 8) * 8;
	unsigned char cri = 0x80;
	*(tmp + i) = *(tmp + i) & ~(cri >> i);
	return tmp;
}

char* kmalloc(int size){
	if(size > 2048){ return 0;}
	int retval = userchoice(size); //300 -> 512
	struct slab *s;
	acquire(&stable.lock);
	for(s = stable.slab; s < &stable.slab[NSLAB]; s++){
		if(s->size == retval){
			if(s->num_free_objects > 0){
				char *objects = 0x0;
				if(s->num_used_objects < s->num_objects_per_page){
					for(int i = 0 ; i < s->num_objects_per_page ; i++){
						if(!get_bit((s->bitmap), i)){
							s->num_free_objects--;
							s->num_used_objects++;
							(s->bitmap) = set_bit(s->bitmap, i);
							objects = s->page[s->num_pages - 1] + (i * s->size);
							break;
						}
					}
				}
				else{
					int index = -1;
					for(int i = 0 ; i < s->num_objects_per_page * (s->num_pages) ; i++){
						if(!get_bit((s->bitmap), i)){
							(s->bitmap) = set_bit((s->bitmap), i);
							index = i;
							break;
						}
					}
					if(index != -1){
						while(true){
							if(index >= 0 && index < s->num_objects_per_page) break;
							index -= s->num_objects_per_page;
						}
					}
					else {cprintf("Cannot allocate\n"); release(&stable.lock); return 0;}
					s->num_free_objects--;
					s->num_used_objects++;
					objects = s->page[s->num_pages - 1] + (index * s->size);
				}
				release(&stable.lock);
				return objects;
			}
			else{
				if(s->num_pages + 1 >= MAX_PAGES_PER_SLAB){ return 0; }
				s->num_pages++;
				s->page[s->num_pages - 1] = kalloc();
				s->num_free_objects += s->num_objects_per_page;
				release(&stable.lock);
				char *objects = kmalloc(size);
				return objects;
			}
		}
	}
	release(&stable.lock);
	return 0;
}

void kmfree(char *addr){
	 struct slab *s;
	 acquire(&stable.lock);
	 for(s = stable.slab ; s < &stable.slab[NSLAB] ; s++){
		int down_count = 0; 
		for(int i = 0 ; i < s->num_pages ; i++){
			for(int j = 0 ; j < s->num_objects_per_page ; j++){
				if(addr == s->page[i] + (j * s->size)){
					s->num_free_objects++;
					s->num_used_objects--;
					clear_bit((s->bitmap), i * s->num_objects_per_page + j);
					if(s->num_used_objects < ((s->num_pages - 1) + down_count) * s->num_objects_per_page){
						s->num_free_objects -= s->num_objects_per_page;
						kfree(s->page[s->num_pages - 1 + down_count]);
						down_count--;
					}
					release(&stable.lock);
					s->num_pages += down_count;
					return;
				}
			}
		}
		s->num_pages += down_count;	 
	}
	release(&stable.lock);
	return;
}

void slabdump(){
	cprintf("__slabdump__\n");

	struct slab *s;

	cprintf("size\tnum_pages\tused_objects\tfree_objects\n");

	for(s = stable.slab; s < &stable.slab[NSLAB]; s++){
		cprintf("%d\t%d\t\t%d\t\t%d\n", 
			s->size, s->num_pages, s->num_used_objects, s->num_free_objects);
	}
}
