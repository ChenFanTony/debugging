/*
 *
 *  fd = open("/dev/mmapmm")
 *  mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, phy_addr & ~MAP_MASK);
 *
 * */

#define MMAPMM_DEV_NAME "mmapmm"
#define MMAPMM_DEV_MAJOR 255 
 
int mmapmm_open(struct inode *inode,struct file *filp) {  
    	return 0;  
}  
 
int mmapmm_release(struct inode *inode,struct file *filp) {  
    	return 0;  
}  
 
int mmapmm_mmap(struct file *filp,struct vm_area_struct *vma) {  
	int result;  
	size_t size = vma->vm_end - vma->vm_start;
 
	vma->vm_flags |= (VM_LOCKED | (VM_DONTEXPAND | VM_DONTDUMP)); 
 
	result = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot);  
	if(result)
        	return -EAGAIN;
 
	return 0;
}  
 
struct file_operations mmapiomem_fops={  
	.owner=THIS_MODULE,  
	.open=mmapmm_open,  
	.release=mmapmm_release,  
	.mmap=mmapmm_mmap,  
};  

struct cdev *mmap_cdev;  
struct class *mmap_class;  

int mmapiomem_init(void)  
{  
	int result;  
	int devno = MKDEV(MMAPMM_DEV_MAJOR,0);  
	
	mmap_cdev = cdev_alloc();  
	result = register_chrdev_region(devno, 1, "mmap_mm");  
	cdev_init(mmap_cdev,&mmapiomem_fops);  
	mmap_cdev->owner = THIS_MODULE;  
	result = cdev_add(mmap_cdev, devno,1);  
	mmap_class = class_create(THIS_MODULE,"mmap_mm_class");  
	if (IS_ERR(mmap_class)) {  
		result= PTR_ERR(mmap_class);  
 
		return -1;  
	}  
	device_create(mmap_class, NULL, devno, NULL, MMAPMM_DEV_NAME);  
	return 0;  
}  
 
void mmapiomem_exit(void)  
{  
 
	if (mmap_cdev != NULL)
		cdev_del(mmap_cdev);  
 
	device_destroy(mmap_class, MKDEV(MMAPMM_DEV_MAJOR, 0));  
	class_destroy(mmap_class);
	unregister_chrdev_region(MKDEV(MMAPMM_DEV_MAJOR, 0), 1);  
}  
 
module_init(mmapmm_init);  
module_exit(mmapmm_exit);  
